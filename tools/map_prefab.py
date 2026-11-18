from __future__ import annotations

import json
import math
import re
import zipfile
from pathlib import Path

import original_cache

ROOT = Path(__file__).resolve().parents[1]
APK = ROOT / 'client-additional-data' / 'Heroes-of-Dragon-Age-v4-4-0-androidp1.com.apk'
APK_DATA = 'assets/bin/Data/'
LIVE_ROOT_PREFIX = 'map.'
CAMERA_NODE = 'MapCamera'
SHARED_BUNDLES = ('shared_resourcecontainer', 'shared_shadercontainer')
DEFAULT_CUTOFF = 0.5
PARTICLE_TINT_GAIN = 2.0
POSITION_DIGITS = 4
UV_DIGITS = 5
MATRIX_DIGITS = 5
COLOR_DIGITS = 4
VALUE_DIGITS = 4
SHADER_NAME = re.compile(r'Shader "([^"]+)"')
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
NO_ROTATION = (0.0, 0.0, 0.0, 1.0)


def unitypy(run_command):
    try:
        import UnityPy  # type: ignore
        from UnityPy.helpers.MeshHelper import MeshHandler  # type: ignore
    except ImportError:
        raise SystemExit(f'this builder needs UnityPy; run {run_command}')
    return UnityPy, MeshHandler


def quat_matrix(q):
    x, y, z, w = q
    return (
        (1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)),
        (2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)),
        (2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)),
    )


def quat_mul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (
        aw * bx + ax * bw + ay * bz - az * by,
        aw * by - ax * bz + ay * bw + az * bx,
        aw * bz + ax * by - ay * bx + az * bw,
        aw * bw - ax * bx - ay * by - az * bz,
    )


def rotation_matrix(quaternion):
    basis = quat_matrix(quaternion)
    return tuple(tuple(basis[row]) + (0.0,) for row in range(3)) + ((0.0, 0.0, 0.0, 1.0),)


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


def rounded(values, digits=VALUE_DIGITS):
    return [round(float(value), digits) + 0.0 for value in values]


def column_major(m):
    return rounded((m[r][c] for c in range(4) for r in range(4)), MATRIX_DIGITS)


def matrix_columns(matrix):
    return rounded((matrix[row][column] for column in range(3) for row in range(3)), MATRIX_DIGITS)


def xyz(value):
    return (value['x'], value['y'], value['z'])


def xyzw(value):
    return (value['x'], value['y'], value['z'], value['w'])


def rgba(value):
    return (value['r'], value['g'], value['b'], value['a'])


class ApkLibrary:
    def __init__(self, unity):
        self.unity = unity
        self.archive = zipfile.ZipFile(APK)
        self.members = {
            name[len(APK_DATA) :].lower(): name for name in self.archive.namelist() if name.startswith(APK_DATA)
        }
        self.loaded = {}

    def objects(self, external):
        key = external.lower().removeprefix('library/')
        if key not in self.members:
            return None
        if key not in self.loaded:
            self.loaded[key] = Bundles.objects(self.unity.load(self.archive.read(self.members[key])))
        return self.loaded[key]


class Bundles:
    def __init__(self, unity, prefab, library):
        self.unity = unity
        self.prefab = prefab
        self.library = library
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
            if path_id in self.local:
                return self.local[path_id], self.prefab
            for name, table in self.shared.items():
                if path_id in table:
                    return table[path_id], name
            return None, self.prefab
        external = owner.assets_file.externals[file_id - 1].path
        for name, table in self.shared.items():
            if name in external:
                return table.get(path_id), name
        table = self.library.objects(external)
        if table is not None and path_id in table:
            return table[path_id], external
        return None, (external, path_id)

    def shader(self, material_obj):
        shader, source = self.resolve(material_obj, material_obj.read_typetree()['m_Shader'])
        if shader is None:
            raise SystemExit(f'{self.prefab}: unresolved shader {source} on {material_obj.read_typetree()["m_Name"]}')
        match = SHADER_NAME.search(shader.read_typetree().get('m_Script', ''))
        if not match:
            raise SystemExit(f'{self.prefab}: shader without a name on {material_obj.read_typetree()["m_Name"]}')
        return match.group(1)

    def texture_ref(self, material_obj):
        for key, value in material_obj.read_typetree()['m_SavedProperties']['m_TexEnvs']:
            if key['name'] != '_MainTex':
                continue
            scale = (value['m_Scale']['x'], value['m_Scale']['y'])
            offset = (value['m_Offset']['x'], value['m_Offset']['y'])
            texture_obj, bundle = self.resolve(material_obj, value['m_Texture'])
            if texture_obj is None:
                return None, scale, offset
            ref = f'{original_cache.CACHE_REF_PREFIX}{bundle}/{texture_obj.read_typetree()["m_Name"]}'
            if not original_cache.resolves(ref, original_cache.TEXTURE_2D):
                raise SystemExit(f'{self.prefab}: {ref} is not in the original cache')
            return ref, scale, offset
        return None, (1.0, 1.0), (0.0, 0.0)


def material_properties(material_obj):
    properties = material_obj.read_typetree()['m_SavedProperties']
    colors = {key['name']: rgba(value) for key, value in properties.get('m_Colors', [])}
    floats = {key['name']: value for key, value in properties.get('m_Floats', [])}
    return colors, floats


def material_tint(colors, color_key, gain):
    color = colors.get(color_key, (1.0, 1.0, 1.0, 1.0)) if color_key else (1.0, 1.0, 1.0, 1.0)
    return tuple(gain * channel for channel in color)


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

    def rotation_chain(self, transform_id, root=None):
        chain = []
        while transform_id in self.transforms and transform_id != root:
            chain.append(transform_id)
            transform_id = self.transforms[transform_id]['m_Father']['m_PathID']
        rotation = NO_ROTATION
        for link in reversed(chain):
            rotation = quat_mul(rotation, xyzw(self.transforms[link]['m_LocalRotation']))
        return rotation

    def walk(self, transform_id, matrix, path, active, visit):
        for child in self.transforms[transform_id]['m_Children']:
            child_id = child['m_PathID']
            game_object = self.game_object(child_id)
            child_active = active and game_object.get('m_IsActive', True)
            child_matrix = mat_mul(matrix, self.local(child_id))
            child_path = f'{path}/{game_object["m_Name"]}' if path else game_object['m_Name']
            visit(child_id, child_matrix, child_path, child_active)
            self.walk(child_id, child_matrix, child_path, child_active, visit)


def is_side_mask(layer, camera):
    if layer['texture'] is not None:
        return False
    heights = layer['positions'][1::3]
    rows = layer['positions'][2::3]
    depth = camera['position'][1] - (min(heights) + max(heights)) / 2.0
    half_height = depth * math.tan(math.radians(camera['fov']) / 2.0)
    centre = camera['position'][2]
    return min(rows) <= centre - half_height and max(rows) >= centre + half_height


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


def serialize(scenes: dict) -> str:
    return json.dumps(scenes, ensure_ascii=False, separators=(',', ':'), sort_keys=True) + '\n'
