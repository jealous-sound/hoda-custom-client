#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

CLI_DESCRIPTION = 'Build data/consume_scene.json from the original TrainingAndPromotionsContainer scene config'
ROOT = Path(__file__).resolve().parents[1]
SCENE_CONFIG = ROOT / 'client-additional-data/meshes/TrainingAndPromotionsContainer/scene_config.json'
OUTPUT = ROOT / 'data/consume_scene.json'
PREFAB_ROOT = 'prefab.TrainingAndPromotionsContainer'
KEPT_BRANCHES = ('_Training', 'test_combine_consume_scene')
SKIPPED_NODES = ('power_helix_01',)
CAMERAS = {
    'Training Camera Marker': ('board', 55.0),
    'Training Camera Marker - Centered': ('all', 45.0),
    'Training Camera Marker - Complete': ('complete', 55.0),
    'Training Camera Marker - Complete_Tall': ('complete_tall', 55.0),
}
SEED_SPOTS = ('SeedPos1', 'SeedPos2', 'SeedPos3', 'SeedPos4', 'SeedPos5')
TARGET_SPOT = 'TargetPos'
SEED_CIRCLE_PREFIX = 'arrow_direction_off_0'
PEDESTAL_NODE = 'pedistal_upgrade'
PEDESTAL_PLATE_HEIGHT = 0.22
PEDESTAL_SHADOW_NODE = 'rockbase_shadow'
SUN_GLOW_NODE = 'Particle System_Sun_glow'


def quat_mul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (
        aw * bx + ax * bw + ay * bz - az * by,
        aw * by - ax * bz + ay * bw + az * bx,
        aw * bz + ax * by - ay * bx + az * bw,
        aw * bw - ax * bx - ay * by - az * bz,
    )


def quat_matrix(q):
    x, y, z, w = q
    return [
        [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
        [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
        [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)],
    ]


def local_matrix(node):
    rotation = quat_matrix(node['local_rotation_quat'])
    scale = node['local_scale']
    position = node['local_position']
    rows = [[rotation[r][c] * scale[c] for c in range(3)] + [position[r]] for r in range(3)]
    return rows + [[0.0, 0.0, 0.0, 1.0]]


def mat_mul(a, b):
    return [[sum(a[r][k] * b[k][c] for k in range(4)) for c in range(4)] for r in range(4)]


def to_right_handed(matrix):
    flip = [1.0, 1.0, -1.0, 1.0]
    return [[matrix[r][c] * flip[r] * flip[c] for c in range(4)] for r in range(4)]


def rounded(values):
    return [round(value, 5) for value in values]


def column_major(matrix):
    return rounded(matrix[r][c] for c in range(4) for r in range(4))


def right_handed_point(matrix):
    return rounded((matrix[0][3], matrix[1][3], -matrix[2][3]))


def node_materials(node):
    materials = node['materials']
    return materials[0] if materials else ''


def walk(node, parent_matrix, parent_quat, by_id, emit):
    matrix = mat_mul(parent_matrix, local_matrix(node))
    quat = quat_mul(parent_quat, node['local_rotation_quat'])
    emit(node, matrix, quat)
    for child in node['children_ids']:
        if child in by_id:
            walk(by_id[child], matrix, quat, by_id, emit)


def camera_entry(key, fov, matrix, quat):
    forward = quat_matrix(quat)
    return {
        'key': key,
        'fov': fov,
        'position': right_handed_point(matrix),
        'forward': rounded((forward[0][2], forward[1][2], -forward[2][2])),
        'up': rounded((forward[0][1], forward[1][1], -forward[2][1])),
    }


def build() -> dict:
    config = json.loads(SCENE_CONFIG.read_text(encoding='utf-8'))
    by_id = {node['transform_id']: node for node in config['nodes']}
    root = next(node for node in config['nodes'] if node['name'] == PREFAB_ROOT)
    identity = [[float(r == c) for c in range(4)] for r in range(4)]
    scene = {'version': 1, 'nodes': [], 'cameras': [], 'seed_spots': {}, 'target_spot': []}

    surfaces = {}

    def emit(node, matrix, quat):
        name = node['name']
        if name.startswith(SEED_CIRCLE_PREFIX):
            surfaces.setdefault('seed', matrix[1][3])
        if name == PEDESTAL_NODE:
            surfaces['target'] = matrix[1][3] + PEDESTAL_PLATE_HEIGHT * matrix[1][1]
        if name in CAMERAS:
            key, fov = CAMERAS[name]
            scene['cameras'].append(camera_entry(key, fov, matrix, quat))
        elif name in SEED_SPOTS:
            scene['seed_spots'][name] = right_handed_point(matrix)
        elif name == TARGET_SPOT:
            scene['target_spot'] = right_handed_point(matrix)
        elif name == PEDESTAL_SHADOW_NODE:
            scene['pedestal_shadow'] = column_major(to_right_handed(matrix))
        elif name == SUN_GLOW_NODE:
            scene['sun_glow'] = right_handed_point(matrix)
        elif node['mesh'] and name not in SKIPPED_NODES:
            scene['nodes'].append(
                {
                    'name': name,
                    'mesh': node['mesh'],
                    'material': node_materials(node),
                    'matrix': column_major(to_right_handed(matrix)),
                }
            )

    for child in root['children_ids']:
        node = by_id[child]
        if node['name'] in KEPT_BRANCHES:
            walk(node, identity, (0.0, 0.0, 0.0, 1.0), by_id, emit)
    scene['nodes'].sort(key=lambda entry: (entry['material'], entry['name'], entry['matrix']))
    scene['cameras'].sort(key=lambda entry: entry['key'])
    seed_spots = [scene['seed_spots'][name] for name in SEED_SPOTS]
    scene['seed_spots'] = [[x, round(surfaces['seed'], 5), z] for x, _, z in seed_spots]
    scene['target_root_y'] = scene['target_spot'][1]
    scene['target_spot'][1] = round(surfaces['target'], 5)
    return scene


def serialize(scene: dict) -> str:
    return json.dumps(scene, ensure_ascii=False, separators=(',', ':'), sort_keys=True) + '\n'


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument('--check', action='store_true', help='fail when the committed scene is stale')
    args = parser.parse_args()
    text = serialize(build())
    if args.check:
        if OUTPUT.read_text(encoding='utf-8') != text:
            print(f'{OUTPUT.relative_to(ROOT)} is stale; rerun tools/build_consume_scene.py', file=sys.stderr)
            return 1
        return 0
    OUTPUT.write_text(text, encoding='utf-8')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
