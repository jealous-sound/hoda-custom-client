#!/usr/bin/env python3

from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path
import re
import subprocess
import sys
from typing import Any


CLI_DESCRIPTION = "Audit and rebuild the player's exact current-tier model bakes"
ROOT = Path(__file__).resolve().parents[1]
ANIM_TOOLS = ROOT / "heroes-of-dragons-cache" / "anim_tools"
DEFAULT_CATALOG = ROOT / "heroes-of-dragons-cache" / "characters.json"
DEFAULT_PLAYER = ROOT / "heroes-of-dragons-cache" / "player_starter.json"
DEFAULT_CHAR_PACK = ROOT / "heroes-of-dragons-cache" / "char_pack"
DEFAULT_CAB_INDEX = ANIM_TOOLS / "cache" / "cab_paths.json"
DEFAULT_REFERENCE_FBX = (
    ROOT / ".claude" / "skills" / "hoda-reference" / "references" / "fbx"
)
DEFAULT_INVENTORY = ANIM_TOOLS / "cache" / "current_tier_inventory.json"

TIER_SUFFIX = re.compile(r"_t\d+$", re.IGNORECASE)
BUILDABLE = {"source_ready", "rebuild_ready"}


def prefab_base(prefab: str) -> str:
    value = prefab.strip()
    if value.startswith("unit."):
        value = value[5:]
    if value.endswith("_pre"):
        value = value[:-4]
    return value


def prefab_family(base: str) -> str:
    return TIER_SUFFIX.sub("", base)


def read_json(path: Path) -> Any:
    return json.loads(path.read_text())


def audit_models(
    catalog_path: Path = DEFAULT_CATALOG,
    player_path: Path = DEFAULT_PLAYER,
    char_pack: Path = DEFAULT_CHAR_PACK,
    cab_index_path: Path = DEFAULT_CAB_INDEX,
    reference_fbx: Path = DEFAULT_REFERENCE_FBX,
) -> dict[str, Any]:
    catalog = read_json(catalog_path)["characters"]
    player = read_json(player_path)["characters"]
    cab_index = {key.casefold(): value for key, value in read_json(cab_index_path).items()}
    by_id = {character["id"]: character for character in catalog}

    rows: list[dict[str, Any]] = []
    for owned in player:
        unit_id = owned["id"]
        tier = owned.get("tier", "I")
        character = by_id.get(unit_id)
        prefab = (character or {}).get("models", {}).get(tier, "")
        row: dict[str, Any] = {"id": unit_id, "tier": tier, "prefab": prefab}
        if not prefab:
            row["status"] = "invalid_catalog"
            rows.append(row)
            continue

        base = prefab_base(prefab)
        dirname = prefab_family(base)
        directory = char_pack / dirname
        output = directory / f"{base}_rl.glb"
        source_glb = directory / f"{base}.glb"
        source_gltf = directory / f"{base}.gltf"
        cab_key = f"cab-{base}_pre".casefold()
        references = sorted(reference_fbx.glob(f"*/unit.{base}_pre/*.png"))
        tierless = directory / f"{base}_t0_rl.glb"

        row.update(
            {
                "base": base,
                "dir": dirname,
                "output": str(output),
                "cab": cab_index.get(cab_key),
                "reference_textures": [str(path) for path in references],
            }
        )
        if output.is_file():
            row["status"] = "exact"
        elif not TIER_SUFFIX.search(base) and tierless.is_file():
            row["status"] = "tierless_t0"
            row["alias_output"] = str(tierless)
        elif not directory.is_dir():
            row["status"] = "no_char_pack_dir"
        elif cab_key not in cab_index:
            row["status"] = "no_cab"
        elif source_glb.is_file() or source_gltf.is_file():
            row["status"] = "source_ready"
        elif references:
            row["status"] = "rebuild_ready"
        else:
            row["status"] = "no_reference_texture"
        rows.append(row)

    targets: dict[str, dict[str, Any]] = {}
    for row in rows:
        base = row.get("base")
        if not base:
            continue
        target = targets.setdefault(
            base,
            {
                key: row[key]
                for key in (
                    "base",
                    "dir",
                    "status",
                    "output",
                    "cab",
                    "reference_textures",
                )
            },
        )
        target.setdefault("ids", []).append(row["id"])
        target.setdefault("tiers", []).append(row["tier"])
        if target["status"] != row["status"]:
            raise ValueError(f"{base}: inconsistent target states")

    character_counts = Counter(row["status"] for row in rows)
    target_counts = Counter(target["status"] for target in targets.values())
    return {
        "catalog": str(catalog_path),
        "player": str(player_path),
        "characters": rows,
        "targets": [targets[base] for base in sorted(targets)],
        "character_counts": dict(sorted(character_counts.items())),
        "target_counts": dict(sorted(target_counts.items())),
    }


def buildable_targets(report: dict[str, Any]) -> list[dict[str, Any]]:
    return [target for target in report["targets"] if target["status"] in BUILDABLE]


def inventory_payload(report: dict[str, Any]) -> dict[str, Any]:
    targets = buildable_targets(report)
    return {
        "version": 1,
        "purpose": "Exact current-tier player model bakes",
        "todo": [[target["dir"], target["base"]] for target in targets],
        "character_counts": report["character_counts"],
        "target_counts": report["target_counts"],
    }


