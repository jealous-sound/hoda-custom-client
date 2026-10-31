#!/usr/bin/env python3

from __future__ import annotations

import json
from pathlib import Path
import re
from typing import Any, Iterable


ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "client-additional-data" / "characters.json"
RECIPES = ROOT / "data" / "ability_recipes.json"
TARGETS = {"one", "row", "column", "all", "one_ally", "back_row", "any"}


def fail(message: str) -> None:
    raise SystemExit(f"ability recipe validation failed: {message}")


def numbers(value: Any) -> Iterable[int]:
    if isinstance(value, bool):
        return
    if isinstance(value, int):
        yield value
    elif isinstance(value, dict):
        for child in value.values():
            yield from numbers(child)
    elif isinstance(value, list):
        for child in value:
            yield from numbers(child)


def attacks(recipe: dict[str, Any]) -> list[dict[str, Any]]:
    return [step["attack"] for step in recipe["steps"] if not step["attack"].get("status_only")]


def mechanic_set(recipe: dict[str, Any]) -> set[str]:
    steps = recipe["steps"]
    fields = [step["attack"] for step in steps]
    effects = [step.get("effects", {}) for step in steps]
    result = {"ATTACK"} if attacks(recipe) else {"ATTACK_NONE"}
    if any(hit.get("drain_pct") for hit in fields):
        result.add("POWER_DRAIN")
    if any(hit.get("stun_pct") for hit in fields):
        result.add("STUN")
    if any(hit.get("slow_pct") for hit in fields):
        result.add("SLOW")
    if any(hit.get("curse_power_pct") for hit in fields):
        result.add("CURSE_POWER")
    if any(hit.get("curse_health_pct") for hit in fields):
        result.add("CURSE_HEALTH")
    if any(hit.get("curse_turns", 0) > 1 for hit in fields):
        result.add("CURSE_TWO_ROUNDS")
    if any(effect.get("ally_heal_pct") for effect in effects):
        result.add("HEAL_ALLIES")
    if any(effect.get("self_heal_pct") for effect in effects) or any(hit.get("self_heal_pct") for hit in fields):
        result.add("HEAL_SELF")
    if any(effect.get("ally_power_pct") or effect.get("ally_power_from_actor_pct") for effect in effects):
        result.add("GAIN_POWER_ALLIES")
    if any(effect.get("self_power_pct", 0) > 0 for effect in effects):
        result.add("GAIN_POWER_SELF")
    if any(effect.get("self_power_pct", 0) < 0 for effect in effects):
        result.add("LOSE_POWER_SELF")
    if any(effect.get("self_damage_pct", 0) < 0 for effect in effects):
        result.add("DAMAGE_SELF")
    if any(effect.get("ally_damage_pct", 0) < 0 for effect in effects):
        result.add("ATTACK_FRIENDLY")
    if any(hit.get("target") in {"one_ally", "any"} for hit in fields):
        result.add("ATTACK_FRIENDLY")
    if any(effect.get("ally_slow_amt", 0) < 0 for effect in effects):
        result.add("SLOW_ALLIES")
    if any(effect.get("ally_crit_pct", 0) > 0 for effect in effects):
        result.add("GAIN_CRIT_ALLIES")
    if any(effect.get("ally_damage_bonus_pct", 0) > 0 for effect in effects):
        result.add("GAIN_DAMAGE_ALLIES")
    if any(effect.get("self_damage_bonus_pct", 0) > 0 for effect in effects):
        result.add("GAIN_DAMAGE_SELF")
    if any(hit.get("mark_pct") for hit in fields):
        result.add("MARK")
    if any(hit.get("levy_pct") for hit in fields):
        result.add("LEVY")
    if any(hit.get("assassinate_pct") for hit in fields):
        result.add("ASSASSINATE")
    attack_steps = attacks(recipe)
    if sum(step.get("repeat", 1) for step in recipe["steps"] if not step["attack"].get("status_only")) > 1 or any(
        step.get("repeat", 1) > 1 and step.get("effects", {}).get("ally_damage_pct") for step in recipe["steps"]
    ) or ("ATTACK_FRIENDLY" in result and "ATTACK" in result):
        result.add("MULTIATTACK")
    return result


