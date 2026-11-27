#!/usr/bin/env python3
import argparse
import hashlib
import json
import random
import re
import sys
from collections import namedtuple
from pathlib import Path

import hero_stats

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "data/pvp_rules.json"
GEAR = ROOT / "data/gear.json"
RUNES = ROOT / "data/runes.json"
STORE_PACKS = ROOT / "data/store_packs.json"
RESEARCH = ROOT / "data/pvp_rewards_research.json"
RESEARCH_SHA256 = "2378ab802a2e76bf318a77b1828524aa831fee3ad9f943bdaf654bc3047d256f"
ROSTER_SOURCES = [ROOT / "data/campaign_rosters.json", ROOT / "data/nexus_rosters.json"]
SQUAD_STRENGTH = ROOT / "data/pvp_squad_strength.json"
CHARACTERS = ROOT / "client-additional-data/characters.json"
LOCALIZATION_DIR = (
    ROOT / "client-additional-data/Heroes-of-Dragon-Age-v4-4-0-cache-androidp1/com.ea.game.dragonage_row/cache/Localization"
)
LOCALIZATION = LOCALIZATION_DIR / "Loc_RUS_RU.txt"
ENGLISH_LOCALIZATION = LOCALIZATION_DIR / "Loc_ENG_US.txt"
CLI_DESCRIPTION = "Build the tunable PvP rule table, bot ladder and opponent squad pool."

SQUAD_CELLS = 5
GIANT_CELL = 4
MIN_SQUAD_UNITS = 3
FIRST_POOL_MIN_UNITS = 4
UNMEASURED_STRENGTH = 0
BOT_COUNT = 240
BOT_SEED = 1604
BOT_TOP_TROPHIES = 6000
BOT_AVATAR_RARITIES = {"Rare", "Epic", "Legendary"}

GEAR_SLOT_CAPACITY = 5
NORMAL_GEAR_SLOTS = (0, 1, 2)
ASCENSION_GEAR_SLOT = 3
PRESTIGE_GEAR_SLOT = 3
ASCENDED_PRESTIGE_GEAR_SLOT = 4
PRESTIGE_MIN_GEAR_RARITY = 5
ASCENSION_BONUS_LEVELS = 5
DAMAGE_ABSORBING_SPECIALTY = "DEFENDER"
NORMAL_RUNE_SLOTS = 2
PRESTIGE_RUNE_RARITY = 7
UNROLLED_RUNE_MARKS = ("-6-", "Special")
NON_BATTLE_RUNE_BUFFS = {"gold", "xp"}
LOADOUT_SEED = "pvp-bot-loadout"
ELITE_SQUAD_SEED = "pvp-elite-squad"
ELITE_SQUAD_COUNT = 32
ELITE_HERO_RARITY = "Legendary"
ELITE_GIANT_RARITIES = {"Legendary", "Epic"}
ELITE_GIANT_CHOICES = 6
ELITE_TIER = "IV"

Loadout = namedtuple(
    "Loadout", "from_share gear_rarity gear_items prestige_rarity ascended rune_count rune_rarity prestige_rune"
)
LOADOUT_BANDS = (
    Loadout(0.00, 0, 0, 0, False, 0, 0, False),
    Loadout(0.20, 1, 1, 0, False, 1, 1, False),
    Loadout(0.35, 2, 2, 0, False, 1, 2, False),
    Loadout(0.50, 3, 3, 0, False, 2, 3, False),
    Loadout(0.65, 4, 3, 0, False, 2, 4, False),
    Loadout(0.80, 5, 3, 5, False, 2, 5, False),
    Loadout(0.92, 5, 3, 6, False, 2, 5, True),
)
ELITE_LOADOUT = Loadout(1.00, 6, 4, 6, True, 2, 6, True)

NAME_STARTS = (
    "Ash", "Black", "Blood", "Bright", "Cold", "Crimson", "Dark", "Dawn", "Dread", "Dusk", "Ember", "Fade", "Fel",
    "Frost", "Gloom", "Golden", "Grey", "Grim", "Iron", "Lyrium", "Night", "Old", "Pale", "Raven", "Red", "Rune",
    "Shadow", "Silver", "Sky", "Spirit", "Steel", "Storm", "Sun", "Thorn", "Veil", "White", "Wild", "Winter",
)
NAME_ENDS = (
    "Archer", "Bane", "Blade", "Bow", "Breaker", "Caller", "Champion", "Claw", "Crow", "Drake", "Fang", "Flame",
    "Fury", "Guard", "Hand", "Heart", "Herald", "Hunter", "Keeper", "Knight", "Mage", "Rider", "Rogue", "Sentinel",
    "Shield", "Singer", "Slayer", "Stalker", "Strider", "Warden", "Watcher", "Weaver", "Wolf", "Wyrm",
)

