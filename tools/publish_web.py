#!/usr/bin/env python3
from __future__ import annotations

import argparse
import base64
import datetime
import hashlib
import hmac
import os
import re
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
import xml.etree.ElementTree as ElementTree
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass, field
from pathlib import Path
from typing import Protocol

import build_web

CLI_DESCRIPTION = (
    "Publish the browser build to web/<release>/ in the R2 bucket through its S3 API: the game files first (copied "
    "inside the bucket from --previous when unchanged), then the file lists, then the page, so nothing points at a "
    "file before it is there. A release is immutable: an object already published with other bytes stops the run."
)

WEB_PREFIX = "web"
HASH_LIST = "game_files.md5"
PAGE_ORDER = ("index.wasm", "index.js", "index.html")
RELEASE_PATTERN = re.compile(r"[A-Za-z0-9][A-Za-z0-9._-]*$")
IMMUTABLE_CACHE_CONTROL = "public, max-age=31536000, immutable"
DEFAULT_CONTENT_TYPE = "application/octet-stream"
CONTENT_TYPES = {
    ".apk": "application/vnd.android.package-archive",
    ".glb": "model/gltf-binary",
    ".html": "text/html; charset=utf-8",
    ".js": "text/javascript; charset=utf-8",
    ".json": "application/json",
    ".md5": "text/plain; charset=utf-8",
    ".mp3": "audio/mpeg",
    ".obj": "text/plain; charset=utf-8",
    ".png": "image/png",
    ".txt": "text/plain; charset=utf-8",
    ".wasm": "application/wasm",
    ".wav": "audio/wav",
}
ACCOUNT_ENV = "R2_ACCOUNT_ID"
ACCESS_KEY_ENV = "R2_ACCESS_KEY_ID"
SECRET_KEY_ENV = "R2_SECRET_ACCESS_KEY"
R2_ENDPOINT = "https://{account}.r2.cloudflarestorage.com"
R2_REGION = "auto"
S3_SERVICE = "s3"
SIGNING_ALGORITHM = "AWS4-HMAC-SHA256"
EMPTY_PAYLOAD_SHA256 = hashlib.sha256(b"").hexdigest()
S3_NAMESPACE = "{http://s3.amazonaws.com/doc/2006-03-01/}"
HTTP_NOT_FOUND = 404
HTTP_TOO_MANY_REQUESTS = 429
HTTP_SERVER_ERROR = 500
REQUEST_ATTEMPTS = 4
RETRY_SECONDS = 2
REQUEST_TIMEOUT_SECONDS = 120
DEFAULT_WORKERS = 16
HASH_CHUNK_BYTES = 1 << 20
MEGABYTE = 1_000_000


def uri_encode(value: str, keep_slashes: bool) -> str:
    return urllib.parse.quote(value, safe="/~" if keep_slashes else "~")


def canonical_query(query: dict[str, str]) -> str:
    return "&".join(f"{uri_encode(name, False)}={uri_encode(value, False)}" for name, value in sorted(query.items()))


def canonical_request(
    method: str, path: str, query: dict[str, str], headers: dict[str, str], payload_sha256: str
) -> str:
    names = sorted(headers, key=str.lower)
    lines = "".join(f"{name.lower()}:{' '.join(headers[name].split())}\n" for name in names)
    signed = ";".join(name.lower() for name in names)
    return "\n".join([method, uri_encode(path, True), canonical_query(query), lines, signed, payload_sha256])


def signing_key(secret_key: str, day: str, region: str, service: str) -> bytes:
    key = f"AWS4{secret_key}".encode()
    for part in (day, region, service, "aws4_request"):
        key = hmac.new(key, part.encode(), hashlib.sha256).digest()
    return key


def authorization(
    access_key: str,
    secret_key: str,
    region: str,
    service: str,
    amz_date: str,
    request: str,
    signed_headers: str,
) -> str:
    day = amz_date[:8]
    scope = f"{day}/{region}/{service}/aws4_request"
    digest = hashlib.sha256(request.encode()).hexdigest()
    to_sign = "\n".join([SIGNING_ALGORITHM, amz_date, scope, digest])
    signature = hmac.new(signing_key(secret_key, day, region, service), to_sign.encode(), hashlib.sha256).hexdigest()
    return f"{SIGNING_ALGORITHM} Credential={access_key}/{scope}, SignedHeaders={signed_headers}, Signature={signature}"


class Bucket(Protocol):
    def listing(self, prefix: str) -> dict[str, str]: ...

    def get(self, key: str) -> bytes | None: ...

    def put(self, key: str, body: bytes, md5: str, content_type: str) -> None: ...

    def copy(self, source: str, key: str) -> bool: ...


@dataclass
class Reply:
    status: int
    headers: dict[str, str]
    body: bytes


