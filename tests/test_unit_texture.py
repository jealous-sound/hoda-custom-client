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
TRANSPARENT_PIXEL = (180, 90, 40, 0)


def material(file_id: int, path_id: int, externals: list[str]):
    reader = SimpleNamespace(
        assets_file=SimpleNamespace(externals=[SimpleNamespace(path=path) for path in externals])
    )
    return reader, {"m_Shader": {"m_FileID": file_id, "m_PathID": path_id}}


def transparent_texture() -> Image.Image:
    return Image.new("RGBA", (4, 4), TRANSPARENT_PIXEL)


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
        baked, dropped = unit_texture.runtime_main_texture(transparent_texture(), reader, typetree)
        self.assertTrue(dropped)
        self.assertEqual(baked.mode, "RGBA")
        self.assertEqual(baked.getchannel("A").getextrema(), (unit_texture.OPAQUE_ALPHA, unit_texture.OPAQUE_ALPHA))
        self.assertEqual(baked.getpixel((0, 0))[:3], TRANSPARENT_PIXEL[:3])

    def test_cutout_material_keeps_its_texture(self) -> None:
        reader, typetree = material(3, 1, ["sharedassets0.assets", BUILTIN_EXTRA, "dff54c5b"])
        texture = transparent_texture()
        baked, dropped = unit_texture.runtime_main_texture(texture, reader, typetree)
        self.assertFalse(dropped)
        self.assertIs(baked, texture)


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


if __name__ == "__main__":
    unittest.main()
