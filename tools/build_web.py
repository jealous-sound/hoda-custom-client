#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import re
import shutil
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import build_dist

CLI_DESCRIPTION = (
    "Lay out the browser build's game files: an objects/ folder holding each file the game reads at runtime under "
    "its MD5, the prebuilt resource index cut down to the bundles that ship among them, and game_files.txt, the list "
    "the page mounts lazily over HTTP, pairing each object with the path the game opens it at."
)

ROOT = build_dist.ROOT
OBJECTS_TREE = "objects"
LOCAL_OBJECTS = f"{OBJECTS_TREE}/"
FILE_LIST = "game_files.txt"
LIST_SEPARATOR = "  "
DIGEST_PATTERN = re.compile(r"[0-9a-f]{32}$")
HASH_WORKERS = 16
HASH_CHUNK_BYTES = 1 << 20
ENTRY_PAGE = "index.html"
APP_FILES = ("manifest.json", "icon-180.png", "icon-192.png", "icon-512.png")
PAGE_FILES = ("index.wasm", "index.js", *APP_FILES, ENTRY_PAGE)
INDEX_HEADER = "hoda-resource-index 1"
INDEX_BUNDLES_LABEL = "bundles"
INDEX_OBJECTS_LABEL = "objects"


def index_section(lines: list[str], at: int, label: str) -> tuple[list[str], int]:
    name, _, count = lines[at].partition(" ") if at < len(lines) else ("", "", "")
    if name != label or not count.isdigit() or at + 1 + int(count) > len(lines):
        raise SystemExit(f"resource index line {at + 1} should start the {label} section")
    end = at + 1 + int(count)
    return lines[at + 1 : end], end


def shipped_index(text: str, shipped: set[str]) -> str:
    lines = text.splitlines()
    if not lines or lines[0] != INDEX_HEADER:
        raise SystemExit("not a resource index")
    bundles, at = index_section(lines, 1, INDEX_BUNDLES_LABEL)
    objects, _ = index_section(lines, at, INDEX_OBJECTS_LABEL)
    renumbered: dict[str, str] = {}
    for container, path in enumerate(bundles):
        if path in shipped:
            renumbered[str(container)] = str(len(renumbered))
    kept_objects = []
    for line in objects:
        container, _, rest = line.partition(" ")
        if container in renumbered:
            kept_objects.append(f"{renumbered[container]} {rest}")
    kept_bundles = [path for path in bundles if path in shipped]
    return "\n".join(
        [
            INDEX_HEADER,
            f"{INDEX_BUNDLES_LABEL} {len(kept_bundles)}",
            *kept_bundles,
            f"{INDEX_OBJECTS_LABEL} {len(kept_objects)}",
            *kept_objects,
            "",
        ]
    )


def file_md5(path: Path) -> str:
    digest = hashlib.md5(usedforsecurity=False)
    with path.open("rb") as source:
        while chunk := source.read(HASH_CHUNK_BYTES):
            digest.update(chunk)
    return digest.hexdigest()


def file_list(digests: dict[str, str], objects: str) -> str:
    return "".join(f"{objects}{digests[relative]}{LIST_SEPARATOR}{relative}\n" for relative in sorted(digests))


def list_digests(text: str, objects: str) -> dict[str, str]:
    digests = {}
    for number, line in enumerate(text.splitlines(), 1):
        location, separator, relative = line.partition(LIST_SEPARATOR)
        digest = location.removeprefix(objects)
        if not (separator and relative and location.startswith(objects) and DIGEST_PATTERN.match(digest)):
            raise SystemExit(f"{FILE_LIST} line {number} should read {objects}<md5>{LIST_SEPARATOR}<path>")
        digests[relative] = digest
    return digests


def link_objects(root: Path, tree: Path, digests: dict[str, str]) -> int:
    total = 0
    for relative, digest in digests.items():
        source = root / relative
        target = tree / digest
        if not target.is_symlink():
            target.symlink_to(source)
        total += source.stat().st_size
    return total


def build(
    root: Path, out: Path, resource_index: Path, served_index: str, workers: int = HASH_WORKERS
) -> tuple[int, int]:
    missing = [name for name in PAGE_FILES if not (out / name).is_file()]
    if missing:
        raise SystemExit(f"{out} lacks {', '.join(missing)}; run make web")
    if not resource_index.is_file():
        raise SystemExit(f"missing resource index {resource_index}")
    files = [relative for relative in build_dist.content_files(root) if relative != served_index]
    tree = out / OBJECTS_TREE
    if tree.exists():
        shutil.rmtree(tree)
    tree.mkdir(parents=True)
    with ThreadPoolExecutor(max_workers=workers) as pool:
        digests = dict(zip(files, pool.map(lambda relative: file_md5(root / relative), files)))
    total = link_objects(root, tree, digests)
    index = shipped_index(resource_index.read_text(), set(files)).encode()
    digests[served_index] = hashlib.md5(index, usedforsecurity=False).hexdigest()
    index_target = tree / digests[served_index]
    if not index_target.exists():
        index_target.write_bytes(index)
    total += len(index)
    (out / FILE_LIST).write_text(file_list(digests, LOCAL_OBJECTS))
    return len(digests), total


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--out", type=Path, required=True, help="folder holding index.html, index.js and index.wasm")
    parser.add_argument("--resource-index", type=Path, required=True, help="index written by HOD_RESOURCE_INDEX_OUT")
    parser.add_argument("--served-index", required=True, help="path the game reads the resource index from")
    args = parser.parse_args()
    count, size = build(ROOT, args.out.resolve(), args.resource_index.resolve(), args.served_index)
    print(f"{args.out}: {count} game files, {size / 1e9:.2f} GB behind {args.out / FILE_LIST}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
