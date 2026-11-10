#!/usr/bin/env python3

from __future__ import annotations

from collections import Counter
import json
import os
import re
import sys

from ability_metadata import MECHANIC_ORDER, infer_ability_mechanics, normalize_text


HERE = os.path.dirname(os.path.abspath(__file__))
CATALOG = os.path.join(HERE, "characters.json")
REFERENCE_DIR = os.path.join(HERE, "..", ".claude", "skills", "hoda-reference", "data")
UNIT_DATA = os.path.join(HERE, "HoDA_character_info", "UnitData.json")
ORIGINAL_LOCALIZATION_DIR = os.path.join(
    HERE, "Heroes-of-Dragon-Age-v4-4-0-cache-androidp1", "com.ea.game.dragonage_row", "cache", "Localization"
)
PROJECT_LOCALIZATION_DIR = os.path.join(HERE, "..", "data", "localization")
LANGUAGE_FILES = (
    "Loc_ENG_US.txt",
    "Loc_FRE_FR.txt",
    "Loc_GER_DE.txt",
    "Loc_ITA_IT.txt",
    "Loc_SPA_XM.txt",
    "Loc_POR_BR.txt",
    "Loc_DUT_NL.txt",
    "Loc_RUS_RU.txt",
    "Loc_JPN_JP.txt",
    "Loc_KOR_KR.txt",
    "Loc_CHS_CN.txt",
    "Loc_CHT_CN.txt",
)
RUSSIAN_FILE = "Loc_RUS_RU.txt"
PROJECT_KEY_PREFIX = "HODA_"
TIERS = ("I", "II", "III", "IV")
GEAR_SLOTS_FOR_EVERY_WEARER = 4
DESCRIPTION_CAPACITY = 1280
INFO_CAPACITY = 320
FACTIONS_CAPACITY = 128
GROUPS_CAPACITY = 256
FACTION_KEYS = (
    (1, "UNIT_DETAIL_FACTION_BLACK"),
    (2, "UNIT_DETAIL_FACTION_WHITE"),
    (4, "UNIT_DETAIL_FACTION_RED"),
    (8, "UNIT_DETAIL_FACTION_BLUE"),
)


def fail(message: str) -> None:
    raise SystemExit(f"character validation failed: {message}")


def localized_faction(source: dict, table: dict[str, str]) -> str:
    if source.get("Giant"):
        return table.get("UNIT_DETAIL_FACTION_GIANT", "")
    parts = []
    for flag, key in FACTION_KEYS:
        if source.get("Faction", 0) & flag:
            parts.append(table.get(key, ""))
    return ", ".join(parts)


def read_pipe_table(path: str) -> dict[str, str]:
    table: dict[str, str] = {}
    if not os.path.isfile(path):
        return table
    with open(path, encoding="utf-8-sig") as src:
        for line in src:
            key, separator, value = line.rstrip("\r\n").partition("|")
            if separator:
                table[key.upper()] = value
    return table


def runtime_table(name: str) -> dict[str, str]:
    table = read_pipe_table(os.path.join(ORIGINAL_LOCALIZATION_DIR, name))
    if os.path.isdir(PROJECT_LOCALIZATION_DIR):
        for area in sorted(os.listdir(PROJECT_LOCALIZATION_DIR)):
            table.update(read_pipe_table(os.path.join(PROJECT_LOCALIZATION_DIR, area, name)))
    return table


def loc_text(table: dict[str, str], key: str) -> str:
    if not key:
        return ""
    return table.get(key.upper()) or table.get(f"{PROJECT_KEY_PREFIX}{key}".upper(), "")


def resolved_tier_name(table: dict[str, str], uid: str, tier: str) -> str:
    return loc_text(table, f"{uid}{tier}") or table.get(uid.upper(), "")


def ability_text(table: dict[str, str], key: str) -> str:
    upper = key.upper()
    return table.get(f"{upper}_DESCRIPTION") or table.get(upper) or table.get(f"{PROJECT_KEY_PREFIX}{upper}", "")


def runtime_faction(row: dict, table: dict[str, str]) -> str:
    parts = [table.get("UNIT_DETAIL_FACTION_GIANT", "")] if row.get("giant") else []
    for flag, key in FACTION_KEYS:
        if (row.get("faction_id") or 0) & flag:
            parts.append(table.get(key, ""))
    return ", ".join(parts)


