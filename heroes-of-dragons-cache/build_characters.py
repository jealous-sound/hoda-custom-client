#!/usr/bin/env python3
import json
import os
import re

from ability_metadata import infer_ability_mechanics, normalize_text

HERE = os.path.dirname(os.path.abspath(__file__))
INFO = os.path.join(HERE, "HoDA_character_info")
REFERENCE_CHARACTERS = os.path.join(
    HERE, "..", ".claude", "skills", "hoda-reference", "data", "characters.json"
)

units = json.load(open(os.path.join(INFO, "UnitData.json")))
loc   = json.load(open(os.path.join(INFO, "Localization_ENG.json")))
vfx   = json.load(open(os.path.join(HERE, "character_vfx_map.json")))["units"]
vfx_by_id = {v["unit_id"]: v for v in vfx}
reference = json.load(open(REFERENCE_CHARACTERS))
reference_loc_en = json.load(
    open(os.path.join(os.path.dirname(REFERENCE_CHARACTERS), "localization", "Loc_ENG_US.json"))
)
reference_loc_ru = json.load(
    open(os.path.join(os.path.dirname(REFERENCE_CHARACTERS), "localization", "Loc_RUS_RU.json"))
)

def load_pipe_loc(language):
    path = os.path.join(
        HERE, "Heroes-of-Dragon-Age-v4-4-0-cache-androidp1", "com.ea.game.dragonage_row",
        "cache", "Localization", f"Loc_{language}.txt"
    )
    entries = {}
    with open(path) as src:
        for line in src:
            key, sep, value = line.rstrip("\n").partition("|")
            if sep:
                entries[key.upper()] = value
    return entries

old_loc_en = load_pipe_loc("ENG_US")
old_loc_ru = load_pipe_loc("RUS_RU")

SPEED   = {0: "Slow", 1: "Normal", 2: "Quick"}
RARITY  = {1: "Common", 2: "Uncommon", 3: "Rare", 4: "Epic", 5: "Legendary", 6: "Mythic"}
ROLE    = {1: "Attack", 2: "Defense", 3: "Support"}
TARGET  = {1: "One", 2: "All", 3: "Row", 5: "Column"}
FACTION = {0: "Large", 2: "White", 4: "Red", 6: "White/Red", 8: "Blue",
           10: "White/Blue", 12: "Red/Blue", 17: "Black", 19: "White/Black",
           21: "Red/Black", 25: "Blue/Black", 31: "All"}
TIERS = ["I", "II", "III", "IV"]

MODEL_OVERRIDES = {
    "UNIT_BEHEMOTH": "unit.c_behemoth_t4_pre",
    "UNIT_GHASTARCHER": "unit.c_ghastarcher_t4_pre",
    "UNIT_GHASTBRUTE": "unit.c_ghastbrute_t4_pre",
    "UNIT_WILDSYLVAN": "unit.c_wildsylvan_t4_pre",
}

def L(key, default=""):
    if not key:
        return default
    return loc.get(key) or loc.get(key.upper()) or default

def ability_loc(table, key):
    if not key:
        return ""
    upper = key.upper()
    return table.get(f"{upper}_DESCRIPTION") or table.get(upper) or ""


def reference_loc(table, key):
    return table.get((key or "").upper(), "")


def localized_groups(groups, table):
    return [reference_loc(table, f"GALLERY_{group}_TITLE") for group in groups]


def localized_faction(u, table):
    if u.get("Giant"):
        return reference_loc(table, "UNIT_DETAIL_FACTION_GIANT")
    faction = u.get("Faction", 0)
    parts = []
    for flag, key in (
        (1, "UNIT_DETAIL_FACTION_BLACK"),
        (2, "UNIT_DETAIL_FACTION_WHITE"),
        (4, "UNIT_DETAIL_FACTION_RED"),
        (8, "UNIT_DETAIL_FACTION_BLUE"),
    ):
        if faction & flag:
            parts.append(reference_loc(table, key))
    return ", ".join(parts)

def clean_name(unit_id, raw):
    v = vfx_by_id.get(unit_id)
    if v and v.get("name"):
        return v["name"]
    return re.sub(r"\s+(I|II|III|IV)$", "", raw or unit_id)

def as_per_tier(value, fallback=""):
    if isinstance(value, dict):
        return {t: value.get(t, fallback) for t in TIERS}
    single = value or fallback
    return {t: single for t in TIERS}

def per_tier_models(unit_id, fallback_prefab):
    if unit_id in MODEL_OVERRIDES:
        return as_per_tier(MODEL_OVERRIDES[unit_id])
    v = vfx_by_id.get(unit_id)
    p = v.get("unit_prefab") if v else None
    return as_per_tier(p, fallback_prefab)

def tget(u, field, t):
    val = u.get(field)
    if isinstance(val, dict):
        return val.get(t)
    return val

def name_tier(uid, u):
    src = (tget(u, "DefaultName", "IV") or uid or "")
    m = re.search(r"(IV|III|II|I)$", src)
    return m.group(1) if m else "IV"

