from __future__ import annotations

import json
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import build_hero_auras


def compact(rows):
    result = []
    for row in rows:
        item = (row["stat"], row["value"], row["on"], row["scope"])
        if row.get("who"):
            item += ("who", tuple(value for _, value in row["who"]))
        if row.get("vs"):
            item += ("vs", tuple(value for _, value in row["vs"]))
        if row.get("basis") == "damage":
            item += ("damage",)
        result.append(item)
    return result


class HeroAuraSentenceTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        characters = json.loads(build_hero_auras.CATALOG.read_text(encoding="utf-8"))["characters"]
        reference = json.loads(build_hero_auras.REFERENCE.read_text(encoding="utf-8"))
        cls.parser = build_hero_auras.Parser(build_hero_auras.Resolver(characters, reference))

    def rows(self, sentence):
        rows = self.parser.sentence(sentence)
        self.assertIsNotNone(rows, sentence)
        return compact(rows)

    def test_defender_takes_the_stated_or_qualitative_share(self):
        self.assertEqual(self.rows("Absorbs 50% AOE damage for allies"), [("defender", 50, "battle", "self")])
        self.assertEqual(self.rows("+65% Defender"), [("defender", 65, "battle", "self")])
        self.assertEqual(self.rows("Absorbs medium AOE damage for allies if attacked also"),
                         [("defender", 50, "battle", "self")])
        self.assertEqual(self.rows("Absorbs AOE damage for allies if attacked also, Deals bonus damage to Blighted"),
                         [("defender", 25, "battle", "self"), ("bonus_damage", 15, "battle", "self", "vs", ("Blight",))])

    def test_stealth_and_perceptive_become_unit_flags(self):
        self.assertEqual(self.rows("Stealth: Attacks without triggering on-hit effects"),
                         [("stealth", 1, "battle", "self")])
        self.assertEqual(self.rows("Perceptive: Negates stealth"), [("perceptive", 1, "battle", "self")])

    def test_on_hit_and_death_triggers(self):
        self.assertEqual(self.rows("Gains power when attacked"), [("power", 15, "self_hit", "self")])
        self.assertEqual(self.rows("Allies gain 25% health and power when hero is attacked, 75% when hero dies"), [
            ("health", 25, "self_hit", "allies"), ("power", 25, "self_hit", "allies"),
            ("health", 75, "self_death", "allies"), ("power", 75, "self_death", "allies")])
        self.assertEqual(self.rows("When hit, reverts 10% of damage received, and heals allies +30%"), [
            ("damage", 10, "self_hit", "enemies", "damage"), ("health", 30, "self_hit", "allies")])
        self.assertEqual(self.rows("Gains small Power when an enemy or ally dies"), [
            ("power", 15, "ally_death", "self"), ("power", 15, "enemy_death", "self")])
        self.assertEqual(self.rows("Converts 15% of damage taken by allies into health for allies"),
                         [("health", 15, "ally_hit", "allies", "damage")])

    def test_a_trailing_trigger_stays_on_its_own_clause(self):
        self.assertEqual(self.rows("Deals bonus damage to Blighted, 25% power to Rogues, Gains power when ally is attacked,"), [
            ("bonus_damage", 15, "battle", "self", "vs", ("Blight",)),
            ("power", 25, "battle", "allies", "who", ("Rogue",)),
            ("power", 15, "ally_hit", "self")])

    def test_resists_immunities_and_enemy_debuffs(self):
        self.assertEqual(self.rows("+100% stun resist (self), +50% for allies"), [
            ("stun_resist", 100, "battle", "self"), ("stun_resist", 50, "battle", "allies")])
        self.assertEqual(self.rows("+35% Power and stun immunity for Spirits"), [
            ("power", 35, "battle", "allies", "who", ("Spirit",)),
            ("stun_resist", 100, "battle", "allies", "who", ("Spirit",))])
        self.assertEqual(self.rows("When hit, allies gain +20% power and reduces enemy power drain resist by 10%"), [
            ("power", 20, "self_hit", "allies"), ("drain_resist", -10, "self_hit", "enemies")])
        self.assertEqual(self.rows("-25% all effect resists"), [
            (stat, -25, "battle", "enemies") for stat in build_hero_auras.ALL_EFFECT_RESISTS])
        self.assertEqual(self.rows("Squad gets +50% power drain resist and +25% damage vs Templars"), [
            ("drain_resist", 50, "battle", "allies"), ("bonus_damage", 25, "battle", "allies", "vs", ("Templar",))])

    def test_health_and_power_the_passives_apply_are_left_to_them(self):
        applied = build_hero_auras.applied_passives([
            {"kind": "stat_percent", "stats": ["health", "power"], "value_pct": 35},
            {"kind": "bonus_damage_percent", "value_pct": 30},
        ])
        self.assertTrue(build_hero_auras.already_applied(
            {"stat": "health", "value": 35, "on": "battle", "scope": "allies"}, applied))
        self.assertTrue(build_hero_auras.already_applied(
            {"stat": "power", "value": 30, "on": "battle", "scope": "self", "vs": [["group", "Blight"]]}, applied))
        self.assertFalse(build_hero_auras.already_applied(
            {"stat": "health", "value": 50, "on": "battle", "scope": "allies"}, applied))
        self.assertFalse(build_hero_auras.already_applied(
            {"stat": "power", "value": 35, "on": "self_hit", "scope": "self"}, applied))


if __name__ == "__main__":
    unittest.main()