def localization_problems(row: dict, language: str, table: dict[str, str]) -> list[str]:
    uid = row["id"]
    problems = []
    for tier in row.get("tiers", {}):
        if not resolved_tier_name(table, uid, tier):
            problems.append(f"{language}: tier {tier} name is not localized")
    ability = ability_text(table, row["ability"]["key"])
    rune = loc_text(table, row.get("equipped_rune", ""))
    target = loc_text(table, row.get("target_weight", ""))
    if not ability:
        problems.append(f"{language}: ability {row['ability']['key']!r} is not localized")
    if row.get("equipped_rune") and not rune:
        problems.append(f"{language}: equipped rune {row['equipped_rune']!r} is not localized")
    if row.get("target_weight") and not target:
        problems.append(f"{language}: target weight {row['target_weight']!r} is not localized")
    titles = [loc_text(table, f"GALLERY_{group}_TITLE") for group in row.get("groups", [])]
    if not all(titles):
        problems.append(f"{language}: a group has no localized display name")
    description = "  ".join(value for value in (ability, rune, target) if value)
    if len(description.encode()) >= DESCRIPTION_CAPACITY:
        problems.append(f"{language}: localized description exceeds the runtime buffer")
    factions = runtime_faction(row, table)
    groups = ", ".join(titles)
    info = table.get("UNIT_DETAIL_FACTION_GROUP_LABEL_FORMAT", "").replace("{0}", factions).replace("{1}", groups)
    if len(factions.encode()) >= FACTIONS_CAPACITY or len(groups.encode()) >= GROUPS_CAPACITY:
        problems.append(f"{language}: localized faction or group list exceeds the runtime buffer")
    if len(info.encode()) >= INFO_CAPACITY:
        problems.append(f"{language}: localized faction and group label exceeds the runtime buffer")
    return problems


with open(CATALOG) as src:
    root = json.load(src)
with open(UNIT_DATA) as src:
    unit_data = json.load(src)
with open(os.path.join(REFERENCE_DIR, "characters.json")) as src:
    reference_characters = json.load(src)
with open(os.path.join(REFERENCE_DIR, "localization", "Loc_ENG_US.json")) as src:
    reference_loc_en = json.load(src)
characters = root.get("characters", [])
runtime_tables = {name: runtime_table(name) for name in LANGUAGE_FILES}

if root.get("version") != 2:
    fail(f"expected schema version 2, got {root.get('version')!r}")
if root.get("count") != 676 or len(characters) != 676:
    fail(f"expected 676 characters, got count={root.get('count')} rows={len(characters)}")
if len({row.get("id") for row in characters}) != len(characters):
    fail("character ids are not unique")

