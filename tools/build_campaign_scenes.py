#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import math
import random
import sys
import zipfile
from pathlib import Path

import build_campaign_node_layout as node_layout
import build_nexus_scenes as nexus

CLI_DESCRIPTION = (
    'Build data/campaign_scenes.json, the campaign map scenes with their layers, wind meshes, particle emitters and '
    'node markers, from the original 4.4.0 map prefabs'
)
ROOT = Path(__file__).resolve().parents[1]
MAPS = ROOT / 'data' / 'campaign_maps.json'
OUTPUT = ROOT / 'data' / 'campaign_scenes.json'
APK = ROOT / 'client-additional-data' / 'Heroes-of-Dragon-Age-v4-4-0-androidp1.com.apk'
APK_DATA = 'assets/bin/Data/'
RUN_COMMAND = 'uv run --offline --with UnityPy python tools/build_campaign_scenes.py'
SCENE_VERSION = 1
TRANSIT_PREFAB = 'map_transit_pre'
TRANSIT_MARKER = 'Transition Object Loc Marker'
NODE_MARKER_PREFIX = 'step'
NODE_PART_CORNERS = ((-1.0, -1.0), (1.0, -1.0), (1.0, 1.0), (-1.0, 1.0))
WIND_SHADER = 'MADFINGER/Environment/Wind'
LATE_TRANSPARENT_SHADER = 'Unlit/Transparent_+100'
LATE_TRANSPARENT_QUEUE = 3100
MESH_BLENDS = {
    **nexus.SHADER_BLENDS,
    'Mobile/Diffuse': (nexus.OPAQUE, None),
    'Unlit/Transparent_+100': (nexus.ALPHA, None),
    'EA/Mobile/Particles/Additive Color': (nexus.ADDITIVE, '_TintColor'),
}
PARTICLE_SHADERS = {
    'EA/Mobile/Particles/Additive Color': (nexus.ADDITIVE, '_TintColor'),
    'Mobile/Particles/Additive': (nexus.ADDITIVE, None),
    'Mobile/Particles/Alpha Blended': (nexus.ALPHA, None),
    'Particles/Alpha Blended': (nexus.ALPHA, '_TintColor'),
}
DOUBLED_TINT = 2.0
RENDER_MODES = {0: 'billboard', 1: 'stretched', 2: 'horizontal'}
SHAPE_SPHERE = 0
SHAPE_SPHERE_SHELL = 1
SHAPE_HEMISPHERE = 2
SHAPE_HEMISPHERE_SHELL = 3
SHAPE_CONE = 4
SHAPE_BOX = 5
SHAPE_MESH = 6
SHAPE_CONE_SHELL = 7
SHAPE_CONE_VOLUME = 8
SHAPE_CONE_VOLUME_SHELL = 9
CURVE_CONSTANT = 0
CURVE_CURVE = 1
CURVE_TWO_CURVES = 2
CURVE_TWO_CONSTANTS = 3
GRADIENT_COLOR = 0
GRADIENT_GRADIENT = 1
GRADIENT_TWO_COLORS = 2
UV_SINGLE_ROW = 1
CURVE_SAMPLES = 16
CURVE_TOLERANCE = 1e-3
MESH_SAMPLE_POINTS = 96
MESH_SAMPLE_SEED = 4400
GRAVITY = 9.81
LEGACY_COLOR_KEYS = 5
LEGACY_SIZE_SPAN = 0.5
COLOR_BYTE = 255.0
UNIFORM_SCALE_TOLERANCE = 1e-4
POSITION_DIGITS = 4
VALUE_DIGITS = 4


def rounded(values, digits=VALUE_DIGITS):
    return [round(float(value), digits) + 0.0 for value in values]


def matrix_columns(matrix):
    return rounded((matrix[row][column] for column in range(3) for row in range(3)), nexus.MATRIX_DIGITS)


def rotation_chain(scene, transform_id):
    rotation = (0.0, 0.0, 0.0, 1.0)
    chain = []
    while transform_id in scene.transforms:
        chain.append(transform_id)
        transform_id = scene.transforms[transform_id]['m_Father']['m_PathID']
    for link in reversed(chain):
        rotation = quat_mul(rotation, nexus.xyzw(scene.transforms[link]['m_LocalRotation']))
    return rotation


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
    basis = nexus.quat_matrix(quaternion)
    return tuple(tuple(basis[row]) + (0.0,) for row in range(3)) + ((0.0, 0.0, 0.0, 1.0),)


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
            self.loaded[key] = nexus.Bundles.objects(self.unity.load(self.archive.read(self.members[key])))
        return self.loaded[key]


