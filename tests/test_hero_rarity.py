from __future__ import annotations

from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import validate_hero_rarity


CATALOG = {
    "UNIT_GRUNT": {"id": "UNIT_GRUNT", "name": "Hurlock Grunt", "rarity": "Common"},
    "UNIT_YOUNG": {"id": "UNIT_YOUNG", "name": "Young Flemeth", "rarity": "Legendary"},
    "UNIT_GODDESS": {"id": "UNIT_GODDESS", "name": "Asha'Bellanar", "rarity": "Legendary"},
    "UNIT_WISP": {"id": "UNIT_WISP", "name": "Legendary Red Wisp", "rarity": "Legendary"},
    "UNIT_BANNER": {"id": "UNIT_BANNER", "name": "Templar Banner", "rarity": "Mythic"},
}


def row(uid, rarity, character=None):
    entry = {"id": uid, "rarity": rarity, "reason": "Because."}
    if character:
        entry["character"] = character
    return entry


def table(*replacements, cap=1):
    rows = {
        "UNIT_GRUNT": row("UNIT_GRUNT", "Common"),
        "UNIT_YOUNG": row("UNIT_YOUNG", "Epic", "Flemeth"),
        "UNIT_GODDESS": row("UNIT_GODDESS", "Legendary", "Flemeth"),
        "UNIT_WISP": row("UNIT_WISP", "Legendary"),
        "UNIT_BANNER": row("UNIT_BANNER", "Mythic"),
    }
    for replacement in replacements:
        rows[replacement["id"]] = replacement
    return {"version": 1, "legendary_cap": cap, "heroes": list(rows.values())}


def problems(proposed):
    return validate_hero_rarity.problems(proposed, CATALOG)


class HeroRarityTable(unittest.TestCase):
    def test_a_sound_table_has_no_problems(self):
        self.assertEqual(problems(table()), [])

    def test_the_shipped_table_is_sound(self):
        shipped = validate_hero_rarity.json.loads(validate_hero_rarity.TABLE.read_text(encoding="utf-8"))
        self.assertEqual(validate_hero_rarity.problems(shipped, validate_hero_rarity.catalog_heroes()), [])

    def test_the_shipped_catalog_carries_the_table(self):
        shipped = validate_hero_rarity.json.loads(validate_hero_rarity.TABLE.read_text(encoding="utf-8"))
        self.assertEqual(validate_hero_rarity.unapplied(shipped, validate_hero_rarity.catalog_heroes()), [])
        stale = validate_hero_rarity.unapplied(table(), CATALOG)
        self.assertEqual(stale, ["UNIT_YOUNG: the catalog ships Legendary, the table says Epic"])

    def test_no_reward_from_play_hands_out_a_legendary(self):
        self.assertEqual(validate_hero_rarity.shipped_play_reward_problems(validate_hero_rarity.catalog_heroes()), [])
        nexus = [{"difficulties": [{"id": "Hard_Map", "nodes": [{"step": 2, "reward_rarity": [4, 5]}]}]}]
        pvp = {
            "events": {
                "catalog": [
                    {
                        "rank_rewards": [
                            {
                                "prizes": [
                                    {"kind": "unit", "id": "UNIT_GODDESS"},
                                    {"kind": "unit", "id": "UNIT_GRUNT"},
                                    {"kind": "voucher", "id": "bloodstone"},
                                    {"kind": "voucher", "id": "iron"},
                                ]
                            }
                        ]
                    }
                ]
            }
        }
        self.assertEqual(
            validate_hero_rarity.play_reward_problems(nexus, pvp, CATALOG),
            [
                "Hard_Map step 2: the Nexus rolls a Legendary hero",
                "UNIT_GODDESS: a PvP prize hands out a Legendary hero",
                "a PvP prize hands out a bloodstone voucher",
            ],
        )

    def test_every_hero_needs_exactly_one_row(self):
        missing = table()
        missing["heroes"] = [entry for entry in missing["heroes"] if entry["id"] != "UNIT_GRUNT"]
        self.assertIn("UNIT_GRUNT: no row", problems(missing))
        doubled = table()
        doubled["heroes"].append(row("UNIT_GRUNT", "Common"))
        self.assertIn("UNIT_GRUNT: 2 rows", problems(doubled))
        stray = table()
        stray["heroes"].append(row("UNIT_NOBODY", "Common"))
        self.assertIn("UNIT_NOBODY: not a hero in the catalog", problems(stray))

    def test_legendary_is_capped_and_rarity_named_units_do_not_count(self):
        self.assertEqual(problems(table(cap=1)), [])
        over = problems(table(row("UNIT_GRUNT", "Legendary"), cap=1))
        self.assertIn("2 Legendary heroes, cap is 1", over)

    def test_a_character_holds_one_legendary_form(self):
        both = problems(table(row("UNIT_YOUNG", "Legendary", "Flemeth"), cap=5))
        self.assertIn("Flemeth: 2 Legendary forms, want at most one", both)

    def test_a_character_key_must_join_at_least_two_forms(self):
        lone = problems(table(row("UNIT_YOUNG", "Epic")))
        self.assertIn("Flemeth: a character key needs at least 2 forms", lone)

    def test_a_unit_named_for_a_tier_keeps_it(self):
        renamed = problems(table(row("UNIT_WISP", "Rare")))
        self.assertIn("UNIT_WISP: named Legendary but proposed Rare", renamed)

    def test_mythic_is_only_for_heroes_born_mythic(self):
        self.assertIn("UNIT_GRUNT: Mythic is only for heroes born Mythic", problems(table(row("UNIT_GRUNT", "Mythic"))))
        self.assertIn("UNIT_BANNER: Mythic is only for heroes born Mythic", problems(table(row("UNIT_BANNER", "Epic"))))

    def test_a_row_needs_a_known_rarity_and_a_reason(self):
        self.assertIn("UNIT_GRUNT: unknown rarity 'Godlike'", problems(table(row("UNIT_GRUNT", "Godlike"))))
        silent = row("UNIT_GRUNT", "Common")
        silent["reason"] = " "
        self.assertIn("UNIT_GRUNT: no reason", problems(table(silent)))


if __name__ == "__main__":
    unittest.main()