mechanic_counts: Counter[str] = Counter()
for row in characters:
    uid = row.get("id", "<missing id>")
    source = unit_data.get(uid)
    if source is None:
        fail(f"{uid}: missing source UnitData row")
    ability = row.get("ability", {})
    for field, value in (
        ("name", row.get("name")),
        ("ability.key", ability.get("key")),
        ("ability.text", ability.get("text")),
    ):
        if not value:
            fail(f"{uid}: empty {field}")
    if any(field.endswith("_ru") for field in list(row) + list(ability)):
        fail(f"{uid}: display text must come from the localization tables, not *_ru fields")
    if ability["text"] != normalize_text(ability["text"]):
        fail(f"{uid}: English ability spacing is not normalized")
    for language, table in runtime_tables.items():
        problems = localization_problems(row, language, table)
        if problems:
            fail(f"{uid}: {problems[0]}")
    russian = runtime_tables[RUSSIAN_FILE]
    if not re.search(r"[А-Яа-яЁё]", resolved_tier_name(russian, uid, row["max_tier"])):
        fail(f"{uid}: Russian name is not localized")
    if not re.search(r"[А-Яа-яЁё]", ability_text(russian, ability["key"])):
        fail(f"{uid}: Russian ability is not localized")
    if ability.get("target") not in {"One", "Row", "Column", "All"}:
        fail(f"{uid}: invalid target {ability.get('target')!r}")

    groups = source.get("Groups", [])
    expected_groups_en = [reference_loc_en.get(f"GALLERY_{group}_TITLE".upper(), "") for group in groups]
    if row.get("groups") != groups:
        fail(f"{uid}: group identifiers differ from UnitData")
    if row.get("groups_en") != expected_groups_en:
        fail(f"{uid}: localized groups differ from the complete reference localization")
    if any(not value for value in expected_groups_en):
        fail(f"{uid}: a group has no localized display name")
    if row.get("faction_en") != localized_faction(source, reference_loc_en):
        fail(f"{uid}: English faction name differs from the complete reference localization")

    rune_key = source.get("EquippedRuneDescriptionKey", "")
    target_key = source.get("TargetWeightDescriptionKey", "")
    expected_rune_en = reference_loc_en.get(rune_key.upper(), "")
    expected_target_en = reference_loc_en.get(target_key.upper(), "")
    expected_fields = {
        "equipped_rune": rune_key,
        "equipped_rune_text": expected_rune_en,
        "target_weight": target_key,
        "target_weight_text": expected_target_en,
    }
    for field, expected in expected_fields.items():
        if row.get(field) != expected:
            fail(f"{uid}: {field} differs from the complete reference localization")
    if rune_key and not expected_rune_en:
        fail(f"{uid}: equipped rune {rune_key!r} has no English reference text")
    if target_key and not expected_target_en:
        fail(f"{uid}: target weight {target_key!r} has no English reference text")
    if any(len(group.encode()) >= 24 for group in groups):
        fail(f"{uid}: group identifier exceeds the runtime buffer")

    expected = infer_ability_mechanics(ability["text"], row.get("specialties", []))
    mechanics = ability.get("mechanics")
    if mechanics != expected:
        fail(f"{uid}: mechanics {mechanics!r} do not match description-derived {expected!r}")
    unknown = set(mechanics) - set(MECHANIC_ORDER)
    if unknown:
        fail(f"{uid}: unknown mechanics {sorted(unknown)!r}")
    if ("ATTACK" in mechanics) == ("ATTACK_NONE" in mechanics):
        fail(f"{uid}: ability must either attack or explicitly not attack")
    mechanic_counts.update(mechanics)

    wears_gear = source.get("NumGearSlots", 0) > 0
    if wears_gear and row.get("gear_slots", 0) < GEAR_SLOTS_FOR_EVERY_WEARER:
        fail(f"{uid}: wears gear with fewer than {GEAR_SLOTS_FOR_EVERY_WEARER} slots")
    if not wears_gear and row.get("gear_slots", 0) != 0:
        fail(f"{uid}: has gear slots but the original unit wears no gear")

    for tier_name, stats in row.get("tiers", {}).items():
        for field in ("power_per_level", "health_per_level"):
            value = stats.get(field)
            if not isinstance(value, int) or value < 0:
                fail(f"{uid} tier {tier_name}: invalid {field} {value!r}")

by_id = {row["id"]: row for row in characters}
if "POWER_DRAIN" not in by_id["UNIT_SHAPESHIFTERMORRIGANI"]["ability"]["mechanics"]:
    fail("Shapeshifter Morrigan I must drain power")
for uid in ("UNIT_SHAPESHIFTERMORRIGANII", "UNIT_SHAPESHIFTERMORRIGANIII", "UNIT_SHAPESHIFTERMORRIGANIV"):
    if "POWER_DRAIN" in by_id[uid]["ability"]["mechanics"]:
        fail(f"{uid}: inherited a drain not present in its description")
if "GAIN_POWER_ALLIES" not in by_id["UNIT_LAMBERTVANREEVES"]["ability"]["mechanics"]:
    fail("Lambert van Reeves must grant ally power")
if set(("CURSE_HEALTH", "STUN")) - set(by_id["UNIT_NECROMANCERDORIAN"]["ability"]["mechanics"]):
    fail("Necromancer Dorian must curse Health and stun")
if "Chance for second attack" not in by_id["UNIT_THORNTON"]["ability"]["text"]:
    fail("Thornton's missing source description was not recovered")
envy = by_id["UNIT_ENVYDEMON"]
if not loc_text(runtime_tables[RUSSIAN_FILE], envy["equipped_rune"]).startswith("<color=green>"):
    fail("Envy Demon must display its green equipped-rune description")
if envy["target_weight"] != "AURA_VERY_ELUSIVE":
    fail("Envy Demon must use the very-elusive targeting description")

