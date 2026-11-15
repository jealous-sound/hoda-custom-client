#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import math
import re
import sys
from pathlib import Path

import original_cache

CLI_DESCRIPTION = 'Build data/nexus_scenes.json, the Nexus event map scenes, from the original 4.4.0 map prefabs'
ROOT = Path(__file__).resolve().parents[1]
MAPS = ROOT / 'data' / 'nexus_maps.json'
OUTPUT = ROOT / 'data' / 'nexus_scenes.json'
RUN_COMMAND = 'uv run --offline --with UnityPy python tools/build_nexus_scenes.py'
LIVE_ROOT_PREFIX = 'map.'
CAMERA_NODE = 'MapCamera'
NODE_FRAME = 'NodeMarkers'
FIRST_MARKER = 'step1'
SHARED_BUNDLES = ('shared_resourcecontainer', 'shared_shadercontainer')
DEFAULT_RESOURCES = 'unity default resources'
BUILTIN_EXTRA = 'unity_builtin_extra'
LIBRARY_RESOURCES = '0000000000000000f000000000000000'
BUILTIN_QUAD = 10210
BUILTIN_UNLIT_TEXTURE = 10752
LIBRARY_DIFFUSE = 2
DEFAULT_CUTOFF = 0.5
PARTICLE_TINT_GAIN = 2.0
POSITION_DIGITS = 4
UV_DIGITS = 5
MATRIX_DIGITS = 5
COLOR_DIGITS = 4
SCENE_VERSION = 1
SHADER_NAME = re.compile(r'Shader "([^"]+)"')
QUAD_POSITIONS = ((-0.5, -0.5, 0.0), (0.5, -0.5, 0.0), (-0.5, 0.5, 0.0), (0.5, 0.5, 0.0))
QUAD_UVS = ((0.0, 0.0), (1.0, 0.0), (0.0, 1.0), (1.0, 1.0))
QUAD_TRIANGLES = (0, 3, 1, 3, 0, 2)
OPAQUE = 'opaque'
CUTOUT = 'cutout'
ALPHA = 'alpha'
ADDITIVE = 'additive'
SHADER_BLENDS = {
    'Unlit/Texture': (OPAQUE, None),
    'Diffuse': (OPAQUE, '_Color'),
    'Unlit/Transparent': (ALPHA, None),
    'Unlit/Transparent Cutout': (CUTOUT, None),
    'Transparent/Diffuse': (ALPHA, '_Color'),
    'Mobile/Particles/Alpha Blended': (ALPHA, None),
    'Mobile/Particles/Additive': (ADDITIVE, None),
    'Particles/Alpha Blended': (ALPHA, '_TintColor'),
}
FLIP_Z = ((1.0, 0.0, 0.0, 0.0), (0.0, 1.0, 0.0, 0.0), (0.0, 0.0, -1.0, 0.0), (0.0, 0.0, 0.0, 1.0))
IDENTITY = tuple(tuple(float(r == c) for c in range(4)) for r in range(4))


def unitypy():
    try:
        import UnityPy  # type: ignore
        from UnityPy.helpers.MeshHelper import MeshHandler  # type: ignore
    except ImportError:
        raise SystemExit(f'build_nexus_scenes.py needs UnityPy; run {RUN_COMMAND}')
    return UnityPy, MeshHandler


def quat_matrix(q):
    x, y, z, w = q
    return (
        (1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)),
        (2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)),
        (2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)),
    )


def trs(position, rotation, scale):
    r = quat_matrix(rotation)
    rows = tuple(tuple(r[i][c] * scale[c] for c in range(3)) + (position[i],) for i in range(3))
    return rows + ((0.0, 0.0, 0.0, 1.0),)


def mat_mul(a, b):
    return tuple(tuple(sum(a[r][k] * b[k][c] for k in range(4)) for c in range(4)) for r in range(4))


def transform_point(m, p):
    return tuple(m[r][0] * p[0] + m[r][1] * p[1] + m[r][2] * p[2] + m[r][3] for r in range(3))


