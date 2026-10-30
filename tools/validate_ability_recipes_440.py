#!/usr/bin/env python3

from __future__ import annotations

import json
from typing import Any

from build_ability_recipes_440 import (
    DEFAULT_OUTPUT,
    DEFAULT_RECIPES,
    DEFAULT_UNITS,
    LOCALIZATION_DIR,
    TARGET_ENUM,
    build,
)


VALID_TARGETS = set(TARGET_ENUM.values()) | {"one_ally", "back_row", "any"}


def fail(message: str) -> None:
    raise SystemExit(f"4.4 ability recipe validation failed: {message}")


def validate_step(recipe_key: str, index: int, step: dict[str, Any]) -> None:
    chance = step.get("chance_pct")
    repeat = step.get("repeat")
    if not isinstance(chance, int) or not 1 <= chance <= 100:
        fail(f"{recipe_key} step {index}: invalid chance_pct={chance!r}")
    if not isinstance(repeat, int) or not 1 <= repeat <= 10:
        fail(f"{recipe_key} step {index}: invalid repeat={repeat!r}")
    hit = step.get("attack")
    if not isinstance(hit, dict) or hit.get("target") not in VALID_TARGETS:
        fail(f"{recipe_key} step {index}: invalid attack target")
    for field, value in hit.items():
        if field in {"target", "status_only"}:
            continue
        if not isinstance(value, int) or not -200 <= value <= 500:
            fail(f"{recipe_key} step {index}: invalid {field}={value!r}")
    effects = step.get("effects", {})
    if not isinstance(effects, dict):
        fail(f"{recipe_key} step {index}: effects is not an object")
    for field, value in effects.items():
        if not isinstance(value, int) or not -500 <= value <= 500:
            fail(f"{recipe_key} step {index}: invalid effect {field}={value!r}")


root = json.loads(DEFAULT_OUTPUT.read_text())
expected = build(DEFAULT_UNITS, DEFAULT_RECIPES, LOCALIZATION_DIR)
if root != expected:
    fail("generated file has drifted; run tools/build_ability_recipes_440.py")

if root.get("version") != 1 or root.get("catalog_version") != "4.4.0":
    fail("unexpected manifest version")
recipes = root.get("recipes")
if (
    not isinstance(recipes, list)
    or len(recipes) != 789
    or root.get("recipe_count") != 789
):
    fail(
        f"expected 789 chain recipes, got {len(recipes) if isinstance(recipes, list) else 'invalid'}"
    )

by_key: dict[str, dict[str, Any]] = {}
all_definitions: set[int] = set()
all_promote_groups: set[int] = set()
for recipe in recipes:
    semantic_key = recipe.get("semantic_recipe_key")
    promote_group = recipe.get("promote_group")
    if semantic_key != f"440:{promote_group}":
        fail(f"promote group {promote_group}: unstable semantic key {semantic_key!r}")
    if semantic_key in by_key:
        fail(f"duplicate semantic key {semantic_key}")
    by_key[semantic_key] = recipe
    if promote_group in all_promote_groups:
        fail(f"duplicate promote group {promote_group}")
    all_promote_groups.add(promote_group)

    definition_ids = recipe.get("definition_ids")
    if (
        not isinstance(definition_ids, list)
        or len(definition_ids) != 4
        or len(set(definition_ids)) != 4
    ):
        fail(f"{semantic_key}: promotion chain must contain four unique definitions")
    overlap = all_definitions.intersection(definition_ids)
    if overlap:
        fail(f"{semantic_key}: definitions occur in multiple chains: {sorted(overlap)}")
    all_definitions.update(definition_ids)

    if recipe.get("target_enum") not in TARGET_ENUM:
        fail(f"{semantic_key}: invalid target enum")
    if recipe.get("target") != TARGET_ENUM[recipe["target_enum"]]:
        fail(f"{semantic_key}: target label does not match target enum")
    if not recipe.get("unitdef_ability_id"):
        fail(f"{semantic_key}: missing UnitDef field-15 ability id")
    if recipe.get("action_id_template") != "{battle_id}-action-{sequence}":
        fail(f"{semantic_key}: action-id template drifted")
    if recipe.get("resolution_source") not in {
        "client_4.4_localization",
        "client_4.4_key_mapping",
        "specialty_fallback",
    }:
        fail(f"{semantic_key}: unknown resolution source")
    if recipe["resolution_source"].startswith("client_4.4_") and not recipe.get(
        "localized_text"
    ):
        fail(f"{semantic_key}: exact-client recipe has no localized text")
    if recipe["resolution_source"] == "specialty_fallback" and recipe.get(
        "localized_text"
    ):
        fail(f"{semantic_key}: fallback recipe incorrectly claims exact localized text")
    if recipe["resolution_source"] == "client_4.4_key_mapping":
        if not recipe.get("mapping_key") or not recipe.get("mapped_ability_id"):
            fail(f"{semantic_key}: mapped recipe lacks its exact mapping provenance")
    if recipe["resolution_source"] == "specialty_fallback" and not recipe.get(
        "ambiguous"
    ):
        fail(f"{semantic_key}: specialty fallback is not marked ambiguous")
    steps = recipe.get("steps")
    if not isinstance(steps, list) or len(steps) > 8:
        fail(f"{semantic_key}: invalid steps array")
    for index, step in enumerate(steps):
        validate_step(semantic_key, index, step)

