#!/usr/bin/env python3
import argparse
import copy
import json
import math
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

import original_cache
from build_custom_heroes import ROOT, TIERS, load_specs, shipped_rows, tier_base

CLI_DESCRIPTION = (
    "Bake the runtime models, clips, textures and portraits of the custom heroes in data/custom_heroes/ "
    "from the baked models of the heroes they derive from."
)
CHAR_PACK = ROOT / "client-additional-data/char_pack"
PORTRAIT_DIR = ROOT / "client-additional-data/extra/portraits"
DISPLAY_SCALES = ROOT / "client-additional-data/anim_tools/display_scales.json"

GLB_HEADER = struct.Struct("<4sII")
GLB_CHUNK = struct.Struct("<I4s")
GLB_VERSION = 2
FLOAT = 5126
UNSIGNED_BYTE = 5121
UNSIGNED_SHORT = 5123
ARRAY_BUFFER = 34962
ELEMENT_ARRAY_BUFFER = 34963
COMPONENT_DTYPES = {
    5120: np.int8,
    5121: np.uint8,
    5122: np.int16,
    5123: np.uint16,
    5125: np.uint32,
    5126: np.float32,
}
TYPE_WIDTHS = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}
IDENTITY_ROTATION = np.array([0.0, 0.0, 0.0, 1.0])

CLIP_FPS = 30.0
FIRST_CLIP = "idle"
SEAM_DILATION_STEPS = 3
PORTRAIT_SUPERSAMPLE = 4
FRAME_PEER_HEROES = 10
FRAME_STD_LIMIT = 12.0
FRAME_EDGE_STD_LIMIT = 36.0
FRAME_EDGE_REACH = 2
PORTRAIT_FOV_DEGREES = 26.0
UP = np.array([0.0, 1.0, 0.0])


def load_glb(path):
    data = path.read_bytes()
    (json_length,) = struct.unpack_from("<I", data, GLB_HEADER.size)
    json_start = GLB_HEADER.size + GLB_CHUNK.size
    gltf = json.loads(data[json_start : json_start + json_length])
    bin_header = json_start + json_length
    (bin_length,) = struct.unpack_from("<I", data, bin_header)
    bin_start = bin_header + GLB_CHUNK.size
    return gltf, data[bin_start : bin_start + bin_length]


def save_glb(path, gltf, blob):
    text = json.dumps(gltf, separators=(",", ":")).encode()
    text += b" " * (-len(text) % 4)
    blob = bytes(blob) + b"\0" * (-len(blob) % 4)
    total = GLB_HEADER.size + 2 * GLB_CHUNK.size + len(text) + len(blob)
    path.write_bytes(
        GLB_HEADER.pack(b"glTF", GLB_VERSION, total)
        + GLB_CHUNK.pack(len(text), b"JSON")
        + text
        + GLB_CHUNK.pack(len(blob), b"BIN\0")
        + blob
    )


def read_accessor(gltf, blob, index):
    accessor = gltf["accessors"][index]
    view = gltf["bufferViews"][accessor["bufferView"]]
    dtype = np.dtype(COMPONENT_DTYPES[accessor["componentType"]])
    width = TYPE_WIDTHS[accessor["type"]]
    stride = view.get("byteStride") or dtype.itemsize * width
    start = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
    return np.ndarray((accessor["count"], width), dtype, blob, start, (stride, dtype.itemsize)).copy()


def read_float_accessor(gltf, blob, index):
    accessor = gltf["accessors"][index]
    values = read_accessor(gltf, blob, index).astype(np.float64)
    if accessor.get("normalized"):
        values /= np.iinfo(COMPONENT_DTYPES[accessor["componentType"]]).max
    return values


class BufferBuilder:
    def __init__(self):
        self.blob = bytearray()
        self.views = []
        self.accessors = []

    def add(self, array, component_type, kind, target=None, normalized=False, bounds=False):
        width = TYPE_WIDTHS[kind]
        array = np.ascontiguousarray(np.asarray(array).reshape(-1, width), COMPONENT_DTYPES[component_type])
        self.blob += b"\0" * (-len(self.blob) % 4)
        view = {"buffer": 0, "byteOffset": len(self.blob), "byteLength": array.nbytes}
        if target:
            view["target"] = target
        self.blob += array.tobytes()
        self.views.append(view)
        accessor = {
            "bufferView": len(self.views) - 1,
            "componentType": component_type,
            "count": len(array),
            "type": kind,
        }
        if normalized:
            accessor["normalized"] = True
        if bounds:
            accessor["min"] = array.min(axis=0).tolist()
            accessor["max"] = array.max(axis=0).tolist()
        self.accessors.append(accessor)
        return len(self.accessors) - 1


def qmul(a, b):
    ax, ay, az, aw = np.moveaxis(np.asarray(a, np.float64), -1, 0)
    bx, by, bz, bw = np.moveaxis(np.asarray(b, np.float64), -1, 0)
    return np.stack(
        (
            aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz,
        ),
        -1,
    )


def qconj(q):
    return np.asarray(q, np.float64) * np.array([-1.0, -1.0, -1.0, 1.0])


def qnormalize(q):
    return q / np.linalg.norm(q, axis=-1, keepdims=True)


def qaxis(axis, degrees):
    axis = np.asarray(axis, np.float64)
    axis = axis / np.linalg.norm(axis)
    half = np.radians(np.asarray(degrees, np.float64))[..., None] / 2.0
    return np.concatenate((np.sin(half) * axis, np.cos(half)), -1)


def qrotate(q, v):
    v = np.asarray(v, np.float64)
    q = np.broadcast_to(np.asarray(q, np.float64), v.shape[:-1] + (4,))
    padded = np.concatenate((v, np.zeros(v.shape[:-1] + (1,))), -1)
    return qmul(qmul(q, padded), qconj(q))[..., :3]


def qmatrix(q):
    x, y, z, w = np.moveaxis(np.asarray(q, np.float64), -1, 0)
    return np.stack(
        (
            np.stack((1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)), -1),
            np.stack((2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)), -1),
            np.stack((2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)), -1),
        ),
        -2,
    )


