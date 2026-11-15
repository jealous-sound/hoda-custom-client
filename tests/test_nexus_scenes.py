from __future__ import annotations

import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import build_nexus_scenes

SCENES = json.loads(build_nexus_scenes.OUTPUT.read_text(encoding="utf-8"))["scenes"]
MAPS = json.loads(build_nexus_scenes.MAPS.read_text(encoding="utf-8"))
BLENDS = {build_nexus_scenes.OPAQUE, build_nexus_scenes.CUTOUT, build_nexus_scenes.ALPHA, build_nexus_scenes.ADDITIVE}
MAP_CAMERA_ROTATION = (0.0, 0.70711, -0.70711, 0.0)
DOWN = (0.0, -1.0, 0.0)
SCREEN_UP = (0.0, 0.0, 1.0)
UNMIRRORED_PREFAB = "nexus_map3_pre"
LETTERBOXED_PREFAB = "SnexusDEM_map12_pre"
SIDE_MASKED_PREFAB = "SnexusFH_map7_pre"
UNMIRRORED_LAYER = "nexus_map3_FBX/mg"


def close(a, b, places=4):
    return all(abs(x - y) < 10 ** -places for x, y in zip(a, b))


class NexusScenesTest(unittest.TestCase):
    def test_every_event_map_has_one_scene(self) -> None:
        prefabs = [scene["prefab"] for scene in SCENES]
        self.assertEqual(len(prefabs), len(set(prefabs)))
        self.assertEqual(set(prefabs), {event["prefab"] for event in MAPS})

    def test_layers_are_well_formed_triangle_lists(self) -> None:
        for scene in SCENES:
            for layer in scene["layers"]:
                with self.subTest(prefab=scene["prefab"], layer=layer["name"]):
                    vertices = len(layer["positions"]) // 3
                    self.assertEqual(len(layer["positions"]), vertices * 3)
                    self.assertEqual(len(layer["uvs"]), vertices * 2)
                    self.assertEqual(len(layer["indices"]) % 3, 0)
                    self.assertTrue(all(0 <= index < vertices for index in layer["indices"]))
                    self.assertIn(layer["blend"], BLENDS)
                    if layer["texture"] is None:
                        self.assertEqual(layer["blend"], build_nexus_scenes.OPAQUE)

    def test_every_map_camera_looks_down_and_scrolls_along_x(self) -> None:
        for scene in SCENES:
            with self.subTest(prefab=scene["prefab"]):
                self.assertTrue(close(scene["camera"]["forward"], DOWN))
                self.assertTrue(close(scene["camera"]["up"], SCREEN_UP))

    def test_map_art_is_not_mirrored(self) -> None:
        scene = next(scene for scene in SCENES if scene["prefab"] == UNMIRRORED_PREFAB)
        layer = next(layer for layer in scene["layers"] if layer["name"] == UNMIRRORED_LAYER)
        xs = layer["positions"][0::3]
        us = layer["uvs"][0::2]
        screen_left = max(range(len(xs)), key=xs.__getitem__)
        screen_right = min(range(len(xs)), key=xs.__getitem__)
        self.assertLess(us[screen_left], us[screen_right])

    def test_side_masks_are_dropped_and_letterbox_frames_kept(self) -> None:
        for scene in SCENES:
            for layer in scene["layers"]:
                with self.subTest(prefab=scene["prefab"], layer=layer["name"]):
                    self.assertFalse(build_nexus_scenes.is_side_mask(layer, scene["camera"]))
        frames = {
            scene["prefab"]: [layer for layer in scene["layers"] if layer["texture"] is None] for scene in SCENES
        }
        self.assertTrue(frames[LETTERBOXED_PREFAB])
        self.assertFalse(frames[SIDE_MASKED_PREFAB])

    def test_a_frame_spanning_the_view_height_is_a_side_mask(self) -> None:
        camera = {"position": [0.0, 10.0, 0.0], "fov": 26.0}
        tall = {"texture": None, "positions": [0.0, 0.0, -5.0, 1.0, 0.0, 5.0]}
        band = {"texture": None, "positions": [0.0, 0.0, 1.0, 1.0, 0.0, 5.0]}
        self.assertTrue(build_nexus_scenes.is_side_mask(tall, camera))
        self.assertFalse(build_nexus_scenes.is_side_mask(band, camera))
        self.assertFalse(build_nexus_scenes.is_side_mask({**tall, "texture": "cache:art"}, camera))

    def test_unity_points_keep_x_and_flip_z(self) -> None:
        world = build_nexus_scenes.mat_mul(
            build_nexus_scenes.FLIP_Z, build_nexus_scenes.trs((1.0, 2.0, 3.0), (0.0, 0.0, 0.0, 1.0), (1.0, 1.0, 1.0))
        )
        self.assertTrue(close(build_nexus_scenes.transform_point(world, (4.0, 0.0, 0.0)), (5.0, 2.0, -3.0)))

    def test_map_camera_rotation_looks_down_with_screen_up_along_z(self) -> None:
        matrix = build_nexus_scenes.trs((0.0, 12.0, 0.0), MAP_CAMERA_ROTATION, (1.0, 1.0, 1.0))
        camera = build_nexus_scenes.camera_entry(matrix, {"field of view": 26.0})
        self.assertTrue(close(camera["forward"], DOWN))
        self.assertTrue(close(camera["up"], SCREEN_UP))

    def test_image_uv_counts_rows_from_the_top(self) -> None:
        self.assertEqual(build_nexus_scenes.image_uv((0.25, 0.0), (1.0, 1.0), (0.0, 0.0)), (0.25, 1.0))
        self.assertEqual(build_nexus_scenes.image_uv((0.5, 0.5), (2.0, 1.0), (0.1, 0.25)), (1.1, 0.25))


if __name__ == "__main__":
    unittest.main()