required_mechanics = {
    "GAIN_POWER_ALLIES": (
        "UNIT_ANORAMACTIR",
        "UNIT_REDNUGLYSWEATER",
        "UNIT_SERSTROUD",
        "UNIT_GOLDENHALLA",
        "UNIT_HAKKONITEDEFENDER",
        "UNIT_TITANHEARTGUARDIAN",
        "UNIT_CASSIAN",
        "UNIT_XENONTHEANTIQUARIAN",
        "UNIT_HUNTERHORNSHRIKE",
        "UNIT_ARCHIVIST",
    ),
    "HEAL_ALLIES": (
        "UNIT_GOLDENHALLA",
        "UNIT_HAKKONITEDEFENDER",
        "UNIT_TITANHEARTGUARDIAN",
        "UNIT_CASSIAN",
    ),
    "GAIN_POWER_SELF": (
        "UNIT_PIOTINAEDUCAN",
        "UNIT_GIBBERINGHORROR",
        "UNIT_GHOSTOFTHESPIRE",
        "UNIT_THEHIGHESTONE",
    ),
    "HEAL_SELF": ("UNIT_NUGGALOPE",),
    "MULTIATTACK": (
        "UNIT_WEREWOLFALPHA",
        "UNIT_AURELIANTITUS",
        "UNIT_IMPERIALARCHON",
        "UNIT_PALA",
        "UNIT_MALIPHANT",
        "UNIT_JERRIKDACE",
        "UNIT_RENN",
        "UNIT_REDTEMPLARSHADOW",
        "UNIT_SHA-BRYTOLSTALKER",
        "UNIT_SABOTEURGARRETTHAWKE",
        "UNIT_SABOTEURMARIANHAWKE",
        "UNIT_NAISHE",
        "UNIT_MARIUS",
        "UNIT_WARDEN-COMMANDERSOPHIA",
        "UNIT_CHAUNCEY",
        "UNIT_THANESVARAHSUN-HAIR",
        "UNIT_REDLYRIUMNUG",
        "UNIT_ARLTEAGANGUERRIN",
        "UNIT_BETHANYHAWKE",
        "UNIT_KITTY",
    ),
    "CURSE_POWER": ("UNIT_KALTENZAHN", "UNIT_XENONTHEANTIQUARIAN", "UNIT_STONEJAW"),
    "CURSE_HEALTH": ("UNIT_KALTENZAHN", "UNIT_XENONTHEANTIQUARIAN", "UNIT_STONEJAW"),
    "ATTACK_FRIENDLY": (
        "UNIT_REDLYRIUMGIANT",
        "UNIT_PROFESSORFREDERIC",
        "UNIT_DWORKINGLAVONAK",
        "UNIT_RUNICGOLEM",
    ),
    "DAMAGE_SELF": (
        "UNIT_GREYWARDENOGHREN",
        "UNIT_HUNGERDEMON",
        "UNIT_REDLYRIUMMEREDITH",
        "UNIT_SPIKE",
        "UNIT_KINGMARIC",
        "UNIT_SLAVESTATUE",
        "UNIT_FELIXALEXIUS",
        "UNIT_LUKA",
        "UNIT_GORDIAN",
        "UNIT_BLIGHTEDHALLA",
    ),
    "LOSE_POWER_SELF": ("UNIT_SLOTHDEMON", "UNIT_CILLIAN"),
    "SLOW_ALLIES": ("UNIT_TEMPESTSERAIII",),
    "GAIN_CRIT_ALLIES": (
        "UNIT_MOTHERGISELLE",
        "UNIT_VAEA",
        "UNIT_TEVINTERBANNER",
        "UNIT_SHAPESHIFTERMORRIGANIV",
        "UNIT_SKYCULTIST",
    ),
    "CURSE_TWO_ROUNDS": (
        "UNIT_GRANDENCHANTERFIONA",
        "UNIT_WYNNE",
        "UNIT_JANEKA",
        "UNIT_XENONTHEANTIQUARIAN",
        "UNIT_STONEJAW",
    ),
    "GAIN_DAMAGE_ALLIES": ("UNIT_THEGRANDOAK", "UNIT_ZITHER", "UNIT_HEROOFRIVERDANE"),
    "GAIN_DAMAGE_SELF": ("UNIT_CILLIAN",),
}
for mechanic, ids in required_mechanics.items():
    for uid in ids:
        if mechanic not in by_id[uid]["ability"]["mechanics"]:
            fail(f"{uid}: description must produce {mechanic}")

summary = {
    "characters": len(characters),
    "localized_languages": len(runtime_tables),
    "unique_ability_keys": len({row["ability"]["key"] for row in characters}),
    "mechanics": {name: mechanic_counts[name] for name in MECHANIC_ORDER},
}
json.dump(summary, sys.stdout, ensure_ascii=False, indent=2)
print()
