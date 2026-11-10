from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import build_custom_heroes


BASE_TIER = {
    "base_power": 100,
    "max_power": 300,
    "base_health": 1000,
    "max_health": 3000,
    "max_level": 20,
    "power_per_level": 10,
    "health_per_level": 100,
    "max_fortify_power": 900,
    "max_fortify_health": 2900,
}


def shipped_base():
    return {
        "UNIT_BASE": {
            "id": "UNIT_BASE",
            "name": "Base Hero",
            "name_ru": "Базовый герой",
            "rarity": "Legendary",
            "ability": {"key": "ABILITY_BASE", "mechanics": ["ATTACK"]},
            "icon": {tier: "tex.base_pt" for tier in build_custom_heroes.TIERS},
            "models": {tier: "unit.base_pre" for tier in build_custom_heroes.TIERS},
            "obtainable": False,
            "tiers": {"I": dict(BASE_TIER), "IV": dict(BASE_TIER)},
        }
    }


def spec(**overrides):
    value = {
        "id": "UNIT_DERIVED",
        "based_on": "UNIT_BASE",
        "prefab": "em_mw_derived",
        "character": {"name": "Derived Hero", "obtainable": True},
    }
    value.update(overrides)
    return value


class CatalogRowTest(unittest.TestCase):
    def test_derived_row_keeps_the_base_kit_under_its_own_identity(self):
        row = build_custom_heroes.catalog_row(spec(), shipped_base())
        self.assertEqual(list(row)[:2], ["id", "based_on"])
        self.assertEqual(row["id"], "UNIT_DERIVED")
        self.assertEqual(row["based_on"], "UNIT_BASE")
        self.assertEqual(row["name"], "Derived Hero")
        self.assertEqual(row["name_ru"], "Базовый герой")
        self.assertTrue(row["obtainable"])
        self.assertEqual(row["ability"], {"key": "ABILITY_BASE", "mechanics": ["ATTACK"]})

    def test_every_tier_names_its_own_model_and_portrait(self):
        row = build_custom_heroes.catalog_row(spec(), shipped_base())
        self.assertEqual(row["models"]["I"], "unit.em_mw_derived_t1_pre")
        self.assertEqual(row["models"]["IV"], "unit.em_mw_derived_t4_pre")
        self.assertEqual(row["icon"]["II"], "tex.em_mw_derived_t2_pt")
        self.assertEqual(row["icon"]["III"], "tex.em_mw_derived_t3_pt")

    def test_stat_scale_rounds_every_power_and_health_field(self):
        row = build_custom_heroes.catalog_row(spec(stat_scale={"power": 1.06, "health": 0.95}), shipped_base())
        tier = row["tiers"]["IV"]
        self.assertEqual(tier["base_power"], 106)
        self.assertEqual(tier["power_per_level"], 11)
        self.assertEqual(tier["max_fortify_power"], 954)
        self.assertEqual(tier["base_health"], 950)
        self.assertEqual(tier["max_fortify_health"], 2755)
        self.assertEqual(tier["max_level"], 20)

    def test_a_hero_placed_above_its_base_takes_that_band(self):
        bands = {
            "Legendary": {"costs": {"hero": {tier: 1000 for tier in "I II III IV".split()}}},
            "Mythic": {
                "costs": {"hero": {tier: 2000 for tier in "I II III IV".split()}},
                "level_caps": {tier: 11 for tier in "I II III IV".split()},
            },
        }
        plain = build_custom_heroes.catalog_row(spec(), shipped_base())
        raised = build_custom_heroes.catalog_row(spec(), shipped_base(), {spec()["id"]: "Mythic"}, bands)
        self.assertEqual(raised["rarity"], "Mythic")
        for tier, stats in raised["tiers"].items():
            self.assertEqual(stats["max_level"], 11)
            self.assertEqual(stats["base_power"], plain["tiers"][tier]["base_power"] * 2)
            self.assertEqual(stats["max_power"], stats["base_power"] + 10 * stats["power_per_level"])
        same = build_custom_heroes.catalog_row(spec(), shipped_base(), {spec()["id"]: "Legendary"}, bands)
        self.assertEqual(same, plain)

    def test_the_base_catalog_is_left_untouched(self):
        shipped = shipped_base()
        before = json.dumps(shipped, sort_keys=True)
        build_custom_heroes.catalog_row(spec(stat_scale={"power": 2.0}), shipped)
        self.assertEqual(json.dumps(shipped, sort_keys=True), before)

    def test_unknown_base_is_rejected(self):
        with self.assertRaises(SystemExit):
            build_custom_heroes.catalog_row(spec(based_on="UNIT_MISSING"), shipped_base())

    def test_shipped_id_is_rejected(self):
        with self.assertRaises(SystemExit):
            build_custom_heroes.catalog_row(spec(id="UNIT_BASE"), shipped_base())

    def test_name_longer_than_the_runtime_buffer_is_rejected(self):
        long_name = "x" * build_custom_heroes.RUNTIME_NAME_CAPACITY
        with self.assertRaises(SystemExit):
            build_custom_heroes.catalog_row(spec(character={"name": long_name}), shipped_base())