def qfrom_matrix(m):
    trace = m[0, 0] + m[1, 1] + m[2, 2]
    if trace > 0:
        s = math.sqrt(trace + 1.0) * 2
        q = ((m[2, 1] - m[1, 2]) / s, (m[0, 2] - m[2, 0]) / s, (m[1, 0] - m[0, 1]) / s, 0.25 * s)
    elif m[0, 0] > m[1, 1] and m[0, 0] > m[2, 2]:
        s = math.sqrt(1.0 + m[0, 0] - m[1, 1] - m[2, 2]) * 2
        q = (0.25 * s, (m[0, 1] + m[1, 0]) / s, (m[0, 2] + m[2, 0]) / s, (m[2, 1] - m[1, 2]) / s)
    elif m[1, 1] > m[2, 2]:
        s = math.sqrt(1.0 + m[1, 1] - m[0, 0] - m[2, 2]) * 2
        q = ((m[0, 1] + m[1, 0]) / s, 0.25 * s, (m[1, 2] + m[2, 1]) / s, (m[0, 2] - m[2, 0]) / s)
    else:
        s = math.sqrt(1.0 + m[2, 2] - m[0, 0] - m[1, 1]) * 2
        q = ((m[0, 2] + m[2, 0]) / s, (m[1, 2] + m[2, 1]) / s, 0.25 * s, (m[1, 0] - m[0, 1]) / s)
    return qnormalize(np.array(q))


def trs_matrix(t, r, s):
    m = np.zeros(np.shape(t)[:-1] + (4, 4))
    m[..., :3, :3] = qmatrix(r) * np.asarray(s)[..., None, :]
    m[..., :3, 3] = t
    m[..., 3, 3] = 1.0
    return m


def rotation_between(a, b):
    a = np.asarray(a, np.float64) / np.linalg.norm(a)
    b = np.asarray(b, np.float64) / np.linalg.norm(b)
    axis = np.cross(a, b)
    if np.linalg.norm(axis) > 1e-9:
        return qaxis(axis, math.degrees(math.acos(np.clip(np.dot(a, b), -1.0, 1.0))))
    if np.dot(a, b) > 0:
        return IDENTITY_ROTATION.copy()
    helper = np.array([1.0, 0.0, 0.0]) if abs(a[0]) < 0.9 else np.array([0.0, 0.0, 1.0])
    return qaxis(np.cross(a, helper), 180.0)


def node_trs(node):
    if "matrix" in node:
        raise SystemExit(f"node {node.get('name')!r} uses a matrix transform")
    return (
        np.array(node.get("translation", (0.0, 0.0, 0.0)), np.float64),
        np.array(node.get("rotation", IDENTITY_ROTATION), np.float64),
        np.array(node.get("scale", (1.0, 1.0, 1.0)), np.float64),
    )


class Rig:
    def __init__(self, gltf, blob):
        skin = gltf["skins"][0]
        nodes = gltf["nodes"]
        self.joints = list(skin["joints"])
        self.names = [nodes[node].get("name", "") for node in self.joints]
        self.node_parent = {child: index for index, node in enumerate(nodes) for child in node.get("children", [])}
        slot = {node: index for index, node in enumerate(self.joints)}
        self.parents = [slot.get(self.node_parent.get(node), -1) for node in self.joints]
        self.order = []
        while len(self.order) < len(self.joints):
            for index, parent in enumerate(self.parents):
                if index not in self.order and (parent < 0 or parent in self.order):
                    self.order.append(index)
        rest = [node_trs(nodes[node]) for node in self.joints]
        self.rest_t = np.array([trs[0] for trs in rest])
        self.rest_r = np.array([trs[1] for trs in rest])
        self.rest_s = np.array([trs[2] for trs in rest])
        self.anchor = np.array([self.ancestor_matrix(nodes, node) for node in self.joints])
        self.anchor_rotation = np.array([qfrom_matrix(m[:3, :3] / np.linalg.norm(m[:3, :3], axis=0)) for m in self.anchor])
        matrices = read_accessor(gltf, blob, skin["inverseBindMatrices"]).astype(np.float64)
        self.inverse_bind = matrices.reshape(-1, 4, 4).transpose(0, 2, 1)
        self.bind = np.linalg.inv(self.inverse_bind)

    def ancestor_matrix(self, nodes, node):
        matrix = np.eye(4)
        parent = self.node_parent.get(node)
        while parent is not None:
            matrix = trs_matrix(*node_trs(nodes[parent])) @ matrix
            parent = self.node_parent.get(parent)
        return matrix

    def index(self, name):
        if name not in self.names:
            raise SystemExit(f"bone {name!r} is not in the skeleton {self.names}")
        return self.names.index(name)

    def world_rotations(self, rotations):
        world = np.empty_like(rotations)
        for joint in self.order:
            parent = self.parents[joint]
            above = world[:, parent] if parent >= 0 else self.anchor_rotation[joint]
            world[:, joint] = qmul(above, rotations[:, joint])
        return world

    def parent_rotations(self, rotations):
        world = self.world_rotations(rotations)
        above = np.empty_like(rotations)
        for joint, parent in enumerate(self.parents):
            above[:, joint] = world[:, parent] if parent >= 0 else self.anchor_rotation[joint]
        return above

    def world_matrices(self, t, r, s):
        local = trs_matrix(t, r, s)
        world = np.empty_like(local)
        for joint in self.order:
            parent = self.parents[joint]
            above = world[parent] if parent >= 0 else self.anchor[joint]
            world[joint] = above @ local[joint]
        return world


@dataclass
class Clip:
    times: np.ndarray
    t: np.ndarray
    r: np.ndarray
    s: np.ndarray


def resample(keys, values, times, rotation):
    if len(keys) == len(times) and np.allclose(keys, times):
        return values
    index = np.clip(np.searchsorted(keys, times, side="right") - 1, 0, max(len(keys) - 2, 0))
    if len(keys) == 1:
        return np.repeat(values[:1], len(times), 0)
    alpha = np.clip((times - keys[index]) / (keys[index + 1] - keys[index]), 0.0, 1.0)[:, None]
    start, end = values[index], values[index + 1]
    if rotation:
        end = np.where(np.sum(start * end, -1, keepdims=True) < 0, -end, end)
        return qnormalize(start + (end - start) * alpha)
    return start + (end - start) * alpha