catalog = json.loads(CATALOG.read_text())
root = json.loads(RECIPES.read_text())
if root.get("version") != 1:
    fail(f"expected version 1, got {root.get('version')!r}")
rows = catalog["characters"]
catalog_by_key: dict[str, dict[str, Any]] = {}
for row in rows:
    catalog_by_key.setdefault(row["ability"]["key"], row)
recipes = root.get("recipes")
if not isinstance(recipes, list):
    fail("recipes is not an array")
by_key = {recipe.get("key"): recipe for recipe in recipes}
if len(recipes) != 421 or root.get("recipe_count") != 421 or len(by_key) != 421:
    fail(f"expected 421 unique recipes, got rows={len(recipes)} unique={len(by_key)}")
if set(by_key) != set(catalog_by_key):
    fail(f"key mismatch missing={sorted(set(catalog_by_key)-set(by_key))} extra={sorted(set(by_key)-set(catalog_by_key))}")

for key, recipe in by_key.items():
    ability = catalog_by_key[key]["ability"]
    if recipe.get("text") != re.sub(r"<[^>]+>", "", ability["text"]).replace("’", "'").strip():
        fail(f"{key}: source text drift")
    if recipe.get("target") not in TARGETS:
        fail(f"{key}: invalid target {recipe.get('target')!r}")
    if not isinstance(recipe.get("steps"), list) or len(recipe["steps"]) > 8:
        fail(f"{key}: invalid step count")
    for index, step in enumerate(recipe["steps"]):
        if not 1 <= step.get("chance_pct", 0) <= 100:
            fail(f"{key} step {index}: chance outside 1..100")
        if not 1 <= step.get("repeat", 0) <= 10:
            fail(f"{key} step {index}: repeat outside 1..10")
        if not 0 <= step.get("chance_group", 0) < 16:
            fail(f"{key} step {index}: invalid chance group")
        hit = step.get("attack")
        if not isinstance(hit, dict) or hit.get("target") not in TARGETS:
            fail(f"{key} step {index}: invalid attack target")
        for field, value in hit.items():
            if field in {"target", "status_only"}:
                continue
            if not isinstance(value, int) or not -200 <= value <= 500:
                fail(f"{key} step {index}: invalid {field}={value!r}")
    groups: dict[int, list[dict[str, Any]]] = {}
    for step in recipe["steps"]:
        group = step.get("chance_group", 0)
        if group:
            groups.setdefault(group, []).append(step)
    for group, members in groups.items():
        if len(members) < 2:
            fail(f"{key}: chance group {group} has only one member")
        chances = {member["chance_pct"] for member in members}
        if len(chances) != 1 or next(iter(chances)) >= 100:
            fail(f"{key}: chance group {group} must share one chance below 100%")
        if any(member["repeat"] != 1 for member in members):
            fail(f"{key}: grouped steps cannot repeat")
    if recipe.get("noncombat"):
        if recipe["steps"]:
            fail(f"{key}: noncombat recipe has battle steps")
        continue

    expected = set(ability["mechanics"])
    implemented = mechanic_set(recipe)
    missing = expected - implemented
    if missing:
        fail(f"{key}: missing advertised mechanics {sorted(missing)}")

    represented = {abs(value) for value in numbers(recipe["steps"])}
    for match in re.finditer(r"(\d+)%", recipe["text"]):
        value = int(match.group(1))
        context = recipe["text"][max(0, match.start() - 35) : match.end() + 55].casefold()
        if "damage resist" in context or "drains" in context and "2x damage chance" in context:
            continue
        if value not in represented:
            fail(f"{key}: advertised {value}% is absent from structured recipe")

    hits = [step["attack"] for step in recipe["steps"]]
    effect_rows = [step.get("effects", {}) for step in recipe["steps"]]

    def require_field(pattern: str, field: str, rows: list[dict[str, Any]]) -> None:
        for match in re.finditer(pattern, recipe["text"], re.I):
            value = int(match.group(1))
            if not any(abs(row.get(field, 0)) == value for row in rows):
                fail(f"{key}: {field} does not preserve advertised {value}%")

    require_field(r"\bdrain(?:s|ed|ing)?\s+(\d+)%\s+power", "drain_pct", hits)
    require_field(r"(\d+)%\s+chance[^.]{0,35}\bto\s+stun", "stun_pct", hits)
    require_field(r"with\s+(?:a\s+)?(\d+)%\s+chance[^.]{0,25}\bto\s+stun", "stun_pct", hits)
    require_field(r"(\d+)%\s+chance[^.]{0,35}\bto\s+slow", "slow_pct", hits)
    require_field(r"with\s+(?:a\s+)?(\d+)%\s+chance[^.]{0,25}\bto\s+slow", "slow_pct", hits)
    require_field(r"(?:with\s+)?(\d+)%\s+[Ll]evy", "levy_pct", hits)
    require_field(r"[Mm]arks?[^.]{0,90}?\+?(\d+)%\s+(?:bonus\s+)?damage", "mark_pct", hits)
    require_field(r"(\d+)%\s+chance[^.]{0,35}\bto\s+assassinate", "assassinate_pct", hits)


