from __future__ import annotations

from pathlib import Path
import struct
import sys
from types import SimpleNamespace
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "client-additional-data" / "anim_tools"))

import audit_character_models as audit
import glb_writer


SPINE, NECK, HEAD = "HumanMale_Spine2", "HumanMale_Neck", "HumanMale_Head"
STRAY_NECK = "HumanMale_"
NECK_LENGTH = 0.267
HEAD_OFFSET = 0.124


def human_avatar() -> SimpleNamespace:
    return SimpleNamespace(node_name=[SPINE, NECK, HEAD], node_parent=[-1, 0, 1])


def skeleton(names: list[str], children: dict[str, list[str]], joints: list[str]) -> dict:
    index = {name: position for position, name in enumerate(names)}
    nodes = []
    for name in names:
        node = {"name": name}
        if children.get(name):
            node["children"] = [index[child] for child in children[name]]
        nodes.append(node)
    return {"nodes": nodes, "skins": [{"joints": [index[name] for name in joints]}]}


def children_of(gltf: dict, name: str) -> list[str]:
    nodes = gltf["nodes"]
    holder = next(node for node in nodes if node["name"] == name)
    return [nodes[child]["name"] for child in holder.get("children", [])]


class AvatarParentTest(unittest.TestCase):
    def test_head_behind_a_stray_neck_node_moves_under_the_avatar_neck(self) -> None:
        gltf = skeleton(
            [SPINE, STRAY_NECK, NECK, HEAD],
            {SPINE: [STRAY_NECK, NECK], STRAY_NECK: [HEAD]},
            [SPINE, NECK, HEAD],
        )
        self.assertEqual(glb_writer.hang_joints_from_avatar_parents(gltf, human_avatar()), [HEAD])
        self.assertEqual(children_of(gltf, NECK), [HEAD])
        self.assertEqual(children_of(gltf, STRAY_NECK), [])

    def test_head_behind_a_plain_twin_the_avatar_also_names_moves_under_its_avatar_parent(self) -> None:
        plain, jointed = "QunariMale_Neck", "QunariMale_Neck 1"
        avatar = SimpleNamespace(node_name=[SPINE, plain, jointed, HEAD], node_parent=[-1, 0, 0, 2])
        gltf = skeleton(
            [SPINE, plain, jointed, HEAD], {SPINE: [plain, jointed], plain: [HEAD]}, [SPINE, jointed, HEAD]
        )
        self.assertEqual(glb_writer.hang_joints_from_avatar_parents(gltf, avatar), [HEAD])
        self.assertEqual(children_of(gltf, jointed), [HEAD])

    def test_joints_already_under_their_avatar_parent_stay(self) -> None:
        gltf = skeleton([SPINE, NECK, HEAD], {SPINE: [NECK], NECK: [HEAD]}, [SPINE, NECK, HEAD])
        self.assertEqual(glb_writer.hang_joints_from_avatar_parents(gltf, human_avatar()), [])
        self.assertEqual(children_of(gltf, NECK), [HEAD])

    def test_joint_under_its_plain_avatar_parent_stays(self) -> None:
        gltf = skeleton([SPINE, NECK, HEAD], {SPINE: [NECK], NECK: [HEAD]}, [SPINE, HEAD])
        self.assertEqual(glb_writer.hang_joints_from_avatar_parents(gltf, human_avatar()), [])
        self.assertEqual(children_of(gltf, NECK), [HEAD])


def translation_matrix(y: float) -> list[float]:
    return [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, y, 0.0, 1.0]


def bound_skeleton(head_holder_offset: float) -> tuple[dict, memoryview]:
    neck_y = 1.3 + NECK_LENGTH
    inverse_binds = [translation_matrix(-1.3), translation_matrix(-neck_y), translation_matrix(-(neck_y + HEAD_OFFSET))]
    blob = memoryview(b"".join(struct.pack("<16f", *matrix) for matrix in inverse_binds))
    gltf = {
        "nodes": [
            {"name": SPINE, "translation": [0.0, 1.3, 0.0], "children": [1, 2]},
            {"name": NECK, "translation": [0.0, NECK_LENGTH, 0.0]},
            {"name": STRAY_NECK, "translation": [0.0, head_holder_offset, 0.0], "children": [3]},
            {"name": HEAD, "translation": [0.0, HEAD_OFFSET, 0.0]},
        ],
        "skins": [{"joints": [0, 1, 3], "inverseBindMatrices": 0}],
        "accessors": [{"bufferView": 0, "componentType": 5126, "count": 3, "type": "MAT4"}],
        "bufferViews": [{"buffer": 0, "byteLength": len(blob)}],
    }
    return gltf, blob


class BindOffsetAuditTest(unittest.TestCase):
    def test_head_whose_nodes_skip_the_neck_offset_is_reported(self) -> None:
        gltf, blob = bound_skeleton(0.0)
        drifts = audit.drifting_bind_offsets(gltf, blob)
        self.assertEqual(drifts, [f"{HEAD} {NECK_LENGTH:.3f} m from {SPINE}"])

    def test_nodes_that_match_the_inverse_binds_pass(self) -> None:
        gltf, blob = bound_skeleton(NECK_LENGTH)
        self.assertEqual(audit.drifting_bind_offsets(gltf, blob), [])


if __name__ == "__main__":
    unittest.main()