def read_clip(gltf, blob, rig, name):
    animation = next((a for a in gltf.get("animations", []) if a.get("name") == name), None)
    if animation is None:
        raise SystemExit(f"clip {name!r} is missing")
    samplers = animation["samplers"]
    times = read_float_accessor(gltf, blob, samplers[animation["channels"][0]["sampler"]]["input"])[:, 0]
    frames = len(times)
    clip = Clip(
        times,
        np.repeat(rig.rest_t[None], frames, 0),
        np.repeat(rig.rest_r[None], frames, 0),
        np.repeat(rig.rest_s[None], frames, 0),
    )
    slot = {node: index for index, node in enumerate(rig.joints)}
    for channel in animation["channels"]:
        joint = slot.get(channel["target"]["node"])
        if joint is None:
            continue
        sampler = samplers[channel["sampler"]]
        if sampler.get("interpolation", "LINEAR") == "CUBICSPLINE":
            raise SystemExit(f"clip {name!r} uses cubic-spline keys")
        keys = read_float_accessor(gltf, blob, sampler["input"])[:, 0]
        values = read_float_accessor(gltf, blob, sampler["output"])
        path = channel["target"]["path"]
        target = {"translation": clip.t, "rotation": clip.r, "scale": clip.s}[path]
        target[:, joint] = resample(keys, values, times, path == "rotation")
    return clip


def sample_clip(clip, at):
    t = np.stack([resample(clip.times, clip.t[:, j], at, False) for j in range(clip.t.shape[1])], 1)
    r = np.stack([resample(clip.times, clip.r[:, j], at, True) for j in range(clip.r.shape[1])], 1)
    s = np.stack([resample(clip.times, clip.s[:, j], at, False) for j in range(clip.s.shape[1])], 1)
    return t, r, s


def retarget(clip, donor, rig):
    t = np.repeat(rig.rest_t[None], len(clip.times), 0)
    r = np.repeat(rig.rest_r[None], len(clip.times), 0)
    s = np.repeat(rig.rest_s[None], len(clip.times), 0)
    for joint, name in enumerate(rig.names):
        if name not in donor.names:
            continue
        source = donor.names.index(name)
        r[:, joint] = clip.r[:, source]
        s[:, joint] = clip.s[:, source]
        if rig.parents[joint] < 0:
            t[:, joint] = clip.t[:, source]
    return Clip(clip.times, t, r, s)


def smoothstep(x):
    x = np.clip(x, 0.0, 1.0)
    return x * x * (3.0 - 2.0 * x)


def keyed(u, keys):
    if not keys:
        return np.zeros_like(u)
    keys = sorted(keys)
    stamps = np.array([key[0] for key in keys], np.float64)
    values = np.array([key[1] for key in keys], np.float64)
    out = np.full_like(u, values[0])
    for index in range(len(keys) - 1):
        span = max(stamps[index + 1] - stamps[index], 1e-9)
        blend = smoothstep((u - stamps[index]) / span)
        inside = (u >= stamps[index]) & (u <= stamps[index + 1])
        out = np.where(inside, values[index] + (values[index + 1] - values[index]) * blend, out)
    return np.where(u > stamps[-1], values[-1], out)


def layer_amount(u, layer):
    amount = keyed(u, layer.get("keys"))
    wave = layer.get("wave")
    if wave:
        amount = amount + wave["amplitude"] * np.sin(2 * np.pi * (wave.get("cycles", 1) * u + wave.get("phase", 0.0)))
    return amount


def warp_time(u, keys):
    if not keys:
        return u
    keys = sorted(keys)
    return np.interp(u, [key[0] for key in keys], [key[1] for key in keys])


def continuous_rotations(r):
    for frame in range(1, len(r)):
        flip = np.sum(r[frame] * r[frame - 1], -1) < 0
        r[frame][flip] *= -1
    return r


class ClipSources:
    def __init__(self, rig, own_gltf, own_blob):
        self.rig = rig
        self.cache = {None: (own_gltf, own_blob, rig)}

    def clip(self, source, name):
        if source not in self.cache:
            gltf, blob = load_glb(runtime_model_path(source))
            self.cache[source] = (gltf, blob, Rig(gltf, blob))
        gltf, blob, donor = self.cache[source]
        clip = read_clip(gltf, blob, donor, name)
        return clip if donor is self.rig else retarget(clip, donor, self.rig)


def body_heading(rig, rotations):
    root = rig.order[0]
    bind_rotation = qfrom_matrix(rig.bind[root][:3, :3] / np.linalg.norm(rig.bind[root][:3, :3], axis=0))
    relative = qmul(rig.world_rotations(rotations)[:, root], qconj(bind_rotation))
    forward = qrotate(relative, np.broadcast_to([0.0, 0.0, 1.0], relative.shape[:-1] + (3,)))
    return math.degrees(math.atan2(forward[:, 0].mean(), forward[:, 2].mean()))


def author_clip(recipe, rig, sources):
    clip = sources.clip(recipe.get("source"), recipe["clip"])
    duration = float(clip.times[-1])
    start, end = recipe.get("window", (0.0, 1.0))
    length = duration * (end - start) / recipe.get("speed", 1.0)
    count = max(2, int(round(length * CLIP_FPS)) + 1)
    u = np.linspace(0.0, 1.0, count)
    at = (start + warp_time(u, recipe.get("time_keys")) * (end - start)) * duration
    t, r, s = sample_clip(clip, at)
    above = rig.parent_rotations(r)
    heading = qaxis(UP, body_heading(rig, r))
    for layer in recipe.get("layers", []):
        joint = rig.index(layer["bone"])
        amount = layer_amount(u, layer)
        if "rotate" in layer:
            offset = qaxis(qrotate(heading, layer["rotate"]), amount)
            local_offset = qmul(qmul(qconj(above[:, joint]), offset), above[:, joint])
            r[:, joint] = qmul(local_offset, r[:, joint])
        if "move" in layer:
            shift = qrotate(heading, layer["move"])[None] * amount[:, None]
            t[:, joint] += qrotate(qconj(above[:, joint]), shift)
    return Clip(u * length, t, continuous_rotations(qnormalize(r)), s)


@dataclass
class Halo:
    joint_node: int
    rest_t: np.ndarray
    rest_r: np.ndarray
    rest_s: np.ndarray
    spin: dict


def torus(radius, tube, segments, sides):
    ring = np.linspace(0.0, 2 * np.pi, segments + 1)
    around = np.linspace(0.0, 2 * np.pi, sides + 1)
    a, b = np.meshgrid(ring, around, indexing="ij")
    center = np.stack((np.cos(a) * radius, np.zeros_like(a), np.sin(a) * radius), -1)
    normal = np.stack((np.cos(a) * np.cos(b), np.sin(b), np.sin(a) * np.cos(b)), -1)
    positions = (center + normal * tube).reshape(-1, 3)
    uvs = np.stack((a / (2 * np.pi), b / (2 * np.pi)), -1).reshape(-1, 2)
    triangles = []
    stride = sides + 1
    for i in range(segments):
        for k in range(sides):
            p0, p1 = i * stride + k, (i + 1) * stride + k
            triangles += [(p0, p0 + 1, p1), (p1, p0 + 1, p1 + 1)]
    return positions, normal.reshape(-1, 3), uvs, np.array(triangles)


