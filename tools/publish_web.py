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
from dataclasses import dataclass, field, replace
from pathlib import Path
from typing import Protocol

import build_web

CLI_DESCRIPTION = (
    "Publish the browser build to the R2 bucket through its S3 API: each game file as web/objects/<md5>, shared by "
    "every release so unchanged bytes keep their URL and stay in players' browser caches, then the release's "
    "game_files.txt and page under web/<release>/, so nothing points at a file before it is there. Objects missing "
    "from web/objects/ are copied inside the bucket from a --previous release that still has its own game/ tree. "
    "A release is immutable: an object already published with other bytes stops the run."
)

WEB_PREFIX = "web"
OBJECTS_PREFIX = f"{WEB_PREFIX}/{build_web.OBJECTS_TREE}/"
PUBLISHED_OBJECTS = f"../{build_web.OBJECTS_TREE}/"
LEGACY_HASH_LIST = "game_files.md5"
LEGACY_HASH_SEPARATOR = "  "
LEGACY_GAME_TREE = "game"
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


def object_key(digest: str) -> str:
    return f"{OBJECTS_PREFIX}{digest}"


def files_url(content_url: str, release: str) -> str:
    return f"{content_url.rstrip('/')}/{release_key(release, '')}"


def content_type(relative: str) -> str:
    return CONTENT_TYPES.get(Path(relative).suffix.lower(), DEFAULT_CONTENT_TYPE)


def parse_legacy_hash_list(text: bytes) -> dict[str, str]:
    digests = {}
    for line in text.decode().splitlines():
        digest, _, relative = line.partition(LEGACY_HASH_SEPARATOR)
        if relative:
            digests[relative] = digest
    return digests


def first_paths(digests: dict[str, str]) -> dict[str, str]:
    paths: dict[str, str] = {}
    for relative in sorted(digests):
        paths.setdefault(digests[relative], relative)
    return paths


@dataclass
class Plan:
    files: int = 0
    upload: list[str] = field(default_factory=list)
    copy: dict[str, str] = field(default_factory=dict)
    present: list[str] = field(default_factory=list)
    endings: list[str] = field(default_factory=list)


@dataclass
class Release:
    root: Path
    name: str
    previous: str
    digests: dict[str, str]
    pages: dict[str, tuple[bytes, str]]
    reused: bool = False


@dataclass
class Previous:
    digests: dict[str, str]
    legacy_tree: bool


def read_release(root: Path, name: str, previous: str, content_url: str, reuse_game_files: bool = False) -> Release:
    if not RELEASE_PATTERN.match(name) or name == build_web.OBJECTS_TREE:
        raise SystemExit(f"release {name!r} must be letters, digits, '.', '_' or '-', such as 2026-11-26")
    if previous and (previous == name or previous == build_web.OBJECTS_TREE or not RELEASE_PATTERN.match(previous)):
        raise SystemExit(f"--previous {previous!r} must be another release")
    if reuse_game_files and not previous:
        raise SystemExit("--reuse-game-files needs --previous, the release whose game files to keep")
    needed = build_web.PAGE_FILES if reuse_game_files else (*build_web.PAGE_FILES, build_web.FILE_LIST)
    missing = [page for page in needed if not (root / page).is_file()]
    if missing:
        raise SystemExit(f"{root} lacks {', '.join(missing)}; run make web WEB_RELEASE={name}")
    expected = files_url(content_url, name)
    if expected.encode() not in (root / "index.wasm").read_bytes():
        raise SystemExit(f"{root}/index.wasm was not built to read {expected}; run make web WEB_RELEASE={name}")
    pages = {page: ((root / page).read_bytes(), content_type(page)) for page in build_web.PAGE_FILES}
    if reuse_game_files:
        return Release(root, name, previous, {}, pages, reused=True)
    digests = build_web.list_digests((root / build_web.FILE_LIST).read_text(), build_web.LOCAL_OBJECTS)
    absent = {digest for digest in digests.values() if not (root / build_web.OBJECTS_TREE / digest).exists()}
    if absent:
        raise SystemExit(f"{root}/{build_web.OBJECTS_TREE} lacks {len(absent)} listed objects; run make web")
    return Release(root, name, previous, digests, pages)


def required(bucket: Bucket, key: str) -> bytes:
    body = bucket.get(key)
    if body is None:
        raise SystemExit(f"{key} is not published; check --previous")
    return body


def read_previous(bucket: Bucket, previous: str) -> Previous:
    if not previous:
        return Previous({}, legacy_tree=False)
    listed = required(bucket, release_key(previous, build_web.FILE_LIST)).decode()
    if listed.startswith(PUBLISHED_OBJECTS):
        return Previous(build_web.list_digests(listed, PUBLISHED_OBJECTS), legacy_tree=False)
    return Previous(parse_legacy_hash_list(required(bucket, release_key(previous, LEGACY_HASH_LIST))), legacy_tree=True)


def release_endings(release: Release) -> dict[str, tuple[bytes, str]]:
    listed = build_web.file_list(release.digests, PUBLISHED_OBJECTS).encode()
    return {build_web.FILE_LIST: (listed, content_type(build_web.FILE_LIST)), **release.pages}


