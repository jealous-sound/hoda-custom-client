from __future__ import annotations

from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "client-additional-data" / "anim_tools"))

import audit_character_models as audit
import prefab_scaffold
from prefab_scaffold import PrefabNode


PREFAB_SCALE = 1.2885
WRAPPER_Y = -0.2676
HIPS_Y = 1.145
KETOJAN_HANDS_VFX_SCALE = 0.05954
KETOJAN_BAKED_HEIGHT = 0.211
NUG_BAKED_HEIGHT = 0.796


class Prefab:
    def __init__(self) -> None:
        self.nodes: dict[int, PrefabNode] = {}
        self.bones: set[int] = set()

    def add(self, name: str, parent: int = prefab_scaffold.NO_PARENT, scale: float = 1.0, y: float = 0.0,
            bone: bool = False) -> int:
        pid = len(self.nodes) + 1
        self.nodes[pid] = PrefabNode(name=name, scale=scale, y=y, parent=parent, children=[])
        if parent != prefab_scaffold.NO_PARENT:
            self.nodes[parent].children.append(pid)
        if bone:
            self.bones.add(pid)
        return pid

    def unit_root(self, name: str) -> int:
        root = self.add(f"unit.{name}_pre")
        for marker in ("base", "fac", "role"):
            self.add(f"{marker}_loc_marker", root, y=-0.39)
        return root

    def arm(self, parent: int, prefix: str, scale: float = 1.0) -> int:
        bone = self.add(f"{prefix}_Root", parent, scale=scale, y=HIPS_Y, bone=True)
        for name in ("Pelvis", "Spine1", "Spine2", "RightCollarbone", "RightUpperArm", "RightLowerArm", "RightHand"):
            bone = self.add(f"{prefix}_{name}", bone, bone=True)
        return bone

    def scaffolding(self):
        return prefab_scaffold.display_scaffolding(self.nodes, self.bones)


def meter_rig_with_hand_vfx() -> Prefab:
    prefab = Prefab()
    root = prefab.unit_root("qm_ketojan_t1")
    wrapper = prefab.add("qm_ketojan_t1_fbx", root, scale=PREFAB_SCALE, y=WRAPPER_Y)
    prefab.add("qm_ketojan_t1_mesh", wrapper)
    hand = prefab.arm(wrapper, "QunariMale")
    hands_fx = prefab.add("vfx.fx_Ketojanhands_pre", hand, scale=KETOJAN_HANDS_VFX_SCALE)
    prefab.add("glow", hands_fx, scale=0.67)
    return prefab


def cm_folded_rig() -> Prefab:
    prefab = Prefab()
    root = prefab.unit_root("qm_mw_saarebas_t1")
    wrapper = prefab.add("qm_mw_saarebas_t1_fbx", root, scale=PREFAB_SCALE, y=WRAPPER_Y)
    prefab.add("qm_mw_saarebas_tier_1", wrapper, scale=prefab_scaffold.CM_UNIT_SCALE)
    prefab.arm(wrapper, "QunariMale", scale=prefab_scaffold.CM_UNIT_SCALE)
    return prefab


class DisplayScaffoldingTest(unittest.TestCase):
    def test_meter_rig_keeps_its_wrapper_scale_despite_a_small_vfx_under_a_hand(self) -> None:
        self.assertEqual(
            meter_rig_with_hand_vfx().scaffolding(),
            (PREFAB_SCALE, WRAPPER_Y, prefab_scaffold.METER_RIG_NOTE),
        )

    def test_cm_folded_skeleton_scales_back_to_the_wrapper_scale(self) -> None:
        scale, y, note = cm_folded_rig().scaffolding()
        self.assertAlmostEqual(scale, PREFAB_SCALE, places=6)
        self.assertAlmostEqual(y, WRAPPER_Y, places=6)
        self.assertEqual(note, "")

    def test_small_scaled_node_without_skinned_bones_is_not_the_skeleton(self) -> None:
        prefab = cm_folded_rig()
        wrapper = next(pid for pid, node in prefab.nodes.items() if node.name.endswith("_fbx"))
        decoy = prefab.add("group", wrapper, scale=KETOJAN_HANDS_VFX_SCALE)
        for index in range(20):
            prefab.add(f"prop{index}", decoy)
        scale, _, note = prefab.scaffolding()
        self.assertAlmostEqual(scale, PREFAB_SCALE, places=6)
        self.assertEqual(note, "")

    def test_prefab_without_a_unit_root_has_no_scaffolding(self) -> None:
        prefab = Prefab()
        prefab.add("qm_ketojan_t1_fbx")
        self.assertEqual(prefab.scaffolding()[:2], (None, None))


class ShrunkModelAuditTest(unittest.TestCase):
    def test_a_model_folded_down_a_decade_is_shrunk(self) -> None:
        self.assertTrue(audit.shrunk_model((0.138, KETOJAN_BAKED_HEIGHT, 0.044)))

    def test_the_smallest_real_creature_is_not_shrunk(self) -> None:
        self.assertFalse(audit.shrunk_model((0.5, NUG_BAKED_HEIGHT, 0.9)))

    def test_a_model_without_skinned_bounds_is_left_to_the_validity_check(self) -> None:
        self.assertFalse(audit.shrunk_model(None))


if __name__ == "__main__":
    unittest.main()
