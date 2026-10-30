#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import re
import struct
from urllib.parse import unquote


CLI_DESCRIPTION = 'Audit every runtime character GLB and every catalog tier binding'
ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "heroes-of-dragons-cache" / "characters.json"
CHAR_PACK = ROOT / "heroes-of-dragons-cache" / "char_pack"
TIERS = ("I", "II", "III", "IV")
TIER_SUFFIX = re.compile(r"_t\d+$", re.IGNORECASE)
COMPONENT = {5120: "b", 5121: "B", 5122: "h", 5123: "H", 5125: "I", 5126: "f"}


def prefab_base(prefab: str) -> str:
    value = prefab.removeprefix("unit.")
    return value[:-4] if value.endswith("_pre") else value


def family(base: str) -> str:
    return TIER_SUFFIX.sub("", base)


def model_path(base: str) -> Path | None:
    directory = CHAR_PACK / family(base)
    exact = directory / f"{base}_rl.glb"
    if exact.is_file():
        return exact
    if not TIER_SUFFIX.search(base):
        tierless = directory / f"{base}_t0_rl.glb"
        if tierless.is_file():
            return tierless
    return None


def load_glb(path: Path, with_blob: bool = False):
    data = path.read_bytes()
    if data[:4] != b"glTF":
        raise ValueError("not a binary glTF")
    json_length = struct.unpack_from("<I", data, 12)[0]
    gltf = json.loads(data[20:20 + json_length])
    if not with_blob:
        return gltf, None
    offset = 20 + json_length
    binary_length, binary_type = struct.unpack_from("<I4s", data, offset)
    if binary_type != b"BIN\0":
        raise ValueError("missing GLB binary chunk")
    return gltf, memoryview(data)[offset + 8:offset + 8 + binary_length]


def primitive_extents(gltf: dict, skinned_only: bool = True) -> tuple[float, float, float] | None:
    low = [math.inf] * 3
    high = [-math.inf] * 3
    for mesh in gltf.get("meshes", []):
        for primitive in mesh.get("primitives", []):
            attrs = primitive.get("attributes", {})
            if skinned_only and "JOINTS_0" not in attrs:
                continue
            accessor = gltf["accessors"][attrs["POSITION"]]
            if "min" not in accessor or "max" not in accessor:
                continue
            for axis in range(3):
                low[axis] = min(low[axis], float(accessor["min"][axis]))
                high[axis] = max(high[axis], float(accessor["max"][axis]))
    if any(not math.isfinite(value) for value in low + high):
        return None
    return tuple(high[axis] - low[axis] for axis in range(3))


def normalized_shape(extents: tuple[float, float, float]) -> tuple[float, float, float]:
    ordered = sorted(extents)
    scale = max(ordered[-1], 1e-9)
    return tuple(value / scale for value in ordered)


def weight_errors(gltf: dict, blob: memoryview) -> tuple[int, int]:
    bad = total = 0
    seen: set[int] = set()
    for mesh in gltf.get("meshes", []):
        for primitive in mesh.get("primitives", []):
            index = primitive.get("attributes", {}).get("WEIGHTS_0")
            if index is None or index in seen:
                continue
            seen.add(index)
            accessor = gltf["accessors"][index]
            view = gltf["bufferViews"][accessor["bufferView"]]
            component = accessor["componentType"]
            fmt = COMPONENT[component]
            component_size = struct.calcsize(fmt)
            stride = view.get("byteStride", component_size * 4)
            start = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
            integer_scale = {5121: 1 / 255.0, 5123: 1 / 65535.0}.get(component, 1.0)
            for row in range(accessor["count"]):
                values = struct.unpack_from("<" + fmt * 4, blob, start + row * stride)
                values = [float(value) * integer_scale for value in values]
                total += 1
                if (any(not math.isfinite(value) or value < -1e-6 for value in values)
                        or abs(sum(values) - 1.0) > 0.02):
                    bad += 1
    return bad, total