def expect_steps(key: str, expected: list[tuple[str, int, int]]) -> None:
    actual = [
        (step["attack"]["target"], step["attack"].get("damage_pct", 0), step["chance_pct"])
        for step in by_key[key]["steps"]
    ]
    if actual != expected:
        fail(f"{key}: steps {actual!r}, expected {expected!r}")


expect_steps(
    "damage_all_and_drainpower_and_10pctchance_attackagain",
    [("all", 100, 100), ("all", 100, 10)],
)
cory = by_key["damage_all_and_drainpower_and_10pctchance_attackagain"]
if [step["attack"].get("drain_pct") for step in cory["steps"]] != [50, 50]:
    fail("Corypheus must drain 50% on both advertised attacks")

expect_steps(
    "attack_one_levy25_drain75_followup35_column_stun",
    [("one", 100, 100), ("column", 100, 35)],
)
loghain = by_key["attack_one_levy25_drain75_followup35_column_stun"]
if loghain["steps"][0]["attack"].get("levy_pct") != 25 or loghain["steps"][0]["attack"].get("drain_pct") != 75:
    fail("Loghain Mac Tir primary must preserve 25% Levy and 75% drain")
if loghain["steps"][1]["attack"].get("stun_pct") != 100:
    fail("Loghain Mac Tir follow-up must stun its column")

expect_steps(
    "attack_row_stun75_followup30_single200_drain40",
    [("row", 100, 100), ("one", 200, 30)],
)
grey_loghain = by_key["attack_row_stun75_followup30_single200_drain40"]
if grey_loghain["steps"][0]["attack"].get("stun_pct") != 75 or grey_loghain["steps"][1]["attack"].get("drain_pct") != 40:
    fail("Grey Warden Loghain status magnitudes drifted")

column_drain = by_key["attackcolumn_20pwrdrn_secondattack30stun_heal20all"]
if len(column_drain["steps"]) != 2:
    fail("20% column drain ability must have primary and follow-up phases")
if column_drain["steps"][0]["attack"].get("drain_pct") != 20:
    fail("20% column drain ability changed its drain magnitude")
if column_drain["steps"][1]["chance_pct"] != 30 or column_drain["steps"][1]["attack"].get("stun_pct") != 100:
    fail("20% column drain ability must keep its 30% guaranteed-stun follow-up")


