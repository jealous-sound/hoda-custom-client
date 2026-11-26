from __future__ import annotations

from pathlib import Path
import re
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import cache_reach
import original_cache


TEXTURE = original_cache.TEXTURE_2D
AUDIO = original_cache.AUDIO_CLIP


def bundle(name: str, *objects: str, class_id: int = TEXTURE) -> cache_reach.Bundle:
    return cache_reach.Bundle(f"cache/{name}/CAB-{name}", name, [(class_id, item) for item in objects])


def reached(bundles: list[cache_reach.Bundle], source=(), data=(), stems=()) -> set[str]:
    reach = cache_reach.gather(list(source), list(data), list(stems))
    return {item.name for item in bundles if item.path in cache_reach.reachable_bundles(bundles, reach)}


class SourceStringsTest(unittest.TestCase):
    def test_reads_string_literals_and_skips_character_literals(self) -> None:
        text = "char quote = '\"'; const char *a = \"Btn_\\\"Close\\\"\"; Load(CACHE_RESOURCE(\"grass_01\"));"
        self.assertEqual(cache_reach.c_strings(text), ['Btn_"Close"', "grass_01"])

    def test_stems_are_defined_strings_that_end_in_a_separator(self) -> None:
        text = '#define TRAY_PREFIX "tex.Btn_MapPVE_map_"\n#define TITLE "Heroes"\n#define SHORT "ab_"\n'
        self.assertEqual(cache_reach.c_stems(text), ["tex.Btn_MapPVE_map_"])

    def test_source_files_skip_vendored_code(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary)
            for name in ("game.c", "game.h", "vendors/lib/lib.c", "notes.md"):
                (source / name).parent.mkdir(parents=True, exist_ok=True)
                (source / name).write_text("")
            found = [path.relative_to(source).as_posix() for path in cache_reach.source_files(source)]
        self.assertEqual(found, ["game.c", "game.h"])


class FormatRegexTest(unittest.TestCase):
    def test_conversions_become_wildcards(self) -> None:
        pattern = re.compile(cache_reach.format_regex("%stex.ico_achievement_%s"))
        self.assertTrue(pattern.fullmatch("tex.ico_achievement_dragon"))
        self.assertFalse(pattern.fullmatch("tex.ico_quest_dragon"))

    def test_integer_conversions_match_digits_only(self) -> None:
        pattern = re.compile(cache_reach.format_regex("map%d_character"))
        self.assertTrue(pattern.fullmatch("map12_character"))
        self.assertFalse(pattern.fullmatch("mapX_character"))

    def test_glue_formats_spell_nothing(self) -> None:
        for glue in ("%s", "%s%s", "%s/%s", "%d.%d"):
            with self.subTest(glue=glue):
                self.assertIsNone(cache_reach.format_regex(glue))


class ReachableBundlesTest(unittest.TestCase):
    def test_keeps_the_alphabetically_first_holder_of_a_spelled_name(self) -> None:
        bundles = [bundle("b_hud", "Btn_Close"), bundle("a_popup", "Btn_Close"), bundle("c_unused", "Bg_Stone")]
        self.assertEqual(reached(bundles, source=["Btn_Close"]), {"a_popup"})

    def test_holders_are_chosen_per_class(self) -> None:
        bundles = [bundle("a_hud", "click"), bundle("b_sounds", "click", class_id=AUDIO)]
        self.assertEqual(reached(bundles, source=["click"]), {"a_hud", "b_sounds"})

    def test_a_qualified_ref_keeps_its_bundle_even_when_another_holds_the_name_first(self) -> None:
        bundles = [bundle("a_shared", "smoke3"), bundle("fx_frost_pre", "smoke3"), bundle("fx_fire_pre", "smoke3")]
        self.assertEqual(reached(bundles, data=["cache:fx_frost_pre/smoke3"]), {"a_shared", "fx_frost_pre"})

    def test_a_bundle_the_sources_name_ships_with_an_object_any_string_spells(self) -> None:
        bundles = [bundle("a_icons", "ico_nodelocked"), bundle("map1_pre", "ico_nodelocked", "unused")]
        source = ["map1_pre", "ico_nodelocked"]
        self.assertEqual(reached(bundles, source=source), {"a_icons", "map1_pre"})

    def test_a_bundle_only_data_names_needs_a_qualified_ref(self) -> None:
        bundles = [bundle("a_fx", "vfx_lvl_flare"), bundle("c_hero_t1_pre", "vfx_lvl_flare", "c_hero_tex")]
        self.assertEqual(reached(bundles, data=["c_hero_t1_pre", "vfx_lvl_flare"]), {"a_fx"})

    def test_formatted_bundle_and_object_names_reach_numbered_prefabs(self) -> None:
        bundles = [
            bundle("a_maps", "map1", "map2", "map2_character"),
            bundle("map_placeholder2_pre", "map2", "map2_character"),
            bundle("map_placeholder_pre", "map1"),
            bundle("map_unrelated_pre", "map2"),
        ]
        source = ["map_placeholder_pre", "map_placeholder%d_pre", "map%d", "map%d_character"]
        self.assertEqual(reached(bundles, source=source), {"a_maps", "map_placeholder2_pre", "map_placeholder_pre"})

    def test_a_qualified_format_pairs_its_bundle_and_name(self) -> None:
        bundles = [bundle("a_shared", "Snowflake"), bundle("fx_frost_pre", "Snowflake"), bundle("fx_fire_pre", "Ember")]
        self.assertEqual(reached(bundles, data=["cache:fx_frost_pre/%s"]), {"fx_frost_pre"})

    def test_stems_reach_names_built_by_appending_to_them(self) -> None:
        bundles = [bundle("a_tray", "tex.Btn_MapPVE_map_01"), bundle("b_other", "tex.Btn_Other")]
        self.assertEqual(reached(bundles, stems=["tex.Btn_MapPVE_map_"]), {"a_tray"})

    def test_apk_refs_and_glue_formats_reach_nothing(self) -> None:
        bundles = [bundle("a_hud", "Btn_Close"), bundle("b_hud", "%s")]
        self.assertEqual(reached(bundles, source=["apk:Btn_Close", "%s/%s", "cache:%s"]), set())

    def test_data_keys_are_spellings_too(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            data = Path(temporary) / "audio.json"
            data.write_text('{"audio_sources": {"click_01": null}, "list": ["Bg_Stone", 3]}')
            strings = cache_reach.data_strings(data)
        self.assertEqual(sorted(strings), ["Bg_Stone", "audio_sources", "click_01", "list"])


if __name__ == "__main__":
    unittest.main()
