#!/usr/bin/env python3

from __future__ import annotations

from collections import Counter, defaultdict
import json
from pathlib import Path
import re
from typing import Any

import original_cache


ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "client-additional-data" / "characters.json"
WIRES = ROOT / "data" / "unit_vfx.json"
PRESETS = ROOT / "data" / "fx_presets.json"
BATTLE_C = ROOT / "source" / "battle.c"
REPORT = ROOT / "client-additional-data" / "vfx_tools" / "build_report.json"
CAB_PATHS = ROOT / "client-additional-data" / "anim_tools" / "cache" / "cab_paths.json"
PLAYER = ROOT / "client-additional-data" / "player_starter.json"
CAMPAIGNS = ROOT / "data" / "campaign_rosters.json"
PARTICLES = ROOT / "client-additional-data" / "vfx" / "particles"
SHARED = (
    ROOT
    / "client-additional-data"
    / "Heroes-of-Dragon-Age-v4-4-0-cache-androidp1"
    / "com.ea.game.dragonage_row"
    / "files"
    / "UnityCache"
    / "Shared"
)

ROLES = ("ability", "impact", "muzzle", "sustained")
FAMILIES = (
    "lightning",
    "spark",
    "shock",
    "chain",
    "smash",
    "slam",
    "stomp",
    "quake",
    "assassin",
    "skull",
    "tripplehit",
    "claw",
    "maul",
    "rend",
    "perforate",
    "heal",
    "imp_",
)


def wire_signature(wire: dict[str, Any]) -> str:
    fields = (
        wire.get("ability_type"),
        wire.get("vfx_type"),
        wire.get("ability"),
        wire.get("impact"),
        wire.get("muzzle"),
        wire.get("sustained"),
        wire.get("projectile_speed"),
        wire.get("cone_delay"),
    )
    return json.dumps(fields, separators=(",", ":"))


def models(character: dict[str, Any]) -> set[str]:
    return {value for value in (character.get("models") or {}).values() if value}


def cab_key(prefab: str) -> str:
    return "cab-" + prefab.removeprefix("unit.").casefold()


def classify_warning(message: str) -> str:
    if "size normalized" in message:
        return "size_normalized"
    if "material " in message and " unresolved" in message:
        return "material_unresolved"
    if "no particle dumps" in message:
        return "no_particle_dumps"
    if "no usable emitters" in message:
        return "no_usable_emitters"
    if "emitters trimmed" in message:
        return "emitters_trimmed"
    if "zero start size" in message:
        return "zero_size_replaced"
    return "other"


def example_groups(
    grouped: dict[str, list[dict[str, Any]]], wires: dict[str, dict[str, Any]], limit: int = 8
) -> list[dict[str, Any]]:
    result = []
    for key, characters in grouped.items():
        signatures = {wire_signature(wires[ch["id"]]) for ch in characters if ch["id"] in wires}
        if len(signatures) <= 1:
            continue
        result.append(
            {
                "key": key,
                "characters": len(characters),
                "wire_signatures": len(signatures),
                "examples": [
                    {
                        "id": ch["id"],
                        "name": ch["name"],
                        "ability_vfx": wires[ch["id"]].get("ability"),
                    }
                    for ch in characters
                    if ch["id"] in wires
                ][:5],
            }
        )
    result.sort(key=lambda row: (-row["wire_signatures"], -row["characters"], row["key"]))
    return result[:limit]