def index_errors(gltf: dict, blob: memoryview) -> tuple[int, int, list[str]]:
    total = uint32 = 0
    errors = []
    component_format = {5121: "B", 5123: "H", 5125: "I"}
    for mesh_index, mesh in enumerate(gltf.get("meshes", [])):
        for primitive_index, primitive in enumerate(mesh.get("primitives", [])):
            index = primitive.get("indices")
            position = primitive.get("attributes", {}).get("POSITION")
            if index is None or position is None:
                continue
            accessor = gltf["accessors"][index]
            component = accessor["componentType"]
            fmt = component_format.get(component)
            if fmt is None:
                errors.append(
                    f"mesh {mesh_index} primitive {primitive_index}: unsupported index type {component}"
                )
                continue
            total += 1
            uint32 += component == 5125
            view = gltf["bufferViews"][accessor["bufferView"]]
            size = struct.calcsize(fmt)
            stride = view.get("byteStride", size)
            start = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
            maximum = max(
                (struct.unpack_from("<" + fmt, blob, start + row * stride)[0]
                 for row in range(accessor["count"])),
                default=-1,
            )
            vertex_count = gltf["accessors"][position]["count"]
            reason = None
            if maximum >= vertex_count:
                reason = f"index {maximum} outside {vertex_count} vertices"
            elif component == 5125 and maximum > 65535:
                reason = f"u32 index {maximum} is truncated by raylib's u16 loader"
            if reason:
                errors.append(f"mesh {mesh_index} primitive {primitive_index}: {reason}")
    return total, uint32, errors


def audit_output(path: Path, deep: bool) -> dict:
    gltf, blob = load_glb(path, with_blob=deep)
    animations = [animation.get("name", "") for animation in gltf.get("animations", [])]
    extents = primitive_extents(gltf)
    mesh_nodes = [node for node in gltf.get("nodes", []) if "mesh" in node]
    unskinned = sum(1 for node in mesh_nodes if "skin" not in node)
    missing_images = []
    for image in gltf.get("images", []):
        uri = image.get("uri")
        if uri and not uri.startswith("data:") and not (path.parent / unquote(uri)).is_file():
            missing_images.append(uri)
    bad_weights = weight_rows = index_accessors = uint32_index_accessors = 0
    bad_indices = []
    if deep and blob is not None:
        bad_weights, weight_rows = weight_errors(gltf, blob)
        index_accessors, uint32_index_accessors, bad_indices = index_errors(gltf, blob)
    valid = bool(gltf.get("meshes") and gltf.get("skins") and animations[:1] == ["idle"] and extents)
    return {
        "valid": valid,
        "animations": animations,
        "extents": extents,
        "flatness": min(extents) / max(extents) if extents else 0.0,
        "unskinned_mesh_nodes": unskinned,
        "missing_images": missing_images,
        "bad_weights": bad_weights,
        "weight_rows": weight_rows,
        "index_accessors": index_accessors,
        "uint32_index_accessors": uint32_index_accessors,
        "bad_indices": bad_indices,
    }


def source_gltf(path: Path) -> dict | None:
    glb = path.with_suffix(".glb")
    gltf = path.with_suffix(".gltf")
    if glb.is_file():
        return load_glb(glb)[0]
    if gltf.is_file():
        return json.loads(gltf.read_text())
    return None


