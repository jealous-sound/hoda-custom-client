from __future__ import annotations

from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "client-additional-data" / "anim_tools"))

import audit_character_models as audit
import unit_texture


BUILTIN_EXTRA = "resources/unity_builtin_extra"
UNLIT_TEXTURE_PATH_ID = 10752
ADDITIVE_COLOR_GUID = "9be333ebb9f7d2249a99c06fe10d85cf"
ADDITIVE_COLOR_PATH_ID = 1
TRANSPARENT_PIXEL = (180, 90, 40, 0)
GHOST_PIXEL = (100, 200, 60, 255)
RIFT_WRAITH_GREY_TINT = (0.191, 0.191, 0.191, 0.502)
RIFT_WRAITH_RED_TINT = (0.985, 0.072, 0.072, 0.502)


def material(file_id: int, path_id: int, externals: list[str], colors: list | None = None):
    reader = SimpleNamespace(
        assets_file=SimpleNamespace(externals=[SimpleNamespace(path=path) for path in externals])
    )
    typetree = {"m_Shader": {"m_FileID": file_id, "m_PathID": path_id}}
    if colors is not None:
        typetree["m_SavedProperties"] = {"m_Colors": colors}
    return reader, typetree


def tint_row(name: str, rgba: tuple[float, float, float, float]):
    return [{"name": name}, dict(zip("rgba", rgba))]


def additive_material(tint: tuple[float, float, float, float] | None):
    colors = [tint_row("_Color", (1.0, 1.0, 1.0, 1.0))]
    if tint is not None:
        colors.append(tint_row("_TintColor", tint))
    return material(2, ADDITIVE_COLOR_PATH_ID, ["sharedassets0.assets", ADDITIVE_COLOR_GUID], colors)


def transparent_texture() -> Image.Image:
    return Image.new("RGBA", (4, 4), TRANSPARENT_PIXEL)


def ghost_texture() -> Image.Image:
    return Image.new("RGBA", (4, 4), GHOST_PIXEL)


class UnlitTextureShaderTest(unittest.TestCase):
    def test_builtin_unlit_texture_ignores_alpha_however_the_path_is_spelled(self) -> None:
        for path in (BUILTIN_EXTRA, "Resources/unity_builtin_extra", "library\\unity_builtin_extra"):
            with self.subTest(path=path):
                self.assertTrue(unit_texture.shader_ignores_alpha(path, UNLIT_TEXTURE_PATH_ID))

    def test_other_shaders_keep_their_alpha(self) -> None:
        self.assertFalse(unit_texture.shader_ignores_alpha(BUILTIN_EXTRA, 7))
        self.assertFalse(unit_texture.shader_ignores_alpha("9be333eb7a0d4b4a8a1bd4c1f1a29e3c", 1))
        self.assertFalse(unit_texture.shader_ignores_alpha("", UNLIT_TEXTURE_PATH_ID))

    def test_material_shader_follows_the_external_file_id(self) -> None:
        reader, typetree = material(2, UNLIT_TEXTURE_PATH_ID, ["sharedassets0.assets", BUILTIN_EXTRA])
        self.assertEqual(unit_texture.material_shader(reader, typetree), (BUILTIN_EXTRA, UNLIT_TEXTURE_PATH_ID))

    def test_in_bundle_shader_has_no_external_path(self) -> None:
        reader, typetree = material(0, UNLIT_TEXTURE_PATH_ID, [BUILTIN_EXTRA])
        self.assertEqual(unit_texture.material_shader(reader, typetree), ("", UNLIT_TEXTURE_PATH_ID))

    def test_unlit_texture_material_bakes_an_opaque_texture_with_the_same_colours(self) -> None:
        reader, typetree = material(2, UNLIT_TEXTURE_PATH_ID, ["sharedassets0.assets", BUILTIN_EXTRA])
        baked, note = unit_texture.runtime_main_texture(transparent_texture(), reader, typetree)
        self.assertEqual(note, unit_texture.UNLIT_TEXTURE_NOTE)
        self.assertEqual(baked.mode, "RGBA")
        self.assertEqual(baked.getchannel("A").getextrema(), (unit_texture.OPAQUE_ALPHA, unit_texture.OPAQUE_ALPHA))
        self.assertEqual(baked.getpixel((0, 0))[:3], TRANSPARENT_PIXEL[:3])

    def test_cutout_material_keeps_its_texture(self) -> None:
        reader, typetree = material(3, 1, ["sharedassets0.assets", BUILTIN_EXTRA, "dff54c5b"])
        texture = transparent_texture()
        baked, note = unit_texture.runtime_main_texture(texture, reader, typetree)
        self.assertIsNone(note)
        self.assertIs(baked, texture)


