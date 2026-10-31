#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile
from urllib.parse import unquote

import glb_writer


CLI_DESCRIPTION = 'Import one reference FBX as a donor-bake source GLB'
ROOT = Path(__file__).resolve().parents[2]
DEFAULT_FBX_ROOT = ROOT / "client-additional-data" / "fbx"
DEFAULT_CHAR_PACK = ROOT / "client-additional-data" / "char_pack"
TIER_SUFFIX = re.compile(r"_t\d+$", re.IGNORECASE)

SPECIAL_MANIFESTS = {
    "c_icegiant_t1": {
        "clip_roles": {
            "idle": "c_ogre_rdy_ani",
            "attack": "c_ogre_atk_v03_ani",
            "attack2": "c_ogre_stomp_ani",
            "hit": "TwoHanded_GetHit",
            "death": "TwoHanded_Death",
            "cheer": "c_ogre_roar_ani",
        },
        "mapping": "Exact 19-joint Giant topology; uses the validated c_giant role set.",
    },
    "c_titanheartguardian_t1": {
        "embedded_clip_roles": ["idle", "cheer", "attack", "hit", "death"],
        "embedded_source_names": ["Take 001"] * 5,
        "embedded_durations": [5.0, 2.2, 4.6667, 2.0, 2.0],
        "mapping": "Five full FBX clips in the standard idle/cheer/attack/hit/death export order.",
    },
    "hm_nc_xenontheantiquarian_t1": {
        "embedded_clip_roles": ["idle"],
        "embedded_source_names": ["_generic_idle_wind_ani"],
        "embedded_durations": [4.9167],
        "duplicate_embedded_clips": {"hit": "idle"},
        "intentionally_absent_roles": ["attack", "death"],
        "mapping": "One-joint non-attacker; keeps its FBX idle and reuses it for hit.",
    },
}


class FbxFormatError(ValueError):
    pass


def _fbx_property(data: bytes, offset: int) -> tuple[object | None, int]:
    if offset >= len(data):
        raise FbxFormatError("truncated FBX property")
    kind = chr(data[offset])
    offset += 1
    scalar = {
        "Y": ("<h", 2),
        "C": ("<?", 1),
        "I": ("<i", 4),
        "F": ("<f", 4),
        "D": ("<d", 8),
        "L": ("<q", 8),
    }
    if kind in scalar:
        fmt, size = scalar[kind]
        if offset + size > len(data):
            raise FbxFormatError("truncated FBX scalar")
        return struct.unpack_from(fmt, data, offset)[0], offset + size
    if kind in {"R", "S"}:
        if offset + 4 > len(data):
            raise FbxFormatError("truncated FBX string/blob length")
        size = struct.unpack_from("<I", data, offset)[0]
        offset += 4
        end = offset + size
        if end > len(data):
            raise FbxFormatError("truncated FBX string/blob")
        raw = data[offset:end]
        if kind == "S":
            return raw.decode("utf-8", "replace"), end
        return None, end
    if kind in {"f", "d", "l", "i", "b", "c"}:
        if offset + 12 > len(data):
            raise FbxFormatError("truncated FBX array header")
        _, _, packed_size = struct.unpack_from("<III", data, offset)
        end = offset + 12 + packed_size
        if end > len(data):
            raise FbxFormatError("truncated FBX array")
        return None, end
    raise FbxFormatError(f"unsupported FBX property type {kind!r}")


