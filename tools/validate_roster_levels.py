#!/usr/bin/env python3
import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CHARACTERS = ROOT / "client-additional-data/characters.json"
CUSTOM_HEROES = ROOT / "data/custom_heroes.json"
REQUIRED_ROSTERS = (ROOT / "data/tutorial_intro.json",)
LOCAL_ROSTERS = (ROOT / "client-additional-data/player_starter.json",)
DEFAULT_TIER = "I"
DEFAULT_LEVEL = 1
CLI_DESCRIPTION = (
    "Check every roster file the game loads at startup: each unit exists in the catalog, has the tier it "
    "is listed at, and sits at a level that tier allows. The game exits on any of these, so a re-tier that "
    "lowers a level cap must not leave a roster behind."
)


def catalog_heroes():
    heroes = json.loads(CHARACTERS.read_text(encoding="utf-8"))["characters"]
    heroes += json.loads(CUSTOM_HEROES.read_text(encoding="utf-8"))["characters"]
    return {hero["id"]: hero for hero in heroes}


def roster_entries(document):
    for entry in document.get("characters", []):
        yield "characters", entry
    for campaign in document.get("campaigns", []):
        for node in campaign.get("nodes", []):
            where = f"{campaign.get('campaign')} step {node.get('step')}"
            for entry in node.get("roster", []):
                yield where, entry


def entry_problem(entry, catalog):
    uid = entry.get("id")
    hero = catalog.get(uid)
    if hero is None:
        return f"{uid!r} is not in the catalog"
    tier = entry.get("tier", DEFAULT_TIER)
    stats = hero.get("tiers", {}).get(tier)
    if stats is None:
        return f"{uid} has no tier {tier}"
    level = entry.get("level", DEFAULT_LEVEL)
    if not 1 <= level <= stats["max_level"]:
        return f"{uid} level {level} outside 1..{stats['max_level']} for tier {tier}"
    return None


def problems(document, catalog):
    found = []
    for where, entry in roster_entries(document):
        problem = entry_problem(entry, catalog)
        if problem:
            found.append(f"{where}: {problem}")
    return found


def main():
    argparse.ArgumentParser(description=CLI_DESCRIPTION).parse_args()
    catalog = catalog_heroes()
    paths = list(REQUIRED_ROSTERS) + [path for path in LOCAL_ROSTERS if path.exists()]
    failures = 0
    for path in paths:
        for problem in problems(json.loads(path.read_text(encoding="utf-8")), catalog):
            print(f"{path.relative_to(ROOT)}: {problem}", file=sys.stderr)
            failures += 1
    if failures:
        return 1
    print(f"roster levels: {len(paths)} files ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
