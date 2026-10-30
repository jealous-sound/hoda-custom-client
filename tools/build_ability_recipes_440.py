#!/usr/bin/env python3

from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from copy import deepcopy
import json
from pathlib import Path
import re
import sys
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_UNITS = ROOT / "heroes-of-dragons-cache" / "data_tables" / "units.json"
DEFAULT_RECIPES = ROOT / "data" / "ability_recipes.json"
DEFAULT_OUTPUT = ROOT / "data" / "ability_recipes_440.json"
LOCALIZATION_DIR = (
    ROOT
    / "heroes-of-dragons-cache"
    / "Heroes-of-Dragon-Age-v4-4-0-cache-androidp1"
    / "com.ea.game.dragonage_row"
    / "cache"
    / "Localization"
)

sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "heroes-of-dragons-cache"))
from build_ability_recipes import (  # noqa: E402
    QUALITATIVE_AMOUNT,
    QUALITATIVE_CHANCE,
    _generic_recipe,
    _override_steps,
)
from ability_metadata import infer_ability_mechanics, normalize_text  # noqa: E402


TARGET_ENUM = {1: "one", 2: "all", 3: "row", 5: "column"}
CATALOG_TARGET = {value: value.title() for value in TARGET_ENUM.values()}
SPEED_ENUM = {0: "slow", 1: "normal", 2: "quick"}

SPECIALTY_MECHANIC = {
    "POWERDRAIN": "POWER_DRAIN",
    "STUN": "STUN",
    "SLOW": "SLOW",
    "CURSEPOWER": "CURSE_POWER",
    "CURSEHEALTH": "CURSE_HEALTH",
    "MULTIATTACK": "MULTIATTACK",
}

KEY_REPEAT = {
    "TRIPLE_STRIKE_ABILITY_KEY": 3,
    "TWIN_FANGS_ABILITY_KEY": 2,
}

EXPLICIT_FOLLOWUP_CHANCE_KEYS = {
    "damage_all_and_drainpower_attack2x_chance10",
    "ABILITY_DAMAGE_AND_HALF_POWER_FOLLOWUP25",
    "ABILITY_DAMAGE_ROW_FOLLOWUP25",
    "ABILITY_DAMAGE_ROW_CURSEPOWER_DESCRIPTION_FOLLOWUP25",
    "damage_row_damage_all_allies_heal_all",
}

TITLE_ONLY_MAPPING_KEYS = {
    "BILE_BURST_ABILITY_KEY",
    "OVERWHELMING_SLAM_ABILITY_KEY",
}

FALLBACK = {
    "damage_pct": 100,
    "status_chance_pct": 50,
    "drain_one_pct": 50,
    "drain_group_pct": 25,
    "heal_pct": 25,
    "power_gain_pct": 25,
    "followup_chance_pct": 25,
}


def relative(path: Path) -> str:
    try:
        return str(path.resolve().relative_to(ROOT))
    except ValueError:
        return str(path.resolve())


def load_pipe_localization(path: Path) -> dict[str, str]:
    result: dict[str, str] = {}
    with path.open(encoding="utf-8-sig") as source:
        for line in source:
            key, separator, value = line.rstrip("\r\n").partition("|")
            if separator:
                result[key.upper()] = value
    return result


def clean_localized_text(value: str) -> str:
    return normalize_text(re.sub(r"<[^>]+>", "", value).replace("’", "'"))


def resolve_localization(table: dict[str, str], key: str) -> tuple[str, str]:
    if not key:
        return "", ""
    upper = key.upper()
    for candidate in (f"{upper}_DESCRIPTION", upper):
        value = table.get(candidate, "")
        if value.strip():
            return candidate, clean_localized_text(value)
    return "", ""


def resolve_key_mapping(table: dict[str, str], key: str) -> tuple[str, str]:
    upper = key.upper()
    for candidate in (f"{upper}_DESCRIPTION", upper):
        mapped = table.get(candidate, "").strip()
        if mapped:
            return candidate, mapped[2:] if mapped.startswith("__") else mapped
    return "", ""


def tier_number(row: dict[str, Any]) -> int:
    return int(row.get("promotion_level", 0))


def chain_specialties(rows: list[dict[str, Any]]) -> list[str]:
    return sorted({specialty for row in rows for specialty in row.get("specialty", [])})