def shards(count, radius, length, width, phase):
    positions, normals, uvs, triangles = [], [], [], []
    for index in range(count):
        angle = 2 * np.pi * (index / count + phase)
        out = np.array([np.cos(angle), 0.0, np.sin(angle)])
        side = np.array([-np.sin(angle), 0.0, np.cos(angle)])
        tip = out * (radius + length)
        tail = out * (radius - length * 0.3)
        waist = [out * (radius + length * 0.35) + side * width, out * (radius + length * 0.35) + UP * width,
                 out * (radius + length * 0.35) - side * width, out * (radius + length * 0.35) - UP * width]
        for k in range(4):
            for apex, (p, q) in ((tip, (waist[k], waist[(k + 1) % 4])), (tail, (waist[(k + 1) % 4], waist[k]))):
                face = [apex, p, q]
                normal = np.cross(face[1] - face[0], face[2] - face[0])
                normal /= np.linalg.norm(normal)
                if np.dot(normal, face[0] - out * radius) < 0:
                    face = [apex, q, p]
                    normal = -normal
                base = len(positions)
                positions += face
                normals += [normal] * 3
                uvs += [(0.5, 0.5)] * 3
                triangles.append((base, base + 1, base + 2))
    return np.array(positions), np.array(normals), np.array(uvs), np.array(triangles)


def halo_texture(spec, size=(64, 128)):
    height, width = size
    u = np.linspace(0.0, 1.0, width)[None, :, None]
    v = np.linspace(0.0, 1.0, height)[:, None, None]
    core = np.array(spec["core"], np.float64) / 255.0
    edge = np.array(spec["edge"], np.float64) / 255.0
    pulse = 0.5 + 0.5 * np.cos(2 * np.pi * spec.get("pulses", 6) * u)
    glow = 0.5 + 0.5 * np.cos(2 * np.pi * v)
    mix = np.clip(0.35 + 0.45 * pulse + 0.2 * glow, 0.0, 1.0)
    rgb = edge + (core - edge) * mix
    return Image.fromarray((np.clip(rgb, 0, 1) * 255).astype(np.uint8), "RGB")


def add_halo(gltf, rig, spec, base, out_dir):
    head = rig.index(spec["bone"])
    head_bind = rig.bind[head]
    center = head_bind[:3, 3] + np.asarray(spec["offset"], np.float64)
    facing = rotation_between(UP, spec["normal"])
    placement = trs_matrix(center, facing, np.ones(3))
    local = np.linalg.inv(head_bind) @ placement
    scale = np.linalg.norm(local[:3, :3], axis=0)
    rest_r = qfrom_matrix(local[:3, :3] / scale)

    nodes = gltf["nodes"]
    joint_node = len(nodes)
    nodes.append({"name": spec.get("name", "VeilHalo"), "translation": local[:3, 3].tolist(),
                  "rotation": rest_r.tolist(), "scale": scale.tolist()})
    nodes[rig.joints[head]].setdefault("children", []).append(joint_node)
    gltf["skins"][0]["joints"].append(joint_node)

    ring = torus(spec["radius"], spec["tube"], spec.get("segments", 48), spec.get("sides", 8))
    crystal = shards(spec.get("shards", 0), spec["radius"], spec.get("shard_length", 0.05),
                     spec.get("shard_width", 0.012), spec.get("shard_phase", 0.0))
    parts = [ring] + ([crystal] if spec.get("shards") else [])
    positions = np.concatenate([p[0] for p in parts])
    normals = np.concatenate([p[1] for p in parts])
    uvs = np.concatenate([p[2] for p in parts])
    offsets = np.cumsum([0] + [len(p[0]) for p in parts])
    triangles = np.concatenate([p[3] + offsets[i] for i, p in enumerate(parts)])
    rotation = qmatrix(facing)
    mesh_positions = positions @ rotation.T + center
    mesh_normals = normals @ rotation.T

    texture_name = f"{base}_halo.png"
    texture = halo_texture(spec)
    texture.save(out_dir / texture_name)
    gltf["images"].append({"uri": texture_name})
    gltf["textures"].append({"source": len(gltf["images"]) - 1})
    gltf["materials"].append({
        "name": f"{base}_halo",
        "pbrMetallicRoughness": {"baseColorFactor": [1.0, 1.0, 1.0, 1.0], "metallicFactor": 0.0,
                                 "roughnessFactor": 1.0, "baseColorTexture": {"index": len(gltf["textures"]) - 1}},
        "alphaMode": "OPAQUE",
    })
    primitive = {
        "material": len(gltf["materials"]) - 1,
        "geometry": (mesh_positions, mesh_normals, uvs, triangles, len(gltf["skins"][0]["joints"]) - 1),
    }
    halo = Halo(joint_node, local[:3, 3], rest_r, scale, spec.get("spin", {}))
    return halo, primitive, np.linalg.inv(placement), np.asarray(texture, np.float64) / 255.0


def rgb_to_hsv(rgb):
    high = rgb.max(-1)
    low = rgb.min(-1)
    delta = high - low
    safe = np.where(delta > 0, delta, 1.0)
    r, g, b = np.moveaxis(rgb, -1, 0)
    hue = np.where(high == r, (g - b) / safe % 6, np.where(high == g, (b - r) / safe + 2, (r - g) / safe + 4))
    hue = np.where(delta > 0, hue * 60.0, 0.0)
    saturation = np.where(high > 0, delta / np.where(high > 0, high, 1.0), 0.0)
    return hue, saturation, high


def hsv_to_rgb(hue, saturation, value):
    sector = (hue % 360.0) / 60.0
    chroma = value * saturation
    x = chroma * (1 - np.abs(sector % 2 - 1))
    zero = np.zeros_like(hue)
    choices = [
        (chroma, x, zero), (x, chroma, zero), (zero, chroma, x),
        (zero, x, chroma), (x, zero, chroma), (chroma, zero, x),
    ]
    index = np.clip(sector.astype(int), 0, 5)
    rgb = np.zeros(hue.shape + (3,))
    for i, (r, g, b) in enumerate(choices):
        mask = index == i
        rgb[mask] = np.stack((r[mask], g[mask], b[mask]), -1)
    return rgb + (value - chroma)[..., None]


