#!/usr/bin/env python3

from __future__ import annotations

import argparse
import collections
import hashlib
import json
import math
import re
from pathlib import Path
from typing import Any

from build_unit_vfx import (
    MODEL_OVERRIDES,
    SEMANTIC_FAMILY_DONORS,
    TIERS,
    WIRE_FIELDS,
    character_family,
    family_key,
    identity_tokens,
    prefab_key,
    requested_prefab,
    wire_resources,
    wire_signature,
)


CLI_DESCRIPTION = 'Validate complete HoDA attack-VFX wiring and local preset coverage'
ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "data" / "unit_vfx.json"
DEFAULT_CATALOG = ROOT / "heroes-of-dragons-cache" / "characters.json"
DEFAULT_REFERENCE = ROOT / ".agents" / "skills" / "hoda-reference" / "data" / "characters.json"
DEFAULT_CAB_INDEX = ROOT / "heroes-of-dragons-cache" / "anim_tools" / "cache" / "cab_paths.json"
DEFAULT_PRESETS = ROOT / "data" / "fx_presets.json"

ROLES = ("ability", "impact", "muzzle", "sustained")
PROVENANCE = {
    "serialized_exact",
    "tier_fallback",
    "model_override",
    "family_fallback",
    "noncombat_fallback",
}
EXPECTED_RESOLUTION = {
    "family_fallback": 580,
    "model_override": 12,
    "noncombat_fallback": 52,
    "serialized_exact": 2054,
    "tier_fallback": 6,
}
EXPECTED_UNSTABLE = {
    "UNIT_CHOCOLATENUG",
    "UNIT_GAMORDANSTORMRIDER",
    "UNIT_MARYDEN",
    "UNIT_NOBLESPIRITOFHOPE",
    "UNIT_SPIRITOFCOLE",
    "UNIT_THELASTINQUISITOR",
}

REFERENCE_WIRE_SHA256 = "3433c72fdaac1948f27aa4cf5c5125f25add534e1901e4f0ba9e3b5ce55ee2fd"


def fail(message: str) -> None:
    raise ValueError(message)


def validate_wire(label: str, wire: dict[str, Any]) -> None:
    if set(WIRE_FIELDS) - set(wire):
        fail(f"{label}: missing wire fields {sorted(set(WIRE_FIELDS) - set(wire))}")
    ability_type = wire["ability_type"]
    vfx_type = wire["vfx_type"]
    if isinstance(ability_type, bool) or not isinstance(ability_type, int) or ability_type not in range(8):
        fail(f"{label}: ability_type must be 0..7")
    if isinstance(vfx_type, bool) or not isinstance(vfx_type, int) or vfx_type not in range(3):
        fail(f"{label}: vfx_type must be 0..2")
    for role in ROLES:
        value = wire[role]
        if value is not None and (not isinstance(value, str) or not value.strip() or value != value.strip()):
            fail(f"{label}: {role} must be null or a direct resource name")
        if isinstance(value, str) and value.startswith(("vfx.", "fx.")):
            fail(f"{label}: {role} retains a serialized namespace: {value!r}")
    if not any(wire[role] for role in ROLES):
        fail(f"{label}: no effect resource is bound")
    for field in ("projectile_speed", "cone_delay"):
        value = wire[field]
        if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value) or value < 0:
            fail(f"{label}: {field} must be a finite non-negative number")


def hand_preset_names() -> set[str]:
    source = (ROOT / "source" / "battle.c").read_text()
    start = source.index("static const FxPreset")
    end = source.index("static const FxPreset *BattleFxFindPreset")
    return set(re.findall(r'\.name\s*=\s*"([^"]+)"', source[start:end]))


def preset_particle_bound(preset: dict[str, Any]) -> float:

    total = 0.0
    for emitter in preset.get("emitters", []):
        curve_peak = max(
            (float(key[1]) for key in emitter.get("rate_curve", [])),
            default=1.0,
        )
        live = float(emitter.get("count", 0))
        live += (
            float(emitter.get("rate", 0))
            * max(curve_peak, 0.0)
            * float(emitter.get("life1", 0))
        )
        maximum = int(emitter.get("max_particles", 0))
        total += min(live, maximum) if maximum > 0 else live
    return total


