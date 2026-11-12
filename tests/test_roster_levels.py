import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))

import validate_roster_levels as roster_levels  # noqa: E402

CATALOG = {"UNIT_RARE": {"id": "UNIT_RARE", "tiers": {"I": {"max_level": 30}, "IV": {"max_level": 45}}}}


def owned(level, tier="IV", uid="UNIT_RARE"):
    return {"characters": [{"id": uid, "tier": tier, "level": level}]}


class RosterLevelsTest(unittest.TestCase):
    def test_level_at_the_cap_passes(self):
        self.assertEqual(roster_levels.problems(owned(45), CATALOG), [])

    def test_level_above_the_cap_is_reported(self):
        self.assertEqual(
            roster_levels.problems(owned(50), CATALOG), ["characters: UNIT_RARE level 50 outside 1..45 for tier IV"]
        )

    def test_missing_tier_and_unknown_unit_are_reported(self):
        self.assertEqual(roster_levels.problems(owned(1, tier="II"), CATALOG), ["characters: UNIT_RARE has no tier II"])
        self.assertEqual(
            roster_levels.problems(owned(1, uid="UNIT_GONE"), CATALOG), ["characters: 'UNIT_GONE' is not in the catalog"]
        )

    def test_enemy_rosters_are_checked(self):
        document = {"campaigns": [{"campaign": "INTRO", "nodes": [{"step": 1, "roster": [{"id": "UNIT_RARE", "level": 31}]}]}]}
        self.assertEqual(
            roster_levels.problems(document, CATALOG), ["INTRO step 1: UNIT_RARE level 31 outside 1..30 for tier I"]
        )

    def test_shipped_intro_roster_is_loadable(self):
        catalog = roster_levels.catalog_heroes()
        for path in roster_levels.REQUIRED_ROSTERS:
            document = roster_levels.json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(roster_levels.problems(document, catalog), [])


if __name__ == "__main__":
    unittest.main()
