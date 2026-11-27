from __future__ import annotations

import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "client-additional-data" / "anim_tools"))

import shared_textures


BIN_CHUNK = b"BIN\0"
SKIN = b"skin pixels"
EYES = b"eye pixels"


def write_model(path: Path, uris: list[str], blob: bytes = b"vertices") -> None:
    text = json.dumps({"asset": {"version": "2.0"}, "images": [{"uri": uri} for uri in uris]}).encode()
    text += b" " * (-len(text) % 4)
    blob += b"\0" * (-len(blob) % 4)
    body = struct.pack("<I4s", len(text), b"JSON") + text + struct.pack("<I4s", len(blob), BIN_CHUNK) + blob
    path.write_bytes(struct.pack("<4sII", b"glTF", 2, 12 + len(body)) + body)


def model_uris(path: Path) -> list[str]:
    _, document, _ = shared_textures.read_glb(path)
    return [image.get("uri") for image in document["images"]]


class ShareFamilyTest(unittest.TestCase):
    def setUp(self) -> None:
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.family = Path(temporary.name) / "hf_hero"
        self.family.mkdir()
        for tier in (1, 2, 3):
            (self.family / f"hf_hero_t{tier}_main.png").write_bytes(SKIN)
            write_model(self.family / f"hf_hero_t{tier}_rl.glb", [f"hf_hero_t{tier}_main.png", "eyes.png"])
        (self.family / "eyes.png").write_bytes(EYES)
        self.eyes = self.shared_name_of("eyes.png")
        self.skin = self.shared_name_of("hf_hero_t1_main.png")

    def shared_name_of(self, texture: str) -> str:
        return shared_textures.shared_name(shared_textures.file_digest(self.family / texture), texture)

    def test_identical_tier_textures_point_at_one_file_named_by_their_bytes(self) -> None:
        result = shared_textures.share_family(self.family)
        for tier in (1, 2, 3):
            self.assertEqual(model_uris(self.family / f"hf_hero_t{tier}_rl.glb"), [self.skin, "eyes.png"])
        self.assertEqual((self.family / self.skin).read_bytes(), SKIN)
        self.assertEqual((result.models, result.textures, result.bytes), (3, 1, 2 * len(SKIN)))
        self.assertFalse((self.family / self.eyes).exists())

    def test_only_the_json_chunk_changes(self) -> None:
        model = self.family / "hf_hero_t2_rl.glb"
        _, _, before = shared_textures.read_glb(model)
        shared_textures.share_family(self.family)
        _, _, after = shared_textures.read_glb(model)
        self.assertEqual(after, before)
        self.assertEqual(struct.unpack_from("<I", model.read_bytes(), 8)[0], model.stat().st_size)

    def test_a_second_run_changes_nothing(self) -> None:
        shared_textures.share_family(self.family)
        snapshot = {path.name: path.read_bytes() for path in self.family.iterdir()}
        result = shared_textures.share_family(self.family)
        self.assertEqual(result.models, 0)
        self.assertEqual({path.name: path.read_bytes() for path in self.family.iterdir()}, snapshot)

    def test_a_rebaked_tier_with_the_same_pixels_joins_the_shared_file(self) -> None:
        shared_textures.share_family(self.family)
        write_model(self.family / "hf_hero_t2_rl.glb", ["hf_hero_t2_main.png", "eyes.png"])
        shared_textures.share_family(self.family)
        self.assertEqual(model_uris(self.family / "hf_hero_t2_rl.glb"), [self.skin, "eyes.png"])

    def test_a_rebaked_tier_with_new_pixels_leaves_the_other_tiers_alone(self) -> None:
        shared_textures.share_family(self.family)
        (self.family / "hf_hero_t1_main.png").write_bytes(b"new skin")
        write_model(self.family / "hf_hero_t1_rl.glb", ["hf_hero_t1_main.png", "eyes.png"])
        shared_textures.share_family(self.family)
        self.assertEqual(model_uris(self.family / "hf_hero_t1_rl.glb"), ["hf_hero_t1_main.png", "eyes.png"])
        self.assertEqual(model_uris(self.family / "hf_hero_t3_rl.glb"), [self.skin, "eyes.png"])
        self.assertEqual((self.family / self.skin).read_bytes(), SKIN)

    def test_embedded_missing_and_outside_images_are_left_as_they_are(self) -> None:
        uris = ["data:image/png;base64,AAAA", "gone.png", "../other/hf_hero_t1_main.png"]
        write_model(self.family / "hf_hero_t4_rl.glb", uris)
        shared_textures.share_family(self.family)
        self.assertEqual(model_uris(self.family / "hf_hero_t4_rl.glb"), uris)


if __name__ == "__main__":
    unittest.main()