RULES = {
    "trophies": {
        "base_gain": 30,
        "base_loss": 15,
        "points_per_step": 40,
        "min_gain": 8,
        "max_gain": 60,
        "min_loss": 5,
        "max_loss": 40,
    },
    "matchmaking": {
        "window_below": 150,
        "window_above": 250,
        "daily_nexts": 10,
        "bot_daily_drift": 40,
        "level_spread": 2,
    },
    "squads": {
        "candidates": 6,
    },
    "rewards": {
        "win_gold_base": 60,
        "win_gold_per_level": 6,
        "loss_gold_pct": 25,
        "win_xp_base": 20,
        "win_xp_per_level": 2,
        "loss_xp_pct": 25,
    },
    "defense": {
        "attack_interval_seconds": 14400,
        "max_attacks_per_login": 3,
        "win_pct": 40,
        "loss_share_pct": 50,
    },
    "battle": {
        "stat_budget": True,
    },
}




def group(name, health=0, power=0):
    return {"groups": [name], "factions": [], "units": [], "health_pct": health, "power_pct": power}


def faction(name, health=0, power=0):
    return {"groups": [], "factions": [name], "units": [], "health_pct": health, "power_pct": power}


def event(stem, ladder, *effects, suffix="_KEY"):
    return {
        "title_key": f"{stem}_TITLE{suffix}",
        "text_key": f"{stem}_DESC{suffix}",
        "effects": list(effects),
        "ladder": ladder,
    }


def heroes(factions=(), groups=()):
    return {"factions": list(factions), "groups": list(groups)}


def squad_rules(max_rarity="", rarities=(), unique_heroes=False, members=None, sides=()):
    return {
        "max_rarity": max_rarity,
        "rarities": list(rarities),
        "unique_heroes": unique_heroes,
        "members": members or heroes(),
        "sides": list(sides),
    }


def rule_event(stem, ladder, rules):
    lower = stem.islower()
    return {
        "title_key": f"{stem}_title_key" if lower else f"{stem}_TITLE_KEY",
        "text_key": "",
        "original_text_key": f"{stem}_desc_key" if lower else f"{stem}_DESC_KEY",
        "effects": [],
        "squad_rules": rules,
        "ladder": ladder,
    }


RARITY_ORDER = ("Common", "Uncommon", "Rare", "Epic", "Legendary", "Mythic")
ASCENDED_SHOWN_RARITY = "Mythic"
COLOUR_FACTIONS = ("Red", "Blue", "White", "Black")
EVERY_FACTION = "All"
FACTION_SEPARATOR = "/"
RULE_EVENT_EVERY = (False, True, False, True, False)
EVENT_POOL_MIN_SQUADS = 64
TWIN_EXCLUDED_GROUP = "Wisp"

RULE_EVENTS = [
    rule_event("PVP_EPIC_BLITZ_MAY_27", "pvp|epic expectations", squad_rules(max_rarity="Epic")),
    rule_event("PVP_PEASANT_REVOLT_MAR_5", "pvp|champions of the just", squad_rules(max_rarity="Uncommon")),
    rule_event(
        "PVP_GO_FOR_GOLD_JULY_21", "pvp|quick draw", squad_rules(rarities=["Rare"], unique_heroes=True)
    ),
    rule_event(
        "PVP_RED_FACTION_BLITZ_OCTOBER_6", "pvp|a single strike", squad_rules(members=heroes(factions=["Red"]))
    ),
    rule_event(
        "pvp_warden_wars_finals_october_2",
        "pvp|weight of the crown",
        squad_rules(members=heroes(groups=["Warden"])),
    ),
    rule_event(
        "pvp_faction_wars_9_21",
        "pvp|break the line",
        squad_rules(sides=[heroes(factions=[colour]) for colour in COLOUR_FACTIONS]),
    ),
]


LADDER_BANNERS = 2000
EARLY_MILESTONES = [
    {"banners": 100, "gold": 500, "gems": 0, "prizes": []},
    {"banners": 250, "gold": 1000, "gems": 0, "prizes": []},
    {"banners": 500, "gold": 0, "gems": 5, "prizes": []},
]
RANK_BRACKETS = [
    (1, 1),
    (2, 3),
    (4, 6),
    (7, 10),
    (11, 15),
    (16, 25),
    (26, 40),
    (41, 60),
    (61, 90),
    (91, 130),
    (131, 180),
    (181, BOT_COUNT + 1),
]
SOURCE_RANK_TIERS = len(RANK_BRACKETS)
REWARD_PRIZE_CAPACITY = 6
PRIZE_NAME_ALIASES = {
    "legionnaire hlemet": "legionnaire helmet",
    "asaarsh saddle": "asaarash saddle",
    "senior warden staff": "senior wardens staff",
    "plaidweave vivienne": "plaideweave vivienne",
}
UNSUPPORTED_PRIZES = {
    "banished plane key",
    "raid portal key",
    "giant quest key",
    "potion drop key",
    "double xp key",
}
VOUCHER_PACKS = {"iron pack": "iron", "bloodstone pack": "bloodstone", "lyrium vein pack": "lyrium_mine"}
TOME_PRIZE = "tome of ascension"
GUILD_EVERY_CYCLES = 4
GUILD_MINIMUM_BANNERS = 100
GUILD_OPEN_RANK = 10000000
GUILD_PRIZE_LINE = re.compile(
    r"(\d+)(?:st|nd|rd|th)(?: - (\d+)(?:st|nd|rd|th))? Place( or below)? = (.+)"
)
GUILD_PRIZE_ARTICLES = {"a": 1, "an": 1}