class Bundles(nexus.Bundles):
    def __init__(self, unity, prefab, library):
        super().__init__(unity, prefab)
        self.library = library

    def resolve(self, owner, pointer):
        found, source = super().resolve(owner, pointer)
        if found is not None or pointer['m_PathID'] == 0:
            return found, source
        path_id = pointer['m_PathID']
        if pointer['m_FileID'] == 0:
            for name, table in self.shared.items():
                if path_id in table:
                    return table[path_id], name
            return None, source
        external = owner.assets_file.externals[pointer['m_FileID'] - 1].path
        table = self.library.objects(external)
        if table is not None and path_id in table:
            return table[path_id], external
        return None, source

    def shader(self, material_obj):
        shader, source = self.resolve(material_obj, material_obj.read_typetree()['m_Shader'])
        if shader is None:
            raise SystemExit(f'{self.prefab}: unresolved shader {source} on {material_obj.read_typetree()["m_Name"]}')
        match = nexus.SHADER_NAME.search(shader.read_typetree().get('m_Script', ''))
        if not match:
            raise SystemExit(f'{self.prefab}: shader without a name on {material_obj.read_typetree()["m_Name"]}')
        return match.group(1)

    def texture_ref(self, material_obj):
        for key, value in material_obj.read_typetree()['m_SavedProperties']['m_TexEnvs']:
            if key['name'] != '_MainTex':
                continue
            texture_obj, bundle = self.resolve(material_obj, value['m_Texture'])
            if texture_obj is None:
                return None, (1.0, 1.0), (0.0, 0.0)
            ref = f'{nexus.original_cache.CACHE_REF_PREFIX}{bundle}/{texture_obj.read_typetree()["m_Name"]}'
            if not nexus.original_cache.resolves(ref, nexus.original_cache.TEXTURE_2D):
                raise SystemExit(f'{self.prefab}: {ref} is not in the original cache')
            scale = (value['m_Scale']['x'], value['m_Scale']['y'])
            offset = (value['m_Offset']['x'], value['m_Offset']['y'])
            return ref, scale, offset
        return None, (1.0, 1.0), (0.0, 0.0)


def material_properties(material_obj):
    properties = material_obj.read_typetree()['m_SavedProperties']
    colors = {key['name']: nexus.rgba(value) for key, value in properties.get('m_Colors', [])}
    floats = {key['name']: value for key, value in properties.get('m_Floats', [])}
    return colors, floats


def material_tint(colors, color_key, gain):
    color = colors.get(color_key, (1.0, 1.0, 1.0, 1.0)) if color_key else (1.0, 1.0, 1.0, 1.0)
    return tuple(gain * channel for channel in color)


def hermite(keys, time):
    if time <= keys[0]['time']:
        return keys[0]['value']
    if time >= keys[-1]['time']:
        return keys[-1]['value']
    for left, right in zip(keys, keys[1:]):
        if time > right['time']:
            continue
        span = right['time'] - left['time']
        if span <= 0.0 or not math.isfinite(left['outSlope']) or not math.isfinite(right['inSlope']):
            return left['value']
        s = (time - left['time']) / span
        return (
            (2 * s**3 - 3 * s**2 + 1) * left['value']
            + (s**3 - 2 * s**2 + s) * span * left['outSlope']
            + (-2 * s**3 + 3 * s**2) * right['value']
            + (s**3 - s**2) * span * right['inSlope']
        )
    return keys[-1]['value']


def curve_samples(keys, scalar):
    if not keys:
        return [(0.0, 0.0)]
    if len(keys) == 1 or all(abs(key['value'] - keys[0]['value']) < 1e-9 for key in keys):
        return [(0.0, scalar * keys[0]['value'])]
    return [(step / CURVE_SAMPLES, scalar * hermite(keys, step / CURVE_SAMPLES)) for step in range(CURVE_SAMPLES + 1)]


def min_max_curves(node):
    state = node['minMaxState']
    scalar = node['scalar']
    high = node['maxCurve']['m_Curve']
    low = node['minCurve']['m_Curve']
    if state == CURVE_CONSTANT:
        return [(0.0, scalar)], [(0.0, scalar)]
    if state == CURVE_CURVE:
        samples = curve_samples(high, scalar)
        return samples, samples
    if state == CURVE_TWO_CONSTANTS:
        return [(0.0, scalar * low[0]['value'])], [(0.0, scalar * high[0]['value'])]
    if state == CURVE_TWO_CURVES:
        return curve_samples(low, scalar), curve_samples(high, scalar)
    raise SystemExit(f'unsupported curve mode {state}')


def sample_linear(samples, time):
    if time <= samples[0][0]:
        return samples[0][1]
    for (t0, v0), (t1, v1) in zip(samples, samples[1:]):
        if time <= t1:
            return v0 + (v1 - v0) * (time - t0) / (t1 - t0)
    return samples[-1][1]


