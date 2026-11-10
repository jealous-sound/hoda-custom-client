#!/usr/bin/env python3
import argparse
import collections
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TABLE = ROOT / "data/hero_rarity.json"
CHARACTERS = ROOT / "client-additional-data/characters.json"
CUSTOM_HEROES = ROOT / "data/custom_heroes.json"
NEXUS_MAPS = ROOT / "data/nexus_maps.json"
PVP_RULES = ROOT / "data/pvp_rules.json"
CLI_DESCRIPTION = (
    "Check the hero rarity table: every hero has one row, Legendary stays under its cap, no character "
    "holds more than one Legendary form, the shipped catalog carries the same rarities, and no reward "
    "from play hands out a Legendary."
)

RARITIES = ("Common", "Uncommon", "Rare", "Epic", "Legendary", "Mythic")
SUPPORTED_VERSION = 1
BORN_ONLY_RARITY = "Mythic"
CAPPED_RARITY = "Legendary"
MIN_FORMS_PER_CHARACTER = 2
UPGRADE_MATERIAL_GROUP = "Wisp"
GUARANTEED_LEGENDARY_VOUCHER = "bloodstone"


def catalog_heroes():
    heroes = json.loads(CHARACTERS.read_text(encoding="utf-8"))["characters"]
    heroes += json.loads(CUSTOM_HEROES.read_text(encoding="utf-8"))["characters"]
    return {hero["id"]: hero for hero in heroes}


def rarity_in_name(hero):
    words = re.findall(r"[A-Za-z]+", hero["name"])
    named = [word for word in words if word in RARITIES]
    return named[0] if named else None


def problems(table, catalog):
    found = []
    if table.get("version") != SUPPORTED_VERSION:
        found.append(f"version {table.get('version')!r}, want {SUPPORTED_VERSION}")
    rows = table.get("heroes", [])
    seen = collections.Counter(row.get("id") for row in rows)
    found += [f"{uid}: {count} rows" for uid, count in seen.items() if count > 1]
    found += [f"{uid}: not a hero in the catalog" for uid in seen if uid not in catalog]
    found += [f"{uid}: no row" for uid in catalog if uid not in seen]

    capped = 0
    legendary_forms = collections.Counter()
    forms = collections.Counter()
    for row in rows:
        hero = catalog.get(row.get("id"))
        if hero is None:
            continue
        uid, rarity = row["id"], row.get("rarity")
        if rarity not in RARITIES:
            found.append(f"{uid}: unknown rarity {rarity!r}")
            continue
        if not str(row.get("reason", "")).strip():
            found.append(f"{uid}: no reason")
        named_tier = rarity_in_name(hero)
        if named_tier and rarity != named_tier:
            found.append(f"{uid}: named {named_tier} but proposed {rarity}")
        if (rarity == BORN_ONLY_RARITY) != (hero["rarity"] == BORN_ONLY_RARITY):
            found.append(f"{uid}: {BORN_ONLY_RARITY} is only for heroes born {BORN_ONLY_RARITY}")
        if rarity == CAPPED_RARITY and not named_tier:
            capped += 1
        character = row.get("character")
        if character:
            forms[character] += 1
            legendary_forms[character] += rarity == CAPPED_RARITY

    cap = table.get("legendary_cap", 0)
    if capped > cap:
        found.append(f"{capped} {CAPPED_RARITY} heroes, cap is {cap}")
    found += [
        f"{character}: {count} {CAPPED_RARITY} forms, want at most one"
        for character, count in legendary_forms.items()
        if count > 1
    ]
    found += [
        f"{character}: a character key needs at least {MIN_FORMS_PER_CHARACTER} forms"
        for character, count in forms.items()
        if count < MIN_FORMS_PER_CHARACTER
    ]
    return found


def unapplied(table, catalog):
    return [
        f"{row['id']}: the catalog ships {catalog[row['id']]['rarity']}, the table says {row['rarity']}"
        for row in table.get("heroes", [])
        if row.get("id") in catalog and catalog[row["id"]]["rarity"] != row.get("rarity")
    ]


def prizes_in(node):
    if isinstance(node, dict):
        if "kind" in node and "id" in node:
            yield node
        for value in node.values():
            yield from prizes_in(value)
    elif isinstance(node, list):
        for value in node:
            yield from prizes_in(value)


def play_reward_problems(nexus_maps, pvp_rules, catalog):
    found = []
    capped_number = RARITIES.index(CAPPED_RARITY) + 1
    for event in nexus_maps:
        for difficulty in event["difficulties"]:
            for node in difficulty["nodes"]:
                if capped_number in node.get("reward_rarity", []):
                    found.append(f"{difficulty['id']} step {node['step']}: the Nexus rolls a {CAPPED_RARITY} hero")
    for prize in prizes_in(pvp_rules.get("events", {})):
        hero = catalog.get(prize["id"]) if prize["kind"] == "unit" else None
        if hero and hero["rarity"] == CAPPED_RARITY and UPGRADE_MATERIAL_GROUP not in hero.get("groups", []):
            found.append(f"{prize['id']}: a PvP prize hands out a {CAPPED_RARITY} hero")
        if prize["kind"] == "voucher" and prize["id"] == GUARANTEED_LEGENDARY_VOUCHER:
            found.append(f"a PvP prize hands out a {GUARANTEED_LEGENDARY_VOUCHER} voucher")
    return found


def shipped_play_reward_problems(catalog):
    nexus_maps = json.loads(NEXUS_MAPS.read_text(encoding="utf-8"))
    pvp_rules = json.loads(PVP_RULES.read_text(encoding="utf-8"))
    return play_reward_problems(nexus_maps, pvp_rules, catalog)


def summary(table):
    counts = collections.Counter(row["rarity"] for row in table["heroes"])
    return ", ".join(f"{rarity} {counts[rarity]}" for rarity in RARITIES)


def main():
    argparse.ArgumentParser(description=CLI_DESCRIPTION).parse_args()
    table = json.loads(TABLE.read_text(encoding="utf-8"))
    catalog = catalog_heroes()
    found = problems(table, catalog) or unapplied(table, catalog) or shipped_play_reward_problems(catalog)
    if found:
        for problem in found:
            print(f"hero rarity: {problem}", file=sys.stderr)
        return 1
    print(f"hero rarity ok: {summary(table)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
