#!/usr/bin/env python3
import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "data/nexus_game_changers.json"
NEXUS_MAPS = ROOT / "data/nexus_maps.json"
CHARACTERS = ROOT / "client-additional-data/characters.json"
LOCALIZATION_FILE = "Loc_ENG_US.txt"
LOCALIZATIONS = (
    ROOT
    / "client-additional-data/Heroes-of-Dragon-Age-v4-4-0-cache-androidp1/com.ea.game.dragonage_row/cache/Localization"
    / LOCALIZATION_FILE,
    *sorted((ROOT / "data/localization").glob(f"*/{LOCALIZATION_FILE}")),
)
CLI_DESCRIPTION = "Build the Nexus game changer table from original keys and archived event rules."

FACTION_BONUS_PCT = 150
FACTION_BONUS_KEY = "HODA_NEXUS_GAME_CHANGER_PLUS{0}HEALTH_PLUS{0}POWER_TO_{1}_FACTION"


def group(*names, health=0, power=0):
    return {"groups": list(names), "factions": [], "units": [], "health_pct": health, "power_pct": power}


def units(*ids, health=0, power=0):
    return {"groups": [], "factions": [], "units": list(ids), "health_pct": health, "power_pct": power}


def faction(name, health=0, power=0):
    return {"groups": [], "factions": [name], "units": [], "health_pct": health, "power_pct": power}


def original(key, *effects):
    return {"text_key": key, "effects": list(effects)}


def faction_day(name):
    effect = faction(name, health=FACTION_BONUS_PCT, power=FACTION_BONUS_PCT)
    return {"text_key": FACTION_BONUS_KEY.format(FACTION_BONUS_PCT, name.upper()), "effects": [effect]}


GAME_CHANGERS = {
    "Danarius": original("game_changer_200pcthealth_to_circle", group("Circle", health=200)),
    "ElvenMages": original(
        "NEXUS_GAME_CHANGER_PLUS300HEALTH_AND_PLUS125POWER_TO_ELVENMAGES", group("Mage", "Elves", health=300, power=125)
    ),
    "Maric": original("game_changer_200pctpower_to_qunari", group("Qunari", power=200)),
    "MaskedMayhem": original(
        "NEXUS_GAME_CHANGER_PLUS150HEALTH_PLUS150POWER_TO_MAGES", group("Mage", health=150, power=150)
    ),
    "Tallis": faction_day("White"),
    "WinterWardenland": original("NEXUS_GAME_CHANGER_PLUS200POWER_TO_WARDENS", group("Warden", power=200)),
    "ADreamMadeReality": original(
        "NEXUS_GAME_CHANGER_PLUS250HEALTH_AND_PLUS350POWER_TO_QUNARI", group("Qunari", health=250, power=350)
    ),
    "BreachOpens": original(
        "NEXUS_GAME_CHANGER_PLUS500PCT_POWER_TO_TEMPLARS_AND_400_Health_TO_SEEKERS_AND_PLUS250PCT_TO_INQUISITION",
        group("Templar", power=500),
        group("Seeker", health=400),
        group("Inquisition", health=250, power=250),
    ),
    "SeekersDuty": original(
        "game_changer_125pctpower_and_125pcthealth_to_chantry", group("Chantry", health=125, power=125)
    ),
    "UrnOfSacredAshes": original("Nexus_game_changer_300_power_blue_faction", faction("Blue", power=300)),
    "LastStraw": original("game_changer_300pctpower_to_mages", group("Mage", power=300)),
    "WitchWilds": original(
        "game_changer_125pctpower_and_200pcthealth_to_templar",
        group("Templar", health=200, power=125),
        units("UNIT_GREYWARDENALISTAIR", "UNIT_KINGALISTAIR", power=75),
    ),
    "AnvilVoid": original(
        "NEXUS_GAME_CHANGER_PLUS300_HEALTH_POWER_TO_BEAST_DEMON",
        group("Beast", health=300, power=300),
        group("Demon", health=100, power=100),
    ),
    "WesternApproachInquisition": original(
        "game_changer_35pctpower_and_health_to_red_lyrium", group("Red_Lyrium", health=35, power=35)
    ),
    "WesternApproachRedLyrium": original(
        "game_changer_35pctpower_and_health_to_inquisition", group("Inquisition", health=35, power=35)
    ),
    "DailyMonday": faction_day("White"),
    "DailyTuesday": faction_day("Red"),
    "DailyWednesday": faction_day("Blue"),
    "DailyThursday": faction_day("Black"),
}


def localization_keys():
    keys = set()
    for path in LOCALIZATIONS:
        with path.open(encoding="utf-8", errors="replace") as file:
            keys |= {line.partition("|")[0].lower() for line in file if "|" in line}
    return keys


def build():
    event_ids = [event["id"] for event in json.loads(NEXUS_MAPS.read_text(encoding="utf-8"))]
    characters = json.loads(CHARACTERS.read_text(encoding="utf-8"))["characters"]
    known_groups = {name for character in characters for name in character["groups"]}
    known_units = {character["id"] for character in characters}
    known_keys = localization_keys()
    unknown = sorted(set(GAME_CHANGERS) - set(event_ids))
    if unknown:
        raise ValueError(f"unknown events: {unknown}")
    table = []
    for event_id in event_ids:
        changer = GAME_CHANGERS.get(event_id)
        if changer is None:
            continue
        if changer["text_key"].lower() not in known_keys:
            raise ValueError(f"{event_id}: missing text {changer['text_key']}")
        for effect in changer["effects"]:
            missing = (set(effect["groups"]) - known_groups) | (set(effect["units"]) - known_units)
            if missing:
                raise ValueError(f"{event_id}: unknown targets {sorted(missing)}")
        table.append({"event": event_id, **changer})
    return table


def main():
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    text = json.dumps(build(), ensure_ascii=False, indent=2) + "\n"
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding="utf-8") != text:
            print(f"{OUTPUT.relative_to(ROOT)} is stale", file=sys.stderr)
            return 1
        return 0
    OUTPUT.write_text(text, encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