def base_character_id(rows: list[dict[str, Any]]) -> str:
    tier_one = min(rows, key=tier_number)
    name = tier_one["name"]
    if not name.endswith("I"):
        raise ValueError(
            f"promotion group {tier_one['promote_group']}: tier-I id {name!r} has no I suffix"
        )
    return name[:-1]


def required_effects(specialties: set[str], key: str) -> set[str]:
    result = {
        mechanic
        for specialty, mechanic in SPECIALTY_MECHANIC.items()
        if specialty in specialties
    }
    if key in KEY_REPEAT:
        result.add("MULTIATTACK")
    return result


def fallback_semantics(
    *,
    key: str,
    target: str,
    specialties: set[str],
) -> tuple[list[str], str, list[dict[str, Any]], list[str]]:
    mechanics = {"ATTACK"}
    mechanics.update(required_effects(specialties, key))
    if "HEAL" in specialties:
        mechanics.add(
            "HEAL_ALLIES"
            if re.search(r"(?:GROUP_HEAL|HEAL_ALL|ALLIES)", key.upper())
            else "HEAL_SELF"
        )
    if "POWERGAIN" in specialties:
        mechanics.add(
            "GAIN_POWER_ALLIES"
            if re.search(r"(?:GAIN_POWER_ALL|POWER_ALL|RALLY|WARCRY)", key.upper())
            else "GAIN_POWER_SELF"
        )

    phrases = {
        "one": "one enemy",
        "row": "a row of enemies",
        "column": "a column of enemies",
        "all": "all enemies",
    }
    sentences = [f"Attacks {phrases[target]}."]
    hit: dict[str, Any] = {"target": target, "damage_pct": FALLBACK["damage_pct"]}
    effects: dict[str, int] = {}

    if "POWER_DRAIN" in mechanics:
        hit["drain_pct"] = (
            FALLBACK["drain_one_pct"]
            if target == "one"
            else FALLBACK["drain_group_pct"]
        )
        if "HALF_POWER" in key.upper():
            hit["drain_pct"] = 50
        sentences.append("Drains Power.")
    if "STUN" in mechanics:
        hit["stun_pct"] = FALLBACK["status_chance_pct"]
        sentences.append("May stun.")
    if "SLOW" in mechanics:
        hit["slow_pct"] = FALLBACK["status_chance_pct"]
        hit["slow_amt"] = -50
        if "INIT_50" in key.upper():
            hit["slow_pct"] = 100
        sentences.append("May slow.")
    if "CURSE_POWER" in mechanics:
        hit["curse_power_pct"] = 100
        sentences.append("Curses Power for one turn.")
    if "CURSE_HEALTH" in mechanics:
        hit["curse_health_pct"] = 100
        sentences.append("Curses Health for one turn.")
    if "HEAL_ALLIES" in mechanics:
        effects["ally_heal_pct"] = FALLBACK["heal_pct"]
        sentences.append("Heals allies.")
    if "HEAL_SELF" in mechanics:
        effects["self_heal_pct"] = FALLBACK["heal_pct"]
        sentences.append("Heals self.")
    if "GAIN_POWER_ALLIES" in mechanics:
        effects["ally_power_pct"] = FALLBACK["power_gain_pct"]
        sentences.append("Grants Power to allies.")
    if "GAIN_POWER_SELF" in mechanics:
        effects["self_power_pct"] = FALLBACK["power_gain_pct"]
        sentences.append("Gains Power.")

    if key == "MASSIVE_ATTACK_ABILITY_KEY":
        hit["damage_pct"] = 200

    primary: dict[str, Any] = {
        "chance_pct": 100,
        "repeat": KEY_REPEAT.get(key, 1),
        "attack": hit,
    }
    if effects:
        primary["effects"] = effects
        if primary["repeat"] > 1:
            primary["effects_once"] = True
    steps = [primary]
    if "MULTIATTACK" in mechanics and key not in KEY_REPEAT:
        steps.append(
            {
                "chance_pct": FALLBACK["followup_chance_pct"],
                "repeat": 1,
                "attack": {"target": target, "damage_pct": FALLBACK["damage_pct"]},
            }
        )

    notes = [
        "4.4 ability localization is absent; executable values are deterministic approximations from UnitDef target and specialties",
        "damage, status chance, drain, healing, power gain, and follow-up values were not recoverable from the preserved 4.4 client data",
    ]
    if "HEAL" in specialties or "POWERGAIN" in specialties:
        notes.append(
            "HEAL/POWERGAIN specialties do not encode self versus allies; key wording or the documented fallback selected the direction"
        )
    return sorted(mechanics), " ".join(sentences), steps, notes