def hue_distance(hue, low, high):
    span = (high - low) % 360.0
    offset = (hue - low) % 360.0
    inside = offset <= span
    outside = np.minimum(np.abs(offset - span), 360.0 - offset)
    return np.where(inside, 0.0, outside)


@dataclass
class TexelMap:
    joint: np.ndarray
    position: np.ndarray
    normal: np.ndarray


def texel_map(size, primitives):
    height, width = size
    joint_map = np.full((height, width), -1, np.int32)
    position_map = np.zeros((height, width, 3))
    normal_map = np.zeros((height, width, 3))
    for positions, normals, uvs, triangles, joints, weights in primitives:
        dominant = joints[np.arange(len(joints)), np.argmax(weights, 1)]
        pixels = uvs * [width, height] - 0.5
        for triangle in triangles:
            corners = pixels[triangle]
            low = np.floor(corners.min(0)).astype(int)
            high = np.ceil(corners.max(0)).astype(int)
            low = np.maximum(low, 0)
            high = np.minimum(high, [width - 1, height - 1])
            if np.any(high < low):
                continue
            xs, ys = np.meshgrid(np.arange(low[0], high[0] + 1), np.arange(low[1], high[1] + 1))
            (x0, y0), (x1, y1), (x2, y2) = corners
            area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)
            if abs(area) < 1e-12:
                continue
            w1 = ((xs - x0) * (y2 - y0) - (x2 - x0) * (ys - y0)) / area
            w2 = ((x1 - x0) * (ys - y0) - (xs - x0) * (y1 - y0)) / area
            w0 = 1.0 - w1 - w2
            inside = (w0 >= -0.02) & (w1 >= -0.02) & (w2 >= -0.02)
            if not inside.any():
                continue
            bary = np.stack((w0[inside], w1[inside], w2[inside]), -1)
            px, py = xs[inside], ys[inside]
            position_map[py, px] = bary @ positions[triangle]
            normal_map[py, px] = bary @ normals[triangle]
            joint_map[py, px] = dominant[triangle][np.argmax(bary, 1)]
    for _ in range(SEAM_DILATION_STEPS):
        empty = joint_map < 0
        for dy, dx in ((0, 1), (0, -1), (1, 0), (-1, 0)):
            shifted_joint = np.roll(joint_map, (dy, dx), (0, 1))
            fill = empty & (shifted_joint >= 0) & (joint_map < 0)
            joint_map[fill] = shifted_joint[fill]
            position_map[fill] = np.roll(position_map, (dy, dx), (0, 1))[fill]
            normal_map[fill] = np.roll(normal_map, (dy, dx), (0, 1))[fill]
    length = np.linalg.norm(normal_map, axis=-1, keepdims=True)
    return TexelMap(joint_map, position_map, normal_map / np.where(length > 0, length, 1.0))


def bone_mask(texels, rig, names, default):
    if not names:
        return np.full(texels.joint.shape, default)
    return np.isin(texels.joint, [rig.index(name) for name in names])


def band_offset(hue, low, high):
    return (hue - low) % 360.0 - ((high - low) % 360.0) / 2


def apply_recolor(rgb, texels, rig, op):
    hue, saturation, value = rgb_to_hsv(rgb)
    weight = 1.0 - smoothstep(hue_distance(hue, *op["hue"]) / max(op.get("feather", 12.0), 1e-6))
    weight = weight * smoothstep((saturation - op.get("min_saturation", 0.0)) / 0.08 + 0.5)
    weight = weight * bone_mask(texels, rig, op.get("bones"), True)
    weight = weight * ~bone_mask(texels, rig, op.get("exclude_bones"), False)
    target = hsv_to_rgb(
        np.full_like(hue, op["to_hue"]) + band_offset(hue, *op["hue"]) * op.get("keep_hue", 0.0),
        np.clip(saturation * op.get("saturation", 1.0), 0.0, 1.0),
        np.clip(value * op.get("value", 1.0), 0.0, 1.0),
    )
    return rgb + (target - rgb) * weight[..., None]


def apply_gradient(rgb, texels, rig, op):
    height = texels.position @ np.asarray(op.get("axis", UP), np.float64)
    weight = np.clip((op["to"] - height) / (op["to"] - op["from"]), 0.0, 1.0) ** op.get("falloff", 1.5)
    weight = weight * op.get("strength", 1.0) * (texels.joint >= 0)
    weight = weight * ~bone_mask(texels, rig, op.get("exclude_bones"), False)
    color = np.asarray(op["color"], np.float64) / 255.0
    return rgb + (color - rgb) * weight[..., None]


def segment_distance(points, a, b):
    ab = b - a
    t = np.clip(((points - a) @ ab) / max(ab @ ab, 1e-12), 0.0, 1.0)
    return np.linalg.norm(points - (a + t[:, None] * ab), axis=-1)


def apply_strokes(rgb, texels, rig, op):
    joint = rig.index(op["bone"])
    origin = rig.bind[joint][:3, 3] + np.asarray(op.get("origin", (0.0, 0.0, 0.0)), np.float64)
    facing = np.asarray(op.get("facing", (0.0, 0.0, 1.0)), np.float64)
    across = np.cross(UP, facing)
    upward = np.cross(facing, across)
    selected = bone_mask(texels, rig, op.get("bones", [op["bone"]]), True)
    selected &= (texels.normal @ facing) > op.get("min_facing", 0.2)
    selected &= ((texels.position - origin) @ facing) > 0
    rows, cols = np.nonzero(selected)
    relative = texels.position[rows, cols] - origin
    points = np.stack((relative @ across, relative @ upward), -1)
    distance = np.full(len(points), np.inf)
    lines = [np.asarray(line, np.float64) for line in op["lines"]]
    if op.get("mirror"):
        lines += [line * [-1.0, 1.0] for line in lines]
    for line in lines:
        for a, b in zip(line[:-1], line[1:]):
            distance = np.minimum(distance, segment_distance(points, a, b))
    width = op["width"]
    glow = op.get("glow", width)
    alpha = np.clip(1.0 - (distance - width) / glow, 0.0, 1.0) * op.get("strength", 1.0)
    color = np.asarray(op["color"], np.float64) / 255.0
    out = rgb.copy()
    out[rows, cols] = rgb[rows, cols] + (color - rgb[rows, cols]) * alpha[:, None]
    return out


