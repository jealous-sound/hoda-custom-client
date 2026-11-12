#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import io
import json
import os
import shutil
import sys
import zipfile
import zlib
from collections import defaultdict
from concurrent.futures import ProcessPoolExecutor
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path

import build_dist

CLI_DESCRIPTION = (
    "Pack the game data into deterministic zip packs named by their SHA-256 and write the manifest.json the game "
    "downloads. Packs whose files did not change since --previous keep their existing entry and are not rebuilt."
)

ROOT = build_dist.ROOT
DEFAULT_OUT = ROOT / "build" / "content"
MANIFEST_FORMAT = 2
MANIFEST_NAME = "manifest.json"
PACKS_DIR = "packs"
PACK_SUFFIX = ".zip"
ZIP_EPOCH = (1980, 1, 1, 0, 0, 0)
ZIP_CREATOR_UNIX = 3
ZIP_FILE_ATTRIBUTES = 0o100644 << 16
DEFLATE_LEVEL = 9
RAW_DEFLATE_WINDOW_BITS = -15
RAYLIB_INFLATE_LIMIT_BYTES = 64 * 1024 * 1024
BUCKET_HASH_BYTES = 4
VERSION_FORMAT = "%Y%m%dT%H%M%SZ"
DEFAULT_PACK = "core"
BOOT_PACKS = ("apk", "core")
WINDOWS_PATH_LIMIT = 259
WINDOWS_PROFILE_NAME_ALLOWANCE = 20
WINDOWS_INSTALL_ROOT = "C:\\Users\\{profile}\\AppData\\Local\\HoDA\\"
WINDOWS_ENTRY_PATH_LIMIT = WINDOWS_PATH_LIMIT - len(
    WINDOWS_INSTALL_ROOT.format(profile="p" * WINDOWS_PROFILE_NAME_ALLOWANCE)
)


@dataclass(frozen=True)
class PackRule:
    prefix: str
    name: str
    buckets: int


PACK_RULES = (
    PackRule(f"{build_dist.ORIGINAL_BUNDLES}/", "unity-cache", 32),
    PackRule(f"{build_dist.CHAR_PACK_DIR}/", "characters", 32),
    PackRule(f"{build_dist.MESH_DIR}/", "scenes", 8),
    PackRule(f"{build_dist.EXTRA_DIR}/", "extra", 2),
    PackRule(build_dist.ORIGINAL_APK, "apk", 1),
    PackRule("data/", "rules", 1),
)


def pack_name(relative: str) -> str:
    for rule in PACK_RULES:
        if not relative.startswith(rule.prefix):
            continue
        if rule.buckets == 1:
            return rule.name
        group = relative[len(rule.prefix) :].split("/", 1)[0]
        bucket = int.from_bytes(hashlib.sha1(group.encode()).digest()[:BUCKET_HASH_BYTES], "big") % rule.buckets
        return f"{rule.name}-{bucket:02d}"
    return DEFAULT_PACK


def group_packs(files: list[str]) -> dict[str, list[str]]:
    groups: dict[str, list[str]] = defaultdict(list)
    for relative in sorted(files):
        groups[pack_name(relative)].append(relative)
    return dict(groups)


def content_digest(root: Path, members: list[str]) -> str:
    digest = hashlib.sha256()
    for relative in members:
        digest.update(relative.encode() + b"\0" + hashlib.sha256((root / relative).read_bytes()).digest())
    return digest.hexdigest()


def deflated_size(data: bytes) -> int:
    compressor = zlib.compressobj(DEFLATE_LEVEL, zlib.DEFLATED, RAW_DEFLATE_WINDOW_BITS)
    return len(compressor.compress(data)) + len(compressor.flush())


def entry_method(data: bytes) -> int:
    if len(data) > RAYLIB_INFLATE_LIMIT_BYTES or deflated_size(data) >= len(data):
        return zipfile.ZIP_STORED
    return zipfile.ZIP_DEFLATED


def pack_bytes(root: Path, members: list[str]) -> tuple[bytes, int]:
    buffer = io.BytesIO()
    raw_bytes = 0
    with zipfile.ZipFile(buffer, "w") as archive:
        for relative in members:
            data = (root / relative).read_bytes()
            raw_bytes += len(data)
            info = zipfile.ZipInfo(relative, ZIP_EPOCH)
            info.create_system = ZIP_CREATOR_UNIX
            info.external_attr = ZIP_FILE_ATTRIBUTES
            info.compress_type = entry_method(data)
            archive.writestr(info, data, compresslevel=DEFLATE_LEVEL)
    return buffer.getvalue(), raw_bytes