def validate_preset_coverage(
    manifest: dict[str, Any], characters: dict[str, dict[str, Any]], preset_path: Path
) -> tuple[int, int, int]:
    root = json.loads(preset_path.read_text())
    presets_by_name = {preset["name"]: preset for preset in root.get("presets", [])}
    presets = set(presets_by_name)
    aliases = root.get("aliases", {})
    hands = hand_preset_names()
    if not isinstance(aliases, dict):
        fail("fx_presets.json: aliases must be an object")
    known = presets | hands
    for source, target in aliases.items():
        if not isinstance(source, str) or not isinstance(target, str):
            fail("fx_presets.json: aliases must map strings to strings")
        if target not in known:
            fail(f"fx_presets.json: alias {source!r} points to missing preset {target!r}")

    referenced: set[str] = set()
    unresolved: dict[str, list[str]] = collections.defaultdict(list)
    for unit_id, character in manifest.items():
        for tier, wire in character["tiers"].items():
            for role in ROLES:
                resource = wire[role]
                if not resource or resource == "fx_null_pre":
                    continue
                referenced.add(resource)
                if resource not in known and resource not in aliases:
                    unresolved[resource].append(f"{unit_id}/{tier}/{role}")
    if unresolved:
        details = ", ".join(f"{name}: {uses[:3]}" for name, uses in sorted(unresolved.items()))
        fail(f"unresolved VFX resources: {details}")

    oversized = {
        name: len(preset.get("emitters", []))
        for name, preset in presets_by_name.items()
        if len(preset.get("emitters", [])) > 24
    }
    if oversized:
        fail(f"presets exceed runtime emitter budget: {oversized}")
    erimond = presets_by_name.get("fx_lorderimond_imp")
    if not erimond or len(erimond.get("emitters", [])) != 17:
        fail("fx_lorderimond_imp must retain all 17 source emitters")

    max_budget = 0.0
    max_label = ""
    for unit_id, entry in manifest.items():
        target = characters[unit_id]["ability"]["target"]
        target_count = 5 if target == "All" else 3 if target in {"Row", "Column"} else 1
        for tier, wire in entry["tiers"].items():
            copies = 1 if wire["vfx_type"] == 2 or wire["ability_type"] in {3, 4, 5} else target_count

            def bound(resource: str | None) -> float:
                if not resource or resource == "fx_null_pre":
                    return 0.0
                name = aliases.get(resource, resource)
                return preset_particle_bound(presets_by_name[name]) if name in presets_by_name else 0.0

            budget = bound(wire["ability"]) * copies + bound(wire["muzzle"])
            if wire["vfx_type"] == 0:
                budget += (bound(wire["impact"]) + bound(wire["sustained"])) * copies
            if budget > max_budget:
                max_budget = budget
                max_label = f"{unit_id}/{tier}"
    if max_budget > 1024:
        fail(f"particle pool can overflow on {max_label}: conservative bound {max_budget:.1f} > 1024")
    return len(referenced), len(presets), math.ceil(max_budget)


def reference_wire_hash(manifest: dict[str, Any]) -> str:
    rows: list[list[Any]] = []
    for unit_id, character in sorted(manifest.items()):
        for tier, wire in sorted(character["tiers"].items()):
            if wire["provenance"] in {"family_fallback", "noncombat_fallback"}:
                continue
            rows.append([unit_id, tier, *[wire[field] for field in WIRE_FIELDS]])
    payload = json.dumps(rows, ensure_ascii=False, separators=(",", ":")).encode()
    return hashlib.sha256(payload).hexdigest()