TEXTURE_OPS = {"recolor": apply_recolor, "gradient": apply_gradient, "strokes": apply_strokes}


def paint_texture(image, texels, rig, ops):
    rgba = np.asarray(image.convert("RGBA"), np.float64) / 255.0
    rgb = rgba[..., :3]
    for op in ops:
        rgb = TEXTURE_OPS[op["op"]](rgb, texels, rig, op)
    rgba[..., :3] = np.clip(rgb, 0.0, 1.0)
    return Image.fromarray((rgba * 255 + 0.5).astype(np.uint8), "RGBA")


def body_primitives(gltf, blob):
    out = []
    for mesh in gltf["meshes"]:
        for primitive in mesh["primitives"]:
            attributes = primitive["attributes"]
            if "JOINTS_0" not in attributes or "TEXCOORD_0" not in attributes:
                continue
            positions = read_float_accessor(gltf, blob, attributes["POSITION"])
            normals = read_float_accessor(gltf, blob, attributes["NORMAL"])
            uvs = read_float_accessor(gltf, blob, attributes["TEXCOORD_0"])
            joints = read_accessor(gltf, blob, attributes["JOINTS_0"]).astype(np.int64)
            weights = read_float_accessor(gltf, blob, attributes["WEIGHTS_0"])
            triangles = read_accessor(gltf, blob, primitive["indices"]).reshape(-1, 3).astype(np.int64)
            out.append((primitive.get("material", 0), positions, normals, uvs, triangles, joints, weights))
    return out


def runtime_model_path(base):
    directory = base.rsplit("_t", 1)[0] if base[-3:-1] == "_t" else base
    return CHAR_PACK / directory / f"{base}_rl.glb"


def rebuild_buffers(gltf, blob, extra_primitives, clips, rig, halo, halo_inverse_bind):
    builder = BufferBuilder()
    carried = {}

    def carry(index, target=None):
        if index not in carried:
            accessor = gltf["accessors"][index]
            carried[index] = builder.add(
                read_accessor(gltf, blob, index), accessor["componentType"], accessor["type"], target,
                accessor.get("normalized", False), "min" in accessor,
            )
        return carried[index]

    out = copy.deepcopy(gltf)
    for mesh in out["meshes"]:
        for primitive in mesh["primitives"]:
            primitive["attributes"] = {k: carry(v, ARRAY_BUFFER) for k, v in primitive["attributes"].items()}
            if "indices" in primitive:
                primitive["indices"] = carry(primitive["indices"], ELEMENT_ARRAY_BUFFER)
    for extra in extra_primitives:
        positions, normals, uvs, triangles, joint = extra["geometry"]
        joint_rows = np.zeros((len(positions), 4))
        joint_rows[:, 0] = joint
        weight_rows = np.zeros((len(positions), 4))
        weight_rows[:, 0] = 1.0
        joint_type = UNSIGNED_BYTE if joint < 256 else UNSIGNED_SHORT
        out["meshes"][0]["primitives"].append({
            "attributes": {
                "POSITION": builder.add(positions, FLOAT, "VEC3", ARRAY_BUFFER, bounds=True),
                "NORMAL": builder.add(normals, FLOAT, "VEC3", ARRAY_BUFFER),
                "TEXCOORD_0": builder.add(uvs, FLOAT, "VEC2", ARRAY_BUFFER),
                "JOINTS_0": builder.add(joint_rows, joint_type, "VEC4", ARRAY_BUFFER),
                "WEIGHTS_0": builder.add(weight_rows, FLOAT, "VEC4", ARRAY_BUFFER),
            },
            "indices": builder.add(triangles.reshape(-1), UNSIGNED_SHORT, "SCALAR", ELEMENT_ARRAY_BUFFER),
            "material": extra["material"],
            "mode": 4,
        })
    for skin in out["skins"]:
        matrices = read_accessor(gltf, blob, skin["inverseBindMatrices"]).reshape(-1, 4, 4)
        if halo is not None:
            matrices = np.concatenate((matrices, halo_inverse_bind.T[None]), 0)
        skin["inverseBindMatrices"] = builder.add(matrices.reshape(-1, 16), FLOAT, "MAT4")
    out["animations"] = []
    for name, clip in clips:
        animation = {"name": name, "channels": [], "samplers": []}
        times = builder.add(clip.times, FLOAT, "SCALAR", bounds=True)
        tracks = [(rig.joints[j], clip.t[:, j], clip.r[:, j], clip.s[:, j]) for j in range(len(rig.joints))]
        if halo is not None:
            spin = halo.spin.get(name, 0.0) * 360.0 * clip.times / max(clip.times[-1], 1e-9)
            spin_rotation = qmul(halo.rest_r, qaxis(UP, spin))
            frames = len(clip.times)
            tracks.append((halo.joint_node, np.repeat(halo.rest_t[None], frames, 0),
                           continuous_rotations(spin_rotation.copy()), np.repeat(halo.rest_s[None], frames, 0)))
        for node, t, r, s in tracks:
            for path, values, kind in (("translation", t, "VEC3"), ("rotation", r, "VEC4"), ("scale", s, "VEC3")):
                animation["samplers"].append(
                    {"input": times, "output": builder.add(values, FLOAT, kind), "interpolation": "LINEAR"}
                )
                animation["channels"].append(
                    {"sampler": len(animation["samplers"]) - 1, "target": {"node": node, "path": path}}
                )
        out["animations"].append(animation)
    for image in out.get("images", []):
        if "bufferView" in image:
            raise SystemExit("embedded images are not supported")
    out["accessors"] = builder.accessors
    out["bufferViews"] = builder.views
    out["buffers"] = [{"byteLength": len(builder.blob)}]
    return out, builder.blob


def rename(gltf, old, new):
    for group in ("nodes", "meshes", "materials"):
        for item in gltf.get(group, []):
            if "name" in item:
                item["name"] = item["name"].replace(old, new)