if len(all_definitions) != 3156:
    fail(f"expected 3156 covered UnitDefs, got {len(all_definitions)}")
if set(root["definition_to_recipe"]) != {str(value) for value in all_definitions}:
    fail("definition lookup does not cover the same UnitDefs as recipes")
if set(root["promote_group_to_recipe"]) != {str(value) for value in all_promote_groups}:
    fail("promote-group lookup does not cover the same chains as recipes")
for definition_id, semantic_key in root["definition_to_recipe"].items():
    if int(definition_id) not in by_key[semantic_key]["definition_ids"]:
        fail(
            f"definition lookup {definition_id}->{semantic_key} points to the wrong chain"
        )
for promote_group, semantic_key in root["promote_group_to_recipe"].items():
    if int(promote_group) != by_key[semantic_key]["promote_group"]:
        fail(
            f"promote-group lookup {promote_group}->{semantic_key} points to the wrong chain"
        )

audit = root.get("audit", {})
expected_audit = {
    "unit_definition_count": 3156,
    "promote_group_count": 789,
    "ability_key_count": 312,
    "direct_localized_ability_key_count": 188,
    "mapped_ability_key_count": 123,
    "resolved_ability_key_count": 311,
    "missing_ability_key_count": 1,
    "passive_key_count": 325,
    "localized_passive_key_count": 325,
}
for field, expected_value in expected_audit.items():
    if audit.get(field) != expected_value:
        fail(f"audit {field}: expected {expected_value}, got {audit.get(field)!r}")
if sum(audit.get("resolution_counts", {}).values()) != 789:
    fail("resolution counts do not sum to 789")
if audit.get("resolution_counts") != {
    "client_4.4_key_mapping": 516,
    "client_4.4_localization": 272,
    "specialty_fallback": 1,
}:
    fail(f"unexpected resolution coverage {audit.get('resolution_counts')!r}")


def one_by_unitdef(key: str) -> dict[str, Any]:
    matches = [recipe for recipe in recipes if recipe["unitdef_ability_id"] == key]
    if len(matches) != 1:
        fail(
            f"expected one chain for semantic parity fixture {key}, got {len(matches)}"
        )
    return matches[0]


winters = next(
    recipe
    for recipe in recipes
    if recipe["unitdef_ability_id"] == "WINTERS_GRASP_ABILITY_KEY"
)
if (
    winters["mapped_ability_id"] != "ABILITY_DAMAGE_INIT_50"
    or winters["steps"][0]["attack"].get("slow_amt") != -50
):
    fail("WINTERS_GRASP key mapping must resolve to the exact one-target slow recipe")

