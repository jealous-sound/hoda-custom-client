#!/usr/bin/env python3
import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "data/hero_pool.json"
CHARACTERS = ROOT / "client-additional-data/characters.json"
CLI_DESCRIPTION = (
    "Build the hero pool the backend rolls store packs from and levels heroes with, in client catalog order."
)

TIER_ORDER = ("I", "II", "III", "IV")
POOL_VERSION = 2


def initial_tier(character):
    tiers = character.get("tiers") or {}
    for tier in TIER_ORDER:
        if tier in tiers and (tiers[tier].get("max_level") or 0) >= 1:
            return tier
    return None


def tier_levels(character):
    tiers = character.get("tiers") or {}
    return {tier: tiers[tier].get("max_level") or 0 for tier in TIER_ORDER if tier in tiers}


def build():
    catalog = json.loads(CHARACTERS.read_text(encoding="utf-8"))["characters"]
    heroes = []
    for character in catalog:
        tier = initial_tier(character)
        if tier is None:
            continue
        heroes.append(
            {
                "id": character["id"],
                "rarity": character["rarity"],
                "obtainable": bool(character.get("obtainable")),
                "initial_tier": tier,
                "name": character["name"],
                "faction": character["faction"],
                "giant": bool(character.get("giant")),
                "max_tier": character["max_tier"],
                "tier_levels": tier_levels(character),
            }
        )
    return {"version": POOL_VERSION, "heroes": heroes}


def render():
    return json.dumps(build(), ensure_ascii=False, indent=1) + "\n"


def main():
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    text = render()
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding="utf-8") != text:
            print(f"{OUTPUT.relative_to(ROOT)} is stale", file=sys.stderr)
            return 1
        return 0
    OUTPUT.write_text(text, encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
