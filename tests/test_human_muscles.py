from __future__ import annotations

from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "client-additional-data" / "anim_tools"))

import human_muscles


SPINE_FRONT_BACK = 0.1
CHEST_FRONT_BACK = -0.56
SPINE_LEFT_RIGHT = 0.05
CHEST_TWIST = 0.2
UNMAPPED = -1
WITH_CHEST = tuple(range(24))
WITHOUT_CHEST = tuple(UNMAPPED if slot == human_muscles.CHEST_SLOT else slot for slot in range(24))


def clip_frame():
    spine_fb, spine_lr, spine_twist = human_muscles.SPINE_MUSCLE_ATTRS
    chest_fb, _, chest_twist = human_muscles.CHEST_MUSCLE_ATTRS
    return {
        spine_fb: SPINE_FRONT_BACK,
        spine_lr: SPINE_LEFT_RIGHT,
        chest_fb: CHEST_FRONT_BACK,
        chest_twist: CHEST_TWIST,
    }


class MusclesForRigTest(unittest.TestCase):
    def test_rig_with_a_chest_keeps_spine_and_chest_apart(self) -> None:
        dof = clip_frame()
        self.assertEqual(human_muscles.muscles_for_rig(dof, WITH_CHEST), dof)

    def test_rig_without_a_chest_bends_its_spine_by_the_chest_muscles_too(self) -> None:
        folded = human_muscles.muscles_for_rig(clip_frame(), WITHOUT_CHEST)
        spine_fb, spine_lr, spine_twist = human_muscles.SPINE_MUSCLE_ATTRS
        self.assertAlmostEqual(folded[spine_fb], SPINE_FRONT_BACK + CHEST_FRONT_BACK)
        self.assertAlmostEqual(folded[spine_lr], SPINE_LEFT_RIGHT)
        self.assertAlmostEqual(folded[spine_twist], CHEST_TWIST)

    def test_folding_leaves_the_clip_frame_untouched(self) -> None:
        dof = clip_frame()
        human_muscles.muscles_for_rig(dof, WITHOUT_CHEST)
        self.assertEqual(dof, clip_frame())


if __name__ == "__main__":
    unittest.main()