def animation_stacks(path: Path) -> list[str]:
    data = path.read_bytes()
    if not data.startswith(b"Kaydara FBX Binary  \x00\x1a\x00") or len(data) < 27:
        raise FbxFormatError(f"{path}: only binary FBX is supported")
    version = struct.unpack_from("<I", data, 23)[0]
    wide = version >= 7500
    header_fmt = "<QQQB" if wide else "<IIIB"
    header_size = struct.calcsize(header_fmt)
    null_size = 25 if wide else 13
    found: list[str] = []

    def walk(offset: int, limit: int) -> int:
        while offset + header_size <= min(limit, len(data)):
            values = struct.unpack_from(header_fmt, data, offset)
            end_offset, prop_count, prop_bytes, name_size = values
            if end_offset == 0:
                return offset + null_size
            name_start = offset + header_size
            name_end = name_start + name_size
            prop_start = name_end
            prop_end = prop_start + prop_bytes
            if not (name_end <= prop_end <= end_offset <= len(data)):
                raise FbxFormatError(f"invalid FBX node at byte {offset}")
            name = data[name_start:name_end].decode("utf-8", "replace")
            properties: list[object | None] = []
            cursor = prop_start
            for _ in range(prop_count):
                value, cursor = _fbx_property(data, cursor)
                properties.append(value)
            if cursor != prop_end:
                raise FbxFormatError(f"property length mismatch in {name!r}")
            if name == "AnimationStack":
                stack = next((value for value in properties if isinstance(value, str)), "")
                stack = stack.split("\0", 1)[0].removeprefix("AnimStack::").strip()
                if stack and stack not in found:
                    found.append(stack)
            child_limit = end_offset - null_size
            if prop_end < child_limit:
                walk(prop_end, end_offset)
            offset = end_offset
        return offset

    walk(27, len(data))
    return found


def _det3(rows: list[list[float]]) -> float:
    return (
        rows[0][0] * (rows[1][1] * rows[2][2] - rows[1][2] * rows[2][1])
        - rows[0][1] * (rows[1][0] * rows[2][2] - rows[1][2] * rows[2][0])
        + rows[0][2] * (rows[1][0] * rows[2][1] - rows[1][1] * rows[2][0])
    )


def _rotation_quaternion(rows: list[list[float]]) -> list[float]:
    trace = rows[0][0] + rows[1][1] + rows[2][2]
    if trace > 0.0:
        root = math.sqrt(trace + 1.0) * 2.0
        x = (rows[2][1] - rows[1][2]) / root
        y = (rows[0][2] - rows[2][0]) / root
        z = (rows[1][0] - rows[0][1]) / root
        w = root / 4.0
    elif rows[0][0] > rows[1][1] and rows[0][0] > rows[2][2]:
        root = math.sqrt(max(0.0, 1.0 + rows[0][0] - rows[1][1] - rows[2][2])) * 2.0
        x = root / 4.0
        y = (rows[0][1] + rows[1][0]) / root
        z = (rows[0][2] + rows[2][0]) / root
        w = (rows[2][1] - rows[1][2]) / root
    elif rows[1][1] > rows[2][2]:
        root = math.sqrt(max(0.0, 1.0 + rows[1][1] - rows[0][0] - rows[2][2])) * 2.0
        x = (rows[0][1] + rows[1][0]) / root
        y = root / 4.0
        z = (rows[1][2] + rows[2][1]) / root
        w = (rows[0][2] - rows[2][0]) / root
    else:
        root = math.sqrt(max(0.0, 1.0 + rows[2][2] - rows[0][0] - rows[1][1])) * 2.0
        x = (rows[0][2] + rows[2][0]) / root
        y = (rows[1][2] + rows[2][1]) / root
        z = root / 4.0
        w = (rows[1][0] - rows[0][1]) / root
    length = math.sqrt(x * x + y * y + z * z + w * w)
    if length < 1e-12:
        raise ValueError("matrix produced a zero quaternion")
    result = [x / length, y / length, z / length, w / length]
    if result[3] < 0.0:
        result = [-value for value in result]
    return result