EVENTS = {
    "duration_seconds": 259200,
    "win_banners_start": 25,
    "win_banners_peak_bonus": 5,
    "win_banners_steps_to_peak": 10,
    "win_banners_decline_slowdown": 2,
    "win_banners_floor_pct": 60,
    "matchmaking_banner_window": 150,
    "loss_banners": 5,
    "top_bot_banners": 3000,
    "ladder_banners": LADDER_BANNERS,
    "catalog": [
        event(
            "PVP_BLESSED_OF_ORLAIS",
            "pvp|political stronghold",
            faction("White", health=300, power=300),
        ),
        event(
            "PVP_BLACK_FACTION_JULY_11",
            "pvp|theirin dynasty",
            faction("Black", health=250, power=250),
        ),
        event(
            "PVP_BLUE_FACTION_AUG_03",
            "pvp|the imperium",
            faction("Blue", health=250, power=250),
        ),
        event(
            "PVP_BLIGHTED_JULY_09",
            "pvp|the fifth blight",
            group("Blight", health=500, power=500),
        ),
        event(
            "PVP_HEDGE_MAGES_FEBRUARY_16",
            "pvp|unbound",
            group("Mage", health=500),
        ),
        event(
            "PVP_BEASTS_AND_OUTLAWS_JUN_25",
            "pvp|untamed harts",
            group("Outlaw", health=250, power=250),
            group("Beast", health=250, power=250),
        ),
        event(
            "PVP_ENTER_THE_FADE_MAY_19",
            "pvp|kindred spirits",
            group("Spirit", health=350, power=350),
            group("Demon", health=350, power=350),
        ),
        event(
            "PVP_BLACK_FACTION_2",
            "pvp|the fourth blight",
            group("Demon", power=200),
            group("Blight", health=300),
        ),
        event(
            "PVP_AVVAR_AND_DRAGONS_JUN_27",
            "pvp|battle royale",
            group("Avvar", health=500, power=500),
            group("Dragon", health=200),
        ),
    ],
}

GUILD_EVENTS = [
    {
        **event("DANGEROUS_PURPOSE", "pvp|battle royale", faction("Red", health=300, power=300), suffix="_KEY"),
        "guild_prizes": "INTERSTITIAL_GUILD_EVENT_ANNOUNCEMENT_MESSAGE1_KEY",
    },
    {
        "title_key": "WAR_OF_ATTRITION_APR_14_TITLE_KEY",
        "text_key": "",
        "effects": [],
        "ladder": "pvp|untamed harts",
        "guild_prizes": "INTERSTITIAL_GUILD_EVENT2_ANNOUNCEMENT_MESSAGE1_KEY",
    },
    {
        **event(
            "ONSLAUGHT_ASCENSION_JUNE_19",
            "pvp|the imperium",
            faction("Blue", health=200),
            faction("White", power=100),
        ),
        "guild_prizes": "INTERSTITIAL_GUILD_EVENT2_ANNOUNCEMENT_MESSAGE1_KEY",
    },
]


def localization_keys():
    with LOCALIZATION.open(encoding="utf-8", errors="replace") as file:
        return {line.partition("|")[0] for line in file if "|" in line}


def validate_heroes_filter(title, members, groups):
    missing = set(members["groups"]) - groups
    if missing:
        raise ValueError(f"{title}: unknown groups {sorted(missing)}")
    unknown = set(members["factions"]) - set(COLOUR_FACTIONS)
    if unknown:
        raise ValueError(f"{title}: unknown factions {sorted(unknown)}")


def validate_squad_rules(title, rules, groups):
    rarities = set(rules["rarities"]) | ({rules["max_rarity"]} if rules["max_rarity"] else set())
    if rarities - set(RARITY_ORDER):
        raise ValueError(f"{title}: unknown rarities {sorted(rarities - set(RARITY_ORDER))}")
    validate_heroes_filter(title, rules["members"], groups)
    if len(rules["sides"]) == 1:
        raise ValueError(f"{title}: a side event needs two sides or more")
    for side in rules["sides"]:
        validate_heroes_filter(title, side, groups)
        if not side["factions"] and not side["groups"]:
            raise ValueError(f"{title}: a side names no faction or group")


