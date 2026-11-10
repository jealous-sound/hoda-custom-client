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
CLI_DESCRIPTION = (
    "Check the proposed hero rarity table: every hero has one row, Legendary stays under its cap "
    "and no character holds more than one Legendary form."
)

RARITIES = ("Common", "Uncommon", "Rare", "Epic", "Legendary", "Mythic")
SUPPORTED_VERSION = 1
BORN_ONLY_RARITY = "Mythic"
CAPPED_RARITY = "Legendary"
MIN_FORMS_PER_CHARACTER = 2


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


def summary(table):
    counts = collections.Counter(row["rarity"] for row in table["heroes"])
    return ", ".join(f"{rarity} {counts[rarity]}" for rarity in RARITIES)


def main():
    argparse.ArgumentParser(description=CLI_DESCRIPTION).parse_args()
    table = json.loads(TABLE.read_text(encoding="utf-8"))
    found = problems(table, catalog_heroes())
    if found:
        for problem in found:
            print(f"hero rarity: {problem}", file=sys.stderr)
        return 1
    print(f"hero rarity ok: {summary(table)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