def write_pack(root: Path, out: Path, name: str, members: list[str], content: str) -> dict:
    data, raw_bytes = pack_bytes(root, members)
    sha256 = hashlib.sha256(data).hexdigest()
    file = f"{PACKS_DIR}/{name}-{sha256}{PACK_SUFFIX}"
    target = out / file
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(data)
    return {
        "name": name,
        "file": file,
        "size": len(data),
        "sha256": sha256,
        "content": content,
        "files": len(members),
        "bytes": raw_bytes,
    }


def write_pack_job(job: tuple[Path, Path, str, list[str], str]) -> dict:
    return write_pack(*job)


def reusable_packs(previous: dict | None) -> dict[str, dict]:
    if not previous or previous.get("format") != MANIFEST_FORMAT:
        return {}
    return {pack["name"]: pack for pack in previous.get("packs", [])}


def install_order(name: str) -> tuple[int, str]:
    return (BOOT_PACKS.index(name) if name in BOOT_PACKS else len(BOOT_PACKS), name)


def require_portable(files: list[str]) -> None:
    unportable = [relative for relative in files if not build_dist.portable_path(relative)]
    if unportable:
        listing = "\n  ".join(unportable)
        raise SystemExit(f"these paths cannot exist on Windows; rename or exclude them:\n  {listing}")
    overlong = [relative for relative in files if len(relative) > WINDOWS_ENTRY_PATH_LIMIT]
    if overlong:
        listing = "\n  ".join(overlong)
        raise SystemExit(
            f"these paths are longer than the {WINDOWS_ENTRY_PATH_LIMIT} characters a Windows install "
            f"can hold; shorten or exclude them:\n  {listing}"
        )


def build(
    root: Path,
    files: list[str],
    out: Path,
    previous: dict | None = None,
    workers: int | None = None,
    version: str | None = None,
) -> tuple[dict, list[str]]:
    require_portable(files)
    shutil.rmtree(out / PACKS_DIR, ignore_errors=True)
    out.mkdir(parents=True, exist_ok=True)
    reusable = reusable_packs(previous)
    entries: dict[str, dict] = {}
    jobs = []
    for name, members in sorted(group_packs(files).items()):
        content = content_digest(root, members)
        earlier = reusable.get(name)
        if earlier and earlier.get("content") == content:
            entries[name] = earlier
        else:
            jobs.append((root, out, name, members, content))
    if workers == 1 or len(jobs) <= 1:
        built = [write_pack_job(job) for job in jobs]
    else:
        with ProcessPoolExecutor(max_workers=workers) as pool:
            built = list(pool.map(write_pack_job, jobs))
    for entry in built:
        entries[entry["name"]] = entry
    manifest = {
        "format": MANIFEST_FORMAT,
        "version": version or datetime.now(timezone.utc).strftime(VERSION_FORMAT),
        "packs": [entries[name] for name in sorted(entries, key=install_order)],
    }
    (out / MANIFEST_NAME).write_text(json.dumps(manifest, indent=1) + "\n")
    return manifest, sorted(entry["file"] for entry in built)


def summary(manifest: dict, written: list[str]) -> str:
    packs = manifest["packs"]
    download = sum(pack["size"] for pack in packs)
    raw = sum(pack["bytes"] for pack in packs)
    largest = max((pack["size"] for pack in packs), default=0)
    return (
        f"{len(packs)} packs ({len(written)} new), {download / 1e9:.2f} GB to download "
        f"from {raw / 1e9:.2f} GB of files, largest pack {largest / 1e6:.0f} MB"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT, help=f"output folder (default {DEFAULT_OUT})")
    parser.add_argument("--previous", type=Path, help="manifest.json of the published content, to reuse its packs")
    parser.add_argument("--workers", type=int, default=os.cpu_count(), help="parallel pack builders")
    args = parser.parse_args()
    previous = json.loads(args.previous.read_text()) if args.previous else None
    manifest, written = build(ROOT, build_dist.content_files(ROOT), args.out, previous, args.workers)
    print(f"{args.out}: {summary(manifest, written)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
