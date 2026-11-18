from __future__ import annotations

import json
import math
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import build_campaign_node_layout
import build_campaign_scenes
import map_prefab

SCENES = json.loads(build_campaign_scenes.OUTPUT.read_text(encoding="utf-8"))["scenes"]
MAPS = json.loads(build_campaign_scenes.MAPS.read_text(encoding="utf-8"))
DOWN = (0.0, -1.0, 0.0)
SCREEN_UP = (0.0, 0.0, 1.0)
MAP_CAMERA_FOV = 26.0
END_CAPS_PER_MAP = 2
END_CAP_PREFIX = f"{build_campaign_scenes.TRANSIT_PREFAB}/"
FIRE_MAP = "PVE_MAP_01"
FIRE_TEXTURE = "cache:shared_resourcecontainer/vfx_fire_ring_fb"
WIND_MAP = "PVE_MAP_01"
LAYER_BLENDS = {"opaque", "cutout", "alpha", "additive"}


def close(a, b, places=4):
    return all(abs(x - y) < 10**-places for x, y in zip(a, b))


def view_half_height(scene, height):
    camera = scene["camera"]
    return (camera["position"][1] - height) * math.tan(math.radians(camera["fov"]) / 2.0)


def corners(flat):
    return [flat[i : i + 3] for i in range(0, len(flat), 3)]


class CampaignScenesTest(unittest.TestCase):
    def test_every_campaign_map_has_its_prefab_scene_in_order(self) -> None:
        self.assertEqual([scene["map"] for scene in SCENES], [chapter["id"] for chapter in MAPS])
        for scene, chapter in zip(SCENES, MAPS):
            with self.subTest(map=chapter["id"]):
                self.assertEqual(scene["prefab"], build_campaign_node_layout.map_prefab(chapter["num"]))
                self.assertEqual(len(scene["nodes"]), chapter["node_count"])

    def test_every_map_camera_is_the_original_top_down_map_camera(self) -> None:
        for scene in SCENES:
            with self.subTest(map=scene["map"]):
                self.assertTrue(close(scene["camera"]["forward"], DOWN))
                self.assertTrue(close(scene["camera"]["up"], SCREEN_UP))
                self.assertEqual(scene["camera"]["fov"], MAP_CAMERA_FOV)

    def test_a_node_centred_by_the_camera_sits_inside_the_view(self) -> None:
        for scene in SCENES:
            camera_z = scene["camera"]["position"][2]
            for step, node in enumerate(scene["nodes"], start=1):
                with self.subTest(map=scene["map"], step=step):
                    x, height, z = node["position"]
                    self.assertLess(abs(z - camera_z), view_half_height(scene, height))

    def test_node_parts_keep_the_energy_banner_below_the_icon(self) -> None:
        for scene in SCENES:
            for step, node in enumerate(scene["nodes"], start=1):
                with self.subTest(map=scene["map"], step=step):
                    icon = corners(node["icon"])
                    banner = corners(node["banner"])
                    flask = corners(node["flask"])
                    self.assertEqual(len(icon), 4)
                    self.assertGreater(min(c[2] for c in icon), max(c[2] for c in banner) - 0.5)
                    self.assertLess(sum(c[2] for c in banner) / 4, sum(c[2] for c in icon) / 4)
                    self.assertGreater(min(c[0] for c in flask), min(c[0] for c in banner))
                    self.assertLess(max(c[0] for c in flask), max(c[0] for c in banner))

    def test_both_end_caps_frame_every_map(self) -> None:
        for scene in SCENES:
            with self.subTest(map=scene["map"]):
                caps = [layer for layer in scene["layers"] if layer["name"].startswith(END_CAP_PREFIX)]
                self.assertEqual(len(caps), END_CAPS_PER_MAP)
                xs = [sum(layer["positions"][0::3]) / len(layer["positions"][0::3]) for layer in caps]
                nodes = [node["position"][0] for node in scene["nodes"]]
                self.assertLess(min(xs), min(nodes))
                self.assertGreater(max(xs), max(nodes))

    def test_layers_are_well_formed_triangle_lists(self) -> None:
        for scene in SCENES:
            for layer in scene["layers"]:
                with self.subTest(map=scene["map"], layer=layer["name"]):
                    vertices = len(layer["positions"]) // 3
                    self.assertEqual(len(layer["positions"]), vertices * 3)
                    self.assertEqual(len(layer["uvs"]), vertices * 2)
                    self.assertEqual(len(layer["indices"]) % 3, 0)
                    self.assertTrue(all(0 <= index < vertices for index in layer["indices"]))
                    self.assertIn(layer["blend"], LAYER_BLENDS)

    def test_wind_meshes_carry_colours_normals_and_a_placement(self) -> None:
        for scene in SCENES:
            for wind in scene["winds"]:
                with self.subTest(map=scene["map"], wind=wind["name"]):
                    vertices = len(wind["positions"]) // 3
                    self.assertEqual(len(wind["normals"]), vertices * 3)
                    self.assertEqual(len(wind["colors"]), vertices * 4)
                    self.assertTrue(all(0.0 <= value <= 1.0 for value in wind["colors"]))
                    self.assertEqual(len(wind["matrix"]), 16)
        self.assertTrue(next(scene for scene in SCENES if scene["map"] == WIND_MAP)["winds"])

    def test_map_one_burns_with_doubled_additive_fire(self) -> None:
        scene = next(scene for scene in SCENES if scene["map"] == FIRE_MAP)
        fires = [emitter for emitter in scene["emitters"] if emitter["texture"] == FIRE_TEXTURE]
        self.assertTrue(fires)
        for fire in fires:
            self.assertEqual(fire["blend"], "additive")
            self.assertGreater(fire["tint"][0], 1.0)

    def test_non_uniform_scale_is_baked_into_the_mesh(self) -> None:
        matrix = map_prefab.trs((1.0, 2.0, 3.0), (0.0, 0.0, 0.0, 1.0), (2.0, 2.0, -3.0))
        model, baked = build_campaign_scenes.renderer_placement(matrix, (0.0, 0.0, 0.0, 1.0))
        self.assertTrue(close(map_prefab.transform_point(model, (0.0, 0.0, 0.0)), (1.0, 2.0, 3.0)))
        self.assertTrue(close(map_prefab.transform_point(baked, (1.0, 1.0, 1.0)), (2.0, 2.0, -3.0)))
        uniform = map_prefab.trs((1.0, 2.0, 3.0), (0.0, 0.0, 0.0, 1.0), (2.0, 2.0, 2.0))
        self.assertEqual(build_campaign_scenes.renderer_placement(uniform, (0.0, 0.0, 0.0, 1.0))[0], uniform)


if __name__ == "__main__":
    unittest.main()