def parsed_client_recipe(
    *, key: str, text: str, target: str, specialties: list[str]
) -> dict[str, Any]:
    mechanics = infer_ability_mechanics(text, specialties)
    row = {
        "ability": {
            "key": key,
            "text": text,
            "target": CATALOG_TARGET[target],
            "mechanics": mechanics,
        }
    }
    recipe = _generic_recipe(row)
    _override_steps(recipe)
    for step in recipe["steps"]:
        effects = step.get("effects", {})
        if effects.get("self_power_chance", 100) != 100 and not re.search(
            r"\bchance[^.]{0,35}\b(?:to\s+)?gain[^.]{0,20}\bpower\b",
            text,
            re.I,
        ):
            effects["self_power_chance"] = 100
        if effects.get("self_heal_chance", 100) != 100 and not re.search(
            r"\bchance[^.]{0,35}\b(?:to\s+)?(?:heal|gain[^.]{0,12}health)\b",
            text,
            re.I,
        ):
            effects["self_heal_chance"] = 100
    return recipe


def apply_440_key_overrides(
    key: str,
    mechanics: list[str],
    steps: list[dict[str, Any]],
) -> tuple[list[str], list[str]]:
    extra_ambiguity: list[str] = []

    def primary_effects() -> dict[str, int]:
        return steps[0].setdefault("effects", {}) if steps else {}

    if key == "damage_all_and_drainpower_attack2x_chance10" and len(steps) > 1:
        steps[1]["chance_pct"] = 10
    elif key == "ABILITY_DAMAGE_ALL_POWERGAIN25_HEAL25" and steps:
        effects = primary_effects()
        effects["self_power_pct"] = 25
        effects["self_power_chance"] = 25
        effects["self_heal_pct"] = 25
        effects["self_heal_chance"] = 25
    elif key == "damage_one_drain_all_gain_health_all" and steps:
        opening = steps[0]["attack"]
        drain = opening.pop("drain_pct", 50)
        opening.pop("drain_chance", None)
        steps.append(
            {
                "chance_pct": 25,
                "repeat": 1,
                "attack": {
                    "target": "all",
                    "damage_pct": 0,
                    "status_only": True,
                    "drain_pct": drain,
                },
            }
        )
        extra_ambiguity.append(
            "Power-drain proc chance is not numeric in 4.4 text; reconstructed as small (25%)"
        )
    elif key == "ability_attack_all_stun_pct25_heal_all_15":
        effects = primary_effects()
        effects["ally_heal_pct"] = 15
    elif key == "attack_all_gain_power15_and_heal_self20_and_chance_to_slow50":
        effects = primary_effects()
        effects["self_power_pct"] = 15
        effects["self_heal_pct"] = 20
    elif key == "attack_all_gainhealth25_all_gainpower50_and_15drain_power_all":
        effects = primary_effects()
        effects["ally_heal_pct"] = 25
        effects["ally_power_pct"] = 50
    elif key == "attack_all_gain_power_all20_and_heal_all20":
        effects = primary_effects()
        effects["ally_heal_pct"] = 20
        effects["ally_power_pct"] = 20
    elif key == "damage_all_heal_all15":
        effects = primary_effects()
        effects["ally_heal_pct"] = 15
    elif key == "damage_single_curse_health_drain_power5" and len(steps) > 1:
        steps[1]["attack"]["drain_pct"] = 5
    elif key == "damage_one_curseheal_column_cursepower_row" and steps:
        opening = steps[0]["attack"]
        opening.pop("curse_health_pct", None)
        opening.pop("curse_power_pct", None)
        steps.extend(
            (
                {
                    "chance_pct": 100,
                    "repeat": 1,
                    "attack": {
                        "target": "column",
                        "damage_pct": 0,
                        "status_only": True,
                        "curse_health_pct": 100,
                    },
                },
                {
                    "chance_pct": 100,
                    "repeat": 1,
                    "attack": {
                        "target": "row",
                        "damage_pct": 0,
                        "status_only": True,
                        "curse_power_pct": 100,
                    },
                },
            )
        )
    elif key == "damage_one_heal_all_small_curse_health_row" and steps:
        steps[0]["attack"].pop("curse_health_pct", None)
        steps.append(
            {
                "chance_pct": 100,
                "repeat": 1,
                "attack": {
                    "target": "row",
                    "damage_pct": 0,
                    "status_only": True,
                    "curse_health_pct": 100,
                },
            }
        )
    elif key == "damage_row_self_massive_power_gain" and steps:
        effects = primary_effects()
        effects["self_power_pct"] = 100
        effects["self_power_chance"] = 25
    elif key == "ABILITY_DAMAGE_COLUMN_POWERGAIN25_POWERDRAIN25" and steps:
        attack = steps[0]["attack"]
        attack.pop("drain_pct", None)
        attack.pop("drain_chance", None)
        effects = primary_effects()
        effects.pop("self_power_pct", None)
        effects.pop("self_power_chance", None)
        attack["exclusive_power_pct"] = 25
        attack["exclusive_power_chance"] = 25
        extra_ambiguity.append(
            "gain-or-drain proc chance is not numeric in 4.4 data; reconstructed as 25%"
        )
    elif key == "ABILITY_DAMAGE_ONE_STUN-SLOW-DRAIN10" and steps:
        attack = steps[0]["attack"]
        for field in (
            "drain_pct",
            "drain_chance",
            "stun_pct",
            "slow_pct",
            "slow_amt",
        ):
            attack.pop(field, None)
        attack["random_ailment_pct"] = 25
        attack["random_ailment_mask"] = 0b00111
        attack["random_drain_pct"] = 10
        attack["random_slow_amt"] = -50
        extra_ambiguity.append(
            "stun/slow/drain proc chance is not numeric in 4.4 data; reconstructed as 25%"
        )
    elif key == "ABILITY_DAMAGE_ONE_STUN25_SLOW25" and steps:
        attack = steps[0]["attack"]
        for field in ("stun_pct", "slow_pct", "slow_amt"):
            attack.pop(field, None)
        attack["random_ailment_pct"] = 50
        attack["random_ailment_mask"] = 0b00011
        attack["random_slow_amt"] = -50
    elif key == "ABILITY_DAMAGE_ALL_BIGHEAL_SELF" and steps:
        primary_effects()["self_heal_pct"] = 75
    elif key == "damage_all_and_large_drainpower" and steps:
        steps[0]["attack"]["drain_pct"] = 75
    elif key == "damage_column_gain_power_self33_selfheal33" and steps:
        primary_effects()["self_power_pct"] = 33
    elif key == "damage_one_increase_power_curse_power_heavy_slow_chance50" and steps:
        steps[0]["attack"]["slow_amt"] = -100
    elif key == "ability_heal_allies_huge_no_attack" and steps:
        primary_effects()["ally_heal_pct"] = 100
    elif key == "attack_all_gainpower_chance50_stun" and steps:
        primary_effects()["self_power_pct"] = 25
        extra_ambiguity.append(
            "self Power amount is not numeric in 4.4 data; reconstructed as 25%"
        )
    elif key == "damage_column_stun50_selfheal10" and steps:
        effects = primary_effects()
        effects["self_heal_pct"] = 75
        effects["self_heal_chance"] = 10
        extra_ambiguity.append(
            "big self-heal magnitude is not numeric in 4.4 data; reconstructed as 75%"
        )
    elif key == "damage_row_stun25_2x_selfheal10" and steps:
        effects = primary_effects()
        effects["self_heal_pct"] = 100
        effects["self_heal_chance"] = 10
        extra_ambiguity.append(
            "massive self-heal magnitude is not numeric in 4.4 data; reconstructed as 100%"
        )
    elif key == "ABILITY_HIGH_STUN_HEAL_25" and steps:
        steps[0]["attack"]["stun_pct"] = 75
        effects = primary_effects()
        effects["self_heal_pct"] = 25
        effects["self_heal_chance"] = 25
        extra_ambiguity.append(
            "self-heal magnitude is not numeric in 4.4 data; reconstructed as 25%"
        )
    elif key == "damage_single_gain_power_all_25_drain_power_all" and steps:
        opening = steps[0]["attack"]
        opening.pop("drain_pct", None)
        opening.pop("drain_chance", None)
        steps.append(
            {
                "chance_pct": 100,
                "repeat": 1,
                "attack": {
                    "target": "all",
                    "damage_pct": 0,
                    "status_only": True,
                    "drain_pct": 10,
                },
            }
        )
    elif key == "damage_row_heal_all_10_powergain_self_small":
        effects = primary_effects()
        effects["ally_heal_pct"] = 10
    elif key == "attack_one_assassinate_10pct":
        steps[0]["attack"]["assassinate_pct"] = 10
    elif key == "damage_heal_self_slow_curse_column_heal_all" and len(steps) > 1:
        first_attack = steps[0]["attack"]
        first_attack.pop("curse_health_pct", None)
        first_effects = steps[0].setdefault("effects", {})
        ally_heal = first_effects.pop("ally_heal_pct", 25)
        second_attack = steps[1]["attack"]
        second_attack["target"] = "column"
        second_attack["curse_health_pct"] = 100
        steps[1].setdefault("effects", {})["ally_heal_pct"] = ally_heal
    elif key == "damage_gainpower_drainpower_cursepower" and len(steps) > 1:
        steps[0]["attack"].pop("drain_pct", None)
        primary_effects()["self_power_pct"] = 50
        second_attack = steps[1]["attack"]
        second_attack["target"] = "column"
        second_attack["drain_pct"] = 50
        second_attack["drain_chance"] = 0
    elif (
        key == "ABILITY_DAMAGE_ONE_INCREASE_POWER_FOLLOWUP2X_CURSEHEALTH"
        and len(steps) > 1
    ):
        steps[0]["attack"].pop("curse_health_pct", None)
        steps[1]["attack"]["curse_health_pct"] = 100
    elif key == "ability_attack_row_slow50_assassinate_single_pct10" and len(steps) > 1:
        steps[0]["attack"].pop("assassinate_pct", None)
        steps[1]["chance_pct"] = 10
        steps[1]["attack"]["target"] = "one"
        steps[1]["attack"]["assassinate_pct"] = 100
    elif key == "damage_single_twice_chance_assassinate" and len(steps) > 1:
        steps[0]["attack"].pop("assassinate_pct", None)
        steps[1]["attack"]["target"] = "one"
        steps[1]["attack"]["assassinate_pct"] = 100
    elif key == "damage_column_then_row_and_gainpower" and len(steps) > 1:
        steps[1]["chance_pct"] = 100
        steps[1]["attack"]["target"] = "row"
    elif key == "damage_one_10x_small_chance_neg_aliment" and len(steps) > 1:
        for step in steps:
            attack = step["attack"]
            for field in (
                "drain_pct",
                "drain_chance",
                "stun_pct",
                "slow_pct",
                "slow_amt",
                "curse_power_pct",
                "curse_health_pct",
            ):
                attack.pop(field, None)
            attack["random_ailment_pct"] = 25
            attack["random_ailment_mask"] = 0b11111
            attack["random_drain_pct"] = 25
            attack["random_slow_amt"] = -50
        extra_ambiguity.extend(
            (
                "per-hit status proc chance is not numeric in 4.4 data; reconstructed as 25%",
                "random Power-drain amount is not numeric in 4.4 data; reconstructed as 25%",
            )
        )
    elif key == "damage_one_5x_150":
        steps[:] = [
            {
                "chance_pct": 100,
                "repeat": 5,
                "final_damage_pct": 150,
                "attack": {"target": "one", "damage_pct": 100},
            }
        ]
    elif key == "damage_row_3x_drainpower" and steps:
        opening = steps[0]
        opening["repeat"] = 3
        opening["attack"].pop("drain_pct", None)
        opening["attack"].pop("drain_chance", None)
        steps.append(
            {
                "chance_pct": 75,
                "repeat": 1,
                "attack": {
                    "target": "row",
                    "damage_pct": 0,
                    "status_only": True,
                    "drain_pct": 50,
                },
            }
        )
    elif key == "damage_row_damage_all_allies_heal_all" and steps:
        opening = steps[0]
        effects = opening.pop("effects", {})
        ally_damage = effects.get("ally_damage_pct", -5)
        ally_heal = effects.get("ally_heal_pct", 25)
        steps.extend(
            (
                {
                    "chance_pct": 100,
                    "repeat": 1,
                    "attack": {
                        "target": "one",
                        "damage_pct": 0,
                        "status_only": True,
                    },
                    "effects": {"ally_damage_pct": ally_damage},
                },
                {
                    "chance_pct": 100,
                    "repeat": 1,
                    "attack": {
                        "target": "one",
                        "damage_pct": 0,
                        "status_only": True,
                    },
                    "effects": {"ally_heal_pct": ally_heal},
                },
            )
        )
    elif key == "damage_all_drain_small_power_chance_drain_again" and steps:
        steps.append(
            {
                "chance_pct": 25,
                "repeat": 1,
                "attack": {
                    "target": "all",
                    "damage_pct": 0,
                    "status_only": True,
                    "drain_pct": 25,
                },
            }
        )
    elif key == "grant_allies_35power_no_attack":
        effects = primary_effects()
        effects["ally_power_pct"] = 35
    elif key == "damage_all_powergain15_heal5_attack_own_all_5":
        effects = primary_effects()
        effects["self_power_pct"] = 15
        effects["self_heal_pct"] = 5
        steps[1] = {
            "chance_pct": 100,
            "repeat": 1,
            "attack": {"target": "one", "damage_pct": 0, "status_only": True},
            "effects": {"ally_damage_pct": -5, "self_damage_pct": -5},
        }
        mechanics = sorted(set(mechanics) | {"ATTACK_FRIENDLY", "DAMAGE_SELF"})
    elif key == "attack_column_50chance_attack_again_125percent" and len(steps) > 1:
        steps[1]["attack"]["target"] = "one"
        steps[1]["attack"]["damage_pct"] = 250
        extra_ambiguity.append(
            "ability key says 125% follow-up damage but exact 4.4 localization says 250%; localization wins"
        )
    elif (
        key == "damage_single_heal_all_medium_chance_followup3x_plus_damage"
        and len(steps) > 1
    ):
        steps[1]["repeat"] = 3
        steps[1]["stop_on_fail"] = True
        extra_ambiguity.append(
            "three additional attacks are explicit, but their increasing damage curve is not numeric"
        )

    return mechanics, extra_ambiguity


