#!/usr/bin/env python3

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

import build_intrinsic_passives as generator


ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "heroes-of-dragons-cache" / "characters.json"
REFERENCE = ROOT / ".agents" / "skills" / "hoda-reference" / "data" / "characters.json"
MANIFEST = ROOT / "data" / "intrinsic_passives.json"
MAX_TERMS = 16


def fail(message: str) -> None:
    raise SystemExit(f"intrinsic passive validation failed: {message}")


catalog = json.loads(CATALOG.read_text())
reference = json.loads(REFERENCE.read_text())
root = json.loads(MANIFEST.read_text())
regenerated = generator.build(CATALOG, REFERENCE)
if root != regenerated:
    fail("manifest is stale; run python3 tools/build_intrinsic_passives.py")

if root.get("version") != 1:
    fail(f"expected version 1, got {root.get('version')!r}")
rows = catalog["characters"]
catalog_by_id = {row["id"]: row for row in rows}
if len(rows) != 676 or len(catalog_by_id) != 676 or len(reference) != 676:
    fail("authoritative 676-character catalog drifted")

manifest_rows = root.get("characters")
if not isinstance(manifest_rows, list):
    fail("characters is not an array")
ids = [row.get("id") for row in manifest_rows]
if ids != sorted(ids) or len(ids) != len(set(ids)):
    fail("manifest character IDs must be unique and sorted")
if not set(ids) <= set(catalog_by_id):
    fail(f"unknown manifest IDs: {sorted(set(ids) - set(catalog_by_id))}")

coverage = root.get("coverage", {})
expected_coverage = {
    "catalog_characters": 676,
    "manifest_characters": 587,
    "supported_battle_start_stat_clauses": 704,
    "unresolved_battle_start_stat_clauses": 0,
    "supported_power_vs_target_clauses": 3,
    "unresolved_power_vs_target_clauses": 0,
    "supported_enemy_battle_start_stat_clauses": 78,
    "unresolved_enemy_battle_start_stat_clauses": 0,
    "target_weight_descriptions": 173,
    "target_weight_effects": 173,
}
if coverage != expected_coverage:
    fail(f"coverage drifted: expected {expected_coverage}, got {coverage}")

groups = {group for row in rows for group in row["groups"]}
factions = {"Black", "Blue", "Red", "White"}
specialties = {specialty for row in rows for specialty in row["specialties"]}
rarities = {row["rarity"] for row in rows}
speeds = {row["speed"] for row in rows}
allowed_tags = {"GIANT", "1STAR", "2STAR", "3STAR"}
allowed_kinds = {
    "stat_percent",
    "stat_percent_per_matching_ally",
    "incoming_target_weight",
    "outgoing_target_preference",
    "heal_percent_max_health",
    "bonus_damage_percent",
}


def validate_match(owner: str, match: dict[str, Any]) -> None:
    if match.get("mode") not in {"any", "each"}:
        fail(f"{owner}: invalid match mode {match.get('mode')!r}")
    terms = match.get("terms")
    if not isinstance(terms, list) or not 1 <= len(terms) <= MAX_TERMS:
        fail(f"{owner}: match term count must be 1..{MAX_TERMS}, got {len(terms or [])}")
    for term in terms:
        kind = term.get("kind")
        value = term.get("value")
        valid = {
            "group": value in groups,
            "faction": value in factions,
            "character_id": value in catalog_by_id,
            "specialty": value in specialties,
            "rarity": value in rarities,
            "speed": value in speeds,
            "tag": value in allowed_tags,
        }.get(kind, False)
        if not valid:
            fail(f"{owner}: invalid match term {term!r}")


def validate_recipient(owner: str, recipient: dict[str, Any], scopes: set[str]) -> None:
    if recipient.get("scope") not in scopes:
        fail(f"{owner}: invalid recipient scope {recipient.get('scope')!r}")
    if "match" in recipient:
        validate_match(owner, recipient["match"])


