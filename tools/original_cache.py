from __future__ import annotations

import functools
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUNDLE_DIR = (
    ROOT
    / "client-additional-data/Heroes-of-Dragon-Age-v4-4-0-cache-androidp1/com.ea.game.dragonage_row/files/UnityCache/Shared"
)
BUNDLE_FILE_PREFIX = "CAB-"
CACHE_REF_PREFIX = "cache:"
SERIALIZED_VERSION = 9
HEADER_SIZE = 20
OBJECT_ENTRY = struct.Struct("<iIIihh")
TEXTURE_2D = 28
AUDIO_CLIP = 83
FONT = 128
NAMED_CLASSES = (TEXTURE_2D, AUDIO_CLIP, FONT)


def bundle_paths() -> list[Path]:
    return sorted(BUNDLE_DIR.glob(f"*/{BUNDLE_FILE_PREFIX}*"))


def bundle_name(path: Path) -> str:
    return path.name.removeprefix(BUNDLE_FILE_PREFIX)


def named_objects(path: Path) -> list[tuple[int, str]]:
    data = path.read_bytes()
    _, _, version, data_offset = struct.unpack_from(">IIII", data)
    if version != SERIALIZED_VERSION:
        raise ValueError(f"{path}: unsupported serialized version {version}")
    pos = data.index(b"\0", HEADER_SIZE) + 1
    _, type_count, big_ids, count = struct.unpack_from("<iiii", data, pos)
    if type_count or big_ids:
        raise ValueError(f"{path}: type trees or 64-bit ids are not supported")
    pos += 16
    objects = []
    for _ in range(count):
        _, start, _, _, class_id, _ = OBJECT_ENTRY.unpack_from(data, pos)
        pos += OBJECT_ENTRY.size
        if class_id in NAMED_CLASSES:
            offset = data_offset + start
            (length,) = struct.unpack_from("<i", data, offset)
            objects.append((class_id, data[offset + 4 : offset + 4 + length].decode("utf-8")))
    return objects


@functools.cache
def catalog() -> dict[tuple[int, str], tuple[str, ...]]:
    found: dict[tuple[int, str], list[str]] = {}
    for path in bundle_paths():
        for class_id, name in named_objects(path):
            found.setdefault((class_id, name), []).append(bundle_name(path))
    return {key: tuple(sorted(bundles)) for key, bundles in found.items()}


def names(class_id: int) -> list[str]:
    return sorted(name for cls, name in catalog() if cls == class_id)


def cache_ref(name: str) -> str:
    return f"{CACHE_REF_PREFIX}{name}"


def resolves(ref: str, class_id: int) -> bool:
    if not ref.startswith(CACHE_REF_PREFIX):
        return False
    bundle, _, name = ref.removeprefix(CACHE_REF_PREFIX).rpartition("/")
    bundles = catalog().get((class_id, name), ())
    return bool(bundles) and (not bundle or bundle in bundles)
