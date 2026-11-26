#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import struct
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

CLI_DESCRIPTION = (
    "Point the runtime models of a hero family at one copy of each texture. Tier bakes write their own "
    "<base>_main.png even when every tier uses the same pixels; this rewrites the image URIs of byte-identical "
    "textures to a file named by their SHA-256, so a later bake that changes one tier's texture cannot change another "
    "tier. Only the glb JSON chunk changes; the copies the bakes wrote stay on disk but no longer ship."
)

CHAR_PACK = Path(__file__).resolve().parents[1] / "char_pack"
RUNTIME_MODEL_GLOB = "*_rl.glb"
SHARED_PREFIX = "shared_"
SHARED_DIGEST_CHARS = 16
GLB_MAGIC = b"glTF"
JSON_CHUNK = b"JSON"
GLB_HEADER = struct.Struct("<4sII")
GLB_CHUNK = struct.Struct("<I4s")
JSON_PADDING = b" "
DATA_URI_PREFIX = "data:"
TEMPORARY_SUFFIX = ".sharing"


@dataclass
class Shared:
    models: int = 0
    textures: int = 0
    bytes: int = 0


def read_glb(path: Path) -> tuple[int, dict, bytes]:
    data = path.read_bytes()
    magic, version, _ = GLB_HEADER.unpack_from(data)
    length, kind = GLB_CHUNK.unpack_from(data, GLB_HEADER.size)
    if magic != GLB_MAGIC or kind != JSON_CHUNK:
        raise ValueError(f"{path} is not a glb with a JSON chunk first")
    start = GLB_HEADER.size + GLB_CHUNK.size
    return version, json.loads(data[start : start + length]), data[start + length :]


def write_glb(path: Path, version: int, document: dict, rest: bytes) -> None:
    text = json.dumps(document, separators=(",", ":")).encode()
    text += JSON_PADDING * (-len(text) % 4)
    body = GLB_CHUNK.pack(len(text), JSON_CHUNK) + text + rest
    temporary = path.with_name(path.name + TEMPORARY_SUFFIX)
    temporary.write_bytes(GLB_HEADER.pack(GLB_MAGIC, version, GLB_HEADER.size + len(body)) + body)
    os.replace(temporary, path)


def local_file_uri(uri: object) -> bool:
    return isinstance(uri, str) and not uri.startswith(DATA_URI_PREFIX) and "/" not in uri


def local_image_uris(document: dict) -> list[str]:
    return [image["uri"] for image in document.get("images", []) if local_file_uri(image.get("uri"))]


def file_digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def shared_name(digest: str, uri: str) -> str:
    return f"{SHARED_PREFIX}{digest[:SHARED_DIGEST_CHARS]}{Path(uri).suffix}"


def share_family(directory: Path) -> Shared:
    models = {path: read_glb(path) for path in sorted(directory.glob(RUNTIME_MODEL_GLOB))}
    uris_by_digest: dict[str, set[str]] = defaultdict(set)
    digests: dict[str, str] = {}
    for _, document, _ in models.values():
        for uri in local_image_uris(document):
            if uri not in digests and (directory / uri).is_file():
                digests[uri] = file_digest(directory / uri)
            if uri in digests:
                uris_by_digest[digests[uri]].add(uri)
    target: dict[str, str] = {}
    shared = Shared()
    for digest, uris in sorted(uris_by_digest.items()):
        if len(uris) < 2:
            continue
        name = shared_name(digest, min(uris))
        destination = directory / name
        if not destination.is_file() or file_digest(destination) != digest:
            shutil.copyfile(directory / min(uris), destination)
        shared.textures += 1
        shared.bytes += destination.stat().st_size * (len(uris) - 1)
        target.update({uri: name for uri in uris})
    for path, (version, document, rest) in models.items():
        changed = False
        for image in document.get("images", []):
            new = target.get(image.get("uri"))
            if new and new != image["uri"]:
                image["uri"] = new
                changed = True
        if changed:
            write_glb(path, version, document, rest)
            shared.models += 1
    return shared


def share_families(directories: list[Path]) -> Shared:
    total = Shared()
    for directory in sorted(set(directories)):
        if not directory.is_dir():
            continue
        family = share_family(directory)
        total.models += family.models
        total.textures += family.textures
        total.bytes += family.bytes
    return total


def report(shared: Shared) -> str:
    return (
        f"shared {shared.textures} textures across tiers, rewrote {shared.models} models, "
        f"{shared.bytes / 1e6:.0f} MB fewer texture bytes to ship"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--char-pack", type=Path, default=CHAR_PACK, help=f"hero model folder (default {CHAR_PACK})")
    parser.add_argument("families", nargs="*", help="family folders to share (default every folder)")
    args = parser.parse_args()
    names = args.families or sorted(path.name for path in args.char_pack.iterdir() if path.is_dir())
    print(report(share_families([args.char_pack / name for name in names])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