def validate_events(characters):
    keys = localization_keys()
    groups = {name for character in characters.values() for name in character["groups"]}
    for entry in EVENTS["catalog"] + RULE_EVENTS + GUILD_EVENTS:
        if entry["effects"] and not entry["text_key"]:
            raise ValueError(f"{entry['title_key']}: an event with effects needs its text")
        if not entry["effects"] and not entry["text_key"] and "squad_rules" not in entry and entry not in GUILD_EVENTS:
            raise ValueError(f"{entry['title_key']}: an event needs effects or squad rules")
        for key in (entry["title_key"], entry["text_key"], entry.get("original_text_key", "")):
            if key and key not in keys:
                raise ValueError(f"missing text {key}")
        for effect in entry["effects"]:
            missing = set(effect["groups"]) - groups
            if missing:
                raise ValueError(f"{entry['title_key']}: unknown groups {sorted(missing)}")
        if "squad_rules" in entry:
            validate_squad_rules(entry["title_key"], entry["squad_rules"], groups)


def rarity_rank(name):
    return RARITY_ORDER.index(name)


def shown_rarity(character, ascended):
    return ASCENDED_SHOWN_RARITY if ascended else character.get("original_rarity") or character["rarity"]


def rarity_allowed(rules, rarity):
    if rules["max_rarity"] and rarity_rank(rarity) > rarity_rank(rules["max_rarity"]):
        return False
    return not rules["rarities"] or rarity in rules["rarities"]


def has_colour(character, colour):
    faction = character["faction"]
    return faction == EVERY_FACTION or colour in faction.split(FACTION_SEPARATOR)


def fits_heroes(members, character):
    if not members["factions"] and not members["groups"]:
        return True
    if members["factions"] and (character["giant"] or any(has_colour(character, f) for f in members["factions"])):
        return True
    return any(group in character["groups"] for group in members["groups"])


def hero_allowed(rules, character, side, ascended=False):
    if not rarity_allowed(rules, shown_rarity(character, ascended)) or not fits_heroes(rules["members"], character):
        return False
    return side is None or fits_heroes(rules["sides"][side], character)


def squad_fits(rules, characters, cells, side=None):
    fielded = [cell for cell in cells if cell is not None]
    if not fielded:
        return False
    units = [cell["id"] for cell in fielded]
    if rules["unique_heroes"] and len(set(units)) != len(units):
        return False
    return all(hero_allowed(rules, characters[cell["id"]], side, cell.get("ascended", False)) for cell in fielded)


def squad_side(rules, characters, cells):
    return next((side for side in range(len(rules["sides"])) if squad_fits(rules, characters, cells, side)), None)


def opponent_fits(rules, characters, cells, player_side):
    if player_side is None:
        return squad_fits(rules, characters, cells)
    if squad_fits(rules, characters, cells, player_side):
        return False
    return any(squad_fits(rules, characters, cells, side) for side in range(len(rules["sides"])) if side != player_side)


def opponent_sides(rules):
    return list(range(len(rules["sides"]))) if rules["sides"] else [None]


def fieldable_tiers(character):
    return [tier for tier, stats in character["tiers"].items() if stats and stats.get("max_level")]


def twin_tier(character, tier):
    tiers = fieldable_tiers(character)
    return tier if tier in tiers else tiers[-1]


def twin_cell(rules, characters, template_cell, index, side, used, rng):
    if template_cell is None:
        return None
    template = characters[template_cell["id"]]
    candidates = [
        character
        for character in characters.values()
        if character["obtainable"]
        and character["giant"] == (index == GIANT_CELL)
        and TWIN_EXCLUDED_GROUP not in character["groups"]
        and fieldable_tiers(character)
        and character["id"] not in used
        and hero_allowed(rules, character, side)
    ]
    if not candidates:
        return None
    distance = min(abs(rarity_rank(c["rarity"]) - rarity_rank(template["rarity"])) for c in candidates)
    nearest = sorted(
        (c for c in candidates if abs(rarity_rank(c["rarity"]) - rarity_rank(template["rarity"])) == distance),
        key=lambda c: c["id"],
    )
    character = rng.choice(nearest)
    tier = twin_tier(character, template_cell["tier"])
    level = min(template_cell["level"], character["tiers"][tier]["max_level"])
    used.add(character["id"])
    return {"id": character["id"], "tier": tier, "level": level}


def twin_squad(rules, characters, template, side, seed):
    rng = random.Random(seed)
    used = set()
    cells = [twin_cell(rules, characters, cell, index, side, used, rng) for index, cell in enumerate(template)]
    return cells if unit_count(cells) >= MIN_SQUAD_UNITS else None


def spread_positions(count):
    return sorted(range(count), key=lambda position: (bin(position)[2:].zfill(count.bit_length())[::-1], position))


def twin_target_side(rules, characters, pool, player_side):
    if player_side is None:
        return None
    others = [side for side in range(len(rules["sides"])) if side != player_side]
    return min(others, key=lambda side: sum(squad_fits(rules, characters, cells, side) for cells in pool))


