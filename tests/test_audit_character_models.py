from __future__ import annotations

from pathlib import Path
import struct
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import audit_character_models as audit


TRIANGLES = (0, 1, 2, 0, 2, 3, 0, 3, 1, 1, 3, 2)
TETRAHEDRON = ((0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0))


def skinned_tetrahedron(positions):
    blob = bytearray()
    for position in positions:
        blob += struct.pack("<3f", *position)
    index_offset = len(blob)
    blob += struct.pack(f"<{len(TRIANGLES)}H", *TRIANGLES)
    gltf = {
        "meshes": [{
            "name": "body",
            "primitives": [{"attributes": {"POSITION": 0, "JOINTS_0": 0}, "indices": 1}],
        }],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": index_offset},
            {"buffer": 0, "byteOffset": index_offset, "byteLength": len(blob) - index_offset},
        ],
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": len(positions), "type": "VEC3"},
            {"bufferView": 1, "componentType": 5123, "count": len(TRIANGLES), "type": "SCALAR"},
        ],
    }
    return gltf, memoryview(bytes(blob))


class TornSkinAuditTest(unittest.TestCase):
    def test_a_uniformly_rescaled_bake_keeps_every_edge(self) -> None:
        source, source_blob = skinned_tetrahedron(TETRAHEDRON)
        output, output_blob = skinned_tetrahedron([tuple(c * 0.01 for c in p) for p in TETRAHEDRON])
        self.assertEqual(audit.torn_edge_share(source, source_blob, output, output_blob), 0.0)

    def test_a_vertex_flung_away_by_the_bake_tears_its_edges(self) -> None:
        source, source_blob = skinned_tetrahedron(TETRAHEDRON)
        flung = list(TETRAHEDRON)
        flung[3] = (0.0, 0.0, 9.0)
        output, output_blob = skinned_tetrahedron(flung)
        share = audit.torn_edge_share(source, source_blob, output, output_blob)
        self.assertGreater(share, audit.TORN_SKIN_EDGE_SHARE)

    def test_a_mesh_without_a_matching_source_is_not_judged(self) -> None:
        source, source_blob = skinned_tetrahedron(TETRAHEDRON)
        source["meshes"][0]["name"] = "other"
        output, output_blob = skinned_tetrahedron(TETRAHEDRON)
        self.assertEqual(audit.torn_edge_share(source, source_blob, output, output_blob), 0.0)


if __name__ == "__main__":
    unittest.main()
