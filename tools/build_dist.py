#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import shutil
import stat
import struct
import subprocess
import sys
from pathlib import Path

CLI_DESCRIPTION = (
    "Assemble a playable copy of the game: the binary, its bundled raylib, data/ and only the parts of "
    "client-additional-data the game reads at runtime."
)

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUT = ROOT.parent / "hoda-custom-dist"
DIST_MARKER = ".hoda-custom-dist"
BINARY = "game"
LAUNCHER = "play.sh"
ASSETS = "client-additional-data"
ORIGINAL_GAME = f"{ASSETS}/Heroes-of-Dragon-Age-v4-4-0-cache-androidp1/com.ea.game.dragonage_row"
SINGLE_FILES = (
    f"{ASSETS}/Heroes-of-Dragon-Age-v4-4-0-androidp1.com.apk",
    f"{ASSETS}/characters.json",
    f"{ASSETS}/player_starter.json",
    f"{ASSETS}/startup_images.txt",
    f"{ASSETS}/anim_tools/display_scales.json",
    f"{ORIGINAL_GAME}/cache/Localization/Loc_RUS_RU.txt",
)
WHOLE_TREES = (
    "data",
    f"{ORIGINAL_GAME}/files/UnityCache/Shared",
    f"{ASSETS}/extra",
)
MESH_DIR = f"{ASSETS}/meshes"
MESH_RUNTIME_SUFFIXES = frozenset({".obj", ".ply", ".png", ".glb", ".json", ".bin", ".mp3"})
CHAR_PACK_DIR = f"{ASSETS}/char_pack"
RUNTIME_MODEL_SUFFIX = "_rl.glb"
GLB_HEADER_BYTES = 12
GLB_CHUNK_HEADER_BYTES = 8
RAYLIB_LIBRARY_PREFIX = "libraylib"
EXECUTABLE_RELATIVE = "@executable_path"
LAUNCHER_TEXT = """#!/bin/sh
cd "$(dirname "$0")" || exit 1
{backend}exec ./game "$@"
"""
EXECUTABLE_BITS = stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH


def glb_image_uris(path: Path) -> list[str]:
    data = path.read_bytes()
    (json_length,) = struct.unpack_from("<I", data, GLB_HEADER_BYTES)
    start = GLB_HEADER_BYTES + GLB_CHUNK_HEADER_BYTES
    document = json.loads(data[start : start + json_length])
    return [image["uri"] for image in document.get("images", []) if "uri" in image]


def tree_files(root: Path, relative: str) -> list[str]:
    base = root / relative
    if not base.is_dir():
        raise SystemExit(f"missing {base}")
    return [str(path.relative_to(root)) for path in base.rglob("*") if path.is_file()]


def character_files(root: Path) -> list[str]:
    found = []
    for model in (root / CHAR_PACK_DIR).rglob(f"*{RUNTIME_MODEL_SUFFIX}"):
        found.append(str(model.relative_to(root)))
        for uri in glb_image_uris(model):
            texture = model.parent / uri
            if not texture.is_file():
                raise SystemExit(f"{model} references missing {texture}")
            found.append(str(texture.relative_to(root)))
    return found


def mesh_files(root: Path) -> list[str]:
    return [path for path in tree_files(root, MESH_DIR) if Path(path).suffix in MESH_RUNTIME_SUFFIXES]


def runtime_files(root: Path) -> list[str]:
    files = [BINARY]
    for relative in SINGLE_FILES:
        if not (root / relative).is_file():
            raise SystemExit(f"missing {root / relative}")
        files.append(relative)
    for relative in WHOLE_TREES:
        files += tree_files(root, relative)
    files += mesh_files(root)
    files += character_files(root)
    return sorted(set(files))


def linked_raylib(binary: Path) -> Path:
    listing = subprocess.run(["otool", "-L", str(binary)], check=True, capture_output=True, text=True).stdout
    for line in listing.splitlines()[1:]:
        library = Path(line.strip().split(" (")[0])
        if library.name.startswith(RAYLIB_LIBRARY_PREFIX):
            return library
    raise SystemExit(f"{binary} does not link raylib")


def bundle_raylib(out: Path) -> str:
    binary = out / BINARY
    library = linked_raylib(binary)
    if library.parts[0] == EXECUTABLE_RELATIVE:
        return library.name
    bundled = out / library.name
    shutil.copy2(library.resolve(), bundled)
    bundled.chmod(bundled.stat().st_mode | stat.S_IWUSR)
    relative = f"{EXECUTABLE_RELATIVE}/{library.name}"
    subprocess.run(["install_name_tool", "-id", relative, str(bundled)], check=True, capture_output=True)
    subprocess.run(
        ["install_name_tool", "-change", str(library), relative, str(binary)], check=True, capture_output=True
    )
    for signed in (bundled, binary):
        subprocess.run(["codesign", "--force", "--sign", "-", str(signed)], check=True, capture_output=True)
    return library.name


def write_launcher(out: Path, backend_url: str) -> None:
    backend = f'export HOD_BACKEND_URL="${{HOD_BACKEND_URL:-{backend_url}}}"\n' if backend_url else ""
    launcher = out / LAUNCHER
    launcher.write_text(LAUNCHER_TEXT.format(backend=backend))
    launcher.chmod(launcher.stat().st_mode | EXECUTABLE_BITS)


def prepare_output(out: Path, replace: bool) -> None:
    if out.exists() and any(out.iterdir()):
        if not replace:
            raise SystemExit(f"{out} is not empty; pass --replace to rebuild it")
        if not (out / DIST_MARKER).is_file():
            raise SystemExit(f"{out} was not made by this tool ({DIST_MARKER} missing); refusing to replace it")
        shutil.rmtree(out)
    out.mkdir(parents=True, exist_ok=True)
    (out / DIST_MARKER).write_text("")


def place(source: Path, target: Path, link: bool) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    if link:
        os.link(source, target)
    else:
        shutil.copy2(source, target)


def build(root: Path, out: Path, link: bool, replace: bool, backend_url: str) -> tuple[int, int]:
    if not (root / BINARY).is_file():
        raise SystemExit(f"{root / BINARY} is missing; run make first")
    files = runtime_files(root)
    prepare_output(out, replace)
    total = 0
    for relative in files:
        place(root / relative, out / relative, link and relative != BINARY)
        total += (root / relative).stat().st_size
    bundle_raylib(out)
    write_launcher(out, backend_url)
    return len(files), total


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT, help=f"output folder (default {DEFAULT_OUT})")
    parser.add_argument("--link", action="store_true", help="hard-link assets instead of copying them")
    parser.add_argument("--replace", action="store_true", help="rebuild an existing output folder made by this tool")
    parser.add_argument("--backend-url", default="", help="HOD_BACKEND_URL that play.sh uses unless one is set")
    args = parser.parse_args()
    count, size = build(ROOT, args.out.resolve(), args.link, args.replace, args.backend_url)
    print(f"{args.out}: {count} files, {size / 1e9:.2f} GB; start it with {args.out / LAUNCHER}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