def _compose_trs(translation: list[float], quaternion: list[float], scale: list[float]) -> list[float]:
    x, y, z, w = quaternion
    rows = [
        [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
        [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
        [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)],
    ]
    return [
        rows[0][0] * scale[0], rows[1][0] * scale[0], rows[2][0] * scale[0], 0.0,
        rows[0][1] * scale[1], rows[1][1] * scale[1], rows[2][1] * scale[1], 0.0,
        rows[0][2] * scale[2], rows[1][2] * scale[2], rows[2][2] * scale[2], 0.0,
        translation[0], translation[1], translation[2], 1.0,
    ]


def matrix_to_trs(matrix: list[float], tolerance: float = 1e-5) -> tuple[list[float], list[float], list[float]]:
    if len(matrix) != 16:
        raise ValueError("glTF node matrix must have 16 values")
    if max(abs(matrix[i] - expected) for i, expected in zip((3, 7, 11, 15), (0, 0, 0, 1))) > tolerance:
        raise ValueError("non-affine glTF node matrix")
    columns = [[float(matrix[c * 4 + r]) for r in range(3)] for c in range(3)]
    scales = [math.sqrt(sum(value * value for value in column)) for column in columns]
    if min(scales) < 1e-10:
        raise ValueError("singular glTF node matrix")
    rotation_columns = [
        [value / scales[column] for value in columns[column]]
        for column in range(3)
    ]
    rows = [[rotation_columns[column][row] for column in range(3)] for row in range(3)]
    determinant = _det3(rows)
    if determinant < 0.0:
        axis = max(range(3), key=lambda index: abs(scales[index]))
        scales[axis] = -scales[axis]
        for row in range(3):
            rows[row][axis] *= -1.0
        determinant = _det3(rows)
    orthogonality_error = max(
        abs(sum(rows[row][axis_a] * rows[row][axis_b] for row in range(3)) - (axis_a == axis_b))
        for axis_a in range(3)
        for axis_b in range(3)
    )
    if abs(determinant - 1.0) > tolerance or orthogonality_error > tolerance:
        raise ValueError(
            f"matrix contains shear (det={determinant:.6g}, orthogonality={orthogonality_error:.6g})"
        )
    translation = [float(matrix[12]), float(matrix[13]), float(matrix[14])]
    rotation = _rotation_quaternion(rows)
    rebuilt = _compose_trs(translation, rotation, scales)
    allowed = tolerance * max(1.0, max(abs(float(value)) for value in matrix))
    error = max(abs(float(left) - right) for left, right in zip(matrix, rebuilt))
    if error > allowed:
        raise ValueError(f"matrix TRS recomposition error {error:.6g} exceeds {allowed:.6g}")
    return translation, rotation, scales


def normalize_assimp_gltf(gltf: dict) -> dict[str, int | float]:
    matrix_count = 0
    for node in gltf.get("nodes", []):
        matrix = node.pop("matrix", None)
        if matrix is None:
            continue
        translation, rotation, scale = matrix_to_trs(matrix)
        node["translation"] = translation
        node["rotation"] = rotation
        node["scale"] = scale
        matrix_count += 1

    whitened = 0
    for material in gltf.get("materials", []):
        pbr = material.get("pbrMetallicRoughness")
        if pbr is not None and "baseColorFactor" in pbr:
            factor = pbr["baseColorFactor"]
            pbr["baseColorFactor"] = [1.0, 1.0, 1.0, factor[3] if len(factor) > 3 else 1.0]
            whitened += 1
        extensions = material.get("extensions")
        if extensions is not None:
            extensions.pop("KHR_materials_specular", None)
            if not extensions:
                material.pop("extensions", None)
    for key in ("extensionsUsed", "extensionsRequired"):
        if key in gltf:
            gltf[key] = [name for name in gltf[key] if name != "KHR_materials_specular"]
            if not gltf[key]:
                gltf.pop(key)
    return {"matrices_converted": matrix_count, "materials_whitened": whitened}


def find_fbx(root: Path, base: str) -> Path:
    unit = f"unit.{base}_pre"
    direct = root / unit / f"{unit}.fbx"
    if direct.is_file():
        return direct
    matches = sorted(root.glob(f"*/{unit}/{unit}.fbx"))
    if len(matches) != 1:
        raise FileNotFoundError(f"expected one {unit}.fbx under {root}, found {len(matches)}")
    return matches[0]


def copy_images(gltf: dict, fbx: Path, destination: Path) -> list[str]:
    copied: list[str] = []
    by_casefold = {path.name.casefold(): path for path in fbx.parent.iterdir() if path.is_file()}
    for image in gltf.get("images", []):
        uri = image.get("uri")
        if not uri or uri.startswith("data:"):
            continue
        name = Path(unquote(uri).replace("\\", "/")).name
        source = fbx.parent / name
        if not source.is_file():
            source = by_casefold.get(name.casefold())
        if source is None or not source.is_file():
            raise FileNotFoundError(f"texture {uri!r} referenced by {fbx.name} was not found")
        target = destination / source.name
        shutil.copy2(source, target)
        image["uri"] = target.name
        if target.name not in copied:
            copied.append(target.name)
    return copied


def import_one(
    base: str,
    fbx_root: Path = DEFAULT_FBX_ROOT,
    char_pack: Path = DEFAULT_CHAR_PACK,
    assimp: str = "assimp",
    force: bool = False,
    skip_existing: bool = False,
) -> dict:
    fbx = find_fbx(fbx_root, base)
    dirname = TIER_SUFFIX.sub("", base)
    destination = char_pack / dirname
    destination.mkdir(parents=True, exist_ok=True)
    output = destination / f"{base}.glb"
    manifest_path = destination / f"{base}_fbx_takes.json"
    if not force and output.exists() and manifest_path.exists() and skip_existing:
        return {"base": base, "directory": dirname, "status": "exists", "output": str(output)}
    if not force and (output.exists() or manifest_path.exists()):
        raise FileExistsError(f"{output} or its take manifest already exists; pass --force to replace")

    stacks = animation_stacks(fbx)
    with tempfile.TemporaryDirectory(prefix=f"{base}_", dir=destination) as temporary:
        converted = Path(temporary) / f"{base}.glb"
        subprocess.run(
            [assimp, "export", str(fbx), str(converted), "-fglb2"],
            check=True,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        gltf, blob = glb_writer.load_glb(converted)
        if len(gltf.get("skins", [])) != 1:
            raise ValueError(f"{base}: expected one skin, found {len(gltf.get('skins', []))}")
        normalized = normalize_assimp_gltf(gltf)
        textures = copy_images(gltf, fbx, destination)
        temporary_output = destination / f".{base}.glb.tmp"
        glb_writer.save_glb(temporary_output, gltf, blob)
        temporary_output.replace(output)

    manifest = {
        "version": 1,
        "source_fbx": fbx.name,
        "animation_stacks": stacks,
    }
    manifest.update(SPECIAL_MANIFESTS.get(base, {}))
    temporary_manifest = manifest_path.with_suffix(manifest_path.suffix + ".tmp")
    temporary_manifest.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")
    temporary_manifest.replace(manifest_path)
    return {
        "base": base,
        "directory": dirname,
        "status": "imported",
        "source": str(fbx),
        "output": str(output),
        "manifest": str(manifest_path),
        "animation_stacks": stacks,
        "textures": textures,
        "nodes": len(gltf.get("nodes", [])),
        "meshes": len(gltf.get("meshes", [])),
        "skin_joints": len(gltf["skins"][0].get("joints", [])),
        "embedded_animations": len(gltf.get("animations", [])),
        **normalized,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("base", nargs="*", help="Prefab base without unit. or _pre")
    parser.add_argument("--inventory", type=Path, help="bake_all-style JSON with a todo list")
    parser.add_argument("--fbx-root", type=Path, default=DEFAULT_FBX_ROOT)
    parser.add_argument("--char-pack", type=Path, default=DEFAULT_CHAR_PACK)
    parser.add_argument("--assimp", default="assimp")
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--report", type=Path, help="write a batch result JSON")
    args = parser.parse_args()
    bases = list(args.base)
    if args.inventory:
        inventory = json.loads(args.inventory.read_text())
        bases.extend(base for _, base in inventory.get("todo", []))
    bases = list(dict.fromkeys(bases))
    if not bases:
        parser.error("provide at least one base or --inventory")

    results = []
    for index, base in enumerate(bases, 1):
        try:
            result = import_one(
                base, args.fbx_root, args.char_pack, args.assimp, args.force,
                skip_existing=len(bases) > 1,
            )
        except Exception as error:
            result = {"base": base, "status": "error", "detail": f"{type(error).__name__}: {error}"}
        results.append(result)
        if len(bases) > 1:
            print(f"[{index}/{len(bases)}] {base}: {result['status']}", flush=True)
    payload = {
        "targets": len(bases),
        "imported": sum(result["status"] == "imported" for result in results),
        "existing": sum(result["status"] == "exists" for result in results),
        "errors": sum(result["status"] == "error" for result in results),
        "results": results,
    }
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        temporary = args.report.with_suffix(args.report.suffix + ".tmp")
        temporary.write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n")
        temporary.replace(args.report)
    print(json.dumps(payload if len(bases) > 1 else results[0], ensure_ascii=False, indent=2))
    if payload["errors"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
