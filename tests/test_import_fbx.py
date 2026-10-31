from __future__ import annotations

import json
import math
from pathlib import Path
import struct
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
ANIM_TOOLS = ROOT / "client-additional-data" / "anim_tools"
sys.path.insert(0, str(ANIM_TOOLS))

import donor_bake
import glb_writer
import import_fbx
from hoda_anim import Xform


def animation_stack_fbx(name: str) -> bytes:
    header = b"Kaydara FBX Binary  \x00\x1a\x00" + struct.pack("<I", 7300)
    object_name = f"AnimStack::{name}\0\x01AnimStack".encode()
    properties = (
        b"L" + struct.pack("<q", 1)
        + b"S" + struct.pack("<I", len(object_name)) + object_name
        + b"S" + struct.pack("<I", 0)
    )
    node_name = b"AnimationStack"
    start = len(header)
    end = start + 13 + len(node_name) + len(properties) + 13
    node = (
        struct.pack("<IIIB", end, 3, len(properties), len(node_name))
        + node_name
        + properties
        + bytes(13)
    )
    return header + node + bytes(13)


class ImportFbxTest(unittest.TestCase):
    def test_bind_unit_correction_detects_meter_fbx_with_cm_donor(self) -> None:
        class AvatarStub:
            node_name = ["Root", "Hips", "Spine", "Chest"]
            default_local = [
                Xform(), Xform((0, 0.5, 0)), Xform((0, 0.3, 0)), Xform((0, 0.25, 0))
            ]
            acc_scale = [0.01] * 4

        gltf = {
            "skins": [{"joints": [0, 1, 2, 3]}],
            "nodes": [
                {"name": "Root"},
                {"name": "Hips", "translation": [0, 0.5, 0]},
                {"name": "Spine", "translation": [0, 0.3, 0]},
                {"name": "Chest", "translation": [0, 0.25, 0]},
            ],
        }
        self.assertEqual(glb_writer.infer_bind_unit_correction(gltf, AvatarStub()), 100.0)

    def test_bind_unit_correction_keeps_native_cm_source(self) -> None:
        class AvatarStub:
            node_name = ["Root", "Hips", "Spine", "Chest"]
            default_local = [
                Xform(), Xform((0, 0.5, 0)), Xform((0, 0.3, 0)), Xform((0, 0.25, 0))
            ]
            acc_scale = [0.01] * 4

        gltf = {
            "skins": [{"joints": [0, 1, 2, 3]}],
            "nodes": [
                {"name": "Root"},
                {"name": "Hips", "translation": [0, 50, 0]},
                {"name": "Spine", "translation": [0, 30, 0]},
                {"name": "Chest", "translation": [0, 25, 0]},
            ],
        }
        self.assertEqual(glb_writer.infer_bind_unit_correction(gltf, AvatarStub()), 1.0)

    def test_malformed_fourth_skin_weight_is_reconstructed(self) -> None:
        repaired = glb_writer.repair_float_skin_weights([
            (13 / 31, 7 / 31, 4 / 31, -23.0),
            (0.25, 0.25, 0.25, 0.25),
        ])
        self.assertAlmostEqual(repaired[0][3], 7 / 31)
        self.assertAlmostEqual(sum(repaired[0]), 1.0)
        self.assertEqual(repaired[1], (0.25, 0.25, 0.25, 0.25))

    def test_root_helper_cube_is_removed_without_touching_skinned_mesh(self) -> None:
        gltf = {
            "accessors": [
                {"count": 24, "min": [-0.03] * 3, "max": [0.03] * 3},
                {"count": 100, "min": [-1] * 3, "max": [1] * 3},
            ],
            "meshes": [
                {"primitives": [{"attributes": {"POSITION": 0, "NORMAL": 0}}]},
                {"primitives": [{"attributes": {"POSITION": 1, "JOINTS_0": 1}}]},
            ],
            "nodes": [
                {"name": "HumanFemale_Root_0", "mesh": 0},
                {"name": "body", "mesh": 1, "skin": 0},
            ],
        }
        self.assertEqual(glb_writer.strip_root_helper_meshes(gltf), 1)
        self.assertNotIn("mesh", gltf["nodes"][0])
        self.assertEqual(gltf["nodes"][1]["mesh"], 0)
        self.assertEqual(len(gltf["meshes"]), 1)

    def test_rebuild_keeps_meter_source_volume_with_cm_donor_avatar(self) -> None:
        class AvatarStub:
            node_name = ["Root", "Hips", "Spine", "Chest"]
            node_parent = [-1, 0, 1, 2]
            default_local = [
                Xform(), Xform((0, 0.5, 0)), Xform((0, 0.3, 0)), Xform((0, 0.25, 0))
            ]
            acc_scale = [0.01] * 4
            residual_scale = [(1, 1, 1)] * 4

            def avatar_worlds(self, overrides=None):
                locals_ = list(self.default_local)
                for index, value in (overrides or {}).items():
                    locals_[index] = value
                worlds = []
                for index, local in enumerate(locals_):
                    parent = self.node_parent[index]
                    worlds.append(local if parent < 0 else worlds[parent].mul(local))
                return worlds

        positions = [
            (-0.4, 0.2, -0.3), (0.4, 0.2, -0.3), (-0.4, 0.8, -0.3), (0.4, 0.8, -0.3),
            (-0.4, 0.2, 0.3), (0.4, 0.2, 0.3), (-0.4, 0.8, 0.3), (0.4, 0.8, 0.3),
        ]
        normals = [(0.0, 1.0, 0.0)] * len(positions)
        joints = [(1, 0, 0, 0)] * len(positions)
        weights = [(1.0, 0.0, 0.0, 0.0)] * len(positions)
        world_y = (0.0, 0.5, 0.8, 1.05)
        inverse_binds = []
        for y in world_y:
            matrix = [1.0, 0.0, 0.0, 0.0,
                      0.0, 1.0, 0.0, 0.0,
                      0.0, 0.0, 1.0, 0.0,
                      0.0, -y, 0.0, 1.0]
            inverse_binds.append(tuple(matrix))

        blob = bytearray()
        views = []
        accessors = []

        def add_accessor(rows, fmt, gltf_type, component, minimum=None, maximum=None):
            while len(blob) % 4:
                blob.append(0)
            offset = len(blob)
            for row in rows:
                blob.extend(struct.pack("<" + fmt * len(row), *row))
            views.append({"buffer": 0, "byteOffset": offset, "byteLength": len(blob) - offset})
            accessor = {
                "bufferView": len(views) - 1,
                "componentType": component,
                "count": len(rows),
                "type": gltf_type,
            }
            if minimum is not None:
                accessor["min"] = minimum
                accessor["max"] = maximum
            accessors.append(accessor)
            return len(accessors) - 1

        pos = add_accessor(positions, "f", "VEC3", 5126, [-0.4, 0.2, -0.3], [0.4, 0.8, 0.3])
        nrm = add_accessor(normals, "f", "VEC3", 5126)
        jnt = add_accessor(joints, "H", "VEC4", 5123)
        wgt = add_accessor(weights, "f", "VEC4", 5126)
        ibm = add_accessor(inverse_binds, "f", "MAT4", 5126)
        gltf = {
            "asset": {"version": "2.0"},
            "buffers": [{"byteLength": len(blob)}],
            "bufferViews": views,
            "accessors": accessors,
            "meshes": [{"primitives": [{"attributes": {
                "POSITION": pos, "NORMAL": nrm, "JOINTS_0": jnt, "WEIGHTS_0": wgt,
            }}]}],
            "nodes": [
                {"name": "Root", "children": [1]},
                {"name": "Hips", "translation": [0, 0.5, 0], "children": [2]},
                {"name": "Spine", "translation": [0, 0.3, 0], "children": [3]},
                {"name": "Chest", "translation": [0, 0.25, 0]},
                {"name": "Body", "mesh": 0, "skin": 0},
            ],
            "skins": [{"joints": [0, 1, 2, 3], "inverseBindMatrices": ibm}],
            "scenes": [{"nodes": [0, 4]}],
            "scene": 0,
        }

        avatar = AvatarStub()
        frame = {name: avatar.default_local[index] for index, name in enumerate(avatar.node_name)}
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary) / "source.glb"
            output = Path(temporary) / "output.glb"
            glb_writer.save_glb(source, gltf, blob)
            glb_writer.rebuild_glb(source, output, avatar, [frame, frame], 60)
            rebuilt, rebuilt_blob = glb_writer.load_glb(output)
            attribute = rebuilt["meshes"][0]["primitives"][0]["attributes"]["POSITION"]
            rows = glb_writer.read_accessor(rebuilt, rebuilt_blob, attribute)
            extents = [max(row[axis] for row in rows) - min(row[axis] for row in rows) for axis in range(3)]
            self.assertGreater(extents[0], 0.79)
            self.assertGreater(extents[2], 0.59)
            self.assertEqual(avatar.last_bind_unit_correction, 100.0)

    def test_animation_stack_names_drop_fbx_class_tag(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "take.fbx"
            path.write_bytes(animation_stack_fbx("sw_ready_ani"))
            self.assertEqual(import_fbx.animation_stacks(path), ["sw_ready_ani"])

    def test_matrix_decomposition_round_trips_negative_nonuniform_scale(self) -> None:
        angle = math.radians(37)
        original = import_fbx._compose_trs(
            [3.0, -2.0, 7.0],
            [0.0, 0.0, math.sin(angle / 2), math.cos(angle / 2)],
            [2.0, -3.0, 4.0],
        )
        translation, rotation, scale = import_fbx.matrix_to_trs(original)
        rebuilt = import_fbx._compose_trs(translation, rotation, scale)
        self.assertLess(max(abs(left - right) for left, right in zip(original, rebuilt)), 1e-12)

    def test_matrix_decomposition_rejects_shear(self) -> None:
        matrix = [
            1.0, 0.0, 0.0, 0.0,
            0.1, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0,
        ]
        with self.assertRaisesRegex(ValueError, "shear"):
            import_fbx.matrix_to_trs(matrix)

    def test_assimp_normalization_converts_trs_and_removes_grey_tint(self) -> None:
        gltf = {
            "nodes": [{"matrix": import_fbx._compose_trs([1, 2, 3], [0, 0, 0, 1], [2, 2, 2])}],
            "materials": [{
                "pbrMetallicRoughness": {"baseColorFactor": [0.5, 0.5, 0.5, 0.75]},
                "extensions": {"KHR_materials_specular": {"specularFactor": 1}},
            }],
            "extensionsUsed": ["KHR_materials_specular"],
        }
        result = import_fbx.normalize_assimp_gltf(gltf)
        self.assertEqual(result, {"matrices_converted": 1, "materials_whitened": 1})
        self.assertNotIn("matrix", gltf["nodes"][0])
        self.assertEqual(gltf["nodes"][0]["translation"], [1.0, 2.0, 3.0])
        self.assertEqual(
            gltf["materials"][0]["pbrMetallicRoughness"]["baseColorFactor"],
            [1.0, 1.0, 1.0, 0.75],
        )
        self.assertNotIn("extensions", gltf["materials"][0])
        self.assertNotIn("extensionsUsed", gltf)

    def test_donor_clip_discovery_merges_sidecars_and_take_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            unit = root / "hf_sw_example"
            unit.mkdir()
            sidecar = unit / "anim_sw_ready_ani_7.json"
            sidecar.write_text("{}")
            (unit / "hf_sw_example_t1_fbx_takes.json").write_text(json.dumps({
                "version": 1,
                "animation_stacks": ["sw_ready_ani", "sw_attack01_ani"],
            }))
            previous = donor_bake.CHAR_PACK
            donor_bake.CHAR_PACK = str(root)
            try:
                clips = donor_bake.sidecar_clips("hf_sw_example", "hf_sw_example_t1")
            finally:
                donor_bake.CHAR_PACK = previous
            self.assertEqual(clips["sw_ready_ani"], str(sidecar))
            self.assertIsNone(clips["sw_attack01_ani"])

    def test_special_mappings_are_explicit(self) -> None:
        ice = import_fbx.SPECIAL_MANIFESTS["c_icegiant_t1"]
        self.assertEqual(ice["clip_roles"]["idle"], "c_ogre_rdy_ani")
        self.assertEqual(ice["clip_roles"]["attack"], "c_ogre_atk_v03_ani")

        titan = import_fbx.SPECIAL_MANIFESTS["c_titanheartguardian_t1"]
        self.assertEqual(
            titan["embedded_clip_roles"], ["idle", "cheer", "attack", "hit", "death"]
        )
        self.assertEqual(titan["embedded_durations"], [5.0, 2.2, 4.6667, 2.0, 2.0])

        xenon = import_fbx.SPECIAL_MANIFESTS["hm_nc_xenontheantiquarian_t1"]
        self.assertEqual(xenon["duplicate_embedded_clips"], {"hit": "idle"})
        self.assertEqual(xenon["intentionally_absent_roles"], ["attack", "death"])


if __name__ == "__main__":
    unittest.main()