def squad_heroes_key(cells):
    heroes = [None if cell is None else {key: cell[key] for key in ("id", "tier", "level")} for cell in cells]
    return json.dumps(heroes, sort_keys=True)


def event_twins(entry, characters, base, pool):
    rules = entry["squad_rules"]
    twins = []
    for player_side in opponent_sides(rules):
        fielded = pool + [cells for cells, _ in twins]
        fitting = sum(opponent_fits(rules, characters, cells, player_side) for cells in fielded)
        wanted = EVENT_POOL_MIN_SQUADS - fitting
        target = twin_target_side(rules, characters, fielded, player_side)
        known = {squad_heroes_key(cells) for cells in fielded}
        for position in spread_positions(len(base)):
            if wanted <= 0:
                break
            seed = f"{entry['title_key']}:{player_side}:{position}"
            cells = twin_squad(rules, characters, base[position], target, seed)
            if cells is None or squad_heroes_key(cells) in known:
                continue
            known.add(squad_heroes_key(cells))
            twins.append((cells, position))
            wanted -= 1
    return twins


def normalized(name):
    return re.sub(r"[^a-z0-9]", "", name.lower().replace("'s", "s"))


PACK_ONLY_RARITY = "Legendary"
GUARANTEED_PACK_ONLY_VOUCHER = "bloodstone"
PACK_FOR_A_PACK_ONLY_PRIZE = "iron"
UPGRADE_MATERIAL_GROUP = "Wisp"


class PrizeCatalog:
    def __init__(self, characters):
        self.units = {normalized(character["name"]): character for character in characters.values()}
        self.characters = characters
        gear = json.loads(GEAR.read_text(encoding="utf-8"))
        self.gear = {normalized(item["id"]): item for item in gear["items"]}
        store = json.loads(STORE_PACKS.read_text(encoding="utf-8"))
        self.vouchers = {row["id"]: (index, row["icon"]) for index, row in enumerate(store["vouchers"])}

    def voucher(self, pack, count):
        index, icon = self.vouchers[pack]
        return {"kind": "voucher", "id": pack, "count": count, "icon": icon, "index": index}

    def resolve(self, name, count):
        key = PRIZE_NAME_ALIASES.get(name.lower(), name)
        lookup = normalized(key)
        if lookup in self.units:
            character = self.units[lookup]
            if character["rarity"] == PACK_ONLY_RARITY and UPGRADE_MATERIAL_GROUP not in character["groups"]:
                return self.voucher(PACK_FOR_A_PACK_ONLY_PRIZE, count)
            return self.unit(character, count)
        for candidate in (lookup, normalized(re.sub(r"^the ", "", key, flags=re.I))):
            if candidate in self.gear:
                item = self.gear[candidate]
                return {"kind": "gear", "id": item["id"], "count": count, "icon": ""}
        singular = name.lower().rstrip("s")
        if singular == TOME_PRIZE:
            return {"kind": "tome", "id": "tome", "count": count, "icon": ""}
        if singular in VOUCHER_PACKS:
            pack = VOUCHER_PACKS[singular]
            return self.voucher(PACK_FOR_A_PACK_ONLY_PRIZE if pack == GUARANTEED_PACK_ONLY_VOUCHER else pack, count)
        raise ValueError(f"unmapped PvP prize {name!r}")

    def unit(self, character, count):
        return {
            "kind": "unit",
            "id": character["id"],
            "count": count,
            "icon": character["icon"]["I"],
        }


def reward_bundle(text, catalog, omitted):
    bundle = {"gold": 0, "gems": 0, "prizes": []}
    for part in text.split(","):
        part = part.strip()
        currency = re.fullmatch(r"([\d,]+) (gems?|gold)", part)
        if currency:
            field = "gold" if currency[2] == "gold" else "gems"
            bundle[field] += int(currency[1].replace(",", ""))
            continue
        quantity = re.fullmatch(r"(\d+)x (.+)", part)
        count, name = (int(quantity[1]), quantity[2]) if quantity else (1, part)
        if name.lower().rstrip("s") in UNSUPPORTED_PRIZES:
            omitted.append(part)
            continue
        bundle["prizes"].append(catalog.resolve(name, count))
    if len(bundle["prizes"]) > REWARD_PRIZE_CAPACITY:
        raise ValueError(f"too many prizes in {text!r}")
    return bundle


def load_research():
    raw = RESEARCH.read_bytes()
    if hashlib.sha256(raw).hexdigest() != RESEARCH_SHA256:
        raise ValueError(f"{RESEARCH} does not match the research snapshot")
    return json.loads(raw)


def scaled_banners(banners, top):
    return round(banners * LADDER_BANNERS / top)


