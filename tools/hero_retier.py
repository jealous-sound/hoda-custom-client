import functools
import json
import statistics
from pathlib import Path

import build_fortify_tables

ROOT = Path(__file__).resolve().parent.parent
TABLE = ROOT / "data/hero_rarity.json"
BANDS = ROOT / "data/hero_stat_bands.json"
STAT_BUDGET = ROOT / "data/stat_budget.json"
FORTIFY_TABLES = ROOT / "data/fortify_tables.json"

TIERS = ("I", "II", "III", "IV")
BANDS_VERSION = 1
NATIVE_BAND_RARITIES = ("Common", "Uncommon", "Rare", "Epic")
TOP_BAND_RARITY = "Legendary"
TOP_BAND_BASE_RARITY = "Epic"
TOP_BAND_OVER_BASE_PCT = 140
LEVEL_CAPS = {
    "Common": (15, 20, 25, 30),
    "Uncommon": (20, 25, 30, 35),
    "Rare": (30, 35, 40, 45),
    "Epic": (50, 55, 60, 65),
    "Legendary": (70, 75, 80, 85),
}
UPGRADE_MATERIAL_GROUP = "Wisp"
BANNER_NAME_SUFFIX = " Banner"
BASE_STATS = ("base_power", "base_health")
HERO_KIND = "hero"
GIANT_KIND = "giant"


def proposed_rarities():
    table = json.loads(TABLE.read_text(encoding="utf-8"))
    return {row["id"]: row["rarity"] for row in table["heroes"]}


def proposed_rarities_of_shipped(rows):
    rarities = proposed_rarities()
    missing = [row["id"] for row in rows if row["id"] not in rarities]
    if missing:
        raise SystemExit(f"{TABLE.name} has no row for {missing[0]}")
    return rarities


def health_per_power():
    return json.loads(STAT_BUDGET.read_text(encoding="utf-8"))["health_per_power"]


def kind_of(row):
    return GIANT_KIND if row.get("giant") else HERO_KIND


def fights_as_a_hero(row):
    return UPGRADE_MATERIAL_GROUP not in row.get("groups", []) and not row["name"].endswith(BANNER_NAME_SUFFIX)


def has_stats(stats):
    return all(isinstance(stats.get(key), int) for key in ("base_power", "max_power", "base_health", "max_health"))


def cost_at_max_level(stats, power_weight):
    return stats["max_power"] * power_weight + stats["max_health"]


def cohort_of(row):
    return (row["rarity"], row["stars"], kind_of(row))


def cohort_costs(rows, power_weight):
    costs = {}
    for row in rows:
        if not fights_as_a_hero(row):
            continue
        for tier, stats in row["tiers"].items():
            if has_stats(stats):
                costs.setdefault(cohort_of(row), {}).setdefault(tier, []).append(cost_at_max_level(stats, power_weight))
    return {
        cohort: {tier: statistics.median(values) for tier, values in tiers.items()} for cohort, tiers in costs.items()
    }


def rounded(value):
    return int(value + 0.5)


def bands_from(cohorts):
    bands = {}
    for rarity in NATIVE_BAND_RARITIES:
        bands[rarity] = {
            "level_caps": dict(zip(TIERS, LEVEL_CAPS[rarity])),
            "costs": {
                kind: {tier: rounded(cohorts[(rarity, 0, kind)][tier]) for tier in TIERS}
                for kind in (HERO_KIND, GIANT_KIND)
            },
        }
    base = bands[TOP_BAND_BASE_RARITY]["costs"]
    bands[TOP_BAND_RARITY] = {
        "level_caps": dict(zip(TIERS, LEVEL_CAPS[TOP_BAND_RARITY])),
        "costs": {
            kind: {tier: rounded(cost * TOP_BAND_OVER_BASE_PCT / 100) for tier, cost in tiers.items()}
            for kind, tiers in base.items()
        },
    }
    return bands


def bands_document(bands, power_weight):
    return {
        "version": BANDS_VERSION,
        "health_per_power": power_weight,
        "top_band_over_base_pct": TOP_BAND_OVER_BASE_PCT,
        "bands": bands,
    }


def load_bands():
    document = json.loads(BANDS.read_text(encoding="utf-8"))
    if document.get("version") != BANDS_VERSION:
        raise SystemExit(f"{BANDS.name} version {document.get('version')!r}, want {BANDS_VERSION}")
    return document["bands"]


@functools.lru_cache(maxsize=None)
def fortify_rules():
    tables = json.loads(FORTIFY_TABLES.read_text(encoding="utf-8"))
    return tables["rarity_tier_bonus"][-1][-1], tables["faction_bonus"]


def fortify_maximum(stat):
    return build_fortify_tables.slot_maximum(stat, *fortify_rules())


def level_step(base, target_maximum, level_cap):
    if level_cap <= 1:
        return 0
    return max(0, rounded((target_maximum - base) / (level_cap - 1)))


def rescaled_tier(stats, scale, level_cap):
    scaled = dict(stats)
    for key in BASE_STATS:
        scaled[key] = rounded(stats[key] * scale)
    for stat in ("power", "health"):
        step = level_step(scaled[f"base_{stat}"], stats[f"max_{stat}"] * scale, level_cap)
        scaled[f"{stat}_per_level"] = step
        scaled[f"max_{stat}"] = scaled[f"base_{stat}"] + (level_cap - 1) * step
        if isinstance(stats.get(f"max_fortify_{stat}"), int):
            scaled[f"max_fortify_{stat}"] = fortify_maximum(scaled[f"max_{stat}"])
    scaled["max_level"] = level_cap
    return scaled


def retiered(row, rarity, source_costs, band):
    tiers = {}
    for tier, stats in row["tiers"].items():
        if has_stats(stats) and tier in source_costs:
            scale = band["costs"][kind_of(row)][tier] / source_costs[tier]
            tiers[tier] = rescaled_tier(stats, scale, band["level_caps"][tier])
        else:
            tiers[tier] = stats
    return {**row, "rarity": rarity, "tiers": tiers}


def needs_new_stats(row, rarity):
    return fights_as_a_hero(row) and (rarity != row["rarity"] or rarity == TOP_BAND_RARITY)


def apply_table(rows, rarities, power_weight):
    cohorts = cohort_costs(rows, power_weight)
    bands = bands_from(cohorts)
    applied = []
    for row in rows:
        rarity = rarities[row["id"]]
        if needs_new_stats(row, rarity):
            applied.append(retiered(row, rarity, cohorts[cohort_of(row)], bands[rarity]))
        else:
            applied.append({**row, "rarity": rarity})
    return applied, bands
