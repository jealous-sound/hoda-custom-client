#!/usr/bin/env python3
import argparse
import json
import random
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "data/pvp_rules.json"
ROSTER_SOURCES = [ROOT / "data/campaign_rosters.json", ROOT / "data/nexus_rosters.json"]
CHARACTERS = ROOT / "client-additional-data/characters.json"
LOCALIZATION = (
    ROOT
    / "client-additional-data/Heroes-of-Dragon-Age-v4-4-0-cache-androidp1/com.ea.game.dragonage_row/cache/Localization"
    / "Loc_RUS_RU.txt"
)
CLI_DESCRIPTION = "Build the tunable PvP rule table, bot ladder and opponent squad pool."

SQUAD_CELLS = 5
GIANT_CELL = 4
MIN_SQUAD_UNITS = 4
BOT_COUNT = 240
BOT_SEED = 1604
BOT_TOP_TROPHIES = 6000
BOT_AVATAR_RARITIES = {"Rare", "Epic", "Legendary"}

NAME_STARTS = (
    "Ash", "Black", "Blood", "Bright", "Cold", "Crimson", "Dark", "Dawn", "Dread", "Dusk", "Ember", "Fade", "Fel",
    "Frost", "Gloom", "Golden", "Grey", "Grim", "Iron", "Lyrium", "Night", "Old", "Pale", "Raven", "Red", "Rune",
    "Shadow", "Silver", "Sky", "Spirit", "Steel", "Storm", "Sun", "Thorn", "Veil", "White", "Wild", "Winter",
)
NAME_ENDS = (
    "Archer", "Bane", "Blade", "Bow", "Breaker", "Caller", "Champion", "Claw", "Crow", "Drake", "Fang", "Flame",
    "Fury", "Guard", "Hand", "Heart", "Herald", "Hunter", "Keeper", "Knight", "Mage", "Rider", "Rogue", "Sentinel",
    "Shield", "Singer", "Slayer", "Stalker", "Strider", "Warden", "Watcher", "Weaver", "Wolf", "Wyrm",
)

RULES = {
    "trophies": {
        "base_gain": 30,
        "base_loss": 15,
        "points_per_step": 40,
        "min_gain": 8,
        "max_gain": 60,
        "min_loss": 5,
        "max_loss": 40,
    },
    "matchmaking": {
        "window_below": 150,
        "window_above": 250,
        "daily_nexts": 10,
        "bot_daily_drift": 40,
        "level_spread": 2,
    },
    "squads": {
        "rating_base": 1500,
        "rating_per_level": 350,
        "rating_per_level_squared": 6,
        "rating_max_shift_pct": 25,
        "trophies_per_shift_pct": 10,
        "candidates": 6,
    },
    "runes": {
        "first_rune_level": 5,
        "levels_per_rune": 20,
        "levels_per_rarity": 15,
        "max_rarity": 6,
    },
    "rewards": {
        "win_gold_base": 60,
        "win_gold_per_level": 6,
        "loss_gold_pct": 25,
        "win_xp_base": 20,
        "win_xp_per_level": 2,
        "loss_xp_pct": 25,
    },
    "defense": {
        "attack_interval_seconds": 14400,
        "max_attacks_per_login": 3,
        "win_pct": 40,
        "loss_share_pct": 50,
    },
}




def group(name, health=0, power=0):
    return {"groups": [name], "factions": [], "units": [], "health_pct": health, "power_pct": power}


def faction(name, health=0, power=0):
    return {"groups": [], "factions": [name], "units": [], "health_pct": health, "power_pct": power}


def event(stem, *effects, suffix="_KEY"):
    return {"title_key": f"{stem}_TITLE{suffix}", "text_key": f"{stem}_DESC{suffix}", "effects": list(effects)}