def expect_hit(key: str, step_index: int, expected: dict[str, Any]) -> None:
    hit = by_key[key]["steps"][step_index]["attack"]
    for field, value in expected.items():
        if hit.get(field) != value:
            fail(f"{key} step {step_index}: expected {field}={value!r}, got {hit.get(field)!r}")


def expect_effect(key: str, step_index: int, expected: dict[str, Any]) -> None:
    effects = by_key[key]["steps"][step_index].get("effects", {})
    for field, value in expected.items():
        if effects.get(field) != value:
            fail(f"{key} step {step_index}: expected {field}={value!r}, got {effects.get(field)!r}")


expect_hit(
    "attack_column50_cursestunrow_followup_attackone300_assassinate20",
    0,
    {"target": "column", "damage_pct": 50},
)
expect_hit(
    "attack_column50_cursestunrow_followup_attackone300_assassinate20",
    1,
    {"target": "row", "stun_pct": 100, "curse_power_pct": 100, "curse_health_pct": 100},
)
expect_hit(
    "attack_column50_cursestunrow_followup_attackone300_assassinate20",
    2,
    {"target": "one", "damage_pct": 300, "assassinate_pct": 20},
)
expect_hit("attack_single_curse75all_mark30", 0, {"target": "one", "mark_pct": 30})
expect_hit(
    "attack_single_curse75all_mark30",
    1,
    {"target": "all", "curse_power_pct": 100, "curse_health_pct": 100},
)
if by_key["attack_single_curse75all_mark30"]["steps"][1]["chance_pct"] != 75:
    fail("single mark ability lost its 75% all-enemy curse phase")
expect_hit(
    "damage_all_stun_50_and_curse_power_and_health_all",
    0,
    {"stun_pct": 50, "curse_power_pct": 100, "curse_health_pct": 100},
)
cole = by_key["damage_one_10x_heal5_crit2_small_chance_neg_ailment"]
if len(cole["steps"]) != 2 or cole["steps"][1]["repeat"] != 9:
    fail("Cole/Profane recipe must preserve up to ten attacks")
for index in range(2):
    expect_hit(
        "damage_one_10x_heal5_crit2_small_chance_neg_ailment",
        index,
        {"self_heal_pct": 5, "self_crit_pct": 2, "drain_pct": 20, "drain_chance": 20},
    )

for key, field in (
    ("ABILITY_DAMAGE_COLUMN_STUN25_CURSEHEALTH", "curse_health_pct"),
    ("damage_all_cursehealth_stun20", "curse_health_pct"),
    ("damage_row_cursehealth_slow50", "curse_health_pct"),
    ("attack_all_curse_health_chance50_stun_drain50", "curse_health_pct"),
    ("attack_row_curse_power_crit5_levy50", "curse_power_pct"),
    ("damage_all_cursepower_heal_all_crit10_all", "curse_power_pct"),
    ("damage_all_stun_25_heal_curse_all", "curse_power_pct"),
    ("damage_one_increase_power_curse_power_heavy_slow_chance50", "curse_power_pct"),
    ("damage_single_curse_power_drain_power_stunchance25", "curse_power_pct"),
):
    if not any(step["attack"].get(field) == 100 for step in by_key[key]["steps"]):
        fail(f"{key}: guaranteed curse was reduced by an unrelated chance clause")
if any(
    step.get("effects", {}).get("self_crit_pct")
    for step in by_key["damage_all_cursepower_heal_all_crit10_all"]["steps"]
):
    fail("Dagna's ally crit buff was incorrectly duplicated as self crit")
expect_hit("damage_gainpower_drainpower_cursepower", 0, {"target": "one", "drain_pct": 30})
expect_hit("damage_gainpower_drainpower_cursepower", 1, {"target": "column", "curse_power_pct": 100})

