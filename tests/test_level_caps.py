from __future__ import annotations

import json
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import hero_retier
import hero_stats


CATALOGS = (ROOT / "client-additional-data/characters.json", ROOT / "data/custom_heroes.json")
TIERS = ("I", "II", "III", "IV")
TIER_I_CAPS = {
    ("Common", 0): 15,
    ("Uncommon", 0): 20,
    ("Rare", 0): 30,
    ("Epic", 0): 50,
    ("Legendary", 1): 70,
    ("Legendary", 2): 75,
    ("Legendary", 3): 80,
    ("Mythic", 1): 75,
}
CAP_RISE_PER_TIER = 5


def shipped_heroes():
    heroes = []
    for path in CATALOGS:
        heroes += json.loads(path.read_text(encoding="utf-8"))["characters"]
    return heroes


def original_cap(hero, tier):
    promotions = TIERS.index(tier) if len(hero["tiers"]) > 1 else len(TIERS) - 1
    return TIER_I_CAPS[(hero["original_rarity"], hero["stars"])] + CAP_RISE_PER_TIER * promotions


def hero_row(rarity, cap):
    stats = {"base_power": 100, "max_power": 300, "base_health": 1000, "max_health": 3000, "max_level": cap}
    return {"id": "UNIT_TEST", "name": "Test", "rarity": rarity, "stars": 1, "groups": [], "tiers": {"I": stats}}


class LevelCaps(unittest.TestCase):
    def test_every_hero_keeps_the_level_caps_of_the_rarity_and_stars_it_shows(self):
        wrong = [
            f"{hero['id']} {tier}: {stats['max_level']}, want {original_cap(hero, tier)}"
            for hero in shipped_heroes()
            for tier, stats in hero["tiers"].items()
            if (stats.get("max_level") or 0) > 1 and stats["max_level"] != original_cap(hero, tier)
        ]
        self.assertEqual(wrong, [])

    def test_a_retiered_hero_keeps_its_cap_and_reaches_the_band_strength_there(self):
        band = {"costs": {hero_retier.HERO_KIND: {"I": 1600}}}
        placed = hero_retier.retiered(hero_row("Legendary", 70), "Rare", {"I": 3200}, band)
        self.assertEqual(placed["rarity"], "Rare")
        self.assertEqual(
            placed["tiers"]["I"],
            {"base_power": 50, "max_power": 150, "base_health": 500, "max_health": 1500, "max_level": 70},
        )

    def test_stats_grow_evenly_to_the_cap_and_on_past_it(self):
        stats = {"base_power": 0, "max_power": 10, "base_health": 100, "max_health": 130, "max_level": 4}
        self.assertEqual([hero_stats.stat_at_level(stats, "power", level) for level in range(1, 5)], [0, 3, 7, 10])
        self.assertEqual(hero_stats.stat_at_level(stats, "power", 9), 27)
        self.assertEqual(hero_stats.stat_total_at_level(stats, 9), 27 + 180)

    def test_a_hero_with_no_growth_stays_at_its_base(self):
        stats = {"base_power": 40, "base_health": 400, "max_level": 1}
        self.assertEqual(hero_stats.stat_total_at_level(stats, 1), 440)


if __name__ == "__main__":
    unittest.main()
