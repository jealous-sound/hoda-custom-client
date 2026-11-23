import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))

import validate_enemy_rosters as enemy_rosters  # noqa: E402

FLAT_RESIST = {"UNIT_SHIELDED": 1500}


def rosters(uid, authenticity="inferred"):
    node = {"step": 2, "authenticity": authenticity, "roster": [None, {"id": uid, "tier": "III", "level": 22}]}
    return {"campaigns": [{"campaign": "Hard_Daily_Sunday", "nodes": [node]}]}


class EnemyRostersTest(unittest.TestCase):
    def test_reconstructed_squad_with_flat_resist_is_reported(self):
        self.assertEqual(
            enemy_rosters.problems(rosters("UNIT_SHIELDED"), FLAT_RESIST),
            ["Hard_Daily_Sunday step 2: UNIT_SHIELDED resists 1500 damage per hit"],
        )

    def test_sourced_squad_keeps_its_evidence(self):
        self.assertEqual(enemy_rosters.problems(rosters("UNIT_SHIELDED", authenticity="sourced"), FLAT_RESIST), [])

    def test_hero_without_flat_resist_passes(self):
        self.assertEqual(enemy_rosters.problems(rosters("UNIT_PLAIN"), FLAT_RESIST), [])

    def test_auras_mark_the_heroes_the_rosters_swapped_out(self):
        flat_resist = enemy_rosters.flat_resist_heroes(json.loads(enemy_rosters.HERO_AURAS.read_text(encoding="utf-8")))
        self.assertEqual(flat_resist["UNIT_CHASINDSCOUT"], 1500)
        self.assertEqual(flat_resist["UNIT_YAVANA"], 2500)
        self.assertEqual(flat_resist["UNIT_VARTERRAL"], 5000)
        self.assertNotIn("UNIT_SUNDERMOUNTVARTERRAL", flat_resist)

    def test_shipped_rosters_field_no_flat_resist(self):
        flat_resist = enemy_rosters.flat_resist_heroes(json.loads(enemy_rosters.HERO_AURAS.read_text(encoding="utf-8")))
        for path in enemy_rosters.ENEMY_ROSTERS:
            document = json.loads(path.read_text(encoding="utf-8"))
            self.assertEqual(enemy_rosters.problems(document, flat_resist), [], path.name)


if __name__ == "__main__":
    unittest.main()