def event_ladder(entry, research, catalog):
    selected = next(row for row in research["latest_events"] if row["event_key"] == entry["ladder"])
    record = selected["record_id"]
    tiers = [
        row for row in research["rank_tiers"] if row["record_id"] == record and row["channel"] == "Individual rank"
    ]
    tiers.sort(key=lambda row: row["rank_min"])
    if len(tiers) != SOURCE_RANK_TIERS:
        raise ValueError(f"{entry['ladder']}: expected {SOURCE_RANK_TIERS} rank tiers, got {len(tiers)}")
    omitted = []
    rank_rewards = [
        {"first": first, "last": last, **reward_bundle(tier["reward"], catalog, omitted)}
        for (first, last), tier in zip(RANK_BRACKETS, tiers)
    ]
    milestones = list(EARLY_MILESTONES)
    last_early = EARLY_MILESTONES[-1]["banners"]
    source = sorted(
        (row for row in research["banner_milestones"] if row["record_id"] == record), key=lambda row: row["banners"]
    )
    top = selected["ladder_banners"]
    for row in source:
        bundle = reward_bundle(row["reward"], catalog, omitted)
        if bundle["gold"] or bundle["gems"] or bundle["prizes"]:
            banners = scaled_banners(row["banners"], top)
            if banners <= last_early:
                raise ValueError(f"{entry['ladder']}: {row['reward']} scales below the early milestones")
            milestones.append({"banners": banners, **bundle})
    if milestones[-1]["banners"] != LADDER_BANNERS:
        raise ValueError(f"{entry['ladder']}: ladder does not end at {LADDER_BANNERS}")
    return {
        "source": {"event_key": entry["ladder"], "record_id": record, "event_date": selected["event_date"]},
        "omitted_prizes": sorted(set(omitted)),
        "milestones": milestones,
        "rank_rewards": rank_rewards,
    }


def english_text(key):
    with ENGLISH_LOCALIZATION.open(encoding="utf-8", errors="replace") as file:
        for line in file:
            name, _, text = line.rstrip("\n").partition("|")
            if name == key:
                return text
    raise ValueError(f"missing announcement {key}")


def guild_prize_bundle(text, catalog):
    bundle = {"gold": 0, "gems": 0, "prizes": []}
    plain = re.sub(r"</?color[^>]*>", "", text)
    for part in re.split(r",? and |, ", plain):
        words = part.strip().split(" ", 1)
        count = GUILD_PRIZE_ARTICLES.get(words[0]) or int(words[0].rstrip("k")) * (1000 if words[0].endswith("k") else 1)
        name = words[1]
        if name in ("gem", "gems"):
            bundle["gems"] += count
        elif name == "gold":
            bundle["gold"] += count
        else:
            bundle["prizes"].append(catalog.resolve(name, count))
    return bundle


def guild_rank_rewards(key, catalog):
    tiers = []
    for line in english_text(key).split("\\n"):
        match = GUILD_PRIZE_LINE.fullmatch(line.strip())
        if not match:
            continue
        first = int(match[1])
        last = GUILD_OPEN_RANK if match[3] else int(match[2] or match[1])
        tiers.append({"first": first, "last": last, **guild_prize_bundle(match[4], catalog)})
    if not tiers or tiers[0]["first"] != 1:
        raise ValueError(f"{key}: no guild prize table")
    for previous, current in zip(tiers, tiers[1:]):
        if current["first"] != previous["last"] + 1:
            raise ValueError(f"{key}: guild prize ranks {previous['last']} and {current['first']} are not adjacent")
    return tiers


def rotation():
    buff_events, rule_events = iter(EVENTS["catalog"]), iter(RULE_EVENTS)
    order = []
    while len(order) < len(EVENTS["catalog"]) + len(RULE_EVENTS):
        for rule_turn in RULE_EVENT_EVERY:
            entry = next(rule_events if rule_turn else buff_events, None)
            if entry is not None:
                order.append(entry)
    return order


def resolve_ladders(characters):
    research = load_research()
    catalog = PrizeCatalog(characters)
    entries = []
    for entry in rotation():
        fields = {key: value for key, value in entry.items() if key != "ladder"}
        entries.append({**fields, **event_ladder(entry, research, catalog)})
    guild_entries = []
    for entry in GUILD_EVENTS:
        fields = {key: value for key, value in entry.items() if key not in ("ladder", "guild_prizes")}
        guild_entries.append(
            {
                **fields,
                **event_ladder(entry, research, catalog),
                "guild_source": entry["guild_prizes"],
                "guild_minimum_banners": GUILD_MINIMUM_BANNERS,
                "guild_rank_rewards": guild_rank_rewards(entry["guild_prizes"], catalog),
            }
        )
    return {**EVENTS, "catalog": entries, "guild_every_cycles": GUILD_EVERY_CYCLES, "guild_catalog": guild_entries}


def catalog_stat_total(characters, roster):
    total = 0
    for cell in roster:
        if cell is None:
            continue
        stats = characters[cell["id"]]["tiers"][cell["tier"]]
        total += hero_stats.stat_total_at_level(stats, min(cell["level"], stats["max_level"]))
    return total