def simplified(points):
    kept = [points[0]]
    for index in range(1, len(points) - 1):
        candidate = kept + [points[index + 1]]
        if all(
            abs(sample_linear(candidate, point[0]) - point[1]) <= CURVE_TOLERANCE * max(1.0, abs(point[1]))
            for point in points[len(kept) - 1 : index + 2]
        ):
            continue
        kept.append(points[index])
    if len(points) > 1:
        kept.append(points[-1])
    return kept


def curve_pair(low, high):
    times = sorted({time for time, _ in low} | {time for time, _ in high})
    points = [(time, sample_linear(low, time), sample_linear(high, time)) for time in times]
    if all(abs(point[1] - points[0][1]) < 1e-9 and abs(point[2] - points[0][2]) < 1e-9 for point in points):
        points = points[:1]
    if len(points) > 2:
        lows = simplified([(time, lo) for time, lo, _ in points])
        highs = simplified([(time, hi) for time, _, hi in points])
        keep = {time for time, _ in lows} | {time for time, _ in highs}
        points = [point for point in points if point[0] in keep]
    return [rounded(point) for point in points]


def node_curve(node):
    return curve_pair(*min_max_curves(node))


def constant_range(node):
    low, high = min_max_curves(node)
    values = [value for _, value in low + high]
    return rounded((min(values), max(values)))


def unpack_rgba(value):
    packed = int(value['rgba']) & 0xFFFFFFFF
    return tuple(((packed >> shift) & 0xFF) / 255.0 for shift in (0, 8, 16, 24))


def gradient_keys(gradient):
    colors = [
        rounded((gradient[f'ctime{i}'] / 65535.0, *unpack_rgba(gradient[f'key{i}'])[:3]))
        for i in range(gradient['m_NumColorKeys'])
    ]
    alphas = [
        rounded((gradient[f'atime{i}'] / 65535.0, unpack_rgba(gradient[f'key{i}'])[3]))
        for i in range(gradient['m_NumAlphaKeys'])
    ]
    return {'color': colors, 'alpha': alphas}


def start_colors(node):
    state = node['minMaxState']
    if state == GRADIENT_TWO_COLORS:
        return [rounded(unpack_rgba(node['minColor'])), rounded(unpack_rgba(node['maxColor']))]
    if state == GRADIENT_COLOR:
        color = rounded(unpack_rgba(node['maxColor']))
        return [color, color]
    raise SystemExit(f'unsupported start colour mode {state}')