def skinned_pose(rig, clip, frame, primitives, extra_joint=None):
    world = rig.world_matrices(clip.t[frame], clip.r[frame], clip.s[frame])
    skins = world @ rig.inverse_bind
    if extra_joint is not None:
        skins = np.concatenate((skins, extra_joint[None]), 0)
    posed = []
    for positions, normals, uvs, triangles, joints, weights, texture in primitives:
        matrices = np.einsum("vk,vkij->vij", weights, skins[joints])
        p = np.einsum("vij,vj->vi", matrices[:, :3, :3], positions) + matrices[:, :3, 3]
        n = np.einsum("vij,vj->vi", matrices[:, :3, :3], normals)
        n /= np.maximum(np.linalg.norm(n, axis=-1, keepdims=True), 1e-9)
        posed.append((p, n, uvs, triangles, texture))
    return world, posed


def look_at(eye, target):
    forward = target - eye
    forward /= np.linalg.norm(forward)
    right = np.cross(forward, UP)
    right /= np.linalg.norm(right)
    up = np.cross(right, forward)
    return right, up, forward


def sample_texture(texture, uv):
    height, width = texture.shape[:2]
    x = np.clip(uv[:, 0] * width - 0.5, 0, width - 1)
    y = np.clip(uv[:, 1] * height - 0.5, 0, height - 1)
    x0, y0 = np.floor(x).astype(int), np.floor(y).astype(int)
    x1, y1 = np.minimum(x0 + 1, width - 1), np.minimum(y0 + 1, height - 1)
    fx, fy = (x - x0)[:, None], (y - y0)[:, None]
    top = texture[y0, x0] * (1 - fx) + texture[y0, x1] * fx
    bottom = texture[y1, x0] * (1 - fx) + texture[y1, x1] * fx
    return top * (1 - fy) + bottom * fy


def rasterize(posed, eye, target, size, light, rim):
    height, width = size
    right, up, forward = look_at(eye, target)
    focal = 1.0 / math.tan(math.radians(PORTRAIT_FOV_DEGREES) / 2)
    color = np.zeros((height, width, 3))
    depth = np.full((height, width), np.inf)
    light = np.asarray(light, np.float64) / np.linalg.norm(light)
    for positions, normals, uvs, triangles, texture in posed:
        relative = positions - eye
        z = relative @ forward
        sx = (relative @ right) / z * focal * height / 2 + width / 2
        sy = height / 2 - (relative @ up) / z * focal * height / 2
        for triangle in triangles:
            if np.any(z[triangle] < 0.05):
                continue
            (x0, x1, x2), (y0, y1, y2) = sx[triangle], sy[triangle]
            area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)
            if area >= 0:
                continue
            low_x, high_x = max(int(min(x0, x1, x2)), 0), min(int(max(x0, x1, x2)) + 1, width - 1)
            low_y, high_y = max(int(min(y0, y1, y2)), 0), min(int(max(y0, y1, y2)) + 1, height - 1)
            if high_x < low_x or high_y < low_y:
                continue
            xs, ys = np.meshgrid(np.arange(low_x, high_x + 1) + 0.5, np.arange(low_y, high_y + 1) + 0.5)
            w1 = ((xs - x0) * (y2 - y0) - (x2 - x0) * (ys - y0)) / area
            w2 = ((x1 - x0) * (ys - y0) - (xs - x0) * (y1 - y0)) / area
            w0 = 1.0 - w1 - w2
            inside = (w0 >= 0) & (w1 >= 0) & (w2 >= 0)
            if not inside.any():
                continue
            inverse_z = 1.0 / z[triangle]
            bary = np.stack((w0[inside], w1[inside], w2[inside]), -1) * inverse_z
            pixel_z = 1.0 / bary.sum(-1)
            bary *= pixel_z[:, None]
            px = (xs[inside] - 0.5).astype(int)
            py = (ys[inside] - 0.5).astype(int)
            nearer = pixel_z < depth[py, px]
            if not nearer.any():
                continue
            px, py, pixel_z, bary = px[nearer], py[nearer], pixel_z[nearer], bary[nearer]
            normal = bary @ normals[triangle]
            normal /= np.maximum(np.linalg.norm(normal, axis=-1, keepdims=True), 1e-9)
            albedo = sample_texture(texture, bary @ uvs[triangle])
            diffuse = np.clip(normal @ light, 0.0, 1.0)[:, None]
            facing = np.clip(-(normal @ forward), 0.0, 1.0)[:, None]
            shade = albedo * (0.62 + 0.5 * diffuse) + np.asarray(rim, np.float64) / 255.0 * (1 - facing) ** 3
            depth[py, px] = pixel_z
            color[py, px] = shade
    return color, np.isfinite(depth)