by_id: dict[str, dict[str, Any]] = {}
target_effect_count = 0
all_effect_ids: set[str] = set()
unresolved_rows: list[tuple[str, str, str]] = []
for manifest_row in manifest_rows:
    unit_id = manifest_row["id"]
    by_id[unit_id] = manifest_row
    source = manifest_row.get("source", {})
    catalog_row = catalog_by_id[unit_id]
    expected_source = {
        "equipped_rune_key": catalog_row.get("equipped_rune", ""),
        "equipped_rune_text": catalog_row.get("equipped_rune_text", ""),
        "target_weight_key": catalog_row.get("target_weight", ""),
        "target_weight_text": catalog_row.get("target_weight_text", ""),
    }
    if source != expected_source:
        fail(f"{unit_id}: source data drift")
    effects = manifest_row.get("effects")
    unresolved = manifest_row.get("unresolved")
    if not isinstance(effects, list) or not isinstance(unresolved, list) or not effects and not unresolved:
        fail(f"{unit_id}: row must contain effects or an explicit unresolved clause")
    for item in unresolved:
        unresolved_rows.append((unit_id, item.get("kind", ""), item.get("reason", "")))
        if not item.get("source_text") or not item.get("reason"):
            fail(f"{unit_id}: malformed unresolved entry")
    for effect in effects:
        effect_id = effect.get("id")
        if not effect_id or effect_id in all_effect_ids:
            fail(f"{unit_id}: duplicate or missing effect id {effect_id!r}")
        all_effect_ids.add(effect_id)
        kind = effect.get("kind")
        if kind not in allowed_kinds:
            fail(f"{unit_id}: unsupported effect kind {kind!r}")
        if not effect.get("source_text") or effect.get("confidence") not in {
            "explicit",
            "key_encoded",
            "reconstructed",
        }:
            fail(f"{effect_id}: missing provenance")
        if kind == "stat_percent":
            if effect.get("trigger") != "battle_start" or effect.get("stacking") != "additive":
                fail(f"{effect_id}: static stat phase/stacking drift")
            stats = effect.get("stats")
            value = effect.get("value_pct")
            if not isinstance(stats, list) or not stats or not set(stats) <= {"health", "power"}:
                fail(f"{effect_id}: invalid stats {stats!r}")
            if not isinstance(value, int) or value == 0 or not -2000 <= value <= 2000:
                fail(f"{effect_id}: invalid signed percent {value!r}")
            validate_recipient(effect_id, effect.get("recipient", {}), {"self", "allies", "enemies"})
            if value < 0 and effect["recipient"]["scope"] != "enemies":
                fail(f"{effect_id}: negative battle-start stat must target enemies")
            if value > 0 and effect["recipient"]["scope"] == "enemies":
                fail(f"{effect_id}: positive battle-start stat cannot target enemies")
            match = effect["recipient"].get("match")
            if match and match["mode"] == "each" and not any(
                word in effect["source_text"].casefold() for word in ("stacking", "stacks")
            ):
                fail(f"{effect_id}: per-match stacking lacks explicit source wording")
        elif kind == "stat_percent_per_matching_ally":
            if effect.get("trigger") != "battle_start" or effect.get("stacking") != "additive":
                fail(f"{effect_id}: composition stat phase/stacking drift")
            stats = effect.get("stats")
            value = effect.get("value_pct")
            if not isinstance(stats, list) or not stats or not set(stats) <= {"health", "power"}:
                fail(f"{effect_id}: invalid composition stats {stats!r}")
            if not isinstance(value, int) or not 1 <= value <= 2000:
                fail(f"{effect_id}: invalid composition percent {value!r}")
            recipient = effect.get("recipient", {})
            validate_recipient(effect_id, recipient, {"self"})
            if recipient != {"scope": "self"}:
                fail(f"{effect_id}: composition stat recipient must be self")
            count = effect.get("count", {})
            validate_recipient(effect_id + " count", count, {"allies"})
            if count.get("include_source") is not False or "match" not in count:
                fail(f"{effect_id}: counted allies must explicitly exclude the source and have a match")
            if "for each" not in effect["source_text"].casefold():
                fail(f"{effect_id}: composition count lacks explicit source wording")
        elif kind == "incoming_target_weight":
            target_effect_count += 1
            validate_recipient(effect_id, effect.get("recipient", {}), {"self"})
            if effect.get("trigger") != "target_selection" or effect.get("value_multiplier") not in {
                0.25,
                0.5,
                2.0,
                4.0,
            }:
                fail(f"{effect_id}: invalid incoming target weight")
        elif kind == "outgoing_target_preference":
            validate_recipient(effect_id, effect.get("recipient", {}), {"enemies"})
            if effect.get("selector") != "lowest_health_percent" or effect.get("value_multiplier") != 2.0:
                fail(f"{effect_id}: unsupported outgoing preference")
        elif kind == "heal_percent_max_health":
            validate_recipient(effect_id, effect.get("recipient", {}), {"self"})
            if effect.get("trigger") != "enemy_death" or effect.get("value_pct") != 25:
                fail(f"{effect_id}: unsupported death-heal recipe")
        elif kind == "bonus_damage_percent":
            validate_recipient(effect_id, effect.get("recipient", {}), {"self"})
            target = effect.get("target", {})
            value = effect.get("value_pct")
            if (
                effect.get("trigger") != "before_damage"
                or not isinstance(value, int)
                or not 1 <= value <= 2000
                or effect.get("stacking") != "additive"
            ):
                fail(f"{effect_id}: unsupported conditional damage recipe")
            if target.get("scope") != "enemies" or "match" not in target:
                fail(f"{effect_id}: conditional damage target missing")
            validate_match(effect_id + " target", target["match"])

