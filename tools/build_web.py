#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

import build_dist

CLI_DESCRIPTION = (
    "Lay out the browser build's game files: a game/ tree of links to the files the game reads at runtime, the "
    "prebuilt resource index, and game_files.txt, the list the page mounts lazily over HTTP."
)

ROOT = build_dist.ROOT
GAME_TREE = "game"
FILE_LIST = "game_files.txt"
PAGE_FILES = ("index.html", "index.js", "index.wasm")


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
    shutil.copyfile(resource_index, index_target)
    total += index_target.stat().st_size
    listed = sorted(files + [served_index])
    (out / FILE_LIST).write_text("".join(f"{relative}\n" for relative in listed))
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
