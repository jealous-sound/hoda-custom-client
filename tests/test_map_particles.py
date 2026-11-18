from __future__ import annotations

import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import build_campaign_scenes
import build_nexus_scenes
import map_particles

SCENE_FILES = (build_campaign_scenes.OUTPUT, build_nexus_scenes.OUTPUT)
PARTICLE_BLENDS = {"alpha", "additive", map_particles.SOFT_ADDITIVE}
SHAPES = {"point", "box", "cone", "sphere", "points"}
GRADIENT_KEYS = ("gradient", "gradient_low", "start_gradient")
RUNTIME_MAX_EMITTERS = 64
RUNTIME_MAX_TEXTURES = 16
RUNTIME_MAX_CURVE_KEYS = 17
RUNTIME_MAX_GRADIENT_KEYS = 8


def scenes():
    for path in SCENE_FILES:
        for scene in json.loads(path.read_text(encoding="utf-8"))["scenes"]:
            yield path.name, scene


class MapParticlesTest(unittest.TestCase):
    def test_every_scene_fits_the_runtime_emitter_tables(self) -> None:
        for file_name, scene in scenes():
            with self.subTest(file=file_name, prefab=scene["prefab"]):
                self.assertLessEqual(len(scene["emitters"]), RUNTIME_MAX_EMITTERS)
                textures = {emitter["texture"] for emitter in scene["emitters"]}
                self.assertLessEqual(len(textures), RUNTIME_MAX_TEXTURES)

    def test_emitters_are_well_formed(self) -> None:
        for file_name, scene in scenes():
            for emitter in scene["emitters"]:
                with self.subTest(file=file_name, prefab=scene["prefab"], emitter=emitter["name"]):
                    self.assertIn(emitter["blend"], PARTICLE_BLENDS)
                    self.assertIn(emitter["render"], set(map_particles.RENDER_MODES.values()))
                    self.assertIn(emitter["shape"]["type"], SHAPES)
                    self.assertEqual(len(emitter["axes"]), 9)
                    self.assertEqual(len(emitter["shape_axes"]), 9)
                    self.assertEqual(len(emitter["colors"]), 2)
                    self.assertGreater(emitter["rate"], 0.0)
                    self.assertGreater(emitter["lifetime"][1], 0.0)
                    for key in ("spin", "size_curve"):
                        if key in emitter:
                            self.assert_curve(emitter[key])
                    for key in ("velocity", "force"):
                        for curve in emitter.get(key, []):
                            self.assert_curve(curve)
                    if "sheet" in emitter:
                        self.assert_curve(emitter["sheet"]["frame"])
                    for key in GRADIENT_KEYS:
                        gradient = emitter.get(key, {"color": [], "alpha": []})
                        self.assertLessEqual(len(gradient["color"]), RUNTIME_MAX_GRADIENT_KEYS)
                        self.assertLessEqual(len(gradient["alpha"]), RUNTIME_MAX_GRADIENT_KEYS)
                    if "gradient_low" in emitter:
                        self.assertIn("gradient", emitter)

    def assert_curve(self, curve) -> None:
        self.assertTrue(1 <= len(curve) <= RUNTIME_MAX_CURVE_KEYS)
        times = [key[0] for key in curve]
        self.assertEqual(times, sorted(times))
        self.assertTrue(all(len(key) == 3 for key in curve))

    def test_hermite_curves_follow_their_tangents(self) -> None:
        keys = [
            {"time": 0.0, "value": 0.0, "inSlope": 0.0, "outSlope": 0.0},
            {"time": 1.0, "value": 1.0, "inSlope": 0.0, "outSlope": 0.0},
        ]
        self.assertAlmostEqual(map_particles.hermite(keys, 0.5), 0.5)
        self.assertAlmostEqual(map_particles.hermite(keys, 0.25), 0.15625)
        self.assertEqual(map_particles.hermite(keys, -1.0), 0.0)
        self.assertEqual(map_particles.hermite(keys, 2.0), 1.0)

    def test_two_constants_become_a_random_range(self) -> None:
        node = {
            "minMaxState": map_particles.CURVE_TWO_CONSTANTS,
            "scalar": 2.0,
            "minCurve": {"m_Curve": [{"time": 0.0, "value": 0.25, "inSlope": 0.0, "outSlope": 0.0}]},
            "maxCurve": {"m_Curve": [{"time": 0.0, "value": 1.0, "inSlope": 0.0, "outSlope": 0.0}]},
        }
        self.assertEqual(map_particles.node_curve(node), [[0.0, 0.5, 2.0]])
        self.assertEqual(map_particles.constant_range(node), [0.5, 2.0])

    def test_packed_colours_unpack_red_first(self) -> None:
        self.assertEqual(map_particles.unpack_rgba({"rgba": 0x80FF0000}), (0.0, 0.0, 1.0, 128 / 255))

    def test_a_start_colour_gradient_starts_white_for_the_runtime_to_tint(self) -> None:
        node = {"minMaxState": map_particles.GRADIENT_GRADIENT}
        self.assertEqual(map_particles.start_colors(node), [[1.0, 1.0, 1.0, 1.0], [1.0, 1.0, 1.0, 1.0]])


if __name__ == "__main__":
    unittest.main()
