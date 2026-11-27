import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import build_pvp_rules  # noqa: E402

RULES = json.loads((ROOT / "data/pvp_rules.json").read_text(encoding="utf-8"))
CHARACTERS = {
    character["id"]: character
    for character in json.loads(build_pvp_rules.CHARACTERS.read_text(encoding="utf-8"))["characters"]
}
KEY_MAPPING = "Loc_Key_Mapping.txt"
ORIGINAL_LOCALIZATIONS = sorted(
    path for path in build_pvp_rules.LOCALIZATION_DIR.glob("Loc_*.txt") if path.name != KEY_MAPPING
)
CATALOG = RULES["events"]["catalog"]
RULE_EVENTS = [entry for entry in CATALOG if "squad_rules" in entry]
POOL = [squad["cells"] for squad in RULES["squad_pool"]]


def hero(rarity="Rare", faction="Red", giant=False, groups=(), unit="UNIT_A"):
    return {"id": unit, "rarity": rarity, "faction": faction, "giant": giant, "groups": list(groups)}


def judged(rules, *heroes, side=None):
    characters = {f"UNIT_{index}": {**character, "id": f"UNIT_{index}"} for index, character in enumerate(heroes)}
    cells = [{"id": f"UNIT_{index}"} for index in range(len(heroes))]
    return build_pvp_rules.squad_fits(rules, characters, cells, side)


def original_keys(path):
    with path.open(encoding="utf-8", errors="replace") as file:
        return {line.partition("|")[0] for line in file if "|" in line}


class RuleEventCatalogTest(unittest.TestCase):
    def test_rule_events_use_original_titles_in_every_language(self):
        self.assertEqual(len(RULE_EVENTS), len(build_pvp_rules.RULE_EVENTS))
        for path in ORIGINAL_LOCALIZATIONS:
            keys = original_keys(path)
            for entry in RULE_EVENTS:
                self.assertIn(entry["title_key"], keys, path.name)
                self.assertIn(entry["original_text_key"], keys, path.name)

    def test_rule_events_replace_buffs_with_a_rule(self):
        for entry in RULE_EVENTS:
            self.assertEqual(entry["effects"], [], entry["title_key"])
            self.assertEqual(entry["text_key"], "", entry["title_key"])
            self.assertEqual(entry["milestones"][-1]["banners"], build_pvp_rules.LADDER_BANNERS)

    def test_a_rule_event_comes_up_within_every_three_events(self):
        flags = ["squad_rules" in entry for entry in CATALOG]
        for index in range(len(flags)):
            window = [flags[(index + step) % len(flags)] for step in range(3)]
            self.assertTrue(any(window), index)
            self.assertFalse(window[0] and window[1], index)

    def test_the_catalog_fits_released_clients(self):
        self.assertLessEqual(len(CATALOG), 16)
        self.assertLessEqual(len(POOL), 1024)

    def test_every_rule_event_fields_enough_bots_for_each_side(self):
        for entry in RULE_EVENTS:
            rules = entry["squad_rules"]
            for side in build_pvp_rules.opponent_sides(rules):
                fitting = sum(build_pvp_rules.opponent_fits(rules, CHARACTERS, cells, side) for cells in POOL)
                self.assertGreaterEqual(fitting, build_pvp_rules.EVENT_POOL_MIN_SQUADS, (entry["title_key"], side))

    def test_events_only_squads_follow_the_ladder_and_serve_a_rule_event(self):
        flags = [squad.get("events_only", False) for squad in RULES["squad_pool"]]
        first = flags.index(True)
        self.assertTrue(all(flags[first:]) and not any(flags[:first]))
        for cells in POOL[first:]:
            served = any(
                build_pvp_rules.opponent_fits(entry["squad_rules"], CHARACTERS, cells, side)
                for entry in RULE_EVENTS
                for side in build_pvp_rules.opponent_sides(entry["squad_rules"])
            )
            self.assertTrue(served, cells)
            fielded = [cell["id"] for cell in cells if cell]
            self.assertEqual(len(fielded), len(set(fielded)))


class SquadRulesTest(unittest.TestCase):
    def test_a_cap_and_a_list_limit_rarities(self):
        capped = build_pvp_rules.squad_rules(max_rarity="Uncommon")
        self.assertTrue(judged(capped, hero("Common"), hero("Uncommon")))
        self.assertFalse(judged(capped, hero("Common"), hero("Rare")))
        rares = build_pvp_rules.squad_rules(rarities=["Rare"], unique_heroes=True)
        self.assertTrue(judged(rares, hero(), hero()))
        self.assertFalse(judged(rares, hero(), hero("Uncommon")))
        characters = {"UNIT_A": hero()}
        self.assertFalse(build_pvp_rules.squad_fits(rares, characters, [{"id": "UNIT_A"}, {"id": "UNIT_A"}]))

    def test_rarity_rules_judge_the_rarity_on_the_card(self):
        rares = build_pvp_rules.squad_rules(rarities=["Rare"])
        shown_legendary = {**hero(), "original_rarity": "Legendary"}
        shown_rare = {**hero("Epic"), "original_rarity": "Rare"}
        self.assertFalse(judged(rares, shown_legendary))
        self.assertTrue(judged(rares, shown_rare))
        characters = {"UNIT_A": shown_rare}
        self.assertFalse(build_pvp_rules.squad_fits(rares, characters, [{"id": "UNIT_A", "ascended": True}]))
        uncapped = build_pvp_rules.squad_rules(members=build_pvp_rules.heroes(factions=["Red"]))
        self.assertTrue(build_pvp_rules.squad_fits(uncapped, characters, [{"id": "UNIT_A", "ascended": True}]))

    def test_factions_let_large_creatures_join_but_groups_do_not(self):
        red = build_pvp_rules.squad_rules(members=build_pvp_rules.heroes(factions=["Red"]))
        self.assertTrue(judged(red, hero(), hero(faction="White/Red"), hero(faction="All"), hero(giant=True)))
        self.assertFalse(judged(red, hero(), hero(faction="Blue")))
        wardens = build_pvp_rules.squad_rules(members=build_pvp_rules.heroes(groups=["Warden"]))
        self.assertTrue(judged(wardens, hero(groups=["Warden"]), hero(giant=True, groups=["Warden"])))
        self.assertFalse(judged(wardens, hero(groups=["Warden"]), hero(giant=True)))

    def test_side_events_face_a_squad_from_another_side(self):
        wars = build_pvp_rules.squad_rules(
            sides=[build_pvp_rules.heroes(factions=["Red"]), build_pvp_rules.heroes(factions=["Blue"])]
        )
        characters = {"UNIT_RED": hero(), "UNIT_BLUE": hero(faction="Blue"), "UNIT_BOTH": hero(faction="Red/Blue")}
        red = [{"id": "UNIT_RED"}, {"id": "UNIT_BOTH"}]
        both = [{"id": "UNIT_BOTH"}]
        self.assertEqual(build_pvp_rules.squad_side(wars, characters, red), 0)
        self.assertEqual(build_pvp_rules.squad_side(wars, characters, [{"id": "UNIT_BLUE"}]), 1)
        self.assertIsNone(build_pvp_rules.squad_side(wars, characters, [{"id": "UNIT_RED"}, {"id": "UNIT_BLUE"}]))
        self.assertTrue(build_pvp_rules.opponent_fits(wars, characters, [{"id": "UNIT_BLUE"}], 0))
        self.assertFalse(build_pvp_rules.opponent_fits(wars, characters, red, 0))
        self.assertFalse(build_pvp_rules.opponent_fits(wars, characters, both, 0))


if __name__ == "__main__":
    unittest.main()