def write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n")
    temporary.replace(path)


def select_targets(
    report: dict[str, Any], only: str | None, limit: int | None, all_targets: bool
) -> list[dict[str, Any]]:
    targets = buildable_targets(report)
    if only:
        requested = {value.strip() for value in only.split(",") if value.strip()}
        selected = [
            target
            for target in targets
            if target["base"] in requested or requested.intersection(target["ids"])
        ]
        matched = {target["base"] for target in selected}
        matched.update(unit_id for target in selected for unit_id in target["ids"])
        missing = sorted(requested - matched)
        if missing:
            raise ValueError(f"not buildable or unknown: {', '.join(missing)}")
        targets = selected
    elif not all_targets and limit is None:
        raise ValueError("use --only, --limit, or --all for a mutating batch")
    if limit is not None:
        if limit < 1:
            raise ValueError("--limit must be positive")
        targets = targets[:limit]
    return targets


def prepare_sources(
    targets: list[dict[str, Any]], reference_fbx: Path, python: str
) -> None:
    script = ANIM_TOOLS / "rebuild_stub_src.py"
    for index, target in enumerate(targets, 1):
        if target["status"] == "source_ready":
            print(f"[{index}/{len(targets)}] {target['base']}: source exists")
            continue
        print(f"[{index}/{len(targets)}] {target['base']}: rebuilding exact source")
        subprocess.run(
            [
                python,
                str(script),
                target["dir"],
                target["base"],
                "--reference-fbx",
                str(reference_fbx),
            ],
            cwd=ROOT,
            check=True,
        )


def bake_targets(
    targets: list[dict[str, Any]], inventory: Path, python: str, workers: int,
    force: bool, max_seconds: float | None,
) -> None:
    command = [
        python,
        str(ANIM_TOOLS / "bake_all.py"),
        "--inventory",
        str(inventory),
        "--only",
        ",".join(target["base"] for target in targets),
        "--workers",
        str(workers),
    ]
    if force:
        command.append("--force")
    if max_seconds is not None:
        command.extend(("--max-seconds", str(max_seconds)))
    subprocess.run(command, cwd=ROOT, check=True)


def print_summary(report: dict[str, Any]) -> None:
    print(f"player characters: {len(report['characters'])}")
    print(f"unique model targets: {len(report['targets'])}")
    print("character states:")
    for status, count in report["character_counts"].items():
        print(f"  {status}: {count}")
    print("unique target states:")
    for status, count in report["target_counts"].items():
        print(f"  {status}: {count}")
    targets = buildable_targets(report)
    characters = sum(len(target["ids"]) for target in targets)
    tierless_characters = report["character_counts"].get("tierless_t0", 0)
    tierless_targets = report["target_counts"].get("tierless_t0", 0)
    print(f"Tierless T0 aliases: {tierless_targets} targets / {tierless_characters} characters")
    print(f"Missing CAB-backed tiers: {len(targets)} targets / {characters} characters")


def main() -> None:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--player", type=Path, default=DEFAULT_PLAYER)
    parser.add_argument("--char-pack", type=Path, default=DEFAULT_CHAR_PACK)
    parser.add_argument("--cab-index", type=Path, default=DEFAULT_CAB_INDEX)
    parser.add_argument("--reference-fbx", type=Path, default=DEFAULT_REFERENCE_FBX)
    parser.add_argument("--inventory", type=Path, default=DEFAULT_INVENTORY)
    parser.add_argument("--report", type=Path, help="write the detailed audit JSON")
    parser.add_argument("--write-inventory", action="store_true")
    parser.add_argument("--prepare", action="store_true", help="prepare exact source GLBs")
    parser.add_argument("--bake", action="store_true", help="prepare, then run bake_all.py")
    parser.add_argument("--only", help="comma-separated model bases or character ids")
    parser.add_argument("--limit", type=int)
    parser.add_argument("--all", action="store_true", dest="all_targets")
    parser.add_argument("--workers", type=int, default=1)
    parser.add_argument("--force-bake", action="store_true")
    parser.add_argument("--max-seconds", type=float)
    parser.add_argument(
        "--python",
        default=sys.executable,
        help="UnityPy-enabled Python used by rebuild_stub_src.py and bake_all.py",
    )
    args = parser.parse_args()

    report = audit_models(
        args.catalog, args.player, args.char_pack, args.cab_index, args.reference_fbx
    )
    print_summary(report)
    if args.report:
        write_json(args.report, report)
    if args.write_inventory or args.prepare or args.bake:
        write_json(args.inventory, inventory_payload(report))
        print(f"inventory: {args.inventory}")

    if args.prepare or args.bake:
        targets = select_targets(report, args.only, args.limit, args.all_targets)
        if not targets:
            print("no missing CAB-backed targets selected")
            return
        prepare_sources(targets, args.reference_fbx, args.python)
        if args.bake:
            bake_targets(
                targets,
                args.inventory,
                args.python,
                args.workers,
                args.force_bake,
                args.max_seconds,
            )


if __name__ == "__main__":
    main()