class MapScene(nexus.Scene):
    def __init__(self, bundles, mesh_handler, transit):
        super().__init__(bundles, mesh_handler)
        self.transit = transit

    def build_campaign(self, parts):
        root = self.live_root()
        layers, winds, emitters, markers, transit_marks = [], [], [], {}, []
        camera = None

        def visit(transform_id, matrix, path, active):
            nonlocal camera
            game_object = self.game_object(transform_id)
            name = game_object['m_Name']
            components = self.components(game_object)
            if name == nexus.CAMERA_NODE and 'Camera' in components:
                camera = (matrix, components['Camera'].read_typetree())
            if name.startswith(NODE_MARKER_PREFIX) and self.father_name(transform_id) == node_layout.NODE_MARKERS:
                markers[int(name[len(NODE_MARKER_PREFIX) :])] = matrix
            if name == TRANSIT_MARKER:
                transit_marks.append((matrix, rotation_chain(self, transform_id)))
            if active:
                self.collect(transform_id, matrix, path, components, layers, winds, emitters)

        self.walk(root, nexus.IDENTITY, '', True, visit)
        if camera is None or not markers:
            raise SystemExit(f'{self.bundles.prefab}: missing {nexus.CAMERA_NODE} or node markers')
        if sorted(markers) != list(range(1, len(markers) + 1)):
            raise SystemExit(f'{self.bundles.prefab}: node markers are not numbered 1..{len(markers)}')
        for matrix, rotation in transit_marks:
            self.transit.place(matrix, rotation, layers, winds, emitters)
        view = nexus.camera_entry(*camera)
        return {
            'prefab': self.bundles.prefab,
            'camera': view,
            'nodes': [node_entry(markers[step], parts) for step in sorted(markers)],
            'layers': [layer for layer in layers if not nexus.is_side_mask(layer, view)],
            'winds': winds,
            'emitters': emitters,
        }

    def father_name(self, transform_id):
        father = self.transforms[transform_id]['m_Father']['m_PathID']
        return self.name(father) if father in self.transforms else ''

    def collect(self, transform_id, matrix, path, components, layers, winds, emitters):
        if 'MeshRenderer' in components and 'MeshFilter' in components:
            self.mesh_layers(
                transform_id, matrix, path, components['MeshRenderer'], components['MeshFilter'], layers, winds
            )
        if 'ParticleSystem' in components and 'ParticleSystemRenderer' in components:
            emitter = self.shuriken(transform_id, matrix, path, components)
            if emitter:
                emitters.append(emitter)
        if 'EllipsoidParticleEmitter' in components and 'ParticleRenderer' in components:
            emitter = self.legacy(transform_id, matrix, path, components)
            if emitter:
                emitters.append(emitter)

    def mesh_layers(self, transform_id, matrix, path, renderer_obj, filter_obj, layers, winds):
        renderer = renderer_obj.read_typetree()
        if not renderer.get('m_Enabled', True):
            return
        mesh = self.mesh(filter_obj)
        if mesh is None:
            return
        world = nexus.mat_mul(nexus.FLIP_Z, matrix)
        for index, pointer in enumerate(renderer['m_Materials']):
            if index >= len(mesh['submeshes']):
                break
            material_obj, _ = self.bundles.resolve(renderer_obj, pointer)
            if material_obj is None:
                continue
            shader = self.bundles.shader(material_obj)
            texture, scale, offset = self.bundles.texture_ref(material_obj)
            colors, floats = material_properties(material_obj)
            triangles = mesh['submeshes'][index]
            used = sorted(set(triangles))
            remap = {old: new for new, old in enumerate(used)}
            uvs = rounded(
                (c for i in used for c in nexus.image_uv(mesh['uvs'][i], scale, offset)), nexus.UV_DIGITS
            )
            indices = [remap[i] for i in triangles]
            if shader == WIND_SHADER:
                placed = renderer_placement(matrix, rotation_chain(self, transform_id))
                winds.append(wind_entry(path, placed, mesh, used, uvs, indices, texture, colors, floats))
                continue
            if shader not in MESH_BLENDS:
                raise SystemExit(f'{self.bundles.prefab}: no blend rule for shader {shader} on {path}')
            blend, color_key = MESH_BLENDS[shader]
            if texture is None and color_key is None:
                continue
            gain = nexus.PARTICLE_TINT_GAIN if color_key == '_TintColor' else 1.0
            color = tuple(min(1.0, c) for c in material_tint(colors, color_key, gain))
            entry = {'name': path, 'blend': blend, 'color': rounded(color, nexus.COLOR_DIGITS), 'texture': texture}
            if blend == nexus.CUTOUT:
                entry['cutoff'] = round(floats.get('_Cutoff', nexus.DEFAULT_CUTOFF), nexus.COLOR_DIGITS)
            if shader == LATE_TRANSPARENT_SHADER:
                entry['queue'] = LATE_TRANSPARENT_QUEUE
            entry['positions'] = rounded(
                (c for i in used for c in nexus.transform_point(world, mesh['positions'][i])), POSITION_DIGITS
            )
            entry['uvs'] = uvs
            entry['indices'] = indices
            layers.append(entry)

    def mesh(self, filter_obj):
        pointer = filter_obj.read_typetree()['m_Mesh']
        mesh_obj, source = self.bundles.resolve(filter_obj, pointer)
        if mesh_obj is None:
            if isinstance(source, tuple) and nexus.DEFAULT_RESOURCES in source[0] and source[1] == nexus.BUILTIN_QUAD:
                return {
                    'positions': nexus.QUAD_POSITIONS,
                    'uvs': nexus.QUAD_UVS,
                    'normals': [(0.0, 0.0, -1.0)] * 4,
                    'colors': [],
                    'submeshes': [list(nexus.QUAD_TRIANGLES)],
                }
            return None
        handler = self.mesh_handler(mesh_obj.read())
        handler.process()
        return {
            'positions': handler.m_Vertices,
            'uvs': handler.m_UV0,
            'normals': handler.m_Normals or [],
            'colors': handler.m_Colors or [],
            'submeshes': [list(sum(map(list, sub), [])) for sub in handler.get_triangles()],
        }

    def particle_material(self, renderer_obj, renderer):
        materials = renderer['m_Materials']
        if not materials:
            return None
        material_obj, _ = self.bundles.resolve(renderer_obj, materials[0])
        if material_obj is None:
            return None
        shader = self.bundles.shader(material_obj)
        if shader not in PARTICLE_SHADERS:
            raise SystemExit(f'{self.bundles.prefab}: no particle rule for shader {shader}')
        blend, color_key = PARTICLE_SHADERS[shader]
        texture, _, _ = self.bundles.texture_ref(material_obj)
        if texture is None:
            return None
        colors, _ = material_properties(material_obj)
        tint = material_tint(colors, color_key, DOUBLED_TINT if color_key else 1.0)
        return {'texture': texture, 'blend': blend, 'tint': rounded(tint)}

    def placement(self, transform_id, matrix):
        world = nexus.mat_mul(nexus.FLIP_Z, matrix)
        rotation = nexus.mat_mul(nexus.FLIP_Z, rotation_matrix(rotation_chain(self, transform_id)))
        return {
            'origin': rounded(nexus.transform_point(world, (0.0, 0.0, 0.0)), POSITION_DIGITS),
            'shape_axes': matrix_columns(world),
            'axes': matrix_columns(rotation),
        }

    def shuriken(self, transform_id, matrix, path, components):
        system = components['ParticleSystem'].read_typetree()
        renderer = components['ParticleSystemRenderer'].read_typetree()
        initial = system['InitialModule']
        emission = system['EmissionModule']
        if not renderer.get('m_Enabled', True) or not system.get('playOnAwake', True):
            return None
        if not emission.get('enabled', True) or initial['maxNumParticles'] <= 0:
            return None
        material = self.particle_material(components['ParticleSystemRenderer'], renderer)
        if material is None:
            return None
        rate = constant_range(emission['rate'])[1]
        if rate <= 0.0:
            return None
        entry = {'name': path, **material, **self.placement(transform_id, matrix)}
        if renderer['m_RenderMode'] not in RENDER_MODES:
            raise SystemExit(f'{self.bundles.prefab}: {path} uses unsupported render mode {renderer["m_RenderMode"]}')
        mode = RENDER_MODES[renderer['m_RenderMode']]
        entry['render'] = mode
        if mode == 'stretched':
            entry['stretch'] = rounded((renderer['m_LengthScale'], renderer['m_VelocityScale']))
        entry['max_size'] = round(renderer['m_MaxParticleSize'], VALUE_DIGITS)
        entry['duration'] = round(system['lengthInSec'], VALUE_DIGITS)
        entry['prewarm'] = bool(system['prewarm'])
        entry['rate'] = round(rate, VALUE_DIGITS)
        entry['max_particles'] = int(initial['maxNumParticles'])
        entry['lifetime'] = constant_range(initial['startLifetime'])
        entry['speed'] = constant_range(initial['startSpeed'])
        entry['size'] = constant_range(initial['startSize'])
        entry['rotation'] = constant_range(initial['startRotation'])
        entry['colors'] = start_colors(initial['startColor'])
        gravity = initial['gravityModifier'] * GRAVITY
        if gravity:
            entry['gravity'] = round(gravity, VALUE_DIGITS)
        entry['shape'] = self.shape(system['ShapeModule'], components['ParticleSystem'])
        spin = [[0.0, 0.0, 0.0]]
        rotation_module = system['RotationModule']
        if rotation_module.get('enabled'):
            spin = node_curve(rotation_module['curve'])
        by_speed = system.get('RotationBySpeedModule', {})
        if by_speed.get('enabled'):
            spin = add_curves(spin, node_curve(by_speed['curve']))
        if any(value for key in spin for value in key[1:]):
            entry['spin'] = spin
        velocity = system['VelocityModule']
        if velocity.get('enabled'):
            entry['velocity'] = [node_curve(velocity[axis]) for axis in 'xyz']
            entry['velocity_world'] = bool(velocity['inWorldSpace'])
        force = system['ForceModule']
        if force.get('enabled'):
            entry['force'] = [node_curve(force[axis]) for axis in 'xyz']
            entry['force_world'] = bool(force['inWorldSpace'])
        color = system['ColorModule']
        if color.get('enabled'):
            if color['gradient']['minMaxState'] != GRADIENT_GRADIENT:
                raise SystemExit(f'{self.bundles.prefab}: {path} colour over lifetime is not a gradient')
            entry['gradient'] = gradient_keys(color['gradient']['maxGradient'])
        size = system['SizeModule']
        if size.get('enabled'):
            entry['size_curve'] = node_curve(size['curve'])
        sheet = system['UVModule']
        if sheet.get('enabled'):
            entry['sheet'] = {
                'tiles': [int(sheet['tilesX']), int(sheet['tilesY'])],
                'row': (-1 if sheet['randomRow'] else int(sheet['rowIndex']))
                if sheet['animationType'] == UV_SINGLE_ROW
                else None,
                'frame': node_curve(sheet['frameOverTime']),
                'cycles': round(sheet['cycles'], VALUE_DIGITS),
            }
            if entry['sheet']['row'] is None:
                del entry['sheet']['row']
        return entry

    def shape(self, module, system_obj):
        if not module.get('enabled'):
            return {'type': 'point'}
        kind = module['type']
        if kind == SHAPE_BOX:
            return {
                'type': 'box',
                'size': rounded((module['boxX'], module['boxY'], module['boxZ'])),
                'random_direction': bool(module['randomDirection']),
            }
        if kind in (SHAPE_CONE, SHAPE_CONE_SHELL, SHAPE_CONE_VOLUME, SHAPE_CONE_VOLUME_SHELL):
            volume = kind in (SHAPE_CONE_VOLUME, SHAPE_CONE_VOLUME_SHELL)
            return {
                'type': 'cone',
                'angle': round(module['angle'], VALUE_DIGITS),
                'radius': round(module['radius'], VALUE_DIGITS),
                'length': round(module['length'], VALUE_DIGITS) if volume else 0.0,
                'random_direction': bool(module['randomDirection']),
            }
        if kind in (SHAPE_SPHERE, SHAPE_SPHERE_SHELL, SHAPE_HEMISPHERE, SHAPE_HEMISPHERE_SHELL):
            return {
                'type': 'sphere',
                'radius': round(module['radius'], VALUE_DIGITS),
                'hemisphere': kind in (SHAPE_HEMISPHERE, SHAPE_HEMISPHERE_SHELL),
                'shell': kind in (SHAPE_SPHERE_SHELL, SHAPE_HEMISPHERE_SHELL),
            }
        if kind == SHAPE_MESH:
            return self.mesh_shape(module, system_obj)
        raise SystemExit(f'{self.bundles.prefab}: unsupported particle shape {kind}')

    def mesh_shape(self, module, system_obj):
        mesh_obj, _ = self.bundles.resolve(system_obj, module['m_Mesh'])
        if mesh_obj is None:
            return {'type': 'point'}
        handler = self.mesh_handler(mesh_obj.read())
        handler.process()
        positions, normals = handler.m_Vertices, handler.m_Normals
        triangles = [tuple(triangle) for sub in handler.get_triangles() for triangle in sub]
        areas = [triangle_area(*(positions[i] for i in triangle)) for triangle in triangles]
        chooser = random.Random(MESH_SAMPLE_SEED)
        points, directions = [], []
        for triangle in chooser.choices(triangles, weights=areas, k=MESH_SAMPLE_POINTS):
            a, b = chooser.random(), chooser.random()
            if a + b > 1.0:
                a, b = 1.0 - a, 1.0 - b
            weights = (1.0 - a - b, a, b)
            points.extend(sum(weights[k] * positions[triangle[k]][axis] for k in range(3)) for axis in range(3))
            if normals:
                normal = [sum(weights[k] * normals[triangle[k]][axis] for k in range(3)) for axis in range(3)]
            else:
                normal = triangle_normal(*(positions[i] for i in triangle))
            length = math.sqrt(sum(c * c for c in normal)) or 1.0
            directions.extend(c / length for c in normal)
        return {'type': 'points', 'points': rounded(points), 'normals': rounded(directions)}

    def legacy(self, transform_id, matrix, path, components):
        emitter = components['EllipsoidParticleEmitter'].read_typetree()
        animator = components['ParticleAnimator'].read_typetree() if 'ParticleAnimator' in components else {}
        renderer = components['ParticleRenderer'].read_typetree()
        if not emitter.get('m_Enabled', True) or not emitter.get('m_Emit', True) or not renderer.get('m_Enabled'):
            return None
        material = self.particle_material(components['ParticleRenderer'], renderer)
        if material is None:
            return None
        ellipsoid = nexus.xyz(emitter['m_Ellipsoid'])
        shaped = nexus.mat_mul(matrix, nexus.trs((0.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0), ellipsoid))
        placement = self.placement(transform_id, matrix)
        placement['shape_axes'] = matrix_columns(nexus.mat_mul(nexus.FLIP_Z, shaped))
        lifetime = rounded((emitter['minEnergy'], emitter['maxEnergy']))
        local = nexus.xyz(emitter['localVelocity'])
        spread = nexus.xyz(emitter['rndVelocity'])
        entry = {'name': path, **material, **placement}
        entry['render'] = 'billboard'
        entry['max_size'] = round(renderer['m_MaxParticleSize'], VALUE_DIGITS)
        entry['duration'] = round(lifetime[1], VALUE_DIGITS)
        entry['prewarm'] = True
        entry['rate'] = round((emitter['minEmission'] + emitter['maxEmission']) * 0.5, VALUE_DIGITS)
        entry['max_particles'] = math.ceil(emitter['maxEmission'] * emitter['maxEnergy'])
        entry['lifetime'] = lifetime
        entry['speed'] = [0.0, 0.0]
        entry['size'] = rounded((emitter['minSize'], emitter['maxSize']))
        entry['rotation'] = [0.0, round(2.0 * math.pi, VALUE_DIGITS)] if emitter['rndRotation'] else [0.0, 0.0]
        entry['colors'] = [[1.0, 1.0, 1.0, 1.0], [1.0, 1.0, 1.0, 1.0]]
        entry['shape'] = {'type': 'sphere', 'radius': LEGACY_SIZE_SPAN, 'hemisphere': False, 'shell': False}
        entry['velocity'] = [
            [rounded((0.0, local[axis] - spread[axis], local[axis] + spread[axis]))] for axis in range(3)
        ]
        entry['velocity_world'] = False
        if animator.get('Does Animate Color?'):
            keys = [unpack_rgba(animator[f'colorAnimation[{i}]']) for i in range(LEGACY_COLOR_KEYS)]
            times = [i / (LEGACY_COLOR_KEYS - 1) for i in range(LEGACY_COLOR_KEYS)]
            entry['gradient'] = {
                'color': [rounded((time, *key[:3])) for time, key in zip(times, keys)],
                'alpha': [rounded((time, key[3])) for time, key in zip(times, keys)],
            }
        grow = animator.get('sizeGrow', 0.0)
        if grow:
            average_life = (lifetime[0] + lifetime[1]) * 0.5
            entry['size_curve'] = [[0.0, 1.0, 1.0], rounded((1.0, 1.0 + grow * average_life, 1.0 + grow * average_life))]
        tiles = renderer.get('UV Animation', {})
        if tiles.get('x Tile', 1) * tiles.get('y Tile', 1) > 1:
            entry['sheet'] = {
                'tiles': [int(tiles['x Tile']), int(tiles['y Tile'])],
                'frame': [[0.0, 0.0, 1.0]],
                'cycles': round(tiles.get('cycles', 0.0), VALUE_DIGITS),
            }
        return entry


