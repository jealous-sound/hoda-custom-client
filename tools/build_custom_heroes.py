#!/usr/bin/env python3
import argparse
import copy
import hashlib
import json
import sys
from pathlib import Path

import hero_retier

ROOT = Path(__file__).resolve().parent.parent
SPEC_DIR = ROOT / "data/custom_heroes"
OUTPUT = ROOT / "data/custom_heroes.json"
CATALOG = ROOT / "client-additional-data/characters.json"
CHAR_PACK = ROOT / "client-additional-data/char_pack"
PORTRAIT_DIR = ROOT / "client-additional-data/extra/portraits"
DISPLAY_SCALES = ROOT / "client-additional-data/anim_tools/display_scales.json"
BAKE_COMMAND = "uv run --offline --with numpy --with Pillow --with UnityPy python tools/bake_custom_heroes.py"
CLI_DESCRIPTION = (
    "Build data/custom_heroes.json, the catalog rows of heroes derived from shipped ones, "
    "from the specs in data/custom_heroes/."
)

CATALOG_VERSION = 1
TIERS = ("I", "II", "III", "IV")
POWER_FIELDS = ("base_power", "max_power", "power_per_level", "max_fortify_power")
HEALTH_FIELDS = ("base_health", "max_health", "health_per_level", "max_fortify_health")
RUNTIME_ID_CAPACITY = 64
RUNTIME_NAME_CAPACITY = 64
SPEC_FIELDS = {"id", "based_on", "prefab", "character", "stat_scale", "look"}


def tier_number(tier):
    return TIERS.index(tier) + 1


def tier_base(prefab, tier):
    return f"{prefab}_t{tier_number(tier)}"


def spec_digest(spec):
    return hashlib.sha256(json.dumps(spec, sort_keys=True, ensure_ascii=False).encode("utf-8")).hexdigest()


def bake_stamp_path(prefab, char_pack=CHAR_PACK):
    return char_pack / prefab / f"{prefab}_bake.json"


def bake_stamp(spec):
    return {"id": spec["id"], "spec_digest": spec_digest(spec)}


def bake_problems(spec, scales, char_pack=CHAR_PACK, portrait_dir=PORTRAIT_DIR):
    prefab = spec["prefab"]
    problems = []
    for tier in TIERS:
        base = tier_base(prefab, tier)
        model = char_pack / prefab / f"{base}_rl.glb"
        portrait = portrait_dir / f"tex.{base}_pt.png"
        for path in (model, portrait):
            if not path.is_file():
                problems.append(f"missing {path.name}")
        if base not in scales:
            problems.append(f"no display scale for {base}")
    stamp_path = bake_stamp_path(prefab, char_pack)
    stamp = json.loads(stamp_path.read_text(encoding="utf-8")) if stamp_path.is_file() else None
    if stamp != bake_stamp(spec):
        problems.append(f"{stamp_path.name} was not baked from the current spec")
    return problems


def load_specs(spec_dir=SPEC_DIR):
    specs = []
    for path in sorted(spec_dir.glob("*.json")):
        spec = json.loads(path.read_text(encoding="utf-8"))
        unknown = set(spec) - SPEC_FIELDS
        if unknown:
            raise SystemExit(f"{path.name}: unknown fields {sorted(unknown)}")
        for field in ("id", "based_on", "prefab"):
            if not isinstance(spec.get(field), str) or not spec[field]:
                raise SystemExit(f"{path.name}: {field} must be a non-empty string")
        specs.append(spec)
    ids = [spec["id"] for spec in specs]
    if len(ids) != len(set(ids)):
        raise SystemExit("custom hero ids are not unique")
    return specs


def shipped_rows(catalog_path=CATALOG):
    return {row["id"]: row for row in json.loads(catalog_path.read_text(encoding="utf-8"))["characters"]}


def scaled_tiers(tiers, stat_scale):
    power = stat_scale.get("power", 1.0)
    health = stat_scale.get("health", 1.0)
    out = {}
    for tier, stats in tiers.items():
        row = dict(stats)
        for field in POWER_FIELDS:
            row[field] = round(row[field] * power)
        for field in HEALTH_FIELDS:
            row[field] = round(row[field] * health)
        out[tier] = row
    return out


def placed_in_band(row, base_rarity, rarity, bands):
    if rarity == base_rarity:
        return row
    source_costs = bands[base_rarity]["costs"][hero_retier.kind_of(row)]
    return hero_retier.retiered(row, rarity, source_costs, bands[rarity])


def catalog_row(spec, shipped, rarities=None, bands=None):
    base = shipped.get(spec["based_on"])
    if base is None:
        raise SystemExit(f"{spec['id']}: based_on {spec['based_on']!r} is not a shipped hero")
    if spec["id"] in shipped:
        raise SystemExit(f"{spec['id']}: id collides with a shipped hero")
    merged = copy.deepcopy(base)
    merged.update(copy.deepcopy(spec.get("character", {})))
    merged["icon"] = {tier: f"tex.{tier_base(spec['prefab'], tier)}_pt" for tier in TIERS}
    merged["models"] = {tier: f"unit.{tier_base(spec['prefab'], tier)}_pre" for tier in TIERS}
    merged["tiers"] = scaled_tiers(base["tiers"], spec.get("stat_scale", {}))
    merged.pop("id")
    row = {"id": spec["id"], "based_on": spec["based_on"], **merged}
    if rarities and spec["id"] in rarities:
        row = placed_in_band(row, base["rarity"], rarities[spec["id"]], bands)
    for field, capacity in (
        ("id", RUNTIME_ID_CAPACITY),
        ("name", RUNTIME_NAME_CAPACITY),
    ):
        if len(row[field].encode("utf-8")) >= capacity:
            raise SystemExit(f"{spec['id']}: {field} exceeds the runtime buffer")
    return row


def build(specs, shipped, rarities=None, bands=None):
    rows = [catalog_row(spec, shipped, rarities, bands) for spec in specs]
    return {"version": CATALOG_VERSION, "characters": rows}


def render(specs, shipped, rarities=None, bands=None):
    return json.dumps(build(specs, shipped, rarities, bands), ensure_ascii=False, indent=1) + "\n"


def main():
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    specs = load_specs()
    text = render(specs, shipped_rows(), hero_retier.proposed_rarities(), hero_retier.load_bands())
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding="utf-8") != text:
            print(f"{OUTPUT.relative_to(ROOT)} is stale", file=sys.stderr)
            return 1
        scales = json.loads(DISPLAY_SCALES.read_text(encoding="utf-8"))
        problems = [f"{spec['id']}: {problem}" for spec in specs for problem in bake_problems(spec, scales)]
        if problems:
            print("\n".join(problems + [f"bake the custom heroes with: {BAKE_COMMAND}"]), file=sys.stderr)
            return 1
        return 0
    OUTPUT.write_text(text, encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