def plan_objects(release: Release, stored: dict[str, str], previous: Previous) -> Plan:
    plan = Plan(files=len(release.digests))
    legacy_sources = first_paths(previous.digests) if previous.legacy_tree else {}
    for digest in sorted(set(release.digests.values())):
        key = object_key(digest)
        if key in stored:
            if stored[key] != digest:
                raise SystemExit(f"{key} holds bytes whose MD5 is {stored[key]}; delete it and publish again")
            plan.present.append(digest)
        elif digest in legacy_sources:
            plan.copy[digest] = release_key(release.previous, f"{LEGACY_GAME_TREE}/{legacy_sources[digest]}")
        elif release.reused:
            raise SystemExit(f"{key} is gone and {release.previous} has no copy of it, so it cannot be reused")
        else:
            plan.upload.append(digest)
    return plan


def ending_digest(body: bytes) -> str:
    return hashlib.md5(body, usedforsecurity=False).hexdigest()


def plan_endings(release: Release, endings: dict[str, tuple[bytes, str]], published: dict[str, str]) -> list[str]:
    missing = []
    for relative, (body, _) in endings.items():
        key = release_key(release.name, relative)
        if key not in published:
            missing.append(relative)
        elif published[key] != ending_digest(body):
            raise SystemExit(f"{key} is already published with other bytes; publish a new release instead")
    return missing


def put_object(bucket: Bucket, release: Release, paths: dict[str, str], digest: str) -> int:
    body = (release.root / build_web.OBJECTS_TREE / digest).read_bytes()
    if ending_digest(body) != digest:
        raise SystemExit(f"{paths[digest]} changed after make web listed it; run make web again")
    bucket.put(object_key(digest), body, digest, content_type(paths[digest]))
    return len(body)


def copy_object(bucket: Bucket, release: Release, paths: dict[str, str], digest: str, source: str) -> int:
    if bucket.copy(source, object_key(digest)):
        return 0
    if release.reused:
        raise SystemExit(f"{source} is gone, so it cannot be reused")
    return put_object(bucket, release, paths, digest)


def publish(bucket: Bucket, release: Release, dry_run: bool, workers: int) -> tuple[Plan, int]:
    previous = read_previous(bucket, release.previous)
    if release.reused:
        release = replace(release, digests=previous.digests)
    plan = plan_objects(release, bucket.listing(OBJECTS_PREFIX), previous)
    endings = release_endings(release)
    plan.endings = plan_endings(release, endings, bucket.listing(release_key(release.name, "")))
    if dry_run:
        return plan, sum((release.root / build_web.OBJECTS_TREE / digest).stat().st_size for digest in plan.upload)
    paths = first_paths(release.digests)
    with ThreadPoolExecutor(max_workers=workers) as pool:
        uploads = [pool.submit(put_object, bucket, release, paths, digest) for digest in plan.upload]
        copies = [
            pool.submit(copy_object, bucket, release, paths, digest, source) for digest, source in plan.copy.items()
        ]
        sent = sum(finished.result() for finished in uploads + copies)
    for relative in plan.endings:
        body, kind = endings[relative]
        bucket.put(release_key(release.name, relative), body, ending_digest(body), kind)
    return plan, sent


def summary(release: Release, plan: Plan, sent: int, dry_run: bool) -> str:
    objects = len(plan.upload) + len(plan.copy) + len(plan.present)
    verb = "would upload" if dry_run else "uploaded"
    copied = f", {'would copy' if dry_run else 'copied'} {len(plan.copy)} from {release.previous}" if plan.copy else ""
    if not plan.endings:
        ending = "list and page were already there"
    else:
        ending = f"{'would publish' if dry_run else 'published'} {', '.join(plan.endings)} last"
    return (
        f"{release_key(release.name, '')}: {plan.files} game files in {objects} objects under "
        f"{OBJECTS_PREFIX}; {verb} {len(plan.upload)} ({sent / MEGABYTE:.1f} MB){copied}, "
        f"{len(plan.present)} already there; {ending}"
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
    parser.add_argument(
        "--previous", default="", help="published release whose game files to copy where web/objects/ lacks them"
    )
    parser.add_argument("--endpoint", default="", help=f"S3 endpoint; defaults to the one for ${ACCOUNT_ENV}")
    parser.add_argument("--workers", type=int, default=DEFAULT_WORKERS, help="parallel requests")
    parser.add_argument("--dry-run", action="store_true", help="show what would be uploaded without writing")
    parser.add_argument(
        "--reuse-game-files",
        action="store_true",
        help="keep every game file of --previous, copying into web/objects/ on the server any it lacks, and upload "
        "only the list and page; the local asset cache is not read",
    )
    args = parser.parse_args()
    release = read_release(args.root.resolve(), args.release, args.previous, args.content_url, args.reuse_game_files)
    bucket = bucket_from_environment(args.bucket, args.endpoint)
    plan, sent = publish(bucket, release, args.dry_run, args.workers)
    print(summary(release, plan, sent, args.dry_run))
    return 0


if __name__ == "__main__":
    sys.exit(main())