def transform_direction(m, d):
    v = tuple(m[r][0] * d[0] + m[r][1] * d[1] + m[r][2] * d[2] for r in range(3))
    length = math.sqrt(sum(c * c for c in v))
    return tuple(c / length for c in v)


def rounded(values, digits):
    return [round(value, digits) + 0.0 for value in values]


def column_major(m):
    return rounded((m[r][c] for c in range(4) for r in range(4)), MATRIX_DIGITS)


def xyz(value):
    return (value['x'], value['y'], value['z'])


def xyzw(value):
    return (value['x'], value['y'], value['z'], value['w'])


def rgba(value):
    return (value['r'], value['g'], value['b'], value['a'])


class Bundles:
    def __init__(self, unity, prefab):
        self.unity = unity
        self.prefab = prefab
        self.env = unity.load(str(self.path(prefab)))
        self.shared = {name: self.objects(unity.load(str(self.path(name)))) for name in SHARED_BUNDLES}
        self.local = self.objects(self.env)

    @staticmethod
    def path(name):
        found = sorted(original_cache.BUNDLE_DIR.glob(f'*/{original_cache.BUNDLE_FILE_PREFIX}{name}'))
        if len(found) != 1:
            raise SystemExit(f'{name}: expected one original bundle, found {len(found)}')
        return found[0]

    @staticmethod
    def objects(env):
        return {obj.path_id: obj for obj in env.objects}

    def resolve(self, owner, pointer):
        file_id, path_id = pointer['m_FileID'], pointer['m_PathID']
        if path_id == 0:
            return None, None
        if file_id == 0:
            return self.local.get(path_id), self.prefab
        external = owner.assets_file.externals[file_id - 1].path
        for name, table in self.shared.items():
            if name in external:
                return table.get(path_id), name
        return None, (external, path_id)


def shader_name(bundles, material_obj, pointer):
    shader, source = bundles.resolve(material_obj, pointer)
    if shader is not None:
        match = SHADER_NAME.search(shader.read_typetree().get('m_Script', ''))
        if match:
            return match.group(1)
    if isinstance(source, tuple):
        external, path_id = source
        if BUILTIN_EXTRA in external and path_id == BUILTIN_UNLIT_TEXTURE:
            return 'Unlit/Texture'
        if LIBRARY_RESOURCES in external and path_id == LIBRARY_DIFFUSE:
            return 'Diffuse'
    raise SystemExit(f'{bundles.prefab}: unknown shader {source} on {material_obj.read_typetree()["m_Name"]}')


def material_entry(bundles, material_obj):
    tree = material_obj.read_typetree()
    properties = tree['m_SavedProperties']
    shader = shader_name(bundles, material_obj, tree['m_Shader'])
    if shader not in SHADER_BLENDS:
        raise SystemExit(f'{bundles.prefab}: no blend rule for shader {shader} ({tree["m_Name"]})')
    blend, color_key = SHADER_BLENDS[shader]
    colors = {key['name']: rgba(value) for key, value in properties.get('m_Colors', [])}
    floats = {key['name']: value for key, value in properties.get('m_Floats', [])}
    color = (1.0, 1.0, 1.0, 1.0)
    if color_key:
        color = colors.get(color_key, color)
        if color_key == '_TintColor':
            color = tuple(min(1.0, PARTICLE_TINT_GAIN * c) for c in color)
    texture, scale, offset = None, (1.0, 1.0), (0.0, 0.0)
    for key, value in properties['m_TexEnvs']:
        if key['name'] != '_MainTex':
            continue
        texture_obj, bundle = bundles.resolve(material_obj, value['m_Texture'])
        if texture_obj is not None:
            name = texture_obj.read_typetree()['m_Name']
            texture = f'{original_cache.CACHE_REF_PREFIX}{bundle}/{name}'
            if not original_cache.resolves(texture, original_cache.TEXTURE_2D):
                raise SystemExit(f'{bundles.prefab}: {texture} is not in the original cache')
        scale = (value['m_Scale']['x'], value['m_Scale']['y'])
        offset = (value['m_Offset']['x'], value['m_Offset']['y'])
    if texture is None and color_key is None:
        return None, scale, offset
    entry = {'blend': blend, 'color': rounded(color, COLOR_DIGITS), 'texture': texture}
    if blend == CUTOUT:
        entry['cutoff'] = round(floats.get('_Cutoff', DEFAULT_CUTOFF), COLOR_DIGITS)
    return entry, scale, offset


