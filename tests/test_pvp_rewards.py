import json
import sys
import unittest
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import build_pvp_rules  # noqa: E402

RULES = json.loads((ROOT / "data/pvp_rules.json").read_text(encoding="utf-8"))
RESEARCH = build_pvp_rules.load_research()


def selected(event_key):
    return next(row for row in RESEARCH["latest_events"] if row["event_key"] == event_key)


class ResearchSnapshotTest(unittest.TestCase):
    def test_counts_match_the_snapshot(self):
        self.assertEqual(len(RESEARCH["latest_events"]), 380)
        self.assertEqual(len(RESEARCH["historical_occurrences"]), 471)
        self.assertEqual(len(RESEARCH["rank_tiers"]), 1878)
        self.assertEqual(len(RESEARCH["banner_milestones"]), 316)
        self.assertEqual(len(RESEARCH["sources"]), 288)

    def test_battle_royale_formats_stay_distinct(self):
        self.assertEqual(selected("onslaught|battle royale")["event_date"], "2022-02-04")
        ladder = next(
            entry for entry in RULES["events"]["catalog"] if entry["source"]["event_key"] == "pvp|battle royale"
        )
        self.assertEqual(ladder["source"]["event_date"], "2019-11-21")

    def test_later_mention_does_not_replace_older_rewards(self):
        arlathan = selected("pvp|battle for arlathan")
        self.assertEqual(arlathan["event_date"], "2014-11-02")
        self.assertEqual(arlathan["latest_mentioned_date"], "2015-01-12")


class EventLadderTest(unittest.TestCase):
    def test_every_rank_is_covered_once(self):
        last_rank = build_pvp_rules.BOT_COUNT + 1
        for entry in RULES["events"]["catalog"]:
            hits = defaultdict(int)
            for reward in entry["rank_rewards"]:
                for rank in range(reward["first"], reward["last"] + 1):
                    hits[rank] += 1
            self.assertEqual(sorted(hits), list(range(1, last_rank + 1)))
            self.assertEqual(set(hits.values()), {1})

    def test_milestones_rise_to_the_ladder_top(self):
        for entry in RULES["events"]["catalog"]:
            banners = [milestone["banners"] for milestone in entry["milestones"]]
            self.assertEqual(banners, sorted(set(banners)))
            self.assertEqual(banners[-1], RULES["events"]["ladder_banners"])

    def test_rewards_come_from_the_selected_record_only(self):
        for entry in RULES["events"]["catalog"]:
            record = selected(entry["source"]["event_key"])["record_id"]
            self.assertEqual(entry["source"]["record_id"], record)

    def test_unsupported_prizes_are_listed_not_granted(self):
        omitted = {name for entry in RULES["events"]["catalog"] for name in entry["omitted_prizes"]}
        self.assertTrue(any("Banished Plane Key" in name for name in omitted))
        for entry in RULES["events"]["catalog"]:
            for row in entry["milestones"] + entry["rank_rewards"]:
                for prize in row["prizes"]:
                    self.assertNotIn("key", prize["id"].split("_"))

    def test_unknown_prize_names_are_rejected(self):
        catalog = build_pvp_rules.PrizeCatalog(
            {row["id"]: row for row in json.loads(build_pvp_rules.CHARACTERS.read_text(encoding="utf-8"))["characters"]}
        )
        with self.assertRaises(ValueError):
            build_pvp_rules.reward_bundle("Definitely Not A Prize", catalog, [])


if __name__ == "__main__":
    unittest.main()


CHARACTERS = {
    character["id"]: character
    for character in json.loads(build_pvp_rules.CHARACTERS.read_text(encoding="utf-8"))["characters"]
}
GEAR = {item["id"]: item for item in json.loads(build_pvp_rules.GEAR.read_text(encoding="utf-8"))["items"]}
RUNES = {rune["id"]: rune for rune in json.loads(build_pvp_rules.RUNES.read_text(encoding="utf-8"))["runes"]}
POOL = RULES["squad_pool"]


