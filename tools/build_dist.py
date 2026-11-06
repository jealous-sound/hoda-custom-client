#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import plistlib
import shutil
import stat
import struct
import subprocess
import sys
from pathlib import Path

CLI_DESCRIPTION = (
    "Assemble a playable copy of the game: the app bundle with raylib inside, data/ and only the parts of "
    "client-additional-data the game reads at runtime."
)

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUT = ROOT.parent / "hoda-custom-dist"
DIST_MARKER = ".hoda-custom-dist"
APP_NAME = "Heroes of Dragon Age"
APP_BUNDLE = f"{APP_NAME}.app"
APP_CONTENTS = f"{APP_BUNDLE}/Contents"
APP_EXECUTABLE = f"{APP_CONTENTS}/MacOS/{APP_NAME}"
APP_FRAMEWORKS = f"{APP_CONTENTS}/Frameworks"
APP_INFO = f"{APP_CONTENTS}/Info.plist"
LAUNCHER = "play.sh"
ASSETS = "client-additional-data"
ORIGINAL_GAME = f"{ASSETS}/Heroes-of-Dragon-Age-v4-4-0-cache-androidp1/com.ea.game.dragonage_row"
ORIGINAL_APK = f"{ASSETS}/Heroes-of-Dragon-Age-v4-4-0-androidp1.com.apk"
ORIGINAL_BUNDLES = f"{ORIGINAL_GAME}/files/UnityCache/Shared"
EXTRA_DIR = f"{ASSETS}/extra"
SINGLE_FILES = (
    ORIGINAL_APK,
    f"{ASSETS}/characters.json",
    f"{ASSETS}/player_starter.json",
    f"{ASSETS}/startup_images.txt",
    f"{ASSETS}/anim_tools/display_scales.json",
    f"{ORIGINAL_GAME}/cache/Localization/Loc_RUS_RU.txt",
)
WHOLE_TREES = (
    "data",
    ORIGINAL_BUNDLES,
    EXTRA_DIR,
)
MESH_DIR = f"{ASSETS}/meshes"
MESH_RUNTIME_SUFFIXES = frozenset({".obj", ".ply", ".png", ".glb", ".json", ".bin", ".mp3"})
CHAR_PACK_DIR = f"{ASSETS}/char_pack"
RUNTIME_MODEL_SUFFIX = "_rl.glb"
GLB_HEADER_BYTES = 12
GLB_CHUNK_HEADER_BYTES = 8
RAYLIB_LIBRARY_PREFIX = "libraylib"
EXECUTABLE_RELATIVE = "@executable_path"
FRAMEWORKS_RELATIVE = f"{EXECUTABLE_RELATIVE}/../Frameworks"
LAUNCHER_TEXT = """#!/bin/sh
cd "$(dirname "$0")" || exit 1
{backend}exec "./{executable}" "$@"
"""
EXECUTABLE_BITS = stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH
WINDOWS_FORBIDDEN_CHARACTERS = frozenset('<>:"\\|?*')
WINDOWS_TRAILING_CHARACTERS = (".", " ")


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


def portable_path(relative: str) -> bool:
    return not WINDOWS_FORBIDDEN_CHARACTERS.intersection(relative) and not any(
        part.endswith(WINDOWS_TRAILING_CHARACTERS) for part in relative.split("/")
    )


def mesh_files(root: Path) -> list[str]:
    return [
        path
        for path in tree_files(root, MESH_DIR)
        if Path(path).suffix in MESH_RUNTIME_SUFFIXES and portable_path(path)
    ]


def content_files(root: Path) -> list[str]:
    files = []
    for relative in SINGLE_FILES:
        if not (root / relative).is_file():
            raise SystemExit(f"missing {root / relative}")
        files.append(relative)
    for relative in WHOLE_TREES:
        files += tree_files(root, relative)
    files += mesh_files(root)
    files += character_files(root)
    return sorted(set(files))


def runtime_files(root: Path) -> list[str]:
    return sorted(set(tree_files(root, APP_BUNDLE) + content_files(root)))


def linked_raylib(binary: Path) -> Path | None:
    listing = subprocess.run(["otool", "-L", str(binary)], check=True, capture_output=True, text=True).stdout
    for line in listing.splitlines()[1:]:
        library = Path(line.strip().split(" (")[0])
        if library.name.startswith(RAYLIB_LIBRARY_PREFIX):
            return library
    return None


def bundle_raylib(out: Path) -> Path | None:
    executable = out / APP_EXECUTABLE
    library = linked_raylib(executable)
    if library is None or library.parts[0] == EXECUTABLE_RELATIVE:
        return None
    bundled = out / APP_FRAMEWORKS / library.name
    bundled.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(library.resolve(), bundled)
    bundled.chmod(bundled.stat().st_mode | stat.S_IWUSR)
    relative = f"{FRAMEWORKS_RELATIVE}/{library.name}"
    subprocess.run(["install_name_tool", "-id", relative, str(bundled)], check=True, capture_output=True)
    subprocess.run(
        ["install_name_tool", "-change", str(library), relative, str(executable)], check=True, capture_output=True
    )
    return bundled


def write_bundle_environment(out: Path, backend_url: str) -> None:
    info_path = out / APP_INFO
    info = plistlib.loads(info_path.read_bytes())
    if backend_url:
        info["LSEnvironment"] = {"HOD_BACKEND_URL": backend_url}
    else:
        info.pop("LSEnvironment", None)
    info_path.write_bytes(plistlib.dumps(info))


def sign_bundle(out: Path, bundled_library: Path | None) -> None:
    signed = [bundled_library] if bundled_library else []
    for path in signed + [out / APP_BUNDLE]:
        subprocess.run(["codesign", "--force", "--sign", "-", str(path)], check=True, capture_output=True)


def write_launcher(out: Path, backend_url: str) -> None:
    backend = f'export HOD_BACKEND_URL="${{HOD_BACKEND_URL:-{backend_url}}}"\n' if backend_url else ""
    launcher = out / LAUNCHER
    launcher.write_text(LAUNCHER_TEXT.format(backend=backend, executable=APP_EXECUTABLE))
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
    if not (root / APP_EXECUTABLE).is_file():
        raise SystemExit(f"{root / APP_EXECUTABLE} is missing; run make first")
    files = runtime_files(root)
    prepare_output(out, replace)
    total = 0
    for relative in files:
        place(root / relative, out / relative, link and not relative.startswith(f"{APP_BUNDLE}/"))
        total += (root / relative).stat().st_size
    write_bundle_environment(out, backend_url)
    sign_bundle(out, bundle_raylib(out))
    write_launcher(out, backend_url)
    return len(files), total


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT, help=f"output folder (default {DEFAULT_OUT})")
    parser.add_argument("--link", action="store_true", help="hard-link assets instead of copying them")
    parser.add_argument("--replace", action="store_true", help="rebuild an existing output folder made by this tool")
    parser.add_argument(
        "--backend-url", default="", help="HOD_BACKEND_URL for the app, and for play.sh unless one is set"
    )
    args = parser.parse_args()
    count, size = build(ROOT, args.out.resolve(), args.link, args.replace, args.backend_url)
    print(f"{args.out}: {count} files, {size / 1e9:.2f} GB; open {args.out / APP_BUNDLE} or run {args.out / LAUNCHER}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
