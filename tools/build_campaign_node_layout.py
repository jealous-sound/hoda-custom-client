#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import math
import struct
import sys
from pathlib import Path

import original_cache

CLI_DESCRIPTION = 'Project the original 4.4.0 campaign map prefabs into node positions and layouts in campaign_maps.json'
ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MAPS = ROOT / 'data/campaign_maps.json'

GAME_OBJECT = 1
TRANSFORM = 4
CAMERA = 20
MESH_FILTER = 33
MESH = 43
UNITY_PLANE_HALF_EXTENT = 5.0
NODE_PREFAB = 'node_placeholder_pre'
NODE_MARKERS = 'NodeMarkers'
NODE_MARKER_PREFIX = 'step'
MAP_CAMERA = 'MapCamera'
REFERENCE_PLANES = {14: 'pve_midground_mesh', 15: 'fg', 16: 'fg'}
DEFAULT_REFERENCE_PLANE = 'pve_midground'
NODE_PARTS = {'icon': ('Cube', None), 'banner': ('Container', 'EnergyCost'), 'flask': ('Energy', None)}
ENERGY_TEXT_ANCHOR = 'EnergyCost'
POSITION_DIGITS = 4
IDENTITY = ((1.0, 0.0, 0.0, 0.0), (0.0, 1.0, 0.0, 0.0), (0.0, 0.0, 1.0, 0.0), (0.0, 0.0, 0.0, 1.0))


def map_prefab(map_number: int) -> str:
    return 'map_placeholder_pre' if map_number == 1 else f'map_placeholder{map_number}_pre'


class Reader:
    def __init__(self, data: bytes, position: int):
        self.data = data
        self.position = position

    def unpack(self, fmt: str):
        values = struct.unpack_from('<' + fmt, self.data, self.position)
        self.position += struct.calcsize('<' + fmt)
        return values

    def int32(self) -> int:
        return self.unpack('i')[0]

    def floats(self, count: int) -> tuple[float, ...]:
        return self.unpack(f'{count}f')

    def align(self) -> None:
        self.position = (self.position + 3) & ~3

    def string(self) -> str:
        length = self.int32()
        text = self.data[self.position : self.position + length].decode('utf-8', 'replace')
        self.position += length
        self.align()
        return text

    def pointer(self) -> tuple[int, int]:
        return self.unpack('ii')


class Bundle:
    def __init__(self, name: str):
        paths = [path for path in original_cache.bundle_paths() if original_cache.bundle_name(path) == name]
        if not paths:
            raise SystemExit(f'{name}: bundle is missing from the original cache')
        self.name = name
        self.data = paths[0].read_bytes()
        _, _, version, data_offset = struct.unpack_from('>IIII', self.data)
        if version != original_cache.SERIALIZED_VERSION:
            raise SystemExit(f'{name}: unsupported serialized version {version}')
        position = self.data.index(b'\0', original_cache.HEADER_SIZE) + 1
        _, type_count, big_ids, count = struct.unpack_from('<iiii', self.data, position)
        if type_count or big_ids:
            raise SystemExit(f'{name}: type trees or 64-bit ids are not supported')
        position += 16
        self.objects: dict[int, tuple[int, int]] = {}
        for _ in range(count):
            path_id, start, _, _, class_id, _ = original_cache.OBJECT_ENTRY.unpack_from(self.data, position)
            position += original_cache.OBJECT_ENTRY.size
            self.objects[path_id] = (class_id, data_offset + start)
        external_count = struct.unpack_from('<i', self.data, position)[0]
        position += 4
        self.externals: list[str] = []
        for _ in range(external_count):
            position = self.data.index(b'\0', position) + 1 + 16 + 4
            end = self.data.index(b'\0', position)
            self.externals.append(self.data[position:end].decode())
            position = end + 1

    def reader(self, path_id: int) -> Reader:
        return Reader(self.data, self.objects[path_id][1])

    def of_class(self, class_id: int) -> list[int]:
        return [path_id for path_id, (kind, _) in self.objects.items() if kind == class_id]