def measured_strengths(squad_count):
    measured = json.loads(SQUAD_STRENGTH.read_text(encoding="utf-8"))["strength"] if SQUAD_STRENGTH.exists() else []
    return measured if len(measured) == squad_count else [UNMEASURED_STRENGTH] * squad_count


def clean_cell(characters, cell, index):
    if cell is None:
        return None
    character = characters.get(cell["id"])
    if character is None or cell["tier"] not in character["tiers"]:
        return None
    if character["giant"] != (index == GIANT_CELL):
        return None
    return {"id": cell["id"], "tier": cell["tier"], "level": cell.get("level", 1)}


def unit_count(cells):
    return sum(cell is not None for cell in cells)


def pool_order(characters, cells):
    joined_later = unit_count(cells) < FIRST_POOL_MIN_UNITS
    return (joined_later, catalog_stat_total(characters, cells), json.dumps(cells, sort_keys=True))


def campaign_squads(characters):
    squads = {}
    for path in ROSTER_SOURCES:
        for campaign in json.loads(path.read_text(encoding="utf-8"))["campaigns"]:
            for node in campaign["nodes"]:
                cells = [clean_cell(characters, cell, i) for i, cell in enumerate(node["roster"][:SQUAD_CELLS])]
                cells += [None] * (SQUAD_CELLS - len(cells))
                if unit_count(cells) < MIN_SQUAD_UNITS:
                    continue
                squads[json.dumps(cells, sort_keys=True)] = cells
    return sorted(squads.values(), key=lambda cells: pool_order(characters, cells))


def fielded_stat_total(characters, cells):
    total = 0
    for cell in cells:
        if cell is not None:
            stats = characters[cell["id"]]["tiers"][cell["tier"]]
            total += hero_stats.stat_total_at_level(stats, cell["level"])
    return total


def loadout_for(share):
    return [band for band in LOADOUT_BANDS if band.from_share <= share][-1]


def campaign_loadouts(characters, squads):
    ranked = sorted(range(len(squads)), key=lambda index: (fielded_stat_total(characters, squads[index]), index))
    last = max(len(squads) - 1, 1)
    loadouts = [LOADOUT_BANDS[0]] * len(squads)
    for rank, index in enumerate(ranked):
        loadouts[index] = loadout_for(rank / last)
    return loadouts


def ascended_level(character):
    return character["tiers"][ELITE_TIER]["max_level"] + ASCENSION_BONUS_LEVELS


def grown_stat_product(character):
    stats = character["tiers"][ELITE_TIER]
    level = ascended_level(character)
    return hero_stats.stat_at_level(stats, "power", level) * hero_stats.stat_at_level(stats, "health", level)


def elite_candidates(characters, giant, rarities):
    eligible = [
        character
        for character in characters.values()
        if character["obtainable"]
        and character["giant"] == giant
        and character["rarity"] in rarities
        and ELITE_TIER in character["tiers"]
        and UPGRADE_MATERIAL_GROUP not in character["groups"]
    ]
    return sorted(eligible, key=lambda character: (-grown_stat_product(character), character["id"]))


def elite_cell(character):
    return {"id": character["id"], "tier": ELITE_TIER, "level": ascended_level(character)}


def elite_squads(characters):
    heroes = elite_candidates(characters, False, {ELITE_HERO_RARITY})
    giants = elite_candidates(characters, True, ELITE_GIANT_RARITIES)[:ELITE_GIANT_CHOICES]
    rng = random.Random(ELITE_SQUAD_SEED)
    squads, seen = [], set()
    while len(squads) < ELITE_SQUAD_COUNT:
        cells = [elite_cell(hero) for hero in rng.sample(heroes, GIANT_CELL)] + [elite_cell(rng.choice(giants))]
        key = json.dumps(cells, sort_keys=True)
        if key not in seen:
            seen.add(key)
            squads.append(cells)
    return squads


def category_matches(character, category):
    value = category["value"]
    if category["kind"] == "faction":
        return character["faction"] == "All" or value in character["faction"].split("/")
    if category["kind"] == "specialty":
        return value in character["specialties"]
    if category["kind"] == "initiative":
        return value == character["speed"]
    return value.lower() in {group.lower() for group in character["groups"]}


def can_wear(character, item):
    if not item["categories"]:
        return True
    matched = sum(category_matches(character, category) for category in item["categories"])
    return matched == len(item["categories"]) if item["categories_all"] else matched > 0


def absorbs_damage(character, worn):
    return DAMAGE_ABSORBING_SPECIALTY in character["specialties"] or any(item["absorbs_damage"] for item in worn)


def wearable_gear(character, gear, rarity, worn):
    taken = {item["id"] for item in worn}
    absorbing = absorbs_damage(character, worn)
    return [
        item
        for item in gear
        if item["rarity"] == rarity
        and item["id"] not in taken
        and can_wear(character, item)
        and not (item["absorbs_damage"] and absorbing)
    ]


