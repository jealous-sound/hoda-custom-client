#!/usr/bin/env python3
import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HERO_AURAS = ROOT / "data/hero_auras.json"
ENEMY_ROSTERS = (ROOT / "data/campaign_rosters.json", ROOT / "data/nexus_rosters.json")
FLAT_RESIST_STAT = "raw_damage_resist"
EVIDENCED_AUTHENTICITY = "sourced"
CLI_DESCRIPTION = (
    "Check that no reconstructed enemy squad fields a hero whose aura resists a flat amount of every hit. 4.4.0 "
    "tuned those auras (1500 to 5000 damage per hit) for fortified late-game power that the reconstructed level "
    "ladders never reach, so every hit on the heroes they cover lands for 1. Squads marked sourced keep what the "
    "evidence shows."
)


def flat_resist_heroes(auras):
    found = {}
    for item in auras["items"]:
        for effect in item["effects"]:
            if effect["stat"] == FLAT_RESIST_STAT:
                found[item["id"]] = max(found.get(item["id"], 0), effect["value"])
    return found


def problems(document, flat_resist):
    found = []
    for campaign in document.get("campaigns", []):
        for node in campaign.get("nodes", []):
            if node.get("authenticity") == EVIDENCED_AUTHENTICITY:
                continue
            for cell in node.get("roster", []):
                uid = cell.get("id") if cell else None
                if uid in flat_resist:
                    found.append(
                        f"{campaign.get('campaign')} step {node.get('step')}: {uid} resists "
                        f"{flat_resist[uid]} damage per hit"
                    )
    return found


def main():
    argparse.ArgumentParser(description=CLI_DESCRIPTION).parse_args()
    flat_resist = flat_resist_heroes(json.loads(HERO_AURAS.read_text(encoding="utf-8")))
    failures = 0
    for path in ENEMY_ROSTERS:
        for problem in problems(json.loads(path.read_text(encoding="utf-8")), flat_resist):
            print(f"{path.relative_to(ROOT)}: {problem}", file=sys.stderr)
            failures += 1
    if failures:
        return 1
    print(f"enemy rosters: {len(ENEMY_ROSTERS)} files field no flat damage resist")
    return 0


if __name__ == "__main__":
    sys.exit(main())