def mesh_geometry(bundles, mesh_handler, filter_obj):
    pointer = filter_obj.read_typetree()['m_Mesh']
    mesh_obj, source = bundles.resolve(filter_obj, pointer)
    if mesh_obj is not None:
        handler = mesh_handler(mesh_obj.read())
        handler.process()
        return handler.m_Vertices, handler.m_UV0, [list(sum(map(list, sub), [])) for sub in handler.get_triangles()]
    if isinstance(source, tuple) and DEFAULT_RESOURCES in source[0] and source[1] == BUILTIN_QUAD:
        return QUAD_POSITIONS, QUAD_UVS, [list(QUAD_TRIANGLES)]
    return None


class Scene:
    def __init__(self, bundles, mesh_handler):
        self.bundles = bundles
        self.mesh_handler = mesh_handler
        self.game_objects = {}
        self.transforms = {}
        for path_id, obj in bundles.local.items():
            if obj.type.name == 'GameObject':
                self.game_objects[path_id] = obj.read_typetree()
            elif obj.type.name == 'Transform':
                self.transforms[path_id] = obj.read_typetree()

    def components(self, game_object):
        found = {}
        for entry in game_object['m_Component']:
            pointer = entry['component'] if isinstance(entry, dict) else entry[1]
            obj = self.bundles.local.get(pointer['m_PathID'])
            if obj is not None:
                found[obj.type.name] = obj
        return found

    def live_root(self):
        roots = [
            transform_id
            for transform_id, transform in self.transforms.items()
            if transform['m_Father']['m_PathID'] == 0
            and self.name(transform_id) == f'{LIVE_ROOT_PREFIX}{self.bundles.prefab}'
        ]
        if len(roots) != 1:
            raise SystemExit(f'{self.bundles.prefab}: expected one {LIVE_ROOT_PREFIX} root, found {len(roots)}')
        return roots[0]

    def game_object(self, transform_id):
        return self.game_objects[self.transforms[transform_id]['m_GameObject']['m_PathID']]

    def name(self, transform_id):
        return self.game_object(transform_id)['m_Name']

    def local(self, transform_id):
        t = self.transforms[transform_id]
        return trs(xyz(t['m_LocalPosition']), xyzw(t['m_LocalRotation']), xyz(t['m_LocalScale']))

    def walk(self, transform_id, matrix, path, active, visit):
        for child in self.transforms[transform_id]['m_Children']:
            child_id = child['m_PathID']
            game_object = self.game_object(child_id)
            child_active = active and game_object.get('m_IsActive', True)
            child_matrix = mat_mul(matrix, self.local(child_id))
            child_path = f'{path}/{game_object["m_Name"]}' if path else game_object['m_Name']
            visit(child_id, child_matrix, child_path, child_active)
            self.walk(child_id, child_matrix, child_path, child_active, visit)

    def build(self):
        root = self.live_root()
        layers, camera, frame, marker = [], None, None, None

        def visit(transform_id, matrix, path, active):
            nonlocal camera, frame, marker
            game_object = self.game_object(transform_id)
            name = game_object['m_Name']
            parts = self.components(game_object)
            if name == CAMERA_NODE and 'Camera' in parts:
                camera = (matrix, parts['Camera'].read_typetree())
            elif name == NODE_FRAME:
                frame = matrix
            elif name == FIRST_MARKER:
                marker = self.transforms[transform_id]
            if active and 'MeshRenderer' in parts and 'MeshFilter' in parts:
                layers.extend(self.layers(matrix, path, parts['MeshRenderer'], parts['MeshFilter']))

        self.walk(root, IDENTITY, '', True, visit)
        if camera is None or frame is None or marker is None:
            raise SystemExit(f'{self.bundles.prefab}: missing {CAMERA_NODE}, {NODE_FRAME} or {FIRST_MARKER}')
        return {
            'prefab': self.bundles.prefab,
            'camera': camera_entry(*camera),
            'nodes': nodes_entry(frame, marker),
            'layers': layers,
        }

    def layers(self, matrix, path, renderer_obj, filter_obj):
        renderer = renderer_obj.read_typetree()
        if not renderer.get('m_Enabled', True):
            return []
        geometry = mesh_geometry(self.bundles, self.mesh_handler, filter_obj)
        if geometry is None:
            return []
        positions, uvs, submeshes = geometry
        world = mat_mul(FLIP_Z, matrix)
        found = []
        for index, pointer in enumerate(renderer['m_Materials']):
            if index >= len(submeshes):
                break
            material_obj, _ = self.bundles.resolve(renderer_obj, pointer)
            if material_obj is None:
                continue
            material, scale, offset = material_entry(self.bundles, material_obj)
            if material is None:
                continue
            used = sorted(set(submeshes[index]))
            remap = {old: new for new, old in enumerate(used)}
            entry = {'name': path, **material}
            entry['positions'] = rounded(
                (c for i in used for c in transform_point(world, positions[i])), POSITION_DIGITS
            )
            entry['uvs'] = rounded((c for i in used for c in image_uv(uvs[i], scale, offset)), UV_DIGITS)
            entry['indices'] = [remap[i] for i in submeshes[index]]
            found.append(entry)
        return found