def pick_gear(character, gear, rarity, floor, worn, rng):
    for wanted in range(rarity, floor - 1, -1):
        candidates = wearable_gear(character, gear, wanted, worn)
        fitted = [item for item in candidates if item["categories"]]
        if fitted or candidates:
            item = rng.choice(fitted or candidates)
            worn.append(item)
            return item["id"]
    return ""


def dressed_cell(character, cell, loadout, gear, rng):
    slots = [""] * GEAR_SLOT_CAPACITY
    worn = []
    normal = NORMAL_GEAR_SLOTS + ((ASCENSION_GEAR_SLOT,) if loadout.ascended else ())
    for slot in normal[: loadout.gear_items]:
        slots[slot] = pick_gear(character, gear, loadout.gear_rarity, 1, worn, rng)
    if loadout.prestige_rarity:
        slot = ASCENDED_PRESTIGE_GEAR_SLOT if loadout.ascended else PRESTIGE_GEAR_SLOT
        slots[slot] = pick_gear(character, gear, loadout.prestige_rarity, PRESTIGE_MIN_GEAR_RARITY, worn, rng)
    dressed = dict(cell)
    if loadout.ascended:
        dressed["ascended"] = True
    if any(slots):
        dressed["gear"] = slots
    return dressed


def battle_runes(runes, rarity):
    return sorted(
        (
            rune
            for rune in runes
            if rune["rarity"] == rarity
            and rune["duration_minutes"] > 0
            and rune["buff_type"] not in NON_BATTLE_RUNE_BUFFS
            and not any(mark in rune["id"] for mark in UNROLLED_RUNE_MARKS)
        ),
        key=lambda rune: rune["id"],
    )


def squad_runes(loadout, runes, rng):
    normal = battle_runes(runes, loadout.rune_rarity) if loadout.rune_count else []
    picked = rng.sample(normal, min(loadout.rune_count, NORMAL_RUNE_SLOTS, len(normal)))
    if loadout.prestige_rune:
        picked.append(rng.choice(battle_runes(runes, PRESTIGE_RUNE_RARITY)))
    return [rune["id"] for rune in picked]


def loadout_squad(characters, index, cells, loadout, gear, runes):
    rng = random.Random(f"{LOADOUT_SEED}:{index}")
    dressed = [
        None if cell is None else dressed_cell(characters[cell["id"]], cell, loadout, gear, rng) for cell in cells
    ]
    squad = {"cells": dressed, "runes": squad_runes(loadout, runes, rng)}
    return {**squad, "elite": True} if loadout is ELITE_LOADOUT else squad


def build_squads(characters):
    gear = sorted(json.loads(GEAR.read_text(encoding="utf-8"))["items"], key=lambda item: item["id"])
    runes = json.loads(RUNES.read_text(encoding="utf-8"))["runes"]
    campaign = campaign_squads(characters)
    elite = elite_squads(characters)
    loadouts = campaign_loadouts(characters, campaign)
    pool = list(zip(campaign, loadouts)) + [(cells, ELITE_LOADOUT) for cells in elite]
    squads = [
        loadout_squad(characters, index, cells, loadout, gear, runes) for index, (cells, loadout) in enumerate(pool)
    ]
    fielded = [squad["cells"] for squad in squads]
    twins = []
    for entry in RULE_EVENTS:
        twins += event_twins(entry, characters, campaign, fielded + [cells for cells, _ in twins])
    for offset, (cells, template) in enumerate(twins):
        twin = loadout_squad(characters, len(pool) + offset, cells, loadouts[template], gear, runes)
        squads.append({**twin, "events_only": True})
    return [{"strength": strength, **squad} for strength, squad in zip(measured_strengths(len(squads)), squads)]


def build_bots(characters):
    rng = random.Random(BOT_SEED)
    names = [start + end for start in NAME_STARTS for end in NAME_ENDS]
    rng.shuffle(names)
    avatars = sorted(
        character["id"]
        for character in characters.values()
        if character["obtainable"] and not character["giant"] and character["rarity"] in BOT_AVATAR_RARITIES
    )
    bots = []
    for index in range(BOT_COUNT):
        position = index / (BOT_COUNT - 1)
        bots.append(
            {
                "name": names[index],
                "avatar": rng.choice(avatars),
                "trophies": round(BOT_TOP_TROPHIES * position * position),
            }
        )
    return bots


def build():
    catalog = json.loads(CHARACTERS.read_text(encoding="utf-8"))["characters"]
    characters = {character["id"]: character for character in catalog}
    validate_events(characters)
    events = resolve_ladders(characters)
    return {**RULES, "events": events, "bots": build_bots(characters), "squad_pool": build_squads(characters)}


def main():
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    text = json.dumps(build(), ensure_ascii=False, indent=1) + "\n"
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding="utf-8") != text:
            print(f"{OUTPUT.relative_to(ROOT)} is stale", file=sys.stderr)
            return 1
        return 0
    OUTPUT.write_text(text, encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