def stat_step(u, step_field, base_field, max_field, t):
    step = tget(u, step_field, t)
    if step is not None:
        return step
    base = tget(u, base_field, t)
    maximum = tget(u, max_field, t)
    levels = tget(u, "MaxLevel", t)
    if all(isinstance(value, (int, float)) for value in (base, maximum, levels)) and levels > 1:
        return max(0, int((maximum - base) / (levels - 1) + 0.5))
    return 0

def tier_stats(u, uid):
    bh = u.get("BaseHealth")
    keys = list(bh.keys()) if isinstance(bh, dict) else [name_tier(uid, u)]
    out = {}
    for t in [t for t in TIERS if t in keys]:
        out[t] = {
            "base_power":        tget(u, "BasePower", t),
            "max_power":         tget(u, "PowerAtMaxLevel", t),
            "base_health":       tget(u, "BaseHealth", t),
            "max_health":        tget(u, "HealthAtMaxLevel", t),
            "max_level":         tget(u, "MaxLevel", t),
            "power_per_level":   stat_step(u, "PowerChangePerLevel", "BasePower", "PowerAtMaxLevel", t),
            "health_per_level":  stat_step(u, "HealthChangePerLevel", "BaseHealth", "HealthAtMaxLevel", t),
            "max_fortify_power": tget(u, "MaxFortifyPowerBonus", t),
            "max_fortify_health":tget(u, "MaxFortifyHealthBonus", t),
        }
    return out

characters = []
for uid, u in units.items():
    v = vfx_by_id.get(uid, {})
    ref = reference.get(uid, {})
    fac = u.get("Faction")
    ability_key = u.get("AbilityKey", "")
    ability_en = normalize_text(
        ref.get("ability_en")
        or ability_loc({k.upper(): value for k, value in loc.items()}, ability_key)
        or ability_loc(old_loc_en, ability_key)
        or v.get("ability_text", "")
    )
    ability_ru = normalize_text(ref.get("ability_ru") or ability_loc(old_loc_ru, ability_key))
    groups = u.get("Groups", [])
    equipped_rune_key = u.get("EquippedRuneDescriptionKey", "")
    target_weight_key = u.get("TargetWeightDescriptionKey", "")
    characters.append({
        "id":     uid,
        "name":   clean_name(uid, tget(u, "DefaultName", "IV") or ""),
        "name_ru": normalize_text(ref.get("name_ru")),
        "icon":   as_per_tier(u.get("ThumbImage", "")),

        "rarity": RARITY.get(u.get("Rarity"), "Unknown"),
        "stars":  u.get("Stars", 0),
        "role":   ROLE.get(u.get("Role"), "Attack"),
        "giant":  bool(u.get("Giant", False)),

        "speed":       SPEED.get(u.get("Initiative"), "Normal"),
        "faction":     FACTION.get(fac, "Unknown"),
        "faction_id":  fac,
        "groups":      groups,
        "groups_en":   localized_groups(groups, reference_loc_en),
        "groups_ru":   localized_groups(groups, reference_loc_ru),
        "faction_en":  localized_faction(u, reference_loc_en),
        "faction_ru":  localized_faction(u, reference_loc_ru),
        "specialties": u.get("Specialties", []),

        "gear_slots":        u.get("NumGearSlots", 0),
        "has_prestige_slot": bool(u.get("HasPrestigeSlot", False)),
        "obtainable":        bool(u.get("Obtainable", True)),

        "ability": {
            "key":    ability_key,
            "text":   ability_en,
            "text_ru": ability_ru,
            "target": TARGET.get(u.get("AbilityUnitTarget"), "One"),
            "mechanics": infer_ability_mechanics(ability_en, u.get("Specialties", [])),
        },
        "equipped_rune": equipped_rune_key,
        "equipped_rune_text": reference_loc(reference_loc_en, equipped_rune_key),
        "equipped_rune_text_ru": reference_loc(reference_loc_ru, equipped_rune_key),
        "target_weight": target_weight_key,
        "target_weight_text": reference_loc(reference_loc_en, target_weight_key),
        "target_weight_text_ru": reference_loc(reference_loc_ru, target_weight_key),

        "models": per_tier_models(uid, u.get("UnitPrefab", "")),
        "vfx":    v.get("shared_vfx_prefabs", []),

        "tiers":     tier_stats(u, uid),
    })
    characters[-1]["max_tier"] = (list(characters[-1]["tiers"]) or ["IV"])[-1]

out = {
    "version": 2,
    "count": len(characters),
    "legend": {
        "speed": list(SPEED.values()),
        "rarity": list(RARITY.values()),
        "role": list(ROLE.values()),
        "target": list(TARGET.values()),
        "faction": FACTION,
        "note_crit": "HoDA has no per-unit crit stat; crit is an ability effect (e.g. '+10 crit') via ability.text/specialties.",
    },
    "characters": characters,
}

dst = os.path.join(HERE, "characters.json")
json.dump(out, open(dst, "w"), indent=2, ensure_ascii=False)
print(f"wrote {dst}: {len(characters)} characters")