def add_curves(a, b):
    times = sorted({key[0] for key in a} | {key[0] for key in b})

    def at(curve, time, column):
        return sample_linear([(key[0], key[column]) for key in curve], time)

    return [rounded((time, at(a, time, 1) + at(b, time, 1), at(a, time, 2) + at(b, time, 2))) for time in times]


def triangle_normal(a, b, c):
    ab = [b[i] - a[i] for i in range(3)]
    ac = [c[i] - a[i] for i in range(3)]
    return [ab[1] * ac[2] - ab[2] * ac[1], ab[2] * ac[0] - ab[0] * ac[2], ab[0] * ac[1] - ab[1] * ac[0]]


def triangle_area(a, b, c):
    return 0.5 * math.sqrt(sum(v * v for v in triangle_normal(a, b, c)))


def linear_part(matrix):
    return tuple(tuple(matrix[row][column] for column in range(3)) for row in range(3))


def uniformly_scaled(matrix):
    lengths = [math.sqrt(sum(matrix[row][column] ** 2 for row in range(3))) for column in range(3)]
    determinant = sum(
        matrix[0][column] * (matrix[1][(column + 1) % 3] * matrix[2][(column + 2) % 3]
        - matrix[1][(column + 2) % 3] * matrix[2][(column + 1) % 3])
        for column in range(3)
    )
    return determinant > 0.0 and max(lengths) - min(lengths) <= UNIFORM_SCALE_TOLERANCE * max(lengths)