def validate(
    manifest_path: Path,
    catalog_path: Path,
    reference_path: Path,
    cab_index_path: Path,
    preset_path: Path,
) -> dict[str, int]:
    manifest = json.loads(manifest_path.read_text())
    characters = json.loads(catalog_path.read_text())["characters"]
    references = json.loads(reference_path.read_text())
    cab_index = json.loads(cab_index_path.read_text())
    by_id = {character["id"]: character for character in characters}

    family_oracles = {
        ("unit.c_hw_possessedogrecommander_t4_pre", "Possessed Ogre Commander"): "creature:ogre",
        ("unit.c_snugthebronto_t4_pre", "Snug the Bronto"): "creature:bronto",
    }
    for (prefab, name), expected in family_oracles.items():
        actual = family_key(prefab, name)
        if actual != expected:
            fail(f"family classifier regression for {prefab}: expected {expected!r}, got {actual!r}")

    if set(manifest) != set(by_id):
        missing = sorted(set(by_id) - set(manifest))
        extra = sorted(set(manifest) - set(by_id))
        fail(f"character key mismatch; missing={missing[:5]} extra={extra[:5]}")

    counts: collections.Counter[str] = collections.Counter()
    noncombat = 0
    attack_tiers = 0
    unstable: set[str] = set()
    direct_units: set[str] = set()
    override_units: set[str] = set()
    for unit_id, entry in manifest.items():
        character = by_id[unit_id]
        reference = references[unit_id]
        if entry.get("manifest_version") != 2 or entry.get("selected_tier") != "IV":
            fail(f"{unit_id}: unsupported manifest version or compatibility tier")
        expected_noncombat = "ATTACK" not in character["ability"]["mechanics"]
        if entry.get("noncombat") is not expected_noncombat:
            fail(f"{unit_id}: noncombat annotation disagrees with ability mechanics")
        noncombat += int(expected_noncombat)
        if not expected_noncombat:
            attack_tiers += len(TIERS)

        tiers = entry.get("tiers")
        if not isinstance(tiers, dict) or set(tiers) != set(TIERS):
            fail(f"{unit_id}: tiers must contain exactly I, II, III, IV")
        for tier in TIERS:
            wire = tiers[tier]
            label = f"{unit_id}/{tier}"
            if not isinstance(wire, dict):
                fail(f"{label}: binding must be an object")
            validate_wire(label, wire)
            provenance = wire.get("provenance")
            if provenance not in PROVENANCE:
                fail(f"{label}: invalid provenance {provenance!r}")
            counts[provenance] += 1
            wanted = prefab_key(requested_prefab(reference, tier))
            source = wire.get("source_prefab")
            source_tier = wire.get("source_tier")
            if wire.get("requested_prefab") != wanted:
                fail(f"{label}: requested_prefab disagrees with reference catalog")
            expected_family = character_family(character, tier)
            if wire.get("family") != expected_family:
                fail(f"{label}: family disagrees with project model")
            if not isinstance(source, str):
                fail(f"{label}: source prefab must be a string")
            if provenance != "noncombat_fallback" and "cab-" + source not in cab_index:
                fail(f"{label}: source prefab has no local CAB: {source!r}")

            if provenance == "serialized_exact":
                direct_units.add(unit_id)
                if source != wanted or source_tier != tier:
                    fail(f"{label}: exact binding must use its requested tier prefab")
                if wire.get("reason") is not None or wire.get("source_unit") is not None:
                    fail(f"{label}: exact binding carries fallback metadata")
            elif provenance == "tier_fallback":
                direct_units.add(unit_id)
                if source_tier not in TIERS or source != prefab_key(requested_prefab(reference, source_tier)):
                    fail(f"{label}: tier fallback source is not another tier of this character")
                if source == wanted or not wire.get("reason"):
                    fail(f"{label}: tier fallback is unnecessary or unexplained")
            elif provenance == "model_override":
                override_units.add(unit_id)
                expected_override = prefab_key(MODEL_OVERRIDES.get(unit_id, ""))
                if not expected_override or source != expected_override or source_tier is not None:
                    fail(f"{label}: invalid renamed-model override")
                if not wire.get("reason"):
                    fail(f"{label}: model override has no reason")
            else:
                if provenance == "noncombat_fallback":
                    if not expected_noncombat:
                        fail(f"{label}: combat unit uses a noncombat fallback")
                    if source != wanted or source_tier != tier or wire.get("source_unit") is not None:
                        fail(f"{label}: noncombat fallback must identify its unavailable requested prefab")
                    expected_null = (1, "fx_null_pre", None, None, None, 0.0, 0.0)
                    actual_null = (
                        wire["vfx_type"],
                        wire["ability"],
                        wire["impact"],
                        wire["muzzle"],
                        wire["sustained"],
                        wire["projectile_speed"],
                        wire["cone_delay"],
                    )
                    if actual_null != expected_null or not wire.get("reason"):
                        fail(f"{label}: invalid neutral noncombat fallback")
                else:
                    source_unit = wire.get("source_unit")
                    if source_unit not in references or source_unit not in by_id or source_tier not in TIERS:
                        fail(f"{label}: family fallback has no exact donor owner")
                    donor_prefab = prefab_key(requested_prefab(references[source_unit], source_tier))
                    if source != donor_prefab:
                        fail(f"{label}: family fallback source does not match its donor owner")
                    donor_family = character_family(by_id[source_unit], source_tier)
                    catalog_hints = {
                        value.casefold()
                        for value in character.get("vfx", [])
                        if isinstance(value, str)
                    }
                    matched_hints = catalog_hints & wire_resources(wire)
                    shared_identity = identity_tokens(character) & identity_tokens(by_id[source_unit])
                    hint_supported = bool(matched_hints and shared_identity)
                    approved_bridge = SEMANTIC_FAMILY_DONORS.get(unit_id) == source_unit
                    if donor_family != expected_family and not hint_supported and not approved_bridge:
                        fail(
                            f"{label}: unsupported cross-family VFX donor: "
                            f"target={expected_family!r}, donor={donor_family!r}, "
                            f"source_unit={source_unit!r}"
                        )
                    if not wire.get("reason"):
                        fail(f"{label}: family fallback has no reason")

        validate_wire(unit_id, entry)
        for field in WIRE_FIELDS:
            if entry[field] != tiers["IV"][field]:
                fail(f"{unit_id}: flat {field} is not the selected tier-IV value")
        if entry.get("provenance") != tiers["IV"]["provenance"]:
            fail(f"{unit_id}: flat provenance is not the selected tier-IV value")
        expected_stable = len({wire_signature(wire) for wire in tiers.values()}) == 1
        if entry.get("tier_stable") is not expected_stable:
            fail(f"{unit_id}: stale tier_stable annotation")
        if not expected_stable:
            unstable.add(unit_id)

    if dict(sorted(counts.items())) != EXPECTED_RESOLUTION:
        fail(f"serialized resolution coverage changed: {dict(sorted(counts.items()))}")
    if attack_tiers != 2636 or noncombat != 17:
        fail(f"attack/noncombat coverage changed: attack_tiers={attack_tiers}, noncombat={noncombat}")
    if unstable != EXPECTED_UNSTABLE:
        fail(f"tier-varying character set changed: {sorted(unstable)}")
    if len(direct_units) != 515 or override_units != {"UNIT_GHASTARCHER", "UNIT_GHASTBRUTE", "UNIT_WILDSYLVAN"}:
        fail(f"direct/override source coverage changed: direct={len(direct_units)}, overrides={sorted(override_units)}")

    oracles = {
        "UNIT_ADVISORLELIANA": (1, 0, "hall_prjctl", "advisorleliana_imp_pre", "fx_hall_Mzl_pre"),
        "UNIT_BEHEMOTH": (2, 1, "fx_imp_Behemoth_pre", None, None),
        "UNIT_EMERALDSHALE": (0, 1, "fx_emeraldshale_pre", None, None),
        "UNIT_GHASTARCHER": (2, 1, "vfx_tripplehit_pre", None, None),
        "UNIT_NECROMANCERDORIAN": (2, 1, "fx_NecroDorian_pre", None, None),
        "UNIT_REDLYRIUMSAMSON": (7, 1, "fx_RedLyriumSamson.pre", None, None),
        "UNIT_SHAPESHIFTERMORRIGANI": (4, 1, "fx_wakingnightmare_pre", None, None),
        "UNIT_SHAPESHIFTERMORRIGANIV": (2, 1, "fx_overwhelm_pre", None, None),
        "UNIT_WILDSYLVAN": (0, 1, "fx_lrgcrtr_massiveattack_pre", None, None),
    }
    for unit_id, expected in oracles.items():
        wire = manifest[unit_id]
        actual = (wire["ability_type"], wire["vfx_type"], wire["ability"], wire["impact"], wire["muzzle"])
        if actual != expected:
            fail(f"{unit_id}: reference oracle changed: expected {expected}, got {actual}")

    fallback_oracles = {
        "UNIT_POSSESSEDOGRECOMMANDER": (
            "family_fallback",
            "creature:ogre",
            "UNIT_OGRECOMMANDER",
            "vfx_ogresmash_pre",
        ),
        "UNIT_GREYWARDENBANNER": (
            "noncombat_fallback",
            "creature:banner",
            None,
            "fx_null_pre",
        ),
        "UNIT_REDLYRIUMIRONBULL": (
            "family_fallback",
            "humanoid:qm:tw",
            "UNIT_IRONBULL",
            "fx_ironbull_pre",
        ),
    }
    for unit_id, expected in fallback_oracles.items():
        for tier, wire in manifest[unit_id]["tiers"].items():
            actual = (
                wire["provenance"],
                wire["family"],
                wire.get("source_unit"),
                wire["ability"],
            )
            if actual != expected:
                fail(f"{unit_id}/{tier}: semantic fallback oracle changed: expected {expected}, got {actual}")

    digest = reference_wire_hash(manifest)
    if REFERENCE_WIRE_SHA256 != "TO_BE_FILLED" and digest != REFERENCE_WIRE_SHA256:
        fail(f"direct reference wire snapshot changed: expected {REFERENCE_WIRE_SHA256}, got {digest}")

    resources, presets, max_particle_budget = validate_preset_coverage(manifest, by_id, preset_path)
    return {
        "characters": len(manifest),
        "tier_bindings": sum(len(entry["tiers"]) for entry in manifest.values()),
        "attack_tiers": attack_tiers,
        "noncombat": noncombat,
        "resources": resources,
        "presets": presets,
        "max_particle_budget": max_particle_budget,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--reference", type=Path, default=DEFAULT_REFERENCE)
    parser.add_argument("--cab-index", type=Path, default=DEFAULT_CAB_INDEX)
    parser.add_argument("--presets", type=Path, default=DEFAULT_PRESETS)
    args = parser.parse_args()
    counts = validate(args.manifest, args.catalog, args.reference, args.cab_index, args.presets)
    print(
        "unit VFX: ok "
        f"({counts['characters']} characters, {counts['tier_bindings']} tier bindings, "
        f"{counts['attack_tiers']} attack-capable, {counts['noncombat']} noncombat, "
        f"{counts['resources']} resources, {counts['presets']} converted presets, "
        f"worst particle bound {counts['max_particle_budget']}/1024)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