def image_uv(uv, scale, offset):
    u = uv[0] * scale[0] + offset[0]
    v = uv[1] * scale[1] + offset[1]
    return u, 1.0 - v


def camera_entry(matrix, camera):
    world = mat_mul(FLIP_Z, matrix)
    right = transform_direction(world, (1.0, 0.0, 0.0))
    if abs(right[1]) > 1e-3 or abs(right[2]) > 1e-3:
        raise SystemExit(f'map camera right axis {right} does not run along x')
    return {
        'position': rounded(transform_point(world, (0.0, 0.0, 0.0)), POSITION_DIGITS),
        'forward': rounded(transform_direction(world, (0.0, 0.0, 1.0)), MATRIX_DIGITS),
        'up': rounded(transform_direction(world, (0.0, 1.0, 0.0)), MATRIX_DIGITS),
        'fov': round(camera['field of view'], COLOR_DIGITS),
    }


def nodes_entry(frame, marker):
    marker_scale = xyz(marker['m_LocalScale'])
    frame_scale = (
        math.sqrt(sum(frame[r][0] ** 2 for r in range(3))),
        math.sqrt(sum(frame[r][2] ** 2 for r in range(3))),
    )
    return {
        'matrix': column_major(mat_mul(FLIP_Z, frame)),
        'unit': rounded((frame_scale[0] * marker_scale[0], frame_scale[1] * marker_scale[2]), POSITION_DIGITS),
    }


def prefabs():
    return sorted({event['prefab'] for event in json.loads(MAPS.read_text(encoding='utf-8'))})


def build() -> dict:
    unity, mesh_handler = unitypy()
    scenes = [Scene(Bundles(unity, prefab), mesh_handler).build() for prefab in prefabs()]
    return {'version': SCENE_VERSION, 'scenes': scenes}


def serialize(scenes: dict) -> str:
    return json.dumps(scenes, ensure_ascii=False, separators=(',', ':'), sort_keys=True) + '\n'


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument('--check', action='store_true', help='fail when the committed scenes are stale')
    args = parser.parse_args()
    text = serialize(build())
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding='utf-8') != text:
            print(f'{OUTPUT.relative_to(ROOT)} is stale; rerun {RUN_COMMAND}', file=sys.stderr)
            return 1
        return 0
    OUTPUT.write_text(text, encoding='utf-8')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