def renderer_placement(matrix, rotation):
    if uniformly_scaled(matrix):
        return matrix, nexus.IDENTITY
    origin = nexus.transform_point(matrix, (0.0, 0.0, 0.0))
    basis = rotation_matrix(rotation)
    model = tuple(tuple(basis[row][:3]) + (origin[row],) for row in range(3)) + ((0.0, 0.0, 0.0, 1.0),)
    return model, nexus.mat_mul(node_layout.invert_affine(model), matrix)


def transformed_normal(baked, normal):
    inverse = linear_part(node_layout.invert_affine(baked))
    turned = [sum(inverse[k][row] * normal[k] for k in range(3)) for row in range(3)]
    length = math.sqrt(sum(c * c for c in turned)) or 1.0
    return [c / length for c in turned]


def wind_entry(path, placed, mesh, used, uvs, indices, texture, colors, floats):
    model, baked = placed
    inverse = node_layout.invert_affine(model)
    wind = colors.get('_Wind', (1.0, 1.0, 1.0, 1.0))
    local_wind = tuple(sum(inverse[row][k] * wind[k] for k in range(3)) for row in range(3))
    origin = nexus.transform_point(model, (0.0, 0.0, 0.0))
    vertex_colors = [
        tuple(channel / COLOR_BYTE for channel in color) for color in mesh['colors']
    ] or [(1.0, 1.0, 1.0, 1.0)] * len(mesh['positions'])
    normals = mesh['normals'] or [(0.0, 1.0, 0.0)] * len(mesh['positions'])
    return {
        'name': path,
        'texture': texture,
        'matrix': nexus.column_major(nexus.mat_mul(nexus.FLIP_Z, model)),
        'positions': rounded(
            (c for i in used for c in nexus.transform_point(baked, mesh['positions'][i])), POSITION_DIGITS
        ),
        'normals': rounded((c for i in used for c in transformed_normal(baked, normals[i])), POSITION_DIGITS),
        'colors': rounded((c for i in used for c in vertex_colors[i]), nexus.COLOR_DIGITS),
        'uvs': uvs,
        'indices': indices,
        'wind': rounded((*local_wind, wind[3])),
        'phase': round(sum(origin), VALUE_DIGITS),
        'flutter': round(floats.get('_WindEdgeFlutter', 0.5), VALUE_DIGITS),
        'flutter_frequency': round(floats.get('_WindEdgeFlutterFreqScale', 0.5), VALUE_DIGITS),
        'color_scale': round(floats.get('_VertColorScale', 1.0), VALUE_DIGITS),
        'color_shift': round(floats.get('_VertColorShift', 0.0), VALUE_DIGITS),
    }


