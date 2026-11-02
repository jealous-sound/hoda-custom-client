#!/usr/bin/env python3
import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "data/news_feed.json"
NEXUS_MAPS = ROOT / "data/nexus_maps.json"
LOCALIZATION_DIR = (
    ROOT / "client-additional-data/Heroes-of-Dragon-Age-v4-4-0-cache-androidp1/com.ea.game.dragonage_row/cache/Localization"
)
LOCALIZATIONS = ("Loc_ENG_US.txt", "Loc_RUS_RU.txt")
CLI_DESCRIPTION = "Build the PvE news feed text table from the original localization keys."

DAILY_SUMMARY_FORMAT = "Nexus_Event_{}_Desc_Key"
DAILY_MESSAGE_FORMAT = "Nexus_Event_{}_Desc_Long_Key"
UPDATED_DAILY_MESSAGE_FORMAT = "Nexus_Event_{}_Desc_Long_Key_UPDATED"


def desc(short, long):
    return {"title": "", "subtitle": "", "summary": short, "message": long}


def announcement(prefix, short):
    return {
        "title": f"{prefix}_TITLE_KEY" if prefix.isupper() else f"{prefix}_title_key",
        "subtitle": f"{prefix}_SUBTITLE_KEY" if prefix.isupper() else f"{prefix}_subtitle_key",
        "summary": short,
        "message": f"{prefix}_MESSAGE_KEY" if prefix.isupper() else f"{prefix}_message_key",
    }


def event_desc(stem):
    return desc(f"NEXUS_EVENT_{stem}_SHORT_DESC_KEY", f"NEXUS_EVENT_{stem}_LONG_DESC_KEY")


def camel_desc(stem):
    return desc(f"{stem}_Desc_Key", f"{stem}_Desc_Long_Key")


def daily(stem):
    return desc(DAILY_SUMMARY_FORMAT.format(stem), DAILY_MESSAGE_FORMAT.format(stem))


def faction_daily(stem):
    return desc(DAILY_SUMMARY_FORMAT.format(stem), UPDATED_DAILY_MESSAGE_FORMAT.format(stem))