bile_bursts = [
    recipe
    for recipe in recipes
    if recipe["unitdef_ability_id"] == "BILE_BURST_ABILITY_KEY"
]
if len(bile_bursts) != 6 or any(
    recipe["mapped_ability_id"] != "ABILITY_DAMAGE_INIT_50_ALL"
    or recipe["steps"][0]["attack"].get("slow_amt") != -50
    for recipe in bile_bursts
):
    fail("Bile Burst title must redirect to its exact all-target slow mapping")

slam = one_by_unitdef("OVERWHELMING_SLAM_ABILITY_KEY")
slam_effects = slam["steps"][0].get("effects", {})
if (
    slam["mapped_ability_id"] != "ABILITY_DAMAGE_ALL_POWERGAIN25_HEAL25"
    or slam_effects.get("self_power_chance") != 25
    or slam_effects.get("self_heal_chance") != 25
):
    fail("Overwhelming Slam title must redirect to two independent 25% procs")

drain_again = one_by_unitdef("damage_all_and_drainpower_attack2x_chance10")
if drain_again["steps"][1]["chance_pct"] != 10:
    fail("encoded 10% all-target drain follow-up chance was lost")

friendly = one_by_unitdef("damage_all_powergain15_heal5_attack_own_all_5")
friendly_fx = friendly["steps"][1].get("effects", {})
if friendly_fx.get("ally_damage_pct") != -5 or friendly_fx.get("self_damage_pct") != -5:
    fail("own-squad 5% attack must damage allies and self, not enemies")

assassinate = one_by_unitdef("attack_one_assassinate_10pct")
if assassinate["steps"][0]["attack"].get("assassinate_pct") != 10:
    fail("encoded 10% assassinate chance was lost")

architect = one_by_unitdef("damage_heal_self_slow_curse_column_heal_all")
if (
    architect["steps"][0]["attack"].get("curse_health_pct")
    or architect["steps"][0].get("effects", {}).get("ally_heal_pct")
    or architect["steps"][1]["attack"].get("curse_health_pct") != 100
    or architect["steps"][1].get("effects", {}).get("ally_heal_pct") != 25
):
    fail("Architect curse and ally heal must execute in the column phase")

spirit_cole = one_by_unitdef("ability_attack_row_slow50_assassinate_single_pct10")
if (
    spirit_cole["steps"][0]["attack"].get("assassinate_pct")
    or spirit_cole["steps"][1]["chance_pct"] != 10
    or spirit_cole["steps"][1]["attack"].get("assassinate_pct") != 100
):
    fail("Spirit of Cole assassination must execute on the 10% follow-up")

red_shadow = one_by_unitdef("damage_single_twice_chance_assassinate")
if (
    red_shadow["steps"][0]["attack"].get("assassinate_pct")
    or red_shadow["steps"][1]["attack"].get("assassinate_pct") != 100
):
    fail("Red Templar Shadow assassination must execute on attack three")

duhaime = one_by_unitdef("damage_column_then_row_and_gainpower")
if (
    duhaime["steps"][1]["chance_pct"] != 100
    or duhaime["steps"][1]["attack"].get("target") != "row"
):
    fail("Commander Duhaime must unconditionally follow the column with a row")

the_first = one_by_unitdef("damage_gainpower_drainpower_cursepower")
if (
    the_first["steps"][0]["attack"].get("drain_pct")
    or the_first["steps"][0]["effects"].get("self_power_pct") != 50
    or the_first["steps"][1]["attack"].get("drain_pct") != 50
):
    fail("The First's medium Power gain/drain must use the column phase at 50%")

swiftrunner = one_by_unitdef("ABILITY_DAMAGE_ONE_INCREASE_POWER_FOLLOWUP2X_CURSEHEALTH")
if (
    swiftrunner["steps"][0]["attack"].get("curse_health_pct")
    or swiftrunner["steps"][1]["attack"].get("curse_health_pct") != 100
):
    fail("Swiftrunner's Health curse must execute on the follow-up")

