#!/usr/bin/env python3

from pathlib import Path
import re

import numpy as np
import UnityPy
from UnityPy.helpers import UnityVersion as UV
from UnityPy.helpers.MeshHelper import MeshHandler


ROOT = Path(__file__).resolve().parents[2]
CACHE = ROOT / "heroes-of-dragons-cache"
SHARED = (
    CACHE
    / "Heroes-of-Dragon-Age-v4-4-0-cache-androidp1"
    / "com.ea.game.dragonage_row"
    / "files/UnityCache/Shared"
)
OUT = ROOT / "assets/legendary_base"
BUNDLES = {
    1: SHARED / "e2846eb8bf9efa9529d25fe92a2695b6499c0fcb/CAB-charbase_t5_s1_pre",
    2: SHARED / "c258037195b866237574f23f52717a51d5a5b650/CAB-charbase_t5_s2_pre",
    3: SHARED / "1da8a87d9b0669a4b8e935614c88b26579e5f970/CAB-charbase_t5_s3_pre",
}
STAR_SPACING_SCALE = {1: 1.0, 2: 0.75, 3: 0.55}


_original_version_parser = UV.UnityVersion.from_str.__func__


def _parse_version(cls, version):
    match = re.search(r"\d+\.\d+\.\d+[a-zA-Z]+\d+", version)
    return _original_version_parser(cls, match.group(0) if match else version)


UV.UnityVersion.from_str = classmethod(_parse_version)


def _vec(value, size):
    if hasattr(value, "x"):
        value = [getattr(value, axis) for axis in "xyzw"[:size]]
    return np.array(list(value)[:size], dtype=np.float64)


def _trs(transform):
    p = _vec(transform.m_LocalPosition, 3)
    q = _vec(transform.m_LocalRotation, 4)
    s = _vec(transform.m_LocalScale, 3)
    x, y, z, w = q
    rotation = np.array(
        [
            [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
            [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
            [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)],
        ],
        dtype=np.float64,
    )
    matrix = np.eye(4, dtype=np.float64)
    matrix[:3, :3] = rotation @ np.diag(s)
    matrix[:3, 3] = p
    return matrix


def _read_bundle(path):
    env = UnityPy.load(str(path))
    mesh_obj = next(
        obj for obj in env.objects if obj.type.name == "Mesh" and obj.read().m_Name == "pCylinder2"
    )
    mesh = MeshHandler(mesh_obj.read())
    mesh.process()

    transforms = [obj.read() for obj in env.objects if obj.type.name == "Transform"]
    stars = next(t for t in transforms if t.m_GameObject.read().m_Name == "Stars")
    children = [
        t
        for t in transforms
        if t.m_Father.path_id == stars.object_reader.path_id
        and t.m_GameObject.read().m_Name == "char_base_star2"
    ]
    children.sort(key=lambda t: tuple(_vec(t.m_LocalPosition, 3)))
    return env, mesh, stars, children


def _write_obj(path, mesh, stars, children, spacing_scale):
    vertices = np.array([_vec(v, 3) for v in mesh.m_Vertices], dtype=np.float64)
    normals = np.array([_vec(v, 3) for v in mesh.m_Normals], dtype=np.float64)
    uv = np.array([_vec(v, 2) for v in mesh.m_UV0], dtype=np.float64)
    triangles = np.asarray(mesh.get_triangles()[0], dtype=np.int64).reshape(-1, 3)

    lines = ["# Original HoDA char_base_star2 mesh, authored transforms baked", "o legendary_stars"]
    align_to_base = np.array(
        [
            [0.0, -1.0, 0.0, 0.0],
            [1.0, 0.0, 0.0, 0.0],
            [0.0, 0.0, 1.0, 0.0],
            [0.0, 0.0, 0.0, 1.0],
        ]
    )
    parent = align_to_base @ _trs(stars)
    baked = []
    for child in children:
        matrix = parent @ _trs(child)
        points = np.c_[vertices, np.ones(len(vertices))] @ matrix.T
        points[:, 0] *= -1

        normal_matrix = np.linalg.inv(matrix[:3, :3]).T
        dirs = normals @ normal_matrix.T
        dirs /= np.linalg.norm(dirs, axis=1, keepdims=True)
        dirs[:, 0] *= -1
        baked.append((points[:, :3], dirs))

    centers = [points.mean(axis=0) for points, _ in baked]
    for side in (-1, 1):
        group = [i for i, center in enumerate(centers) if (-1 if center[1] < 0 else 1) == side]
        group_x = sum(centers[i][0] for i in group) / len(group)
        for i in group:
            target_x = group_x + (centers[i][0] - group_x) * spacing_scale
            baked[i][0][:, 0] += target_x - centers[i][0]

    for points, _ in baked:
        lines.extend(f"v {p[0]:.9g} {p[1]:.9g} {p[2]:.9g}" for p in points)
    for _ in baked:
        lines.extend(f"vt {t[0]:.9g} {1.0 - t[1]:.9g}" for t in uv)
    for _, dirs in baked:
        lines.extend(f"vn {n[0]:.9g} {n[1]:.9g} {n[2]:.9g}" for n in dirs)

    vertex_count = len(vertices)
    for instance, _ in enumerate(baked):
        offset = instance * vertex_count
        for a, b, c in triangles:
            indices = (int(a) + 1 + offset, int(c) + 1 + offset, int(b) + 1 + offset)
            lines.append("f " + " ".join(f"{i}/{i}/{i}" for i in indices))

    path.write_text("\n".join(lines) + "\n")


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    for rating, bundle in BUNDLES.items():
        env, mesh, stars, children = _read_bundle(bundle)
        assert len(children) == rating * 2
        _write_obj(
            OUT / f"stars_{rating}.obj",
            mesh,
            stars,
            children,
            STAR_SPACING_SCALE[rating],
        )
        if rating == 3:
            texture = next(
                obj.read()
                for obj in env.objects
                if obj.type.name == "Texture2D" and obj.read().m_Name == "base_gold_star"
            )
            texture.image.save(OUT / "base_gold_star.png")
        print(f"stars_{rating}.obj: {len(children)} instances")


if __name__ == "__main__":
    main()
