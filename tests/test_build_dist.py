from __future__ import annotations

import json
import plistlib
from pathlib import Path
import struct
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import build_dist


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

    def test_mesh_files_skip_notes_and_scripts(self) -> None:
        meshes = self.root / build_dist.MESH_DIR
        for name in ("scene/terrain.obj", "scene/terrain.png", "scene/README.md", "scene/convert.py"):
            touch(meshes / name)
        self.assertEqual(
            sorted(build_dist.mesh_files(self.root)),
            [f"{build_dist.MESH_DIR}/scene/terrain.obj", f"{build_dist.MESH_DIR}/scene/terrain.png"],
        )

    def test_mesh_files_skip_names_windows_cannot_store(self) -> None:
        meshes = self.root / build_dist.MESH_DIR
        for name in ("scene/ply_vertexcolor/rock.ply", "scene/ply_vertexcolor/unity_14:grass.ply"):
            touch(meshes / name)
        self.assertEqual(build_dist.mesh_files(self.root), [f"{build_dist.MESH_DIR}/scene/ply_vertexcolor/rock.ply"])

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


if __name__ == "__main__":
    unittest.main()