cole = one_by_unitdef("damage_one_10x_small_chance_neg_aliment")
if any(
    step["attack"].get("random_ailment_pct") != 25
    or step["attack"].get("random_ailment_mask") != 0b11111
    for step in cole["steps"]
):
    fail("Cole must roll one of all five authored ailments on every hit")

barkspawn = one_by_unitdef("BRUTAL_MAUL_ABILITY_KEY")
if (
    barkspawn["steps"][0]["attack"].get("random_ailment_mask") != 0b00111
    or barkspawn["steps"][0]["attack"].get("random_drain_pct") != 10
    or any(
        field in barkspawn["steps"][0]["attack"]
        for field in ("stun_pct", "slow_pct", "drain_pct")
    )
):
    fail("Barkspawn must choose one of stun, slow, or encoded 10% drain")

spiderling = one_by_unitdef("ABILITY_DAMAGE_ONE_STUN25_SLOW25")
if (
    spiderling["steps"][0]["attack"].get("random_ailment_pct") != 50
    or spiderling["steps"][0]["attack"].get("random_ailment_mask") != 0b00011
    or any(
        field in spiderling["steps"][0]["attack"] for field in ("stun_pct", "slow_pct")
    )
):
    fail("Corrupted Spiderling must choose stun or slow, never apply both")

pride = one_by_unitdef("UNPREDICTABLE_PRIDE_ABILITY_KEY")
if (
    pride["steps"][0]["attack"].get("exclusive_power_pct") != 25
    or pride["steps"][0]["attack"].get("exclusive_power_chance") != 25
    or pride["steps"][0]["attack"].get("drain_pct")
    or pride["steps"][0].get("effects", {}).get("self_power_pct")
):
    fail("Pride Abomination must choose gain OR drain, never apply both")

red_jenny = one_by_unitdef("damage_one_5x_150")
if (
    len(red_jenny["steps"]) != 1
    or red_jenny["steps"][0].get("repeat") != 5
    or red_jenny["steps"][0].get("final_damage_pct") != 150
):
    fail("Red Jenny Sera must strike five total times with an upgraded final hit")

ancient_darkspawn = one_by_unitdef("damage_one_drain_all_gain_health_all")
if (
    ancient_darkspawn["steps"][0]["attack"].get("drain_pct")
    or ancient_darkspawn["steps"][1]["attack"].get("target") != "all"
    or ancient_darkspawn["steps"][1]["attack"].get("drain_pct") != 50
):
    fail("Ancient Darkspawn must drain all enemies in a separate phase")

ruby_shale = one_by_unitdef("damage_one_curseheal_column_cursepower_row")
if [step["attack"]["target"] for step in ruby_shale["steps"]] != [
    "one",
    "column",
    "row",
] or (
    ruby_shale["steps"][1]["attack"].get("curse_health_pct") != 100
    or ruby_shale["steps"][2]["attack"].get("curse_power_pct") != 100
):
    fail("Ruby Shale curses must use their authored column and row phases")

josephine = one_by_unitdef("damage_one_heal_all_small_curse_health_row")
if (
    josephine["steps"][0]["attack"].get("curse_health_pct")
    or josephine["steps"][1]["attack"].get("target") != "row"
):
    fail("Josephine's Health curse must execute on a separate row phase")

samson = one_by_unitdef("damage_row_self_massive_power_gain")
if samson["steps"][0]["effects"].get("self_power_chance") != 25:
    fail("Red Lyrium Samson's massive Power gain must retain its small chance")

blackwall = by_key["440:2250"]
if (
    blackwall["steps"][0]["effects"].get("self_heal_chance") != 10
    or blackwall["steps"][0]["effects"].get("self_heal_pct") != 75
):
    fail("Blackwall's encoded 10 must be the big self-heal proc chance")

goat = one_by_unitdef("damage_row_stun25_2x_selfheal10")
if (
    goat["steps"][0]["effects"].get("self_heal_chance") != 10
    or goat["steps"][0]["effects"].get("self_heal_pct") != 100
):
    fail("Avvar Goat's encoded 10 must be the massive self-heal proc chance")