def main() -> None:
    catalog_root = json.loads(CATALOG.read_text())
    characters = catalog_root["characters"]
    by_id = {character["id"]: character for character in characters}
    wires = json.loads(WIRES.read_text())
    preset_root = json.loads(PRESETS.read_text())
    preset_names = {preset["name"] for preset in preset_root["presets"]}
    aliases = preset_root.get("aliases", {})

    source = BATTLE_C.read_text()
    hand_table = source[source.index("static const FxPreset") : source.index("static const FxPreset *BattleFxFindPreset")]
    hand_names = set(re.findall(r'\.name\s*=\s*"([^"]+)"', hand_table))

    def resolution(name: str | None) -> str:
        if not name or name == "fx_null_pre":
            return "none"
        if name in hand_names:
            return "hand_exact"
        if name in preset_names:
            return "generated_exact"
        if name in aliases:
            return "alias_exact"
        if any(needle in name for needle in FAMILIES):
            return "family_fallback"
        return "generic_slash_fallback"

    wire_ids = set(wires)
    catalog_ids = set(by_id)
    missing_ids = catalog_ids - wire_ids
    role_occurrences = Counter()
    role_unique: dict[str, set[str]] = {role: set() for role in ROLES}
    for wire in wires.values():
        for role in ROLES:
            name = wire.get(role)
            if name:
                role_occurrences[(role, resolution(name))] += 1
                role_unique[role].add(name)

    signature_groups: dict[str, list[str]] = defaultdict(list)
    for unit_id, wire in wires.items():
        signature_groups[wire_signature(wire)].append(unit_id)

    ability_groups: dict[str, list[dict[str, Any]]] = defaultdict(list)
    prefab_groups: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for character in characters:
        ability_groups[character["ability"]["key"]].append(character)
        for prefab in models(character):
            prefab_groups[prefab].append(character)

    ability_coverage = Counter()
    ability_signature_conflicts = 0
    ability_unique_signature_missing_units = 0
    for group in ability_groups.values():
        present = [ch for ch in group if ch["id"] in wires]
        if not present:
            ability_coverage["none"] += 1
        elif len(present) == len(group):
            ability_coverage["full"] += 1
        else:
            ability_coverage["partial"] += 1
        signatures = {wire_signature(wires[ch["id"]]) for ch in present}
        if len(signatures) > 1:
            ability_signature_conflicts += 1
        if len(signatures) == 1:
            ability_unique_signature_missing_units += len(group) - len(present)

    prefab_signature_map: dict[str, set[str]] = defaultdict(set)
    for prefab, group in prefab_groups.items():
        for character in group:
            if character["id"] in wires:
                prefab_signature_map[prefab].add(wire_signature(wires[character["id"]]))

    missing_prefab_candidates = Counter()
    missing_combined_candidates = Counter()
    for character in characters:
        if character["id"] in wires:
            continue
        candidates = set()
        combined = set()
        for prefab in models(character):
            candidates.update(prefab_signature_map[prefab])
            for donor in prefab_groups[prefab]:
                if donor["id"] in wires and donor["ability"]["key"] == character["ability"]["key"]:
                    combined.add(wire_signature(wires[donor["id"]]))
        missing_prefab_candidates["none" if not candidates else "unique" if len(candidates) == 1 else "ambiguous"] += 1
        missing_combined_candidates[
            "none" if not combined else "unique" if len(combined) == 1 else "ambiguous"
        ] += 1

    preset_texture_names = {
        emitter["tex"] for preset in preset_root["presets"] for emitter in preset["emitters"] if emitter.get("tex")
    }
    particle_files = list(PARTICLES.glob("*.json"))
    particle_bundles = {path.stem.rsplit("__", 1)[0] for path in particle_files}
    build_report = json.loads(REPORT.read_text()) if REPORT.exists() else {}
    cab_paths = json.loads(CAB_PATHS.read_text())
    missing_with_cab = {
        unit_id
        for unit_id in missing_ids
        if any(cab_key(prefab) in cab_paths for prefab in models(by_id[unit_id]))
    }
    wisp_keys = {"WISP_ATTACK_ABILITY_KEY", "SUPER_WISP_ATTACK_ABILITY_KEY"}
    missing_wisps = {unit_id for unit_id in missing_ids if by_id[unit_id]["ability"]["key"] in wisp_keys}

    player = json.loads(PLAYER.read_text())
    campaigns = json.loads(CAMPAIGNS.read_text())
    live_ids = {
        unit_id
        for squad in player["squads"]
        for unit_id in squad
        if isinstance(unit_id, str)
    }
    live_ids.update(
        unit["id"]
        for campaign in campaigns["campaigns"]
        for node in campaign["nodes"]
        for unit in node["roster"]
        if isinstance(unit, dict) and isinstance(unit.get("id"), str)
    )

    exact_duplicate_presets = []
    for preset in preset_root["presets"]:
        serialized = [json.dumps(emitter, sort_keys=True) for emitter in preset["emitters"]]
        if len(set(serialized)) < len(serialized):
            exact_duplicate_presets.append(preset["name"])

    missing_fallback_projectiles = {
        unit_id
        for unit_id in missing_ids
        if any(
            needle in prefab.casefold()
            for prefab in models(by_id[unit_id])
            for needle in ("bolter", "crossbow", "ladyoftheforest", "emissary")
        )
    }
    direct_generic = {
        unit_id for unit_id in wire_ids if wires[unit_id].get("ability") == "fx_atk_lineslash_pre"
    }
    unresolved_generic = {
        unit_id for unit_id in wire_ids if resolution(wires[unit_id].get("ability")) == "generic_slash_fallback"
    }

    output = {
        "characters": {
            "catalog": len(characters),
            "wired_exact_unit_ids": len(wire_ids & catalog_ids),
            "missing_unit_ids": len(missing_ids),
            "missing_wisp_consumables": len(missing_wisps),
            "missing_battle_units": len(missing_ids - missing_wisps),
            "missing_with_a_local_model_cab": len(missing_with_cab),
            "missing_without_a_local_model_cab": len(missing_ids - missing_with_cab),
            "wire_ids_not_in_catalog": len(wire_ids - catalog_ids),
            "ability_vfx_resolution": dict(
                sorted(Counter(resolution(wires[unit_id].get("ability")) for unit_id in wire_ids & catalog_ids).items())
            ),
            "current_player_and_campaign_rosters": {
                "unique_units": len(live_ids),
                "wired": len(live_ids & wire_ids),
                "missing": sorted(live_ids - wire_ids),
            },
            "generic_slash_visual": {
                "unwired_fallback": len(missing_ids - missing_fallback_projectiles),
                "directly_wired_to_slash": len(direct_generic),
                "unresolved_bundle_fallback": len(unresolved_generic),
                "total": len(missing_ids - missing_fallback_projectiles) + len(direct_generic) + len(unresolved_generic),
            },
        },
        "wires": {
            "unique_signatures": len(signature_groups),
            "largest_shared_signatures": [
                {
                    "units": len(unit_ids),
                    "ability_vfx": wires[unit_ids[0]].get("ability"),
                    "sample_ids": sorted(unit_ids)[:6],
                }
                for _signature, unit_ids in sorted(signature_groups.items(), key=lambda pair: -len(pair[1]))[:8]
            ],
            "vfx_type": dict(sorted(Counter(wire.get("vfx_type") for wire in wires.values()).items())),
            "ability_type": dict(sorted(Counter(wire.get("ability_type") for wire in wires.values()).items())),
            "role_unique_names": {role: len(names) for role, names in role_unique.items()},
            "role_resolution_occurrences": {
                role: {
                    kind: role_occurrences[(role, kind)]
                    for kind in (
                        "hand_exact",
                        "generated_exact",
                        "alias_exact",
                        "family_fallback",
                        "generic_slash_fallback",
                    )
                    if role_occurrences[(role, kind)]
                }
                for role in ROLES
            },
        },
        "ability_keys": {
            "total": len(ability_groups),
            "coverage": dict(ability_coverage),
            "keys_with_multiple_unit_vfx_signatures": ability_signature_conflicts,
            "missing_units_with_one_ability_key_candidate": ability_unique_signature_missing_units,
            "conflict_examples": example_groups(ability_groups, wires),
        },
        "model_prefabs": {
            "unique": len(prefab_groups),
            "with_a_wired_donor": sum(bool(prefab_signature_map[prefab]) for prefab in prefab_groups),
            "with_conflicting_wired_signatures": sum(
                len(prefab_signature_map[prefab]) > 1 for prefab in prefab_groups
            ),
            "missing_unit_candidate_count": dict(missing_prefab_candidates),
            "missing_unit_prefab_plus_ability_candidate_count": dict(missing_combined_candidates),
            "conflict_examples": example_groups(prefab_groups, wires),
        },
        "presets": {
            "hand_exact": len(hand_names),
            "generated_exact": len(preset_names),
            "aliases": len(aliases),
            "hand_generated_overlap": len(hand_names & preset_names),
            "emitters": sum(len(preset["emitters"]) for preset in preset_root["presets"]),
            "presets_with_exact_duplicate_emitters": exact_duplicate_presets,
            "unique_textures_referenced": len(preset_texture_names),
            "unresolved_texture_refs": sorted(
                ref for ref in preset_texture_names if not original_cache.resolves(ref, original_cache.TEXTURE_2D)
            ),
            "build_missing": [
                name for name in build_report.get("missing", []) if name != "fx_null_pre"
            ],
            "build_warnings": dict(
                Counter(classify_warning(message) for message in build_report.get("warnings", []))
            ),
        },
        "reference_inventory": {
            "cab_files": sum(1 for path in SHARED.rglob("CAB-*") if path.is_file()),
            "particle_dumps": len(particle_files),
            "particle_bundles": len(particle_bundles),
            "cache_textures": len(original_cache.names(original_cache.TEXTURE_2D)),
        },
    }
    print(json.dumps(output, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