class R2Bucket:
    def __init__(self, endpoint: str, bucket: str, access_key: str, secret_key: str, region: str = R2_REGION):
        parts = urllib.parse.urlsplit(endpoint)
        self.scheme, self.host = parts.scheme, parts.netloc
        self.bucket, self.access_key, self.secret_key, self.region = bucket, access_key, secret_key, region

    def request(
        self, method: str, key: str = "", query: dict[str, str] | None = None, body: bytes = b"", **extra: str
    ) -> Reply:
        query = query or {}
        path = f"/{self.bucket}/{key}" if key else f"/{self.bucket}"
        payload_sha256 = hashlib.sha256(body).hexdigest() if body else EMPTY_PAYLOAD_SHA256
        amz_date = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        headers = {"Host": self.host, "x-amz-date": amz_date, "x-amz-content-sha256": payload_sha256, **extra}
        signed = ";".join(sorted(name.lower() for name in headers))
        headers["Authorization"] = authorization(
            self.access_key,
            self.secret_key,
            self.region,
            S3_SERVICE,
            amz_date,
            canonical_request(method, path, query, headers, payload_sha256),
            signed,
        )
        url = f"{self.scheme}://{self.host}{uri_encode(path, True)}"
        if query:
            url += "?" + canonical_query(query)
        return send(urllib.request.Request(url, data=body if method == "PUT" else None, headers=headers, method=method))

    def listing(self, prefix: str) -> dict[str, str]:
        found: dict[str, str] = {}
        token = ""
        while True:
            query = {"list-type": "2", "prefix": prefix}
            if token:
                query["continuation-token"] = token
            reply = self.request("GET", query=query)
            expect_ok(reply, f"list {prefix}")
            root = ElementTree.fromstring(reply.body)
            for contents in root.iter(f"{S3_NAMESPACE}Contents"):
                found[contents.findtext(f"{S3_NAMESPACE}Key", "")] = bare_etag(
                    contents.findtext(f"{S3_NAMESPACE}ETag", "")
                )
            token = root.findtext(f"{S3_NAMESPACE}NextContinuationToken", "")
            if root.findtext(f"{S3_NAMESPACE}IsTruncated", "false") != "true" or not token:
                return found

    def get(self, key: str) -> bytes | None:
        reply = self.request("GET", key)
        if reply.status == HTTP_NOT_FOUND:
            return None
        expect_ok(reply, f"get {key}")
        return reply.body

    def put(self, key: str, body: bytes, md5: str, content_type: str) -> None:
        reply = self.request(
            "PUT",
            key,
            body=body,
            **{
                "Content-Type": content_type,
                "Cache-Control": IMMUTABLE_CACHE_CONTROL,
                "Content-MD5": base64.b64encode(bytes.fromhex(md5)).decode(),
            },
        )
        expect_ok(reply, f"upload {key}")

    def copy(self, source: str, key: str) -> bool:
        reply = self.request("PUT", key, **{"x-amz-copy-source": uri_encode(f"/{self.bucket}/{source}", True)})
        if reply.status == HTTP_NOT_FOUND:
            return False
        expect_ok(reply, f"copy {source} to {key}")
        if b"<Error>" in reply.body:
            raise SystemExit(f"copy {source} to {key} failed: {reply.body[:500]!r}")
        return True


def send(request: urllib.request.Request) -> Reply:
    for attempt in range(REQUEST_ATTEMPTS):
        try:
            with urllib.request.urlopen(request, timeout=REQUEST_TIMEOUT_SECONDS) as response:
                return Reply(response.status, dict(response.headers.items()), response.read())
        except urllib.error.HTTPError as error:
            with error:
                reply = Reply(error.code, dict(error.headers.items()), error.read())
            retry = error.code == HTTP_TOO_MANY_REQUESTS or error.code >= HTTP_SERVER_ERROR
            if not retry or attempt == REQUEST_ATTEMPTS - 1:
                return reply
        except urllib.error.URLError:
            if attempt == REQUEST_ATTEMPTS - 1:
                raise
        time.sleep(RETRY_SECONDS * (attempt + 1))
    raise AssertionError("unreachable")


def expect_ok(reply: Reply, action: str) -> None:
    if not 200 <= reply.status < 300:
        raise SystemExit(f"{action} failed with HTTP {reply.status}: {reply.body[:500]!r}")


def bare_etag(etag: str) -> str:
    return etag.strip().strip('"').lower()


def release_key(release: str, relative: str) -> str:
    return f"{WEB_PREFIX}/{release}/{relative}"


def files_url(content_url: str, release: str) -> str:
    return f"{content_url.rstrip('/')}/{release_key(release, '')}"