if target_effect_count != 173:
    fail(f"expected 173 target-weight effects, got {target_effect_count}")
for row in rows:
    key = row.get("target_weight", "")
    actual = [effect for effect in by_id.get(row["id"], {}).get("effects", []) if effect["kind"] == "incoming_target_weight"]
    if bool(key) != bool(actual):
        fail(f"{row['id']}: target-weight effect presence drift")
    if key:
        expected_multiplier = generator.TARGET_WEIGHTS[key][0]
        if len(actual) != 1 or actual[0]["value_multiplier"] != expected_multiplier:
            fail(f"{row['id']}: target-weight multiplier drift")

expected_unresolved: set[tuple[str, str, str]] = set()
if set(unresolved_rows) != expected_unresolved or len(unresolved_rows) != len(expected_unresolved):
    fail(f"unresolved classification drift: {unresolved_rows!r}")
if any("grammar" in reason or reason.startswith("unsupported recipient:") for _, _, reason in unresolved_rows):
    fail("straightforward literal group/faction/ID stat clause remained unresolved")


def effects(unit_id: str, kind: str) -> list[dict[str, Any]]:
    return [effect for effect in by_id[unit_id]["effects"] if effect["kind"] == kind]


def terms(effect: dict[str, Any], field: str = "recipient") -> set[tuple[str, str]]:
    return {
        (term["kind"], term["value"])
        for term in effect[field].get("match", {}).get("terms", [])
    }


envy = by_id["UNIT_ENVYDEMON"]
if len(envy["effects"]) != 7 or envy["unresolved"]:
    fail("Envy Demon must have exactly seven resolved mechanics")
envy_stats = effects("UNIT_ENVYDEMON", "stat_percent")
if not any(e["stats"] == ["health"] and e["value_pct"] == 50 and terms(e) == {("group", "Demon")} for e in envy_stats):
    fail("Envy Demon +50% Demon Health missing")
if not any(
    e["stats"] == ["power"]
    and e["value_pct"] == 50
    and e["recipient"]["match"]["mode"] == "any"
    and terms(e) == {("group", "Demon"), ("group", "Red_Lyrium")}
    for e in envy_stats
):
    fail("Envy Demon +50% Demon/Red Lyrium Power union missing")
if not any(e["stats"] == ["power"] and e["value_pct"] == 25 and terms(e) == {("character_id", "UNIT_LUCIUSCORIN")} for e in envy_stats):
    fail("Envy Demon +25% Lucius Power missing")
if effects("UNIT_ENVYDEMON", "incoming_target_weight")[0]["value_multiplier"] != 0.25:
    fail("Envy Demon must be very elusive (0.25 incoming target weight)")
if effects("UNIT_ENVYDEMON", "heal_percent_max_health")[0]["value_pct"] != 25:
    fail("Envy Demon 25% enemy-death heal missing")
