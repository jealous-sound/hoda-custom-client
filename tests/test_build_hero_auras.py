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
        if row.get("per"):
            item += ("per", tuple(value for _, value in row["per"]), bool(row.get("per_self")))
        if row.get("by"):
            item += ("by", tuple(value for _, value in row["by"]))
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
            ("stun_immune", 1, "battle", "allies", "who", ("Spirit",))])
        self.assertEqual(self.rows("When hit, allies gain +20% power and reduces enemy power drain resist by 10%"), [
            ("power", 20, "self_hit", "allies"), ("drain_resist", -10, "self_hit", "enemies")])
        self.assertEqual(self.rows("-25% all effect resists"), [
            (stat, -25, "battle", "enemies") for stat in build_hero_auras.ALL_EFFECT_RESISTS])
        self.assertEqual(self.rows("Squad gets +50% power drain resist and +25% damage vs Templars"), [
            ("drain_resist", 50, "battle", "allies"), ("bonus_damage", 25, "battle", "allies", "vs", ("Templar",))])

    def test_immunities_stay_flags_that_trims_and_enemy_debuffs_cannot_lower(self):
        self.assertEqual(self.rows("Immune to stun and slow"), [
            ("stun_immune", 1, "battle", "self"), ("slow_immune", 1, "battle", "self")])
        self.assertEqual(self.rows("Cannot be stunned"), [("stun_immune", 1, "battle", "self")])
        self.assertEqual(self.rows("Immune to slow, stun, and power drain, 25% each for Demons"), [
            ("slow_immune", 1, "battle", "self"), ("stun_immune", 1, "battle", "self"),
            ("drain_immune", 1, "battle", "self"), ("slow_resist", 25, "battle", "allies", "who", ("Demon",)),
            ("stun_resist", 25, "battle", "allies", "who", ("Demon",)),
            ("drain_resist", 25, "battle", "allies", "who", ("Demon",))])
        self.assertEqual(self.rows("Reavers resist 2500 damage per hit, and are immune to stun"), [
            ("raw_damage_resist", 2500, "battle", "allies", "who", ("Reavers",)),
            ("stun_immune", 1, "battle", "allies", "who", ("Reavers",))])
        self.assertEqual(self.rows("+100% stun resist"), [("stun_resist", 100, "battle", "self")])

    def test_any_hero_hit_fires_once_because_the_hit_hero_is_its_own_ally(self):
        self.assertEqual(self.rows("Gains +10% power when any hero is hit"), [
            ("power", 10, "ally_hit", "self"), ("power", 10, "enemy_hit", "self")])
        self.assertEqual(self.rows("Heals allies 5% when an ally or enemy is hit or dies"), [
            ("health", 5, on, "allies") for on in ("ally_hit", "enemy_hit", "ally_death", "enemy_death")])

    def test_less_likely_and_squad_wide_target_preferences(self):
        self.assertEqual(self.rows("Less likely to attack Creatures or Elves"), [
            ("target_avoid", 1, "battle", "self", "vs", ("Beast", "Elves"))])
        self.assertEqual(self.rows("Allies more likely to attack Nobility heroes"), [
            ("target_match", 1, "battle", "allies", "vs", ("Nobility",))])
        self.assertEqual(self.rows("Werewolves are more likely to attack Humans and Elves"), [
            ("target_match", 1, "battle", "allies", "who", ("Werewolves",), "vs", ("Human", "Elves"))])
        self.assertEqual(self.rows("Grants hatred of Tevinter to allies"), [
            ("target_match", 1, "battle", "allies", "vs", ("Tevinter",))])
        self.assertEqual(self.rows("Allies more likely to target slowest enemy"), [
            ("target_slowest", 1, "battle", "allies")])
        self.assertEqual(self.rows("More likely to attack Nugs"), [("target_match", 1, "battle", "self", "vs", ("Nug",))])

    def test_row_stun_and_assassination_preferences(self):
        self.assertEqual(self.rows("More likely to attack the front row"), [("target_front_row", 1, "battle", "self")])
        self.assertEqual(self.rows("Equal chance to attack front and back row"), [
            ("target_even_rows", 1, "battle", "self")])
        self.assertEqual(self.rows("More likely to stun the slowest enemy"), [("target_slowest", 1, "battle", "self")])
        self.assertEqual(self.rows("Much more likely to assassinate Nobility"), [
            ("assassinate_match", 1, "battle", "self", "vs", ("Nobility",))])

    def test_per_ally_bonuses_count_the_squad(self):
        dragonlings = self.rows("Gains +20% damage resist for each dragonling in the party")
        self.assertEqual(len(dragonlings), 1)
        self.assertEqual(dragonlings[0][:4], ("damage_resist", 20, "battle", "self"))
        self.assertIn("UNIT_GAMORDANSTORMRIDERDRAGONLING", dragonlings[0][5])
        self.assertTrue(dragonlings[0][6])
        self.assertEqual(self.rows("Receives +250% Health and Power per each common/uncommon ally"), [
            (stat, 250, "battle", "self", "per", ("Common", "Uncommon"), False) for stat in ("health", "power")])
        self.assertEqual(self.rows("Gains +25% Power for every Nug in the squad"), [
            ("power", 25, "battle", "self", "per", ("Nug",), True)])

    def test_trigger_filters_name_who_set_the_effect_off(self):
        self.assertEqual(self.rows("Inquisition allies gain +10% power when Inquisition allies attack"), [
            ("power", 10, "ally_attack", "allies", "who", ("Inquisition",), "by", ("Inquisition",))])
        self.assertEqual(self.rows("Converts 5% damage taken by Carta allies into power for Carta allies"), [
            ("power", 5, "ally_hit", "allies", "who", ("Carta",), "damage", "by", ("Carta",))])
        self.assertEqual(self.rows("Dragon allies gain +3% all effect resists when Dragon allies are attacked"), [
            (stat, 3, "ally_hit", "allies", "who", ("Dragon",), "by", ("Dragon",))
            for stat in build_hero_auras.ALL_EFFECT_RESISTS])

    def test_wording_the_builder_used_to_skip(self):
        self.assertEqual(self.rows("Takes less damage vs Mages"), [("damage_resist", 15, "battle", "self", "vs", ("Mage",))])
        self.assertEqual(self.rows("Allies resist slow"), [("slow_resist", 15, "battle", "allies")])
        self.assertEqual(self.rows("Allies gain medium power when this banner is attacked and when it's killed"), [
            ("power", 35, "self_hit", "allies"), ("power", 35, "self_death", "allies")])
        self.assertEqual(self.rows("When allies are hit or attack, all enemies take 5% damage dealt or received"), [
            ("damage", 5, "ally_hit", "enemies", "damage"), ("damage", 5, "ally_attack", "enemies", "damage")])
        self.assertEqual(self.rows("On death, allies suffer 15% health loss and power drain"), [
            ("damage", 15, "self_death", "allies"), ("drain", 15, "self_death", "allies")])
        self.assertEqual(self.rows("35% bonus damage for allies vs opposing faction (stacking)"), [
            ("bonus_damage", 35, "battle", "allies", "vs", ("*",))])
        self.assertEqual(self.rows("White faction allies take 20% less damage from all other factions"), [
            ("damage_resist", 20, "battle", "allies", "who", ("White",), "vs", ("Black", "Red", "Blue"))])
        self.assertEqual(self.rows(
            "Allies get: +40% Health and Power (extra +10% for Nobility), +50% curse power resist, and resist 2k damage "
            "per hit"), [
            ("health", 40, "battle", "allies"), ("power", 40, "battle", "allies"),
            ("health", 10, "battle", "allies", "who", ("Nobility",)),
            ("power", 10, "battle", "allies", "who", ("Nobility",)),
            ("curse_power_resist", 50, "battle", "allies"), ("raw_damage_resist", 2000, "battle", "allies")])

    def test_a_hero_naming_itself_means_this_hero(self):
        snoufleur = {"name": "Snoufleur"}
        self.assertEqual(self.rows(build_hero_auras.hero_sentence("When Snoufleur dies, allies gain 100% Power", snoufleur)),
                         [("power", 100, "self_death", "allies")])
        nug_king = {"name": "Nug King"}
        self.assertEqual(build_hero_auras.hero_sentence("Nug King gains +25% Power for every Nug in the squad", nug_king),
                         "gains +25% Power for every Nug in the squad")

    def test_every_shipped_aura_sentence_parses(self):
        catalog, unparsed, _ = build_hero_auras.build()
        self.assertEqual(unparsed, [])
        self.assertEqual(catalog["unparsed"], [])

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
        per_ally = {"stat": "health", "value": 35, "on": "battle", "scope": "self", "per": [["group", "Avvar"]]}
        self.assertFalse(build_hero_auras.already_applied(per_ally, applied))
        self.assertTrue(build_hero_auras.already_applied(per_ally, build_hero_auras.applied_passives([
            {"kind": "stat_percent_per_matching_ally", "stats": ["health"], "value_pct": 35}])))


if __name__ == "__main__":
    unittest.main()