def run(deep: bool) -> dict:
    catalog = json.loads(CATALOG.read_text())["characters"]
    attack_bases = {
        prefab_base(prefab)
        for character in catalog
        if "ATTACK" in character.get("ability", {}).get("mechanics", [])
        for prefab in character.get("models", {}).values()
        if prefab
    }
    bindings = []
    unresolved = []
    exact = tierless = fallback = 0
    for character in catalog:
        models = character.get("models", {})
        for tier_index, tier in enumerate(TIERS):
            base = prefab_base(models.get(tier, ""))
            path = model_path(base) if base else None
            resolution = "exact"
            if path is not None and path.stem != f"{base}_rl":
                resolution = "tierless"
            if path is None:
                for distance in range(1, len(TIERS)):
                    candidates = (tier_index + distance, tier_index - distance)
                    for candidate in candidates:
                        if candidate < 0 or candidate >= len(TIERS):
                            continue
                        other = prefab_base(models.get(TIERS[candidate], ""))
                        path = model_path(other) if other else None
                        if path is not None:
                            resolution = "fallback"
                            break
                    if path is not None:
                        break
            if path is None:
                resolution = "unresolved"
                unresolved.append(f"{character['id']}:{tier}:{base}")
            elif resolution == "exact":
                exact += 1
            elif resolution == "tierless":
                tierless += 1
            else:
                fallback += 1
            bindings.append((character["id"], tier, base, resolution, str(path) if path else None))

    outputs = sorted(CHAR_PACK.glob("*/*_rl.glb"))
    invalid = []
    bad_weights = missing_images = unskinned = weight_rows = 0
    index_accessors = uint32_index_accessors = 0
    bad_indices = []
    output_rows = {}
    for path in outputs:
        try:
            row = audit_output(path, deep)
        except Exception as error:
            invalid.append(f"{path}: {type(error).__name__}: {error}")
            continue
        output_rows[path] = row
        if not row["valid"]:
            invalid.append(str(path))
        bad_weights += row["bad_weights"]
        weight_rows += row["weight_rows"]
        index_accessors += row["index_accessors"]
        uint32_index_accessors += row["uint32_index_accessors"]
        bad_indices.extend(f"{path}: {error}" for error in row["bad_indices"])
        missing_images += len(row["missing_images"])
        unskinned += row["unskinned_mesh_nodes"]

    collapsed = []
    flat_non_banner = []
    attack_models_without_attack_clip = []
    for path, row in output_rows.items():
        base = path.name.removesuffix("_rl.glb")
        extents = row["extents"]
        if (extents and not base.lower().startswith("b_")
                and extents[0] > 0.2 and extents[1] > 0.2
                and (extents[2] < 0.12 or extents[2] / extents[1] < 0.10)):
            flat_non_banner.append(f"{base}: extents={extents}")
        if base in attack_bases and "attack" not in row["animations"]:
            attack_models_without_attack_clip.append(
                f"{base}: animations={row['animations']}"
            )

    manifests = sorted(CHAR_PACK.glob("*/*_fbx_takes.json"))
    for manifest in manifests:
        base = manifest.name.removesuffix("_fbx_takes.json")
        output = manifest.parent / f"{base}_rl.glb"
        source = source_gltf(manifest.parent / base)
        row = output_rows.get(output)
        if source is None or row is None or row["extents"] is None:
            collapsed.append(f"{base}: missing source/output audit")
            continue
        source_extents = primitive_extents(source)
        if source_extents is None:
            collapsed.append(f"{base}: source has no skinned bounds")
            continue
        source_shape = normalized_shape(source_extents)
        output_shape = normalized_shape(row["extents"])
        shape_retention = min(
            output_shape[index] / max(source_shape[index], 1e-6) for index in range(3)
        )
        if row["flatness"] < 0.08 and shape_retention < 0.45:
            collapsed.append(
                f"{base}: source={source_extents} output={row['extents']} retention={shape_retention:.3f}"
            )

    return {
        "catalog_characters": len(catalog),
        "tier_bindings": len(bindings),
        "unique_requested_prefabs": len({binding[2] for binding in bindings if binding[2]}),
        "binding_resolution": {
            "exact": exact,
            "tierless": tierless,
            "nearest_tier_fallback": fallback,
            "unresolved": len(unresolved),
        },
        "runtime_glbs": len(outputs),
        "fbx_imports": len(manifests),
        "invalid_glbs": invalid,
        "collapsed_fbx": collapsed,
        "flat_non_banner_models": flat_non_banner,
        "attack_models_without_attack_clip": attack_models_without_attack_clip,
        "unskinned_mesh_nodes": unskinned,
        "missing_external_images": missing_images,
        "weight_rows": weight_rows,
        "bad_weight_rows": bad_weights,
        "index_accessors": index_accessors,
        "uint32_index_accessors": uint32_index_accessors,
        "bad_index_accessors": bad_indices,
        "unresolved_bindings": unresolved,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--deep", action="store_true", help="scan every binary WEIGHTS_0 row")
    parser.add_argument("--json", type=Path, help="write the detailed summary")
    parser.add_argument(
        "--loadtest-list", type=Path,
        help="write all runtime GLB paths for the game's HOD_LOADTEST loader",
    )
    args = parser.parse_args()
    report = run(args.deep)
    print(json.dumps(report, ensure_ascii=False, indent=2))
    if args.json:
        args.json.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    if args.loadtest_list:
        paths = sorted(CHAR_PACK.glob("*/*_rl.glb"))
        args.loadtest_list.write_text(
            "\n".join(str(path.relative_to(ROOT)) for path in paths) + "\n"
        )
    hard_fail = (
        report["binding_resolution"]["unresolved"]
        or report["invalid_glbs"]
        or report["collapsed_fbx"]
        or report["flat_non_banner_models"]
        or report["attack_models_without_attack_clip"]
        or report["unskinned_mesh_nodes"]
        or report["missing_external_images"]
        or report["bad_weight_rows"]
        or report["bad_index_accessors"]
    )
    if hard_fail:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