envy_damage = effects("UNIT_ENVYDEMON", "bonus_damage_percent")[0]
if envy_damage["value_pct"] != 35 or terms(envy_damage, "target") != {("specialty", "POWERGAIN")}:
    fail("Envy Demon +35% damage vs Power gainers missing")
envy_preference = effects("UNIT_ENVYDEMON", "outgoing_target_preference")[0]
if envy_preference["selector"] != "lowest_health_percent" or envy_preference["value_multiplier"] != 2.0:
    fail("Envy Demon wounded-target preference missing")

blue_nug = effects("UNIT_BLUENUGLYSWEATER", "stat_percent")
if not any(e["stats"] == ["health"] and e["value_pct"] == 25 and terms(e) == {("tag", "GIANT")} for e in blue_nug):
    fail("Blue Nugly Sweater must use one GIANT predicate, not an expanded ID list")
cassandra = effects("UNIT_CASSANDRAPENTAGHAST", "stat_percent")
if not any(e["stats"] == ["power"] and e["value_pct"] == -5 and e["recipient"] == {"scope": "enemies"} for e in cassandra):
    fail("Cassandra -5% enemy Power setup debuff missing")
amber = effects("UNIT_AMBERSHALE", "stat_percent")
if not any(e["stats"] == ["health", "power"] and e["value_pct"] == -10 and terms(e) == {("group", "Blight")} for e in amber):
    fail("Amber Shale filtered enemy Health/Power debuff missing")
star_debuffs = [e for e in effects("UNIT_EMERALDGRAVESNUG", "stat_percent") if e["value_pct"] < 0]
if {(e["value_pct"], next(iter(terms(e)))) for e in star_debuffs} != {
    (-20, ("tag", "3STAR")),
    (-15, ("tag", "2STAR")),
    (-10, ("tag", "1STAR")),
}:
    fail("Emerald Graves Nug tiered star Power debuffs missing")
hivernal = [e for e in effects("UNIT_HIVERNAL", "stat_percent") if e["value_pct"] < 0]
if not any(e["stats"] == ["power"] and e["value_pct"] == -15 and terms(e) == {("speed", "Quick")} for e in hivernal):
    fail("Hivernal -15% Power to Quick-round enemies missing")

avvar_nug = effects("UNIT_AVVARWARNUG", "stat_percent_per_matching_ally")
if len(avvar_nug) != 1:
    fail("Avvar War Nug must have one composition-scaled Health effect")
avvar_count = avvar_nug[0]["count"]
if (
    avvar_nug[0]["stats"] != ["health"]
    or avvar_nug[0]["value_pct"] != 35
    or avvar_nug[0]["recipient"] != {"scope": "self"}
    or avvar_count.get("scope") != "allies"
    or avvar_count.get("include_source") is not False
    or terms(avvar_nug[0], "count") != {("group", "Avvar")}
):
    fail("Avvar War Nug composition-scaled Health semantics drifted")

power_vs_cases = {
    "UNIT_MALIPHANT": (35, {("group", "Inquisition")}),
    "UNIT_NIGHTMAREASPECT": (15, {("group", "Mage"), ("group", "Spirit")}),
    "UNIT_THELOST": (30, {("group", "Blight")}),
}
for unit_id, (value, expected_terms) in power_vs_cases.items():
    conditional = [
        effect
        for effect in effects(unit_id, "bonus_damage_percent")
        if " power vs " in f" {effect['source_text'].casefold()} "
    ]
    if len(conditional) != 1:
        fail(f"{unit_id}: expected one resolved Power-vs-target effect")
    effect = conditional[0]
    if (
        effect["value_pct"] != value
        or effect["recipient"] != {"scope": "self"}
        or effect["target"].get("scope") != "enemies"
        or effect["target"].get("match", {}).get("mode") != "any"
        or terms(effect, "target") != expected_terms
    ):
        fail(f"{unit_id}: Power-vs-target semantics drifted")

print(
    "intrinsic passives validated: "
    f"{len(manifest_rows)} characters, {len(all_effect_ids)} effects, "
    f"{coverage['supported_battle_start_stat_clauses']} ally stat clauses, "
    f"{coverage['supported_enemy_battle_start_stat_clauses']} enemy stat clauses, "
    f"{len(unresolved_rows)} explicitly unresolved"
)