def worn_cells(squad):
    return [cell for cell in squad["cells"] if cell is not None and "gear" in cell]


def mean_gear_rarity(squads):
    rarities = [GEAR[item]["rarity"] for squad in squads for cell in worn_cells(squad) for item in cell["gear"] if item]
    return sum(rarities) / max(len(rarities), 1)


class BotLoadoutTest(unittest.TestCase):
    def test_every_item_fits_its_wearer_once(self):
        for squad in POOL:
            for cell in worn_cells(squad):
                character = CHARACTERS[cell["id"]]
                worn = [GEAR[item] for item in cell["gear"] if item]
                self.assertEqual(len(cell["gear"]), build_pvp_rules.GEAR_SLOT_CAPACITY)
                self.assertEqual(len(worn), len({item["id"] for item in worn}))
                self.assertTrue(all(build_pvp_rules.can_wear(character, item) for item in worn))
                absorbers = sum(item["absorbs_damage"] for item in worn)
                defender = build_pvp_rules.DAMAGE_ABSORBING_SPECIALTY in character["specialties"]
                self.assertLessEqual(absorbers, 0 if defender else 1)
                prestige = cell["gear"][
                    build_pvp_rules.ASCENDED_PRESTIGE_GEAR_SLOT if cell.get("ascended") else
                    build_pvp_rules.PRESTIGE_GEAR_SLOT
                ]
                if prestige:
                    self.assertGreaterEqual(GEAR[prestige]["rarity"], build_pvp_rules.PRESTIGE_MIN_GEAR_RARITY)

    def test_runes_follow_the_players_slot_rules(self):
        for squad in POOL:
            runes = [RUNES[rune] for rune in squad["runes"]]
            normal = [rune for rune in runes if rune["rarity"] != build_pvp_rules.PRESTIGE_RUNE_RARITY]
            self.assertLessEqual(len(normal), build_pvp_rules.NORMAL_RUNE_SLOTS)
            self.assertEqual(runes[: len(normal)], normal)
            self.assertFalse(any(rune["buff_type"] in build_pvp_rules.NON_BATTLE_RUNE_BUFFS for rune in runes))

    def test_loadouts_grow_with_squad_strength(self):
        campaign = sorted((squad for squad in POOL if not squad.get("elite")), key=lambda squad: squad["strength"])
        tenth = len(campaign) // 10
        self.assertFalse(any(worn_cells(squad) or squad["runes"] for squad in campaign[:tenth]))
        self.assertLess(mean_gear_rarity(campaign[-3 * tenth : -2 * tenth]), mean_gear_rarity(campaign[-tenth:]))

    def test_elite_squads_close_the_ladder_maxed_in_mythic_gear(self):
        elite = [index for index, squad in enumerate(POOL) if squad.get("elite")]
        ladder = [index for index, squad in enumerate(POOL) if not squad.get("events_only")]
        self.assertEqual(elite, ladder[-build_pvp_rules.ELITE_SQUAD_COUNT :])
        for index in elite:
            cells = POOL[index]["cells"]
            self.assertTrue(all(cell is not None and cell.get("ascended") for cell in cells))
            self.assertTrue(all(GEAR[item]["rarity"] == build_pvp_rules.ELITE_LOADOUT.gear_rarity
                                for cell in cells for item in cell["gear"]))
            self.assertEqual([RUNES[rune]["rarity"] for rune in POOL[index]["runes"]],
                             [build_pvp_rules.ELITE_LOADOUT.rune_rarity] * build_pvp_rules.NORMAL_RUNE_SLOTS +
                             [build_pvp_rules.PRESTIGE_RUNE_RARITY])
            for cell in cells:
                self.assertEqual(cell["level"], build_pvp_rules.ascended_level(CHARACTERS[cell["id"]]))