def portrait_background(size, spec, seed):
    height, width = size
    y = np.linspace(0.0, 1.0, height)[:, None, None]
    x = np.linspace(-1.0, 1.0, width)[None, :, None]
    top = np.asarray(spec["top"], np.float64) / 255.0
    bottom = np.asarray(spec["bottom"], np.float64) / 255.0
    glow = np.asarray(spec["glow"], np.float64) / 255.0
    base = top + (bottom - top) * y
    radial = np.exp(-((x * 1.1) ** 2 + ((y - 0.38) * 2.2) ** 2) * 2.2)
    rng = np.random.default_rng(seed)
    noise = Image.fromarray((rng.random((height // 16, width // 16)) * 255).astype(np.uint8))
    mist = np.asarray(noise.resize((width, height), Image.BICUBIC).filter(ImageFilter.GaussianBlur(height / 24)),
                      np.float64)[..., None] / 255.0
    return np.clip(base + glow * radial * 0.8 + glow * (mist - 0.5) * 0.35, 0.0, 1.0)


def cache_portrait(icon):
    bundles = original_cache.catalog().get((original_cache.TEXTURE_2D, icon))
    if bundles:
        import UnityPy

        for path in original_cache.bundle_paths():
            if original_cache.bundle_name(path) not in bundles:
                continue
            for obj in UnityPy.load(str(path)).objects:
                if obj.type.name == "Texture2D":
                    texture = obj.read()
                    if texture.m_Name == icon:
                        return texture.image.convert("RGBA")
    extra = PORTRAIT_DIR / f"{icon}.png"
    return Image.open(extra).convert("RGBA") if extra.is_file() else None


def frame_template(base_row, shipped):
    peers = [
        row for row in shipped.values()
        if row["rarity"] == base_row["rarity"] and row["stars"] == base_row["stars"] and row.get("obtainable")
    ][:FRAME_PEER_HEROES]
    stack = []
    for row in peers:
        for icon in sorted(set(row["icon"].values())):
            image = cache_portrait(icon)
            if image is not None:
                stack.append(np.asarray(image, np.float64))
    if len(stack) < 4:
        raise SystemExit(f"too few {base_row['rarity']} portraits to recover the frame")
    stack = np.array(stack)
    spread = stack.std(0)[..., :3].mean(-1)
    core = Image.fromarray((spread < FRAME_STD_LIMIT).astype(np.uint8) * 255).filter(ImageFilter.MedianFilter(3))
    near_core = core.filter(ImageFilter.MaxFilter(FRAME_EDGE_REACH * 2 + 1))
    frame = (np.asarray(core) > 0) | ((np.asarray(near_core) > 0) & (spread < FRAME_EDGE_STD_LIMIT))
    return np.median(stack, 0), frame


def render_portrait(rig, clip, primitives, halo_skin, spec, template, size, seed):
    world, posed = skinned_pose(rig, clip, 0, primitives, halo_skin)
    focus = world[rig.index(spec["focus_bone"])][:3, 3] + np.asarray(spec.get("focus_offset", (0, 0, 0)), np.float64)
    yaw = math.radians(spec.get("yaw", 0.0))
    eye = focus + np.array([math.sin(yaw), math.tan(math.radians(spec.get("pitch", 0.0))), math.cos(yaw)]) * spec["distance"]
    supersampled = (size[0] * PORTRAIT_SUPERSAMPLE, size[1] * PORTRAIT_SUPERSAMPLE)
    color, covered = rasterize(posed, eye, focus, supersampled, spec["light"], spec["rim"])
    background = portrait_background(supersampled, spec["background"], seed)
    picture = np.where(covered[..., None], color, background)
    picture = picture.reshape(size[0], PORTRAIT_SUPERSAMPLE, size[1], PORTRAIT_SUPERSAMPLE, 3).mean((1, 3))
    frame_rgba, frame = template
    out = np.concatenate((np.clip(picture, 0, 1) * 255, np.full(size + (1,), 255.0)), -1)
    out[frame] = frame_rgba[frame]
    return Image.fromarray(out.round().astype(np.uint8), "RGBA")


def bake_tier(spec, tier, template, scales):
    look = spec["look"]
    tier_look = look["tiers"][tier]
    source = tier_look["source"]
    base = tier_base(spec["prefab"], tier)
    out_dir = CHAR_PACK / spec["prefab"]
    out_dir.mkdir(parents=True, exist_ok=True)

    gltf, blob = load_glb(runtime_model_path(source))
    rig = Rig(gltf, blob)
    sources = ClipSources(rig, gltf, blob)
    clips = [(name, author_clip(recipe, rig, sources)) for name, recipe in look["clips"].items()]
    if clips[0][0] != FIRST_CLIP:
        raise SystemExit(f"{spec['id']}: the first clip must be {FIRST_CLIP!r}")

    out = copy.deepcopy(gltf)
    rename(out, source, base)
    body = body_primitives(gltf, blob)
    textures = {}
    for material_index, positions, normals, uvs, triangles, joints, weights in body:
        texture_index = out["materials"][material_index]["pbrMetallicRoughness"]["baseColorTexture"]["index"]
        image_index = out["textures"][texture_index]["source"]
        if image_index in textures:
            continue
        source_image = Image.open(runtime_model_path(source).parent / gltf["images"][image_index]["uri"])
        primitives = [(p[1], p[2], p[3], p[4], p[5], p[6]) for p in body if p[0] == material_index]
        texels = texel_map((source_image.height, source_image.width), primitives)
        painted = paint_texture(source_image, texels, rig, look.get("texture", []) + tier_look.get("texture", []))
        name = f"{base}_main.png" if not textures else f"{base}_main{len(textures)}.png"
        painted.save(out_dir / name)
        out["images"][image_index] = {"uri": name}
        textures[image_index] = np.asarray(painted.convert("RGB"), np.float64) / 255.0

    halo, extra, halo_inverse_bind, halo_pixels = None, [], None, None
    if "halo" in look:
        halo_spec = {**look["halo"], **tier_look.get("halo", {})}
        halo, primitive, halo_inverse_bind, halo_pixels = add_halo(out, rig, halo_spec, base, out_dir)
        extra.append(primitive)
    rebuilt, rebuilt_blob = rebuild_buffers(out, blob, extra, clips, rig, halo, halo_inverse_bind)
    save_glb(out_dir / f"{base}_rl.glb", rebuilt, rebuilt_blob)

    render_primitives = []
    for material_index, positions, normals, uvs, triangles, joints, weights in body:
        texture_index = out["materials"][material_index]["pbrMetallicRoughness"]["baseColorTexture"]["index"]
        render_primitives.append((positions, normals, uvs, triangles, joints, weights,
                                  textures[out["textures"][texture_index]["source"]]))
    halo_skin = None
    if halo is not None:
        positions, normals, uvs, triangles, joint = extra[0]["geometry"]
        local = np.zeros((len(positions), 4), np.int64)
        local[:, 0] = len(rig.joints)
        weights = np.zeros((len(positions), 4))
        weights[:, 0] = 1.0
        render_primitives.append((positions, normals, uvs, triangles, local, weights, halo_pixels))
        head = rig.index(look["halo"]["bone"])
        idle = clips[0][1]
        head_world = rig.world_matrices(idle.t[0], idle.r[0], idle.s[0])[head]
        halo_skin = head_world @ rig.inverse_bind[head]
    portrait = render_portrait(rig, clips[0][1], render_primitives, halo_skin, look["portrait"], template,
                               (template[0].shape[0], template[0].shape[1]), TIERS.index(tier))
    PORTRAIT_DIR.mkdir(parents=True, exist_ok=True)
    portrait.save(PORTRAIT_DIR / f"tex.{base}_pt.png")
    scales[base] = {"scale": look["display_scale"], "y": scales.get(source, {}).get("y", 0.0)}
    print(f"{spec['id']} {tier}: {base}_rl.glb, {len(clips)} clips, portrait tex.{base}_pt.png")


def main():
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--only", help="bake one custom hero id")
    args = parser.parse_args()
    shipped = shipped_rows()
    scales = json.loads(DISPLAY_SCALES.read_text())
    for spec in load_specs():
        if args.only and spec["id"] != args.only:
            continue
        template = frame_template(shipped[spec["based_on"]], shipped)
        for tier in TIERS:
            bake_tier(spec, tier, template, scales)
    DISPLAY_SCALES.write_text(json.dumps(scales, indent=1) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