class Scene:
    def __init__(self, bundle: Bundle):
        self.bundle = bundle
        self.names: dict[int, str] = {}
        for path_id in bundle.of_class(GAME_OBJECT):
            reader = bundle.reader(path_id)
            component_count = reader.int32()
            reader.position += 12 * component_count
            reader.int32()
            self.names[path_id] = reader.string()
        self.transforms: dict[int, dict] = {}
        for path_id in bundle.of_class(TRANSFORM):
            reader = bundle.reader(path_id)
            game_object = reader.pointer()[1]
            rotation = reader.floats(4)
            position = reader.floats(3)
            scale = reader.floats(3)
            child_count = reader.int32()
            reader.position += 8 * child_count
            father = reader.pointer()[1]
            self.transforms[path_id] = {
                'game_object': game_object,
                'matrix': trs_matrix(position, rotation, scale),
                'father': father,
            }
        self.mesh_filters: dict[int, tuple[int, int]] = {}
        for path_id in bundle.of_class(MESH_FILTER):
            reader = bundle.reader(path_id)
            game_object = reader.pointer()[1]
            self.mesh_filters[game_object] = reader.pointer()

    def name(self, transform_id: int) -> str:
        return self.names[self.transforms[transform_id]['game_object']]

    def father_name(self, transform_id: int) -> str:
        father = self.transforms[transform_id]['father']
        return self.name(father) if father in self.transforms else ''

    def find(self, name: str, father: str | None = None) -> int:
        for transform_id in sorted(self.transforms):
            if self.name(transform_id) == name and (father is None or self.father_name(transform_id) == father):
                return transform_id
        raise SystemExit(f'{self.bundle.name}: no transform named {name}')

    def matrix(self, transform_id: int, stop: int | None = None):
        chain = []
        while transform_id in self.transforms and transform_id != stop:
            chain.append(transform_id)
            transform_id = self.transforms[transform_id]['father']
        result = IDENTITY
        for link in reversed(chain):
            result = multiply(result, self.transforms[link]['matrix'])
        return result

    def mesh_extent(self, transform_id: int) -> tuple[float, float]:
        file_id, path_id = self.mesh_filters[self.transforms[transform_id]['game_object']]
        bundle = self.bundle if file_id == 0 else Bundle(self.bundle.externals[file_id - 1].removeprefix('cab-'))
        reader = bundle.reader(path_id)
        reader.string()
        if reader.int32() < 1:
            raise SystemExit(f'{self.bundle.name}: reference plane mesh has no submesh')
        reader.position += 20
        center = reader.floats(3)
        extent = reader.floats(3)
        if abs(center[0]) > 1e-4 or abs(center[2]) > 1e-4:
            raise SystemExit(f'{self.bundle.name}: reference plane mesh is not centred')
        return extent[0], extent[2]


def trs_matrix(position, rotation, scale):
    x, y, z, w = rotation
    basis = (
        (1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)),
        (2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)),
        (2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)),
    )
    return tuple(
        tuple(basis[row][column] * scale[column] for column in range(3)) + (position[row],) for row in range(3)
    ) + ((0.0, 0.0, 0.0, 1.0),)


def multiply(a, b):
    return tuple(tuple(sum(a[i][k] * b[k][j] for k in range(4)) for j in range(4)) for i in range(4))


def transform_point(matrix, point):
    return tuple(sum(matrix[i][k] * point[k] for k in range(3)) + matrix[i][3] for i in range(3))


def invert_affine(matrix):
    a = [row[:3] for row in matrix[:3]]
    determinant = (
        a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1])
        - a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0])
        + a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0])
    )
    cofactor = lambda r, c: (
        a[(r + 1) % 3][(c + 1) % 3] * a[(r + 2) % 3][(c + 2) % 3]
        - a[(r + 1) % 3][(c + 2) % 3] * a[(r + 2) % 3][(c + 1) % 3]
    )
    inverse = [[cofactor(c, r) / determinant for c in range(3)] for r in range(3)]
    translation = [-sum(inverse[i][k] * matrix[k][3] for k in range(3)) for i in range(3)]
    return tuple(tuple(inverse[i]) + (translation[i],) for i in range(3)) + ((0.0, 0.0, 0.0, 1.0),)


def subtract(a, b):
    return tuple(a[i] - b[i] for i in range(3))


def dot(a, b):
    return sum(a[i] * b[i] for i in range(3))


def normalized(v):
    length = math.sqrt(dot(v, v))
    return tuple(c / length for c in v)


def column(matrix, index):
    return tuple(matrix[row][index] for row in range(3))


def node_parts(scene: Scene) -> dict[str, tuple]:
    root = next(t for t in scene.transforms if scene.transforms[t]['father'] not in scene.transforms)
    parts = {key: scene.matrix(scene.find(name, father), root) for key, (name, father) in NODE_PARTS.items()}
    parts['text'] = scene.matrix(scene.find(ENERGY_TEXT_ANCHOR), root)
    return parts