def content_type(relative: str) -> str:
    return CONTENT_TYPES.get(Path(relative).suffix.lower(), DEFAULT_CONTENT_TYPE)


def file_md5(path: Path) -> str:
    digest = hashlib.md5(usedforsecurity=False)
    with path.open("rb") as source:
        while chunk := source.read(HASH_CHUNK_BYTES):
            digest.update(chunk)
    return digest.hexdigest()


def hash_list(digests: dict[str, str]) -> bytes:
    return "".join(f"{digests[relative]}  {relative}\n" for relative in sorted(digests)).encode()


def parse_hash_list(text: bytes) -> dict[str, str]:
    digests = {}
    for line in text.decode().splitlines():
        digest, _, relative = line.partition("  ")
        if relative:
            digests[relative] = digest
    return digests


@dataclass
class Plan:
    upload: list[str] = field(default_factory=list)
    copy: list[str] = field(default_factory=list)
    present: list[str] = field(default_factory=list)
    endings: list[str] = field(default_factory=list)


def plan_game_files(release: str, digests: dict[str, str], published: dict[str, str], previous: dict[str, str]) -> Plan:
    plan = Plan()
    for relative in sorted(digests):
        key = release_key(release, f"{build_web.GAME_TREE}/{relative}")
        if key in published:
            if published[key] != digests[relative]:
                raise SystemExit(f"{key} is already published with other bytes; publish a new release instead")
            plan.present.append(relative)
        elif previous.get(relative) == digests[relative]:
            plan.copy.append(relative)
        else:
            plan.upload.append(relative)
    return plan


@dataclass
class Release:
    root: Path
    name: str
    previous: str
    digests: dict[str, str]
    endings: dict[str, tuple[bytes, str]]
    reused: bool = False


def read_release(
    root: Path, name: str, previous: str, content_url: str, workers: int, reuse_game_files: bool = False
) -> Release:
    if not RELEASE_PATTERN.match(name):
        raise SystemExit(f"release {name!r} must be letters, digits, '.', '_' or '-', such as v0.31.0")
    if previous and (previous == name or not RELEASE_PATTERN.match(previous)):
        raise SystemExit(f"--previous {previous!r} must be another release")
    if reuse_game_files and not previous:
        raise SystemExit("--reuse-game-files needs --previous, the release whose game files to copy")
    needed = build_web.PAGE_FILES if reuse_game_files else (*build_web.PAGE_FILES, build_web.FILE_LIST)
    missing = [page for page in needed if not (root / page).is_file()]
    if missing:
        raise SystemExit(f"{root} lacks {', '.join(missing)}; run make web WEB_RELEASE={name}")
    expected = files_url(content_url, name)
    if expected.encode() not in (root / "index.wasm").read_bytes():
        raise SystemExit(f"{root}/index.wasm was not built to read {expected}; run make web WEB_RELEASE={name}")
    if reuse_game_files:
        pages = {page: ((root / page).read_bytes(), content_type(page)) for page in PAGE_ORDER}
        return Release(root, name, previous, {}, pages, reused=True)
    listed = (root / build_web.FILE_LIST).read_text().split()
    tree = root / build_web.GAME_TREE
    with ThreadPoolExecutor(max_workers=workers) as pool:
        digests = dict(zip(listed, pool.map(lambda relative: file_md5(tree / relative), listed)))
    endings = {HASH_LIST: (hash_list(digests), content_type(HASH_LIST))}
    for page in (build_web.FILE_LIST, *PAGE_ORDER):
        endings[page] = ((root / page).read_bytes(), content_type(page))
    return Release(root, name, previous, digests, endings)


def previous_digests(bucket: Bucket, release: Release) -> dict[str, str]:
    if not release.previous:
        return {}
    published = bucket.get(release_key(release.previous, HASH_LIST))
    if published is None:
        raise SystemExit(f"{release_key(release.previous, HASH_LIST)} is not published; check --previous")
    return parse_hash_list(published)


def adopt_previous_game_files(bucket: Bucket, release: Release) -> Release:
    digests = previous_digests(bucket, release)
    listed = bucket.get(release_key(release.previous, build_web.FILE_LIST))
    if listed is None:
        raise SystemExit(f"{release_key(release.previous, build_web.FILE_LIST)} is not published; check --previous")
    endings = {
        HASH_LIST: (hash_list(digests), content_type(HASH_LIST)),
        build_web.FILE_LIST: (listed, content_type(build_web.FILE_LIST)),
        **release.endings,
    }
    return Release(release.root, release.name, release.previous, digests, endings, reused=True)


def put_game_file(bucket: Bucket, release: Release, relative: str) -> int:
    body = (release.root / build_web.GAME_TREE / relative).read_bytes()
    bucket.put(
        release_key(release.name, f"{build_web.GAME_TREE}/{relative}"),
        body,
        release.digests[relative],
        content_type(relative),
    )
    return len(body)