for key, expected in (
    ("ABILITY_DAMAGE_INIT_50_STUN25_COLUMN", {"slow_pct": 100, "slow_amt": -50, "stun_pct": 25}),
    ("attack_all_slow_25chancecursehealth", {"slow_pct": 100, "slow_amt": -50, "curse_health_pct": 25}),
    ("damage_row_drainpower_25_and_slow_50", {"drain_pct": 25, "slow_pct": 100, "slow_amt": -50}),
    ("damage_row_slow_25stun", {"slow_pct": 100, "slow_amt": -50, "stun_pct": 25}),
):
    expect_hit(key, 0, expected)
expect_steps(
    "ABILITY_DAMAGE_ROW_SLOWINIT100_FOLLOWUP25",
    [("row", 100, 100), ("row", 100, 25)],
)
expect_hit("ABILITY_DAMAGE_ROW_SLOWINIT100_FOLLOWUP25", 0, {"slow_pct": 100, "slow_amt": -100})
expect_steps("damage_one_slow_5x", [("one", 100, 100), ("one", 100, 50)])
if by_key["damage_one_slow_5x"]["steps"][1]["repeat"] != 4:
    fail("Chocolate Nug must preserve up to five slow attempts")

for key, expected in (
    ("damage_column_allies_gain_medium_power_health", {"ally_heal_pct": 50, "ally_power_pct": 50}),
    ("attack_row_all_gain_power10_and_health10", {"ally_heal_pct": 10, "ally_power_pct": 10}),
    ("damage_one_power_all_30", {"ally_power_pct": 30}),
    ("damage_row_gain_power_all_10", {"ally_power_pct": 10}),
    ("ABILITY_DAMAGE_ONE_POWERGAIN50_ALL", {"ally_power_pct": 25, "ally_power_chance": 50}),
    ("grant_allies_100power_no_attack", {"ally_power_from_actor_pct": 100}),
    ("noattack_heal100all_gain100all", {"ally_heal_pct": 100, "ally_power_from_actor_pct": 100}),
    ("attack_all_alliesgain15power20health_slow50", {"ally_heal_pct": 20, "ally_power_pct": 15}),
):
    expect_effect(key, 0, expected)

for key, field, value in (
    ("damage_column_drainpower_2x", "drain_pct", 50),
    ("ABILITY_DAMAGE_AND_HALF_POWER_FOLLOWUP25", "drain_pct", 50),
    ("damage_row_slow_cursepower_2x", "curse_power_pct", 100),
    ("ABILITY_DAMAGE_ROW_CURSEPOWER_DESCRIPTION_FOLLOWUP25", "curse_power_pct", 100),
):
    if len(by_key[key]["steps"]) != 2 or any(step["attack"].get(field) != value for step in by_key[key]["steps"]):
        fail(f"{key}: second attack must preserve {field}={value}")
if len(by_key["attack_row_levy20_stun_gain20all_attack_ally5"]["steps"]) != 1:
    fail("Grey Warden Knight must not invent an enemy follow-up after attacking allies")

red_nug = by_key["damage_one_row_column_all_plus50crit"]
if [step["attack"]["target"] for step in red_nug["steps"]] != ["one", "row", "column", "all"]:
    fail("Red Lyrium Nug must keep all four target phases")
if red_nug["steps"][-1].get("effects", {}).get("self_crit_pct") != 50:
    fail("Red Lyrium Nug must gain 50% crit")

for key in ("WISP_ATTACK_ABILITY_KEY", "SUPER_WISP_ATTACK_ABILITY_KEY"):
    if not by_key[key].get("noncombat") or by_key[key]["steps"]:
        fail(f"{key}: consumable was incorrectly made into a battle attack")

ambiguous = [recipe for recipe in recipes if recipe.get("ambiguous")]
unsupported = [recipe for recipe in recipes if recipe.get("unsupported")]
print(
    f"ability recipes: ok ({len(recipes)} keys, {len(ambiguous)} with explicit ambiguity notes, "
    f"{len(unsupported)} with unsupported/passive notes)"
)
