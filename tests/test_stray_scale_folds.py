from __future__ import annotations

import copy
from pathlib import Path
import struct
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "client-additional-data" / "anim_tools"))

import glb_writer


CENTIMETRE_FOLD = 0.01
ALAMARRI_FOOT_SCALE = (1.0, 1.0, 1.46)
LEG_NAMES = ("Origin", "Root", "Pelvis", "RightUpperLeg", "RightLowerLeg", "RightFoot")
LEG_PARENTS = (-1, 0, 1, 2, 3, 4)
SKIN_JOINTS = LEG_NAMES[1:]


def scale_of(value):
    x, y, z = value if isinstance(value, tuple) else (value, value, value)
    return {"x": x, "y": y, "z": z, "w": 1.0}


def leg_avatar(scales: dict, parents=LEG_PARENTS) -> tuple[dict, dict]:
    hash_to_name = {index + 1: name for index, name in enumerate(LEG_NAMES)}
    poses = [
        {
            "t": {"x": 0.0, "y": 0.0, "z": 0.0},
            "q": {"x": 0.0, "y": 0.0, "z": 0.0, "w": 1.0},
            "s": scale_of(scales.get(name, 1.0)),
        }
        for name in LEG_NAMES
    ]
    avatar = {
        "m_Avatar": {
            "m_AvatarSkeleton": {"data": {
                "m_Node": [{"m_ParentId": parent} for parent in parents],
                "m_ID": list(hash_to_name),
            }},
            "m_DefaultPose": {"data": {"m_X": poses}},
        }
    }
    return avatar, hash_to_name


def bound_skin(bind_scales: dict) -> tuple[dict, bytearray]:
    blob = bytearray()
    for name in SKIN_JOINTS:
        inverse = 1.0 / bind_scales.get(name, 1.0)
        blob += struct.pack("<16f", inverse, 0, 0, 0, 0, inverse, 0, 0, 0, 0, inverse, 0, 0, 0, 0, 1)
    gltf = {
        "nodes": [{"name": name} for name in SKIN_JOINTS],
        "skins": [{"joints": list(range(len(SKIN_JOINTS))), "inverseBindMatrices": 0}],
        "bufferViews": [{"buffer": 0, "byteOffset": 0, "byteLength": len(blob)}],
        "accessors": [{"bufferView": 0, "componentType": 5126, "count": len(SKIN_JOINTS), "type": "MAT4"}],
    }
    return gltf, blob


def default_scales(avatar: dict) -> dict:
    poses = avatar["m_Avatar"]["m_DefaultPose"]["data"]["m_X"]
    return {name: pose["s"]["x"] for name, pose in zip(LEG_NAMES, poses)}


class StrayScaleFoldTest(unittest.TestCase):
    def test_joint_bind_scales_come_out_of_the_inverse_bind_matrices(self) -> None:
        gltf, blob = bound_skin({"RightFoot": 0.5})
        scales = glb_writer.bind_world_scales(gltf, blob)
        self.assertAlmostEqual(scales["RightFoot"], 0.5, places=6)
        self.assertAlmostEqual(scales["Pelvis"], 1.0, places=6)

    def test_a_second_centimetre_fold_the_mesh_was_not_bound_with_is_dropped(self) -> None:
        avatar, hash_to_name = leg_avatar({"Root": CENTIMETRE_FOLD, "RightLowerLeg": CENTIMETRE_FOLD})
        fixed, unfolded = glb_writer.drop_stray_scale_folds(
            avatar, hash_to_name, glb_writer.bind_world_scales(*bound_skin({})))
        self.assertEqual(list(unfolded), ["RightLowerLeg"])
        self.assertAlmostEqual(unfolded["RightLowerLeg"], 100.0)
        scales = default_scales(fixed)
        self.assertAlmostEqual(scales["RightLowerLeg"], 1.0)
        self.assertAlmostEqual(scales["Root"], CENTIMETRE_FOLD)

    def test_the_callers_avatar_is_left_as_it_was(self) -> None:
        avatar, hash_to_name = leg_avatar({"Root": CENTIMETRE_FOLD, "RightLowerLeg": CENTIMETRE_FOLD})
        original = copy.deepcopy(avatar)
        glb_writer.drop_stray_scale_folds(avatar, hash_to_name, glb_writer.bind_world_scales(*bound_skin({})))
        self.assertEqual(avatar, original)

    def test_a_scale_the_mesh_was_bound_with_is_kept(self) -> None:
        avatar, hash_to_name = leg_avatar({"Root": CENTIMETRE_FOLD, "RightLowerLeg": CENTIMETRE_FOLD})
        bound = bound_skin({"RightLowerLeg": CENTIMETRE_FOLD, "RightFoot": CENTIMETRE_FOLD})
        fixed, unfolded = glb_writer.drop_stray_scale_folds(
            avatar, hash_to_name, glb_writer.bind_world_scales(*bound))
        self.assertEqual(unfolded, {})
        self.assertIs(fixed, avatar)

    def test_the_skeleton_units_fold_is_kept_even_under_a_bound_joint(self) -> None:
        avatar, hash_to_name = leg_avatar({"Pelvis": CENTIMETRE_FOLD})
        fixed, unfolded = glb_writer.drop_stray_scale_folds(
            avatar, hash_to_name, glb_writer.bind_world_scales(*bound_skin({})))
        self.assertEqual(unfolded, {})
        self.assertAlmostEqual(default_scales(fixed)["Pelvis"], CENTIMETRE_FOLD)

    def test_a_non_uniform_leaf_scale_is_left_for_the_bind_residual(self) -> None:
        avatar, hash_to_name = leg_avatar({"Root": CENTIMETRE_FOLD, "RightFoot": ALAMARRI_FOOT_SCALE})
        _, unfolded = glb_writer.drop_stray_scale_folds(
            avatar, hash_to_name, glb_writer.bind_world_scales(*bound_skin({})))
        self.assertEqual(unfolded, {})


if __name__ == "__main__":
    unittest.main()