class SpecLoadingTest(unittest.TestCase):
    def write_specs(self, directory, *specs):
        for index, value in enumerate(specs):
            (Path(directory) / f"{index}.json").write_text(json.dumps(value), encoding="utf-8")

    def test_specs_load_in_file_order(self):
        with tempfile.TemporaryDirectory() as directory:
            self.write_specs(directory, spec(id="UNIT_A"), spec(id="UNIT_B"))
            self.assertEqual([s["id"] for s in build_custom_heroes.load_specs(Path(directory))], ["UNIT_A", "UNIT_B"])

    def test_unknown_spec_field_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            self.write_specs(directory, spec(colour="red"))
            with self.assertRaises(SystemExit):
                build_custom_heroes.load_specs(Path(directory))

    def test_duplicate_ids_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            self.write_specs(directory, spec(), spec())
            with self.assertRaises(SystemExit):
                build_custom_heroes.load_specs(Path(directory))

    def test_render_is_a_versioned_catalog(self):
        document = json.loads(build_custom_heroes.render([spec()], shipped_base()))
        self.assertEqual(document["version"], build_custom_heroes.CATALOG_VERSION)
        self.assertEqual([row["id"] for row in document["characters"]], ["UNIT_DERIVED"])


class BakeCheckTest(unittest.TestCase):
    def bake(self, root, value):
        char_pack, portraits = root / "char_pack", root / "portraits"
        (char_pack / value["prefab"]).mkdir(parents=True)
        portraits.mkdir()
        scales = {}
        for tier in build_custom_heroes.TIERS:
            base = build_custom_heroes.tier_base(value["prefab"], tier)
            (char_pack / value["prefab"] / f"{base}_rl.glb").write_bytes(b"glTF")
            (portraits / f"tex.{base}_pt.png").write_bytes(b"png")
            scales[base] = {"scale": 1.0}
        stamp = build_custom_heroes.bake_stamp_path(value["prefab"], char_pack)
        stamp.write_text(json.dumps(build_custom_heroes.bake_stamp(value)), encoding="utf-8")
        return char_pack, portraits, scales

    def problems(self, value, char_pack, portraits, scales):
        return build_custom_heroes.bake_problems(value, scales, char_pack, portraits)

    def test_a_complete_bake_of_the_current_spec_passes(self):
        with tempfile.TemporaryDirectory() as directory:
            char_pack, portraits, scales = self.bake(Path(directory), spec())
            self.assertEqual(self.problems(spec(), char_pack, portraits, scales), [])

    def test_a_missing_portrait_is_named(self):
        with tempfile.TemporaryDirectory() as directory:
            char_pack, portraits, scales = self.bake(Path(directory), spec())
            (portraits / "tex.em_mw_derived_t3_pt.png").unlink()
            self.assertEqual(
                self.problems(spec(), char_pack, portraits, scales), ["missing tex.em_mw_derived_t3_pt.png"]
            )

    def test_a_missing_display_scale_is_named(self):
        with tempfile.TemporaryDirectory() as directory:
            char_pack, portraits, scales = self.bake(Path(directory), spec())
            del scales["em_mw_derived_t1"]
            self.assertEqual(
                self.problems(spec(), char_pack, portraits, scales), ["no display scale for em_mw_derived_t1"]
            )

    def test_a_spec_edited_after_the_bake_is_stale(self):
        with tempfile.TemporaryDirectory() as directory:
            char_pack, portraits, scales = self.bake(Path(directory), spec())
            edited = spec(stat_scale={"power": 1.1})
            self.assertEqual(
                self.problems(edited, char_pack, portraits, scales),
                ["em_mw_derived_bake.json was not baked from the current spec"],
            )

    def test_spec_digest_ignores_key_order(self):
        reordered = dict(reversed(list(spec().items())))
        self.assertEqual(build_custom_heroes.spec_digest(reordered), build_custom_heroes.spec_digest(spec()))


if __name__ == "__main__":
    unittest.main()
