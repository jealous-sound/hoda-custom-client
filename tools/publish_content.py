#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import time
import urllib.error
import urllib.request
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from typing import Callable

import build_content_packs
import build_dist

CLI_DESCRIPTION = (
    "Build the content packs, upload the packs the bucket does not have yet, then publish manifest.json last so "
    "players only ever see a manifest whose packs are already uploaded."
)

WRANGLER = ("npx", "--yes", "wrangler@4")
PACK_CONTENT_TYPE = "application/zip"
MANIFEST_CONTENT_TYPE = "application/json"
PACK_CACHE_CONTROL = "public, max-age=31536000, immutable"
MANIFEST_CACHE_CONTROL = "no-cache"
UPLOAD_ATTEMPTS = 3
UPLOAD_RETRY_SECONDS = 5
UPLOAD_WORKERS = 4
FETCH_TIMEOUT_SECONDS = 30
HTTP_NOT_FOUND = 404
USER_AGENT = "hoda-content-publisher"
ERROR_TAIL_CHARACTERS = 2000

Uploader = Callable[[str, str, Path, str, str], None]
Fetcher = Callable[[str], "dict | None"]


def manifest_url(content_url: str) -> str:
    return f"{content_url.rstrip('/')}/{build_content_packs.MANIFEST_NAME}"


def fetch_published(content_url: str) -> dict | None:
    request = urllib.request.Request(
        manifest_url(content_url), headers={"User-Agent": USER_AGENT, "Cache-Control": "no-cache"}
    )
    try:
        with urllib.request.urlopen(request, timeout=FETCH_TIMEOUT_SECONDS) as response:
            return json.load(response)
    except urllib.error.HTTPError as error:
        if error.code == HTTP_NOT_FOUND:
            return None
        raise


def wrangler_upload(bucket: str, key: str, path: Path, content_type: str, cache_control: str) -> None:
    command = [
        *WRANGLER,
        "r2",
        "object",
        "put",
        f"{bucket}/{key}",
        "--file",
        str(path),
        "--remote",
        "--content-type",
        content_type,
        "--cache-control",
        cache_control,
    ]
    failure = ""
    for attempt in range(UPLOAD_ATTEMPTS):
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode == 0:
            print(f"uploaded {key}", flush=True)
            return
        failure = (result.stderr or result.stdout)[-ERROR_TAIL_CHARACTERS:]
        time.sleep(UPLOAD_RETRY_SECONDS * (attempt + 1))
    raise SystemExit(f"upload of {key} failed:\n{failure}")


def same_packs(manifest: dict, published: dict | None) -> bool:
    return bool(published) and published.get("packs") == manifest["packs"]


def publish(
    root: Path,
    files: list[str],
    out: Path,
    bucket: str,
    content_url: str,
    upload: Uploader = wrangler_upload,
    fetch: Fetcher = fetch_published,
    dry_run: bool = False,
    workers: int | None = None,
) -> tuple[dict, list[str], bool]:
    published = fetch(content_url)
    manifest, written = build_content_packs.build(root, files, out, published, workers)
    if dry_run or (not written and same_packs(manifest, published)):
        return manifest, written, False
    with ThreadPoolExecutor(max_workers=UPLOAD_WORKERS) as pool:
        uploads = [
            pool.submit(upload, bucket, key, out / key, PACK_CONTENT_TYPE, PACK_CACHE_CONTROL) for key in written
        ]
        for finished in uploads:
            finished.result()
    manifest_name = build_content_packs.MANIFEST_NAME
    upload(bucket, manifest_name, out / manifest_name, MANIFEST_CONTENT_TYPE, MANIFEST_CACHE_CONTROL)
    return manifest, written, True


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--bucket", required=True, help="R2 bucket name")
    parser.add_argument("--content-url", required=True, help="public base URL of the bucket")
    parser.add_argument("--out", type=Path, default=build_content_packs.DEFAULT_OUT, help="folder for built packs")
    parser.add_argument("--workers", type=int, default=os.cpu_count(), help="parallel pack builders")
    parser.add_argument("--dry-run", action="store_true", help="build packs and the manifest without uploading")
    args = parser.parse_args()
    root = build_dist.ROOT
    manifest, written, released = publish(
        root,
        build_dist.content_files(root),
        args.out,
        args.bucket,
        args.content_url,
        dry_run=args.dry_run,
        workers=args.workers,
    )
    state = f"published version {manifest['version']}" if released else "nothing published"
    print(f"{build_content_packs.summary(manifest, written)}; {state}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
