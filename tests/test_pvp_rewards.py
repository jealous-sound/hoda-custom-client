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
