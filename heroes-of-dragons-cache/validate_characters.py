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
REFERENCE_DIR = os.path.join(HERE, "..", ".agents", "skills", "hoda-reference", "data")
UNIT_DATA = os.path.join(HERE, "HoDA_character_info", "UnitData.json")


def fail(message: str) -> None:
    raise SystemExit(f"character validation failed: {message}")


def localized_faction(source: dict, table: dict[str, str]) -> str:
    if source.get("Giant"):
        return table.get("UNIT_DETAIL_FACTION_GIANT", "")
    parts = []
    for flag, key in (
        (1, "UNIT_DETAIL_FACTION_BLACK"),
        (2, "UNIT_DETAIL_FACTION_WHITE"),
        (4, "UNIT_DETAIL_FACTION_RED"),
        (8, "UNIT_DETAIL_FACTION_BLUE"),
    ):
        if source.get("Faction", 0) & flag:
            parts.append(table.get(key, ""))
    return ", ".join(parts)


with open(CATALOG) as src:
    root = json.load(src)
with open(UNIT_DATA) as src:
    unit_data = json.load(src)
with open(os.path.join(REFERENCE_DIR, "characters.json")) as src:
    reference_characters = json.load(src)
with open(os.path.join(REFERENCE_DIR, "localization", "Loc_ENG_US.json")) as src:
    reference_loc_en = json.load(src)
with open(os.path.join(REFERENCE_DIR, "localization", "Loc_RUS_RU.json")) as src:
    reference_loc_ru = json.load(src)
characters = root.get("characters", [])

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
        ("name_ru", row.get("name_ru")),
        ("ability.key", ability.get("key")),
        ("ability.text", ability.get("text")),
        ("ability.text_ru", ability.get("text_ru")),
    ):
        if not value:
            fail(f"{uid}: empty {field}")
    if not re.search(r"[А-Яа-яЁё]", row["name_ru"]):
        fail(f"{uid}: Russian name is not localized")
    if not re.search(r"[А-Яа-яЁё]", ability["text_ru"]):
        fail(f"{uid}: Russian ability is not localized")
    if ability["text"] != normalize_text(ability["text"]):
        fail(f"{uid}: English ability spacing is not normalized")
    if ability["text_ru"] != normalize_text(ability["text_ru"]):
        fail(f"{uid}: Russian ability spacing is not normalized")
    if ability.get("target") not in {"One", "Row", "Column", "All"}:
        fail(f"{uid}: invalid target {ability.get('target')!r}")

    groups = source.get("Groups", [])
    expected_groups_en = [reference_loc_en.get(f"GALLERY_{group}_TITLE".upper(), "") for group in groups]
    expected_groups_ru = [reference_loc_ru.get(f"GALLERY_{group}_TITLE".upper(), "") for group in groups]
    if row.get("groups") != groups:
        fail(f"{uid}: group identifiers differ from UnitData")
    if row.get("groups_en") != expected_groups_en or row.get("groups_ru") != expected_groups_ru:
        fail(f"{uid}: localized groups differ from the complete reference localization")
    if any(not value for value in expected_groups_en + expected_groups_ru):
        fail(f"{uid}: a group has no localized display name")
    if row.get("faction_en") != localized_faction(source, reference_loc_en):
        fail(f"{uid}: English faction name differs from the complete reference localization")
    if row.get("faction_ru") != localized_faction(source, reference_loc_ru):
        fail(f"{uid}: Russian faction name differs from the complete reference localization")

    rune_key = source.get("EquippedRuneDescriptionKey", "")
    target_key = source.get("TargetWeightDescriptionKey", "")
    expected_rune_en = reference_loc_en.get(rune_key.upper(), "")
    expected_rune_ru = reference_loc_ru.get(rune_key.upper(), "")
    expected_target_en = reference_loc_en.get(target_key.upper(), "")
    expected_target_ru = reference_loc_ru.get(target_key.upper(), "")
    expected_fields = {
        "equipped_rune": rune_key,
        "equipped_rune_text": expected_rune_en,
        "equipped_rune_text_ru": expected_rune_ru,
        "target_weight": target_key,
        "target_weight_text": expected_target_en,
        "target_weight_text_ru": expected_target_ru,
    }
    for field, expected in expected_fields.items():
        if row.get(field) != expected:
            fail(f"{uid}: {field} differs from the complete reference localization")
    if rune_key and (not expected_rune_en or not expected_rune_ru):
        fail(f"{uid}: equipped rune {rune_key!r} is not fully localized")
    if target_key and (not expected_target_en or not expected_target_ru):
        fail(f"{uid}: target weight {target_key!r} is not fully localized")

    description_ru = "  ".join(value for value in (ability["text_ru"], expected_rune_ru, expected_target_ru) if value)
    if len(description_ru.encode()) >= 1280:
        fail(f"{uid}: localized description exceeds the runtime buffer")
    if len(expected_rune_ru.encode()) >= 768:
        fail(f"{uid}: localized rune text exceeds the runtime buffer")
    if len(expected_target_ru.encode()) >= 96:
        fail(f"{uid}: localized targeting text exceeds the runtime buffer")
    if any(len(group.encode()) >= 24 for group in groups):
        fail(f"{uid}: group identifier exceeds the runtime buffer")
    if any(len(group.encode()) >= 64 for group in expected_groups_ru):
        fail(f"{uid}: localized group exceeds the runtime buffer")

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
if not envy["equipped_rune_text_ru"].startswith("<color=green>"):
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
    "localized_names_ru": len(characters),
    "localized_abilities_ru": len(characters),
    "unique_ability_keys": len({row["ability"]["key"] for row in characters}),
    "mechanics": {name: mechanic_counts[name] for name in MECHANIC_ORDER},
}
json.dump(summary, sys.stdout, ensure_ascii=False, indent=2)
print()