NEWS = {
    "Danarius": announcement("interstitial_tevinter_nexus_event_announcement", "Nexus_Event_Danarius_Chase_Desc_Key"),
    "ElvenMages": event_desc("ELVEN_MAGES"),
    "ElvenRogues": event_desc("ELVEN_ROGUES"),
    "EnemiesOfTevinter": event_desc("ENEMIES_OF_TEVINTER"),
    "Maric": announcement(
        "interstitial_tevinter_ferelden_event_announcement", "Nexus_Event_King_Maric_Chase_Desc_Key"
    ),
    "MaskedMayhem": event_desc("MASKED_MAYHEM"),
    "SkyHorror": event_desc("BLACK_FRIDAY"),
    "Tallis": announcement(
        "INTERSTITIAL_QUNARI_QUEST_NEXUS_ANNOUNCEMENT", "NEXUS_QUNARI_QUEST_NOVEMBER_13_SHORT_MESSAGE_KEY"
    ),
    "WinterWardenland": event_desc("WINTER_WARDENLAND"),
    "WoodlandCritters": event_desc("WOODLAND_CRITTERS"),
    "WoodlandSpirits": desc(
        "NEXUS_EVENT_WOODLAND_CRITTERS_SHORT_DESC_KEY_JUSTINIA", "NEXUS_EVENT_WOODLAND_CRITTERS_LONG_DESC_KEY_JUSTINIA"
    ),
    "ADreamMadeReality": desc("NEXUS_EVENT_DREAM_MADE_REALITY_DESC_SHORT", "NEXUS_EVENT_DREAM_MADE_REALITY_DESC_LONG"),
    "BreachOpens": announcement(
        "INTERSTITIAL_THE_BREACH_OPENS_NEXUS_ANNOUNCEMENT", "NEXUS_THE_BREACH_OPENS_NOVEMBER_20_SHORT_MESSAGE_KEY"
    ),
    "BreachOpensAgain": desc(
        "NEXUS_THE_BREACH_OPENS_NOVEMBER_20_SHORT_MESSAGE_KEY", "NEXUS_THE_BREACH_OPENS_LONG_MESSAGE_KEY_WOOLSLEY"
    ),
    "Halloween2015": camel_desc("Nexus_Event_Halloween_2015_Boss"),
    "SeekersDuty": announcement(
        "INTERSTITIAL_SEEKERS_DUTY_NEXUS_ANNOUNCEMENT", "NEXUS_SEEKERS_DUTY_NOVEMBER_20_SHORT_MESSAGE_KEY"
    ),
    "UrnOfSacredAshes": announcement(
        "interstitial_sacred_ashes_nexus_announcement", "Nexus_Event_UrnOfSacredAshes_Boss_Desc_Key"
    ),
    "LastStraw": announcement("interstitial_last_straw_nexus", "Nexus_Event_Last_Straw_Gauntlet_Desc_Key"),
    "WitchWilds": announcement(
        "interstitial_witch_of_the_wilds_nexus", "Nexus_Event_Witch_Of_The_Wilds_Gauntlet_Desc_Key"
    ),
    "Thanksgiving2014": event_desc("THANKSGIVING"),
    "AnvilVoid": event_desc("ANVIL_OF_THE_VOID"),
    "BanishedPlane": desc("NEXUS_BANISHED_PLANE_SHORT_MESSAGE_KEY", "NEXUS_BANISHED_PLANE_LONG_MESSAGE_KEY"),
    "DoubleXP2015": desc("NEXUS_EVENT_EXP_NUGS_SUBTITLE_KEY", "NEXUS_EVENT_EXP_NUGS_DESC_KEY"),
    "GiantWisps2304": desc("NEXUS_EVENT_GIANT_WISP_DESC_KEY", "NEXUS_EVENT_GIANT_WISP_DESC_KEY"),
    "Giants2302": desc("NEXUS_EVENT_GIANT_DESC_KEY", "NEXUS_EVENT_GIANT_DESC_KEY"),
    "PotionDrop2015": desc("NEXUS_EVENT_POTION_DROP_SUBTITLE_KEY", "NEXUS_EVENT_POTION_DROP_DESC_KEY"),
    "WesternApproachInquisition": camel_desc("Nexus_Event_Western_Approach_Inquisition_Rising"),
    "WesternApproachRedLyrium": camel_desc("Nexus_Event_Western_Approach_Written_In_Red"),
    "PHnT": desc("", ""),
    "DailyMonday": faction_daily("Monday_Red_Faction_Daily"),
    "DailyTuesday": faction_daily("Tuesday_Blue_Faction_Daily"),
    "DailyWednesday": faction_daily("Wednesday_Black_Faction_Daily"),
    "DailyThursday": faction_daily("Thursday_White_Faction_Daily"),
    "DailyFriday": daily("Friday_Daily"),
    "DailySaturday": daily("Saturday_Wisp_Daily"),
    "DailySunday": daily("Sunday_GoldXP_Daily"),
}


def localization_keys(name):
    with (LOCALIZATION_DIR / name).open(encoding="utf-8", errors="replace") as file:
        return {line.partition("|")[0].lower() for line in file if "|" in line}


def build():
    event_ids = [event["id"] for event in json.loads(NEXUS_MAPS.read_text(encoding="utf-8"))]
    if set(event_ids) != set(NEWS):
        raise ValueError(f"news table differs from nexus events: {sorted(set(event_ids) ^ set(NEWS))}")
    for name in LOCALIZATIONS:
        known = localization_keys(name)
        missing = sorted(key for text in NEWS.values() for key in text.values() if key and key.lower() not in known)
        if missing:
            raise ValueError(f"{name} lacks {missing}")
    return [{"event": event_id, **NEWS[event_id]} for event_id in event_ids]


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
