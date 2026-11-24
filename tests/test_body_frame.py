from __future__ import annotations

import math
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "client-additional-data" / "anim_tools"))

import body_frame


HUMAN_MALE_TPOSE_PITCH_DEGREES = 3.94
MAGE_IDLE_BODY_YAW_DEGREES = 46.8
HUMAN_MALE_SCALE = 1.073
STANDING_ROOT_T = (0.0, 0.958, 0.0)
LEFT_FOOT_GOAL = (-0.23, -0.96, -0.08)
RIGHT_FOOT_GOAL = (0.20, -0.96, -0.08)
HIPS_ABOVE_FEET = (0.0, 0.95, 0.0)


def axis_rotation(axis, degrees):
    half = math.radians(degrees) / 2
    return (axis[0] * math.sin(half), axis[1] * math.sin(half), axis[2] * math.sin(half), math.cos(half))


def tpose_pitch():
    return axis_rotation((-1.0, 0.0, 0.0), HUMAN_MALE_TPOSE_PITCH_DEGREES)


def body_yaw():
    return axis_rotation((0.0, 1.0, 0.0), MAGE_IDLE_BODY_YAW_DEGREES)


def clip_dof(root_q=body_frame.IDENTITY, feet=(LEFT_FOOT_GOAL, RIGHT_FOOT_GOAL)):
    dof = {}
    for i, value in enumerate(STANDING_ROOT_T):
        dof[body_frame.ROOT_T_FIRST + i] = value
    for i, value in enumerate(root_q):
        dof[body_frame.ROOT_Q_FIRST + i] = value
    for first, goal in zip(body_frame.FOOT_GOAL_T_FIRSTS, feet):
        for i, value in enumerate(goal):
            dof[first + i] = value
    return dof


def same_rotation(a, b):
    return abs(sum(x * y for x, y in zip(a, b)))


class BodyFrameTest(unittest.TestCase):
    def assertVectorAlmostEqual(self, actual, expected, places=6):
        for a, e in zip(actual, expected):
            self.assertAlmostEqual(a, e, places=places)

    def test_planted_feet_centre_is_the_mean_foot_goal_in_clip_space(self) -> None:
        centre = body_frame.planted_feet_centre(clip_dof(), HUMAN_MALE_SCALE)
        expected_x = (LEFT_FOOT_GOAL[0] + RIGHT_FOOT_GOAL[0]) / 2 * HUMAN_MALE_SCALE
        expected_y = (STANDING_ROOT_T[1] + LEFT_FOOT_GOAL[1]) * HUMAN_MALE_SCALE
        self.assertVectorAlmostEqual(centre, (expected_x, expected_y, LEFT_FOOT_GOAL[2] * HUMAN_MALE_SCALE))

    def test_planted_feet_centre_falls_back_to_the_body_centre_without_foot_goals(self) -> None:
        centre = body_frame.planted_feet_centre(clip_dof(feet=()), HUMAN_MALE_SCALE)
        self.assertVectorAlmostEqual(centre, tuple(c * HUMAN_MALE_SCALE for c in STANDING_ROOT_T))

    def test_identity_tpose_body_frame_leaves_the_solved_hips_alone(self) -> None:
        hips_q = axis_rotation((0.0, 0.0, 1.0), 30.0)
        hips_t, turned_q = body_frame.body_in_tpose_frame(
            HIPS_ABOVE_FEET, hips_q, clip_dof(body_yaw()), body_frame.IDENTITY, HUMAN_MALE_SCALE
        )
        self.assertVectorAlmostEqual(hips_t, HIPS_ABOVE_FEET)
        self.assertAlmostEqual(same_rotation(turned_q, hips_q), 1.0)

    def test_tpose_pitch_tips_the_body_back_about_the_planted_feet(self) -> None:
        dof = clip_dof()
        pivot = body_frame.planted_feet_centre(dof, HUMAN_MALE_SCALE)
        hips_t, hips_q = body_frame.body_in_tpose_frame(
            HIPS_ABOVE_FEET, body_frame.IDENTITY, dof, tpose_pitch(), HUMAN_MALE_SCALE
        )
        self.assertAlmostEqual(same_rotation(hips_q, tpose_pitch()), 1.0)
        before = math.dist(HIPS_ABOVE_FEET, pivot)
        self.assertAlmostEqual(math.dist(hips_t, pivot), before)
        self.assertLess(hips_t[2], HIPS_ABOVE_FEET[2])

    def test_tpose_pitch_turns_about_the_bodys_own_side_axis_when_the_clip_is_yawed(self) -> None:
        turn = body_frame.tpose_body_turn(clip_dof(body_yaw()), tpose_pitch())
        side_axis = body_frame.quat_rotate(body_yaw(), (-1.0, 0.0, 0.0))
        expected = axis_rotation(side_axis, HUMAN_MALE_TPOSE_PITCH_DEGREES)
        self.assertAlmostEqual(same_rotation(turn, expected), 1.0)
        self.assertLess(same_rotation(turn, tpose_pitch()), 1.0 - 1e-6)


if __name__ == "__main__":
    unittest.main()