def build(
    units_path: Path,
    recipes_path: Path,
    localization_dir: Path,
) -> dict[str, Any]:
    units = json.loads(units_path.read_text())
    established_recipes = {
        row["key"].upper(): row
        for row in json.loads(recipes_path.read_text())["recipes"]
    }
    loc_en = load_pipe_localization(localization_dir / "Loc_ENG_US.txt")
    loc_ru = load_pipe_localization(localization_dir / "Loc_RUS_RU.txt")
    key_mapping = load_pipe_localization(localization_dir / "Loc_Key_Mapping.txt")

    grouped: dict[int, list[dict[str, Any]]] = defaultdict(list)
    for row in units:
        grouped[int(row["promote_group"])].append(row)

    recipes: list[dict[str, Any]] = []
    definition_to_recipe: dict[str, str] = {}
    promote_group_to_recipe: dict[str, str] = {}
    resolution_counts: Counter[str] = Counter()
    resolution_mode_counts: Counter[str] = Counter()
    ability_keys: set[str] = set()
    direct_ability_keys: set[str] = set()
    mapped_ability_keys: set[str] = set()
    unresolved_ability_keys: set[str] = set()
    passive_keys: set[str] = set()
    localized_passive_keys: set[str] = set()

    for promote_group in sorted(grouped):
        rows = sorted(grouped[promote_group], key=tier_number)
        first = rows[0]
        key = first["ability_description"]
        ability_keys.add(key)
        target_enum = int(first["ability_unit_target"])
        if target_enum not in TARGET_ENUM:
            raise ValueError(
                f"promotion group {promote_group}: unsupported target enum {target_enum}"
            )
        target = TARGET_ENUM[target_enum]
        specialties = chain_specialties(rows)
        specialty_set = set(specialties)
        character_id = base_character_id(rows)
        loc_key_en, client_text = resolve_localization(loc_en, key)
        loc_key_ru, client_text_ru = resolve_localization(loc_ru, key)
        mapping_key = ""
        mapped_ability_id = ""
        reference_key = ""
        resolution_mode = ""
        if client_text and key not in TITLE_ONLY_MAPPING_KEYS:
            direct_ability_keys.add(key)
            parsed = parsed_client_recipe(
                key=key,
                text=client_text,
                target=target,
                specialties=specialties,
            )
            source = "client_4.4_localization"
            resolution_mode = "parsed_4.4_prose"
            semantic_text = client_text
            semantic_text_ru = client_text_ru
            mechanics = parsed["mechanics"]
            steps = parsed["steps"]
            ambiguous = list(parsed.get("ambiguous", []))
            unsupported = list(parsed.get("unsupported", []))
        else:
            mapping_key, mapped_ability_id = resolve_key_mapping(key_mapping, key)
            mapped_loc_key_en, mapped_text = resolve_localization(
                loc_en, mapped_ability_id
            )
            _, mapped_text_ru = resolve_localization(loc_ru, mapped_ability_id)
            if mapped_ability_id and mapped_text:
                mapped_ability_keys.add(key)
                loc_key_en, client_text = mapped_loc_key_en, mapped_text
                client_text_ru = mapped_text_ru
                semantic_text = client_text
                semantic_text_ru = client_text_ru
                source = "client_4.4_key_mapping"
                established = established_recipes.get(mapped_ability_id.upper())
                if (
                    established
                    and established["target"] == target
                    and clean_localized_text(established["text"]) == client_text
                ):
                    copied = deepcopy(established)
                    reference_key = copied["key"]
                    resolution_mode = "copied_exact_mapped_recipe"
                    mechanics = copied["mechanics"]
                    steps = copied["steps"]
                    ambiguous = list(copied.get("ambiguous", []))
                    unsupported = list(copied.get("unsupported", []))
                else:
                    parsed = parsed_client_recipe(
                        key=mapped_ability_id,
                        text=client_text,
                        target=target,
                        specialties=specialties,
                    )
                    resolution_mode = "parsed_mapped_4.4_prose"
                    mechanics = parsed["mechanics"]
                    steps = parsed["steps"]
                    ambiguous = list(parsed.get("ambiguous", []))
                    unsupported = list(parsed.get("unsupported", []))
            else:
                unresolved_ability_keys.add(key)
                source = "specialty_fallback"
                resolution_mode = "deterministic_specialty_fallback"
                semantic_text_ru = ""
                mechanics, semantic_text, steps, ambiguous = fallback_semantics(
                    key=key,
                    target=target,
                    specialties=specialty_set,
                )
                unsupported = []

        canonical_ability_id = mapped_ability_id or key
        mechanics, key_ambiguity = apply_440_key_overrides(
            canonical_ability_id,
            mechanics,
            steps,
        )
        ambiguous.extend(key_ambiguity)
        if canonical_ability_id in EXPLICIT_FOLLOWUP_CHANCE_KEYS:
            ambiguous = [
                note for note in ambiguous if not note.startswith("follow-up chance")
            ]
        resolution_counts[source] += 1
        resolution_mode_counts[resolution_mode] += 1
        semantic_recipe_key = f"440:{promote_group}"
        definition_ids = [int(row["id"]) for row in rows]
        passive_key = first.get("equipped_rune_description_key", "")
        passive_loc_key_en, passive_text = resolve_localization(loc_en, passive_key)
        passive_loc_key_ru, passive_text_ru = resolve_localization(loc_ru, passive_key)
        if passive_key:
            passive_keys.add(passive_key)
            if passive_text:
                localized_passive_keys.add(passive_key)
        weight_key = first.get("target_weight_description_key", "")
        weight_loc_key_en, weight_text = resolve_localization(loc_en, weight_key)
        weight_loc_key_ru, weight_text_ru = resolve_localization(loc_ru, weight_key)

        specialty_signatures = {tuple(row.get("specialty", [])) for row in rows}
        entry: dict[str, Any] = {
            "semantic_recipe_key": semantic_recipe_key,
            "promote_group": promote_group,
            "definition_ids": definition_ids,
            "unit_names": [row["name"] for row in rows],
            "display_names": [row.get("display_name", "") for row in rows],
            "character_id": character_id,
            "rarity": int(first["rarity"]),
            "rarity_name": first.get("rarity_name", ""),
            "initiative": int(first.get("initiative", 1)),
            "initiative_name": SPEED_ENUM[int(first.get("initiative", 1))],
            "unitdef_ability_id": key,
            "action_id_template": "{battle_id}-action-{sequence}",
            "mapping_key": mapping_key,
            "mapped_ability_id": mapped_ability_id,
            "localization_key": loc_key_en,
            "localized_text": client_text,
            "localized_text_ru": client_text_ru,
            "text": semantic_text,
            "text_ru": semantic_text_ru,
            "resolution_source": source,
            "resolution_mode": resolution_mode,
            "reference_recipe_key": reference_key,
            "target": target,
            "target_enum": target_enum,
            "specialties": specialties,
            "groups": first.get("group", []),
            "mechanics": mechanics,
            "steps": steps,
            "passive": {
                "key": passive_key,
                "localization_key": passive_loc_key_en,
                "text": passive_text,
                "localization_key_ru": passive_loc_key_ru,
                "text_ru": passive_text_ru,
            },
            "target_weight": {
                "key": weight_key,
                "localization_key": weight_loc_key_en,
                "text": weight_text,
                "localization_key_ru": weight_loc_key_ru,
                "text_ru": weight_text_ru,
            },
        }
        if len(specialty_signatures) > 1:
            entry["specialties_by_definition"] = {
                str(row["id"]): row.get("specialty", []) for row in rows
            }
        if ambiguous:
            entry["ambiguous"] = sorted(set(ambiguous))
        if unsupported:
            entry["unsupported"] = sorted(set(unsupported))
        recipes.append(entry)
        promote_group_to_recipe[str(promote_group)] = semantic_recipe_key
        for definition_id in definition_ids:
            definition_to_recipe[str(definition_id)] = semantic_recipe_key

    return {
        "version": 1,
        "catalog_version": "4.4.0",
        "sources": {
            "units": relative(units_path),
            "localization_en": relative(localization_dir / "Loc_ENG_US.txt"),
            "localization_ru": relative(localization_dir / "Loc_RUS_RU.txt"),
            "localization_key_mapping": relative(
                localization_dir / "Loc_Key_Mapping.txt"
            ),
            "established_recipe_reference": relative(recipes_path),
        },
        "conventions": {
            "qualitative_amount_pct": QUALITATIVE_AMOUNT,
            "qualitative_chance_pct": QUALITATIVE_CHANCE,
            "specialty_fallback": FALLBACK,
            "note": (
                "Exact 4.4 prose and Loc_Key_Mapping win. An established recipe is copied only when "
                "mapped key, exact 4.4 text, and target match; otherwise the exact 4.4 prose is parsed."
            ),
        },
        "audit": {
            "unit_definition_count": len(units),
            "promote_group_count": len(recipes),
            "ability_key_count": len(ability_keys),
            "direct_localized_ability_key_count": len(direct_ability_keys),
            "mapped_ability_key_count": len(mapped_ability_keys),
            "resolved_ability_key_count": len(
                direct_ability_keys | mapped_ability_keys
            ),
            "missing_ability_key_count": len(unresolved_ability_keys),
            "passive_key_count": len(passive_keys),
            "localized_passive_key_count": len(localized_passive_keys),
            "resolution_counts": dict(sorted(resolution_counts.items())),
            "resolution_mode_counts": dict(sorted(resolution_mode_counts.items())),
            "ambiguous_recipe_count": sum(
                bool(row.get("ambiguous")) for row in recipes
            ),
            "unsupported_recipe_count": sum(
                bool(row.get("unsupported")) for row in recipes
            ),
            "noncombat_recipe_count": sum(not row["steps"] for row in recipes),
        },
        "promote_group_to_recipe": promote_group_to_recipe,
        "definition_to_recipe": definition_to_recipe,
        "recipe_count": len(recipes),
        "recipes": recipes,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--units", type=Path, default=DEFAULT_UNITS)
    parser.add_argument("--recipes", type=Path, default=DEFAULT_RECIPES)
    parser.add_argument("--localization-dir", type=Path, default=LOCALIZATION_DIR)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument(
        "--check",
        action="store_true",
        help="fail instead of writing if output has drifted",
    )
    args = parser.parse_args()
    result = build(
        args.units.resolve(),
        args.recipes.resolve(),
        args.localization_dir.resolve(),
    )
    if args.check:
        if not args.output.exists() or json.loads(args.output.read_text()) != result:
            raise SystemExit(f"{args.output} is stale; rerun without --check")
        print(f"{args.output}: up to date ({result['recipe_count']} chain recipes)")
        return
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n")
    audit = result["audit"]
    print(
        f"wrote {args.output}: {result['recipe_count']} chain recipes, "
        f"resolution={audit['resolution_counts']}, ambiguous={audit['ambiguous_recipe_count']}"
    )


if __name__ == "__main__":
    main()