def copy_game_file(bucket: Bucket, release: Release, relative: str) -> int:
    key = f"{build_web.GAME_TREE}/{relative}"
    if bucket.copy(release_key(release.previous, key), release_key(release.name, key)):
        return 0
    if release.reused:
        raise SystemExit(f"{release_key(release.previous, key)} is gone, so it cannot be reused")
    return put_game_file(bucket, release, relative)


def ending_digest(release: Release, relative: str) -> str:
    return hashlib.md5(release.endings[relative][0], usedforsecurity=False).hexdigest()


def plan_endings(release: Release, published: dict[str, str]) -> list[str]:
    missing = []
    for relative in release.endings:
        key = release_key(release.name, relative)
        if key not in published:
            missing.append(relative)
        elif published[key] != ending_digest(release, relative):
            raise SystemExit(f"{key} is already published with other bytes; publish a new release instead")
    return missing


def publish(bucket: Bucket, release: Release, dry_run: bool, workers: int) -> tuple[Plan, int]:
    if release.reused and HASH_LIST not in release.endings:
        release = adopt_previous_game_files(bucket, release)
    published = bucket.listing(release_key(release.name, ""))
    previous = release.digests if release.reused else previous_digests(bucket, release)
    plan = plan_game_files(release.name, release.digests, published, previous)
    plan.endings = plan_endings(release, published)
    if dry_run:
        return plan, sum((release.root / build_web.GAME_TREE / relative).stat().st_size for relative in plan.upload)
    with ThreadPoolExecutor(max_workers=workers) as pool:
        uploads = [pool.submit(put_game_file, bucket, release, relative) for relative in plan.upload]
        copies = [pool.submit(copy_game_file, bucket, release, relative) for relative in plan.copy]
        sent = sum(finished.result() for finished in uploads + copies)
    for relative in plan.endings:
        body, kind = release.endings[relative]
        bucket.put(release_key(release.name, relative), body, ending_digest(release, relative), kind)
    return plan, sent


def summary(release: Release, plan: Plan, sent: int, dry_run: bool) -> str:
    verb = "would upload" if dry_run else "uploaded"
    copied = f", {len(plan.copy)} copied from {release.previous}" if release.previous else ""
    if not plan.endings:
        ending = "lists and page were already there"
    else:
        ending = f"{'would publish' if dry_run else 'published'} {', '.join(plan.endings)} last"
    return (
        f"{release_key(release.name, '')}: {len(release.digests)} game files; {verb} {len(plan.upload)} "
        f"({sent / MEGABYTE:.1f} MB){copied}, {len(plan.present)} already there; {ending}"
    )


def bucket_from_environment(bucket: str, endpoint: str) -> R2Bucket:
    names = [ACCESS_KEY_ENV, SECRET_KEY_ENV] + ([] if endpoint else [ACCOUNT_ENV])
    missing = [name for name in names if not os.environ.get(name)]
    if missing:
        raise SystemExit(f"set {', '.join(missing)} from an R2 API token with Object Read & Write on {bucket}")
    endpoint = endpoint or R2_ENDPOINT.format(account=os.environ[ACCOUNT_ENV])
    return R2Bucket(endpoint, bucket, os.environ[ACCESS_KEY_ENV], os.environ[SECRET_KEY_ENV])


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--root", type=Path, required=True, help="folder made by make web")
    parser.add_argument("--bucket", required=True, help="R2 bucket name")
    parser.add_argument("--content-url", required=True, help="public base URL of the bucket")
    parser.add_argument("--release", required=True, help="folder under web/ to publish, such as v0.31.0")
    parser.add_argument("--previous", default="", help="published release to copy unchanged game files from")
    parser.add_argument("--endpoint", default="", help=f"S3 endpoint; defaults to the one for ${ACCOUNT_ENV}")
    parser.add_argument("--workers", type=int, default=DEFAULT_WORKERS, help="parallel requests")
    parser.add_argument("--dry-run", action="store_true", help="show what would be uploaded without writing")
    parser.add_argument(
        "--reuse-game-files",
        action="store_true",
        help="copy every game file and both lists from --previous on the server and upload only the page; "
        "the local asset cache is not read",
    )
    args = parser.parse_args()
    release = read_release(
        args.root.resolve(), args.release, args.previous, args.content_url, args.workers, args.reuse_game_files
    )
    bucket = bucket_from_environment(args.bucket, args.endpoint)
    plan, sent = publish(bucket, release, args.dry_run, args.workers)
    print(summary(release, plan, sent, args.dry_run))
    return 0


if __name__ == "__main__":
    sys.exit(main())