templar_assassin = one_by_unitdef("ABILITY_HIGH_STUN_HEAL_25")
if (
    templar_assassin["steps"][0]["attack"].get("stun_pct") != 75
    or templar_assassin["steps"][0]["effects"].get("self_heal_chance") != 25
):
    fail("Templar Assassin must retain high stun and encoded 25% self-heal chance")

phoenix = one_by_unitdef("ABILITY_DAMAGE_ALL_BIGHEAL_SELF")
if phoenix["steps"][0]["effects"].get("self_heal_pct") != 75:
    fail("Phoenix's big self-heal must use the catalog's 75% convention")

dorian = one_by_unitdef("damage_all_and_large_drainpower")
if dorian["steps"][0]["attack"].get("drain_pct") != 75:
    fail("Dorian's large all-target drain must use the 75% convention")

hissrad = one_by_unitdef("damage_column_gain_power_self33_selfheal33")
if hissrad["steps"][0]["effects"].get("self_power_pct") != 33:
    fail("Hissrad's encoded self Power gain must remain 33%")

cullen = one_by_unitdef("damage_one_increase_power_curse_power_heavy_slow_chance50")
if cullen["steps"][0]["attack"].get("slow_amt") != -100:
    fail("Knight-Captain Cullen's heavy slow must reduce initiative by 100")

faith = one_by_unitdef("ability_heal_allies_huge_no_attack")
if faith["steps"][0]["effects"].get("ally_heal_pct") != 100:
    fail("Spirit of Faith must heal allies for the full amount of her Power")

great_bear = one_by_unitdef("attack_all_gainpower_chance50_stun")
if (
    great_bear["steps"][0]["attack"].get("stun_pct") != 50
    or great_bear["steps"][0]["effects"].get("self_power_pct") != 25
):
    fail("Great Bear's moderate value must scope stun, not its Power gain")

kenric = one_by_unitdef("damage_single_gain_power_all_25_drain_power_all")
if (
    kenric["steps"][0]["attack"].get("drain_pct")
    or kenric["steps"][1]["attack"].get("target") != "all"
    or kenric["steps"][1]["attack"].get("drain_pct") != 10
):
    fail("Professor Kenric must drain very-low Power from all enemies")

varric = one_by_unitdef("damage_row_3x_drainpower")
if (
    varric["steps"][0].get("repeat") != 3
    or varric["steps"][0]["attack"].get("drain_pct")
    or varric["steps"][1].get("chance_pct") != 75
    or varric["steps"][1]["attack"].get("drain_pct") != 50
):
    fail("Artificer Varric must drain once after his three row attacks")

frederic = one_by_unitdef("damage_row_damage_all_allies_heal_all")
if (
    len(frederic["steps"]) != 3
    or frederic["steps"][1].get("effects", {}).get("ally_damage_pct") != -5
    or frederic["steps"][2].get("effects", {}).get("ally_heal_pct") != 25
):
    fail("Professor Frederic's ally damage and heal must be sequential phases")

divine_leliana = one_by_unitdef("damage_all_drain_small_power_chance_drain_again")
if (
    len(divine_leliana["steps"]) != 2
    or divine_leliana["steps"][1].get("chance_pct") != 25
    or divine_leliana["steps"][1]["attack"].get("drain_pct") != 25
):
    fail("Divine Leliana must retain her optional second Power drain")

extra_three = one_by_unitdef(
    "damage_single_heal_all_medium_chance_followup3x_plus_damage"
)
if extra_three["steps"][1].get("repeat") != 3 or not extra_three["steps"][1].get(
    "stop_on_fail"
):
    fail("up-to-three additional attacks were not preserved")

print(
    "4.4 ability recipes: ok "
    f"({len(recipes)} chains, {len(all_definitions)} UnitDefs, "
    f"resolution={audit['resolution_counts']})"
)