def project_map(map_number: int, parts: dict[str, tuple]) -> dict:
    scene = Scene(Bundle(map_prefab(map_number)))
    plane = scene.find(REFERENCE_PLANES.get(map_number, DEFAULT_REFERENCE_PLANE))
    extent_x, extent_z = scene.mesh_extent(plane)
    plane_matrix = scene.matrix(plane)
    plane_inverse = invert_affine(plane_matrix)
    plane_origin = transform_point(plane_matrix, (0.0, 0.0, 0.0))
    plane_normal = column(plane_matrix, 1)
    camera_matrix = scene.matrix(scene.find(MAP_CAMERA))
    camera = column(camera_matrix, 3)
    pan_axis = normalized(column(camera_matrix, 0))
    if transform_point(plane_inverse, tuple(camera[i] + pan_axis[i] for i in range(3)))[0] >= transform_point(
        plane_inverse, camera
    )[0]:
        raise SystemExit(f'map {map_number}: camera pans against the panorama direction')

    def on_panorama(eye, point):
        ray = subtract(point, eye)
        distance = dot(subtract(plane_origin, eye), plane_normal) / dot(ray, plane_normal)
        local = transform_point(plane_inverse, tuple(eye[i] + ray[i] * distance for i in range(3)))
        return (extent_x - local[0]) / (2.0 * extent_x), (local[2] + extent_z) / (2.0 * extent_z)

    markers = {}
    for transform_id in scene.transforms:
        name = scene.name(transform_id)
        if name.startswith(NODE_MARKER_PREFIX) and scene.father_name(transform_id) == NODE_MARKERS:
            markers[int(name[len(NODE_MARKER_PREFIX) :])] = transform_id
    if sorted(markers) != list(range(1, len(markers) + 1)):
        raise SystemExit(f'map {map_number}: node markers are not numbered 1..{len(markers)}')

    positions, layouts = [], []
    for step in sorted(markers):
        marker = scene.matrix(markers[step])
        node = transform_point(marker, (0.0, 0.0, 0.0))
        offset = dot(subtract(node, camera), pan_axis)
        eye = tuple(camera[i] + pan_axis[i] * offset for i in range(3))
        centre = on_panorama(eye, node)
        positions.append(centre)
        layout = {}
        for key, part in parts.items():
            world = multiply(marker, part)
            if key == 'text':
                anchor = on_panorama(eye, transform_point(world, (0.0, 0.0, 0.0)))
                layout[key] = (anchor[0] - centre[0], anchor[1] - centre[1])
                continue
            corners = [
                on_panorama(eye, transform_point(world, (sx * UNITY_PLANE_HALF_EXTENT, 0.0, sz * UNITY_PLANE_HALF_EXTENT)))
                for sx in (-1.0, 1.0)
                for sz in (-1.0, 1.0)
            ]
            xs = [corner[0] for corner in corners]
            ys = [corner[1] for corner in corners]
            layout[key] = (min(xs) - centre[0], min(ys) - centre[1], max(xs) - min(xs), max(ys) - min(ys))
        layouts.append(layout)
    averaged = {
        key: [round(sum(layout[key][i] for layout in layouts) / len(layouts), POSITION_DIGITS) for i in range(len(layouts[0][key]))]
        for key in layouts[0]
    }
    return {
        'positions': [{'x': round(x, POSITION_DIGITS), 'y': round(y, POSITION_DIGITS)} for x, y in positions],
        'node_layout': averaged,
    }


def apply_layout(maps: list[dict], parts: dict[str, tuple]) -> list[dict]:
    for map_number, chapter in enumerate(maps, start=1):
        projected = project_map(map_number, parts)
        if len(projected['positions']) != len(chapter['nodes']):
            raise SystemExit(
                f"{chapter['id']}: prefab has {len(projected['positions'])} node markers, data has {len(chapter['nodes'])}"
            )
        for node, position in zip(chapter['nodes'], projected['positions']):
            node['position'] = position
        chapter['node_layout'] = projected['node_layout']
    return maps


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument('--maps', type=Path, default=DEFAULT_MAPS)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    original = args.maps.read_text(encoding='utf-8')
    maps = apply_layout(json.loads(original), node_parts(Scene(Bundle(NODE_PREFAB))))
    rendered = json.dumps(maps, ensure_ascii=False, indent=1) + '\n'
    if args.check:
        if rendered != original:
            print(f'{args.maps}: node positions differ from the original map prefabs', file=sys.stderr)
            return 1
        print(f'{args.maps}: node positions match the original map prefabs')
        return 0
    args.maps.write_text(rendered, encoding='utf-8')
    print(f'{args.maps}: wrote positions for {sum(len(chapter["nodes"]) for chapter in maps)} nodes')
    return 0


if __name__ == '__main__':
    sys.exit(main())
