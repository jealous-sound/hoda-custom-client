from __future__ import annotations

import json
import plistlib
from pathlib import Path
import re
import struct
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import build_dist
import cache_reach
import original_cache
import validate_cache_refs


GLB_MAGIC = b"glTF"
GLB_VERSION = 2
JSON_CHUNK = b"JSON"


def write_glb(path: Path, image_uris: list[str]) -> None:
    document = json.dumps({"asset": {"version": "2.0"}, "images": [{"uri": uri} for uri in image_uris]}).encode()
    document += b" " * (-len(document) % 4)
    header = GLB_MAGIC + struct.pack("<II", GLB_VERSION, 12 + 8 + len(document))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(header + struct.pack("<I", len(document)) + JSON_CHUNK + document)


def touch(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(b"x")


class BuildDistTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.unit = self.root / build_dist.CHAR_PACK_DIR / "Unit"

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def test_glb_image_uris_reads_the_json_chunk(self) -> None:
        model = self.root / "model.glb"
        write_glb(model, ["skin.png", "eyes.png"])
        self.assertEqual(build_dist.glb_image_uris(model), ["skin.png", "eyes.png"])

    def test_character_files_keep_runtime_models_and_their_textures_only(self) -> None:
        write_glb(self.unit / "Unit_rl.glb", ["skin.png"])
        write_glb(self.unit / "Unit_t0.glb", ["source.png"])
        for name in ("skin.png", "source.png", "unused.png", "Unit_t0.gltf", "Unit_t0.bin"):
            touch(self.unit / name)
        expected = {
            f"{build_dist.CHAR_PACK_DIR}/Unit/Unit_rl.glb",
            f"{build_dist.CHAR_PACK_DIR}/Unit/skin.png",
        }
        self.assertEqual(set(build_dist.character_files(self.root)), expected)

    def test_character_files_reject_a_model_with_a_missing_texture(self) -> None:
        write_glb(self.unit / "Unit_rl.glb", ["gone.png"])
        with self.assertRaises(SystemExit):
            build_dist.character_files(self.root)

    def write_runtime_meshes(self, consume_meshes: list[str]) -> Path:
        meshes = self.root / build_dist.MESH_DIR
        for mesh in build_dist.RUNTIME_MESHES:
            if mesh.endswith(".glb"):
                write_glb(meshes / mesh, [])
            else:
                touch(meshes / mesh)
        scene = self.root / build_dist.CONSUME_SCENE
        scene.parent.mkdir(parents=True, exist_ok=True)
        scene.write_text(json.dumps({"nodes": [{"mesh": name, "children": []} for name in consume_meshes]}))
        return meshes

    def test_mesh_files_ship_the_listed_scenes_and_the_consume_meshes_only(self) -> None:
        meshes = self.write_runtime_meshes(["anvil", "banner"])
        consume = meshes / build_dist.CONSUME_MESH_DIR
        for name in ("ply_vertexcolor/anvil.ply", "anvil.obj", "banner.obj", "ply_vertexcolor/unused.ply"):
            touch(consume / name)
        for name in ("SnexusDP_map1_pre/ply_vertexcolor/bg.ply", "_shared_textures/sky.png", "README.md"):
            touch(meshes / name)
        expected = {f"{build_dist.MESH_DIR}/{mesh}" for mesh in build_dist.RUNTIME_MESHES} | {
            f"{build_dist.MESH_DIR}/{build_dist.CONSUME_MESH_DIR}/ply_vertexcolor/anvil.ply",
            f"{build_dist.MESH_DIR}/{build_dist.CONSUME_MESH_DIR}/banner.obj",
        }
        self.assertEqual(set(build_dist.mesh_files(self.root)), expected)

    def test_mesh_files_add_the_images_a_scene_model_names(self) -> None:
        meshes = self.write_runtime_meshes([])
        glb = next(mesh for mesh in build_dist.RUNTIME_MESHES if mesh.endswith(".glb"))
        write_glb(meshes / glb, ["atlas.png"])
        touch((meshes / glb).parent / "atlas.png")
        self.assertIn(f"{build_dist.MESH_DIR}/{Path(glb).parent}/atlas.png", build_dist.mesh_files(self.root))

    def test_mesh_files_reject_a_missing_scene(self) -> None:
        meshes = self.write_runtime_meshes(["gone"])
        with self.assertRaises(SystemExit):
            build_dist.mesh_files(self.root)
        (meshes / build_dist.CONSUME_MESH_DIR / "gone.obj").parent.mkdir(parents=True, exist_ok=True)
        touch(meshes / build_dist.CONSUME_MESH_DIR / "gone.obj")
        (meshes / build_dist.RUNTIME_MESHES[0]).unlink()
        with self.assertRaises(SystemExit):
            build_dist.mesh_files(self.root)

    def test_names_data_is_shipped_json_and_text_except_translations(self) -> None:
        self.assertTrue(build_dist.names_data("data/fx_presets.json"))
        self.assertTrue(build_dist.names_data(f"{build_dist.ASSETS}/startup_images.txt"))
        self.assertFalse(build_dist.names_data("data/localization/core/Loc_ENG_US.txt"))
        self.assertFalse(build_dist.names_data(f"{build_dist.MESH_DIR}/bg_plains_01_pre/reconstructed/scene.bin"))

    def test_portable_path_rejects_windows_reserved_spelling(self) -> None:
        self.assertTrue(build_dist.portable_path("data/gear.json"))
        for unportable in ("data/a:b.json", "data/a?.json", "data/trailing./x.json", "data/space /x.json"):
            self.assertFalse(build_dist.portable_path(unportable), unportable)

    def test_prepare_output_refuses_a_folder_it_did_not_make(self) -> None:
        foreign = self.root / "someone_else"
        touch(foreign / "keep.txt")
        with self.assertRaises(SystemExit):
            build_dist.prepare_output(foreign, replace=True)
        self.assertTrue((foreign / "keep.txt").is_file())

    def test_prepare_output_replaces_its_own_folder(self) -> None:
        out = self.root / "dist"
        build_dist.prepare_output(out, replace=False)
        touch(out / "stale.txt")
        build_dist.prepare_output(out, replace=True)
        self.assertFalse((out / "stale.txt").exists())
        self.assertTrue((out / build_dist.DIST_MARKER).is_file())

    def test_launcher_defaults_the_backend_but_lets_the_player_override_it(self) -> None:
        build_dist.write_launcher(self.root, "http://example.test:1")
        text = (self.root / build_dist.LAUNCHER).read_text()
        self.assertIn('HOD_BACKEND_URL="${HOD_BACKEND_URL:-http://example.test:1}"', text)
        build_dist.write_launcher(self.root, "")
        self.assertNotIn("HOD_BACKEND_URL", (self.root / build_dist.LAUNCHER).read_text())

    def test_bundle_environment_carries_the_backend_only_when_one_is_given(self) -> None:
        info = self.root / build_dist.APP_INFO
        info.parent.mkdir(parents=True)
        info.write_bytes(plistlib.dumps({"CFBundleName": build_dist.APP_NAME}))
        build_dist.write_bundle_environment(self.root, "http://example.test:1")
        written = plistlib.loads(info.read_bytes())
        self.assertEqual(written["LSEnvironment"], {"HOD_BACKEND_URL": "http://example.test:1"})
        self.assertEqual(written["CFBundleName"], build_dist.APP_NAME)
        build_dist.write_bundle_environment(self.root, "")
        self.assertNotIn("LSEnvironment", plistlib.loads(info.read_bytes()))

    def test_launcher_runs_the_executable_inside_the_bundle(self) -> None:
        build_dist.write_launcher(self.root, "")
        self.assertIn(f'exec "./{build_dist.APP_EXECUTABLE}"', (self.root / build_dist.LAUNCHER).read_text())


ORIGINAL_CACHE_PRESENT = (ROOT / build_dist.ORIGINAL_BUNDLES).is_dir()
MESH_PATH_SUFFIXES = (".obj", ".ply", ".glb")
MESH_DIR_ARGUMENT = "%s/"
MESH_DIR_UNDER_CACHE = "meshes/"
MESH_FOLDER_CONTENTS = "/.+"


@unittest.skipUnless(ORIGINAL_CACHE_PRESENT, "needs the original 4.4.0 cache")
class ShippedContentTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.loose = build_dist.loose_content_files(ROOT)
        cls.shipped = set(build_dist.runtime_bundle_files(ROOT, cls.loose))
        cls.holders: dict[tuple[int, str], list[cache_reach.Bundle]] = {}
        every = cache_reach.load_bundles(ROOT, cache_reach.bundle_files(ROOT, build_dist.ORIGINAL_BUNDLES))
        for bundle in every:
            for key in dict.fromkeys(bundle.objects):
                cls.holders.setdefault(key, []).append(bundle)
        cls.names_data = [ROOT / relative for relative in cls.loose if build_dist.names_data(relative)]

    def resolve(self, ref: str, class_id: int, shipped_only: bool) -> set[str]:
        bundle, _, name = ref.removeprefix(original_cache.CACHE_REF_PREFIX).rpartition("/")
        holders = [
            holder
            for holder in self.holders.get((class_id, name), [])
            if (not bundle or holder.name == bundle) and (not shipped_only or holder.path in self.shipped)
        ]
        first = min((holder.name for holder in holders), default=None)
        return {holder.path for holder in holders if holder.name == first}

    def assert_resolves_as_with_every_bundle(self, refs: set[str]) -> None:
        moved = []
        for ref in sorted(refs):
            for class_id in original_cache.NAMED_CLASSES:
                full = self.resolve(ref, class_id, shipped_only=False)
                if full and self.resolve(ref, class_id, shipped_only=True) != full:
                    moved.append(ref)
        self.assertEqual(moved, [])

    def test_every_written_out_cache_ref_finds_the_object_it_found_before(self) -> None:
        sources = sorted((ROOT / "source").glob("*.c"))
        refs = {ref for path in sources for _, ref in validate_cache_refs.literal_refs(path)}
        for path in self.names_data:
            strings = cache_reach.data_strings(path)
            refs |= {text for text in strings if text.startswith(original_cache.CACHE_REF_PREFIX)}
        self.assertGreater(len(refs), 1000)
        self.assert_resolves_as_with_every_bundle(refs)

    def test_every_name_the_shipped_data_spells_finds_the_object_it_found_before(self) -> None:
        names = {text for path in self.names_data for text in cache_reach.data_strings(path)}
        self.assert_resolves_as_with_every_bundle({original_cache.cache_ref(name) for name in names})

    def test_every_mesh_path_the_sources_name_ships(self) -> None:
        mesh_root = ROOT / build_dist.MESH_DIR
        folders = {path.name for path in mesh_root.iterdir() if path.is_dir()}
        shipped = [relative.removeprefix(f"{build_dist.MESH_DIR}/") for relative in self.loose]
        missing = []
        for path in cache_reach.source_files(ROOT / "source"):
            for text in cache_reach.c_strings(path.read_text()):
                mesh = text.removeprefix(MESH_DIR_ARGUMENT).removeprefix(MESH_DIR_UNDER_CACHE)
                if "/" not in mesh or mesh.split("/")[0] not in folders:
                    continue
                pattern = cache_reach.format_regex(mesh) or re.escape(mesh)
                if not mesh.endswith(MESH_PATH_SUFFIXES):
                    pattern += MESH_FOLDER_CONTENTS
                if not any(re.fullmatch(pattern, relative) for relative in shipped):
                    missing.append(f"{path.name}: {text}")
        self.assertEqual(missing, [])


if __name__ == "__main__":
    unittest.main()