def node_entry(marker, parts):
    centre = nexus.transform_point(nexus.mat_mul(nexus.FLIP_Z, marker), (0.0, 0.0, 0.0))
    entry = {'position': rounded(centre, POSITION_DIGITS)}
    for key, part in parts.items():
        world = nexus.mat_mul(nexus.FLIP_Z, nexus.mat_mul(marker, part))
        if key == 'text':
            point = nexus.transform_point(world, (0.0, 0.0, 0.0))
            entry[key] = rounded((point[i] - centre[i] for i in range(3)), POSITION_DIGITS)
            continue
        corners = [
            nexus.transform_point(
                world, (sx * node_layout.UNITY_PLANE_HALF_EXTENT, 0.0, sz * node_layout.UNITY_PLANE_HALF_EXTENT)
            )
            for sx, sz in NODE_PART_CORNERS
        ]
        entry[key] = rounded((corner[i] - centre[i] for corner in corners for i in range(3)), POSITION_DIGITS)
    return entry


class Transit:
    def __init__(self, unity, mesh_handler, library):
        self.scene = MapScene(Bundles(unity, TRANSIT_PREFAB, library), mesh_handler, None)
        self.root = self.scene.live_root()
        root = self.scene.transforms[self.root]
        self.root_scale = nexus.xyz(root['m_LocalScale'])

    def place(self, marker, marker_rotation, layers, winds, emitters):
        start = nexus.mat_mul(marker, nexus.trs((0.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0), self.root_scale))

        def visit(transform_id, matrix, path, active):
            if not active:
                return
            components = self.scene.components(self.scene.game_object(transform_id))
            found = []
            self.scene.collect(transform_id, matrix, f'{TRANSIT_PREFAB}/{path}', components, layers, winds, found)
            for emitter in found:
                axes = nexus.mat_mul(
                    nexus.FLIP_Z,
                    rotation_matrix(quat_mul(marker_rotation, rotation_chain_below(self.scene, transform_id, self.root))),
                )
                emitter['axes'] = matrix_columns(axes)
                emitters.append(emitter)

        self.scene.walk(self.root, start, '', True, visit)


def rotation_chain_below(scene, transform_id, root):
    rotation = (0.0, 0.0, 0.0, 1.0)
    chain = []
    while transform_id in scene.transforms and transform_id != root:
        chain.append(transform_id)
        transform_id = scene.transforms[transform_id]['m_Father']['m_PathID']
    for link in reversed(chain):
        rotation = quat_mul(rotation, nexus.xyzw(scene.transforms[link]['m_LocalRotation']))
    return rotation


def build() -> dict:
    unity, mesh_handler = nexus.unitypy()
    library = ApkLibrary(unity)
    transit = Transit(unity, mesh_handler, library)
    parts = node_layout.node_parts(node_layout.Scene(node_layout.Bundle(node_layout.NODE_PREFAB)))
    scenes = []
    for chapter in json.loads(MAPS.read_text(encoding='utf-8')):
        prefab = node_layout.map_prefab(chapter['num'])
        scene = MapScene(Bundles(unity, prefab, library), mesh_handler, transit).build_campaign(parts)
        if len(scene['nodes']) != chapter['node_count']:
            raise SystemExit(f"{chapter['id']}: {prefab} has {len(scene['nodes'])} node markers, not {chapter['node_count']}")
        scenes.append({'map': chapter['id'], **scene})
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