EVENTS = {
    "duration_seconds": 259200,
    "win_banners_start": 25,
    "win_banners_peak_bonus": 5,
    "win_banners_steps_to_peak": 10,
    "win_banners_decline_slowdown": 2,
    "win_banners_floor_pct": 60,
    "matchmaking_banner_window": 150,
    "loss_banners": 5,
    "top_bot_banners": 3000,
    "milestones": [
        {"banners": 100, "gold": 500, "gems": 0},
        {"banners": 250, "gold": 1000, "gems": 0},
        {"banners": 500, "gold": 0, "gems": 5},
        {"banners": 1000, "gold": 0, "gems": 10},
        {"banners": 2000, "gold": 0, "gems": 20},
    ],
    "rank_rewards": [
        {"first": 1, "last": 1, "gold": 0, "gems": 50},
        {"first": 2, "last": 10, "gold": 0, "gems": 25},
        {"first": 11, "last": 50, "gold": 0, "gems": 10},
        {"first": 51, "last": 120, "gold": 5000, "gems": 0},
        {"first": 121, "last": BOT_COUNT + 1, "gold": 1000, "gems": 0},
    ],
    "catalog": [
        event("PVP_BLESSED_OF_ORLAIS", faction("White", health=300, power=300)),
        event("PVP_BLACK_FACTION_JULY_11", faction("Black", health=250, power=250)),
        event("PVP_BLUE_FACTION_AUG_03", faction("Blue", health=250, power=250)),
        event("PVP_BLIGHTED_JULY_09", group("Blight", health=500, power=500)),
        event("PVP_HEDGE_MAGES_FEBRUARY_16", group("Mage", health=500)),
        event(
            "PVP_BEASTS_AND_OUTLAWS_JUN_25",
            group("Outlaw", health=250, power=250),
            group("Beast", health=250, power=250),
        ),
        event(
            "PVP_ENTER_THE_FADE_MAY_19",
            group("Spirit", health=350, power=350),
            group("Demon", health=350, power=350),
        ),
        event("PVP_BLACK_FACTION_2", group("Demon", power=200), group("Blight", health=300)),
        event(
            "PVP_AVVAR_AND_DRAGONS_JUN_27",
            group("Avvar", health=500, power=500),
            group("Dragon", health=200),
        ),
    ],
}


def localization_keys():
    with LOCALIZATION.open(encoding="utf-8", errors="replace") as file:
        return {line.partition("|")[0] for line in file if "|" in line}


def validate_events(characters):
    keys = localization_keys()
    groups = {name for character in characters.values() for name in character["groups"]}
    for entry in EVENTS["catalog"]:
        for key in (entry["title_key"], entry["text_key"]):
            if key not in keys:
                raise ValueError(f"missing text {key}")
        for effect in entry["effects"]:
            missing = set(effect["groups"]) - groups
            if missing:
                raise ValueError(f"{entry['title_key']}: unknown groups {sorted(missing)}")


def rating(characters, roster):
    total = 0
    for cell in roster:
        if cell is None:
            continue
        stats = characters[cell["id"]]["tiers"][cell["tier"]]
        steps = min(cell["level"], stats["max_level"]) - 1
        total += stats["base_power"] + stats["power_per_level"] * steps
        total += stats["base_health"] + stats["health_per_level"] * steps
    return total


def clean_cell(characters, cell, index):
    if cell is None:
        return None
    character = characters.get(cell["id"])
    if character is None or cell["tier"] not in character["tiers"]:
        return None
    if character["giant"] != (index == GIANT_CELL):
        return None
    return {"id": cell["id"], "tier": cell["tier"], "level": cell.get("level", 1)}


def build_squads(characters):
    squads = {}
    for path in ROSTER_SOURCES:
        for campaign in json.loads(path.read_text(encoding="utf-8"))["campaigns"]:
            for node in campaign["nodes"]:
                cells = [clean_cell(characters, cell, i) for i, cell in enumerate(node["roster"][:SQUAD_CELLS])]
                cells += [None] * (SQUAD_CELLS - len(cells))
                if sum(cell is not None for cell in cells) < MIN_SQUAD_UNITS:
                    continue
                squads[json.dumps(cells, sort_keys=True)] = cells
    table = [{"rating": rating(characters, cells), "cells": cells} for cells in squads.values()]
    return sorted(table, key=lambda squad: (squad["rating"], json.dumps(squad["cells"], sort_keys=True)))


def build_bots(characters):
    rng = random.Random(BOT_SEED)
    names = [start + end for start in NAME_STARTS for end in NAME_ENDS]
    rng.shuffle(names)
    avatars = sorted(
        character["id"]
        for character in characters.values()
        if character["obtainable"] and not character["giant"] and character["rarity"] in BOT_AVATAR_RARITIES
    )
    bots = []
    for index in range(BOT_COUNT):
        position = index / (BOT_COUNT - 1)
        bots.append(
            {
                "name": names[index],
                "avatar": rng.choice(avatars),
                "trophies": round(BOT_TOP_TROPHIES * position * position),
            }
        )
    return bots


def build():
    catalog = json.loads(CHARACTERS.read_text(encoding="utf-8"))["characters"]
    characters = {character["id"]: character for character in catalog}
    validate_events(characters)
    return {**RULES, "events": EVENTS, "bots": build_bots(characters), "squad_pool": build_squads(characters)}


def main():
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    text = json.dumps(build(), ensure_ascii=False, indent=1) + "\n"
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding="utf-8") != text:
            print(f"{OUTPUT.relative_to(ROOT)} is stale", file=sys.stderr)
            return 1
        return 0
    OUTPUT.write_text(text, encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
