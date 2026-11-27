#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

import build_dist

CLI_DESCRIPTION = (
    "Lay out the browser build's game files: a game/ tree of links to the files the game reads at runtime, the "
    "prebuilt resource index cut down to the bundles that ship, and game_files.txt, the list of paths and sizes the "
    "page mounts lazily over HTTP."
)

ROOT = build_dist.ROOT
GAME_TREE = "game"
FILE_LIST = "game_files.txt"
LIST_SIZE_SEPARATOR = "\t"
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


def list_line(relative: str, size: int) -> str:
    return f"{relative}{LIST_SIZE_SEPARATOR}{size}\n"


def listed_paths(text: str) -> list[str]:
    return [line.partition(LIST_SIZE_SEPARATOR)[0] for line in text.splitlines() if line]


def link_tree(root: Path, tree: Path, files: list[str]) -> int:
    total = 0
    for relative in files:
        source = root / relative
        target = tree / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.symlink_to(source)
        total += source.stat().st_size
    return total


def build(root: Path, out: Path, resource_index: Path, served_index: str) -> tuple[int, int]:
    missing = [name for name in PAGE_FILES if not (out / name).is_file()]
    if missing:
        raise SystemExit(f"{out} lacks {', '.join(missing)}; run make web")
    if not resource_index.is_file():
        raise SystemExit(f"missing resource index {resource_index}")
    files = [relative for relative in build_dist.content_files(root) if relative != served_index]
    tree = out / GAME_TREE
    if tree.exists():
        shutil.rmtree(tree)
    total = link_tree(root, tree, files)
    index_target = tree / served_index
    index_target.parent.mkdir(parents=True, exist_ok=True)
    index_target.write_text(shipped_index(resource_index.read_text(), set(files)))
    total += index_target.stat().st_size
    listed = sorted(files + [served_index])
    (out / FILE_LIST).write_text("".join(list_line(relative, (tree / relative).stat().st_size) for relative in listed))
    return len(listed), total


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