class AdditiveColorShaderTest(unittest.TestCase):
    def test_only_the_additive_color_shader_adds_tinted_light(self) -> None:
        self.assertTrue(unit_texture.shader_adds_tinted_light(ADDITIVE_COLOR_GUID, ADDITIVE_COLOR_PATH_ID))
        self.assertFalse(unit_texture.shader_adds_tinted_light(ADDITIVE_COLOR_GUID, 2))
        self.assertFalse(unit_texture.shader_adds_tinted_light(BUILTIN_EXTRA, UNLIT_TEXTURE_PATH_ID))

    def test_tint_dimmer_than_its_texture_is_raised_to_full_strength_keeping_its_hue(self) -> None:
        for channel in unit_texture.opaque_additive_gain(RIFT_WRAITH_GREY_TINT[:3]):
            self.assertAlmostEqual(channel, 1.0)
        yellow = unit_texture.opaque_additive_gain((0.213, 0.169, 0.025))
        self.assertAlmostEqual(max(yellow), 1.0)
        self.assertAlmostEqual(yellow[1] / yellow[0], 0.169 / 0.213)

    def test_tint_brighter_than_its_texture_keeps_the_shader_gain(self) -> None:
        red = unit_texture.opaque_additive_gain(RIFT_WRAITH_RED_TINT[:3])
        for channel, tint in zip(red, RIFT_WRAITH_RED_TINT):
            self.assertAlmostEqual(channel, unit_texture.ADDITIVE_COLOR_GAIN * tint)

    def test_black_tint_adds_nothing(self) -> None:
        self.assertEqual(unit_texture.opaque_additive_gain((0.0, 0.0, 0.0)), (0.0, 0.0, 0.0))

    def test_red_tier_bakes_its_tint_into_an_opaque_texture(self) -> None:
        reader, typetree = additive_material(RIFT_WRAITH_RED_TINT)
        baked, note = unit_texture.runtime_main_texture(ghost_texture(), reader, typetree)
        self.assertIn("_TintColor", note)
        gain = unit_texture.opaque_additive_gain(RIFT_WRAITH_RED_TINT[:3])
        expected = tuple(min(unit_texture.CHANNEL_MAX, round(v * k)) for v, k in zip(GHOST_PIXEL, gain))
        self.assertEqual(baked.getpixel((0, 0)), expected + (unit_texture.OPAQUE_ALPHA,))

    def test_untinted_additive_material_uses_the_shader_default_tint(self) -> None:
        reader, typetree = additive_material(None)
        baked, _ = unit_texture.runtime_main_texture(ghost_texture(), reader, typetree)
        doubled = tuple(min(unit_texture.CHANNEL_MAX, 2 * v) for v in GHOST_PIXEL[:3])
        self.assertEqual(baked.getpixel((0, 0)), doubled + (unit_texture.OPAQUE_ALPHA,))

    def test_material_color_reads_pair_rows(self) -> None:
        _, typetree = additive_material(RIFT_WRAITH_RED_TINT)
        typetree["m_SavedProperties"]["m_Colors"] = [tuple(row) for row in typetree["m_SavedProperties"]["m_Colors"]]
        self.assertEqual(unit_texture.material_color(typetree, "_TintColor"), RIFT_WRAITH_RED_TINT[:3])
        self.assertIsNone(unit_texture.material_color(typetree, "_Emission"))


class InvisibleModelAuditTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.directory = Path(self.temporary.name)

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def gltf_with(self, name: str, image: Image.Image) -> dict:
        image.save(self.directory / name)
        return {
            "images": [{"uri": name}],
            "textures": [{"source": 0}],
            "materials": [{"pbrMetallicRoughness": {"baseColorTexture": {"index": 0}}}],
        }

    def test_texture_below_the_unit_shader_cutoff_everywhere_is_invisible(self) -> None:
        texture = Image.new("RGBA", (4, 4), (200, 100, 50, audit.UNIT_SHADER_ALPHA_CUTOFF - 1))
        gltf = self.gltf_with("hidden_main.png", texture)
        self.assertEqual(audit.invisible_images(gltf, self.directory), ["hidden_main.png"])

    def test_cutout_texture_with_any_visible_texel_passes(self) -> None:
        texture = transparent_texture()
        texture.putpixel((1, 1), (200, 100, 50, audit.UNIT_SHADER_ALPHA_CUTOFF))
        gltf = self.gltf_with("cutout_main.png", texture)
        self.assertEqual(audit.invisible_images(gltf, self.directory), [])

    def test_texture_without_alpha_passes(self) -> None:
        gltf = self.gltf_with("plain_main.png", Image.new("RGB", (4, 4), (200, 100, 50)))
        self.assertEqual(audit.invisible_images(gltf, self.directory), [])

    def test_opaque_bake_of_a_transparent_texture_passes(self) -> None:
        gltf = self.gltf_with("baked_main.png", unit_texture.opaque(transparent_texture()))
        self.assertEqual(audit.invisible_images(gltf, self.directory), [])


class UntexturedModelAuditTest(unittest.TestCase):
    def test_material_without_a_base_colour_texture_is_untextured(self) -> None:
        gltf = {"materials": [{"name": "c_riftwraith_t4", "alphaMode": "OPAQUE"}]}
        self.assertTrue(audit.untextured(gltf))

    def test_model_without_materials_is_untextured(self) -> None:
        self.assertTrue(audit.untextured({}))

    def test_material_with_a_base_colour_texture_passes(self) -> None:
        gltf = {"materials": [{"pbrMetallicRoughness": {"baseColorTexture": {"index": 0}}}]}
        self.assertFalse(audit.untextured(gltf))


if __name__ == "__main__":
    unittest.main()
