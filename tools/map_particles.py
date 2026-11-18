from __future__ import annotations

import math
import random

import map_prefab as prefab

SOFT_ADDITIVE = 'soft_additive'
PARTICLE_SHADERS = {
    'EA/Mobile/Particles/Additive Color': (prefab.ADDITIVE, '_TintColor'),
    'Mobile/Particles/Additive': (prefab.ADDITIVE, None),
    'Mobile/Particles/Alpha Blended': (prefab.ALPHA, None),
    'Particles/Additive': (prefab.ADDITIVE, '_TintColor'),
    'Particles/Additive (Soft)': (SOFT_ADDITIVE, None),
    'Particles/Alpha Blended': (prefab.ALPHA, '_TintColor'),
}
DOUBLED_TINT = 2.0
RENDER_MODES = {0: 'billboard', 1: 'stretched', 2: 'horizontal'}
MESH_RENDER_MODE = 4
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
GRADIENT_TWO_GRADIENTS = 3
UV_SINGLE_ROW = 1
CURVE_SAMPLES = 16
CURVE_TOLERANCE = 1e-3
MESH_SAMPLE_POINTS = 96
MESH_SAMPLE_SEED = 4400
GRAVITY = 9.81
LEGACY_COLOR_KEYS = 5
LEGACY_SIZE_SPAN = 0.5
WHITE = (1.0, 1.0, 1.0, 1.0)


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
    return [prefab.rounded(point) for point in points]


def node_curve(node):
    return curve_pair(*min_max_curves(node))


def constant_range(node):
    low, high = min_max_curves(node)
    values = [value for _, value in low + high]
    return prefab.rounded((min(values), max(values)))


def unpack_rgba(value):
    packed = int(value['rgba']) & 0xFFFFFFFF
    return tuple(((packed >> shift) & 0xFF) / 255.0 for shift in (0, 8, 16, 24))


def gradient_keys(gradient):
    colors = [
        prefab.rounded((gradient[f'ctime{i}'] / 65535.0, *unpack_rgba(gradient[f'key{i}'])[:3]))
        for i in range(gradient['m_NumColorKeys'])
    ]
    alphas = [
        prefab.rounded((gradient[f'atime{i}'] / 65535.0, unpack_rgba(gradient[f'key{i}'])[3]))
        for i in range(gradient['m_NumAlphaKeys'])
    ]
    return {'color': colors, 'alpha': alphas}


def start_colors(node):
    state = node['minMaxState']
    if state == GRADIENT_TWO_COLORS:
        return [prefab.rounded(unpack_rgba(node['minColor'])), prefab.rounded(unpack_rgba(node['maxColor']))]
    if state == GRADIENT_COLOR:
        color = prefab.rounded(unpack_rgba(node['maxColor']))
        return [color, color]
    if state == GRADIENT_GRADIENT:
        return [list(WHITE), list(WHITE)]
    raise SystemExit(f'unsupported start colour mode {state}')


def add_curves(a, b):
    times = sorted({key[0] for key in a} | {key[0] for key in b})

    def at(curve, time, column):
        return sample_linear([(key[0], key[column]) for key in curve], time)

    return [prefab.rounded((time, at(a, time, 1) + at(b, time, 1), at(a, time, 2) + at(b, time, 2))) for time in times]


def triangle_normal(a, b, c):
    ab = [b[i] - a[i] for i in range(3)]
    ac = [c[i] - a[i] for i in range(3)]
    return [ab[1] * ac[2] - ab[2] * ac[1], ab[2] * ac[0] - ab[0] * ac[2], ab[0] * ac[1] - ab[1] * ac[0]]


def triangle_area(a, b, c):
    return 0.5 * math.sqrt(sum(v * v for v in triangle_normal(a, b, c)))


class ParticleScene(prefab.Scene):
    def emitters(self, transform_id, matrix, path, components):
        found = []
        if 'ParticleSystem' in components and 'ParticleSystemRenderer' in components:
            found.append(self.shuriken(transform_id, matrix, path, components))
        if 'EllipsoidParticleEmitter' in components and 'ParticleRenderer' in components:
            found.append(self.legacy(transform_id, matrix, path, components))
        return [emitter for emitter in found if emitter]

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
        colors, _ = prefab.material_properties(material_obj)
        tint = prefab.material_tint(colors, color_key, DOUBLED_TINT if color_key else 1.0)
        return {'texture': texture, 'blend': blend, 'tint': prefab.rounded(tint)}

    def placement(self, transform_id, matrix):
        world = prefab.mat_mul(prefab.FLIP_Z, matrix)
        rotation = prefab.mat_mul(prefab.FLIP_Z, prefab.rotation_matrix(self.rotation_chain(transform_id)))
        return {
            'origin': prefab.rounded(prefab.transform_point(world, (0.0, 0.0, 0.0)), prefab.POSITION_DIGITS),
            'shape_axes': prefab.matrix_columns(world),
            'axes': prefab.matrix_columns(rotation),
        }

    def shuriken(self, transform_id, matrix, path, components):
        system = components['ParticleSystem'].read_typetree()
        renderer = components['ParticleSystemRenderer'].read_typetree()
        initial = system['InitialModule']
        emission = system['EmissionModule']
        if not renderer.get('m_Enabled', True) or not system.get('playOnAwake', True):
            return None
        if renderer['m_RenderMode'] == MESH_RENDER_MODE:
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
            entry['stretch'] = prefab.rounded((renderer['m_LengthScale'], renderer['m_VelocityScale']))
        entry['max_size'] = round(renderer['m_MaxParticleSize'], prefab.VALUE_DIGITS)
        entry['duration'] = round(system['lengthInSec'], prefab.VALUE_DIGITS)
        entry['prewarm'] = bool(system['prewarm'])
        entry['rate'] = round(rate, prefab.VALUE_DIGITS)
        entry['max_particles'] = int(initial['maxNumParticles'])
        entry['lifetime'] = constant_range(initial['startLifetime'])
        entry['speed'] = constant_range(initial['startSpeed'])
        entry['size'] = constant_range(initial['startSize'])
        entry['rotation'] = constant_range(initial['startRotation'])
        entry['colors'] = start_colors(initial['startColor'])
        if initial['startColor']['minMaxState'] == GRADIENT_GRADIENT:
            entry['start_gradient'] = gradient_keys(initial['startColor']['maxGradient'])
        gravity = initial['gravityModifier'] * GRAVITY
        if gravity:
            entry['gravity'] = round(gravity, prefab.VALUE_DIGITS)
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
            over_life = color['gradient']
            if over_life['minMaxState'] not in (GRADIENT_GRADIENT, GRADIENT_TWO_GRADIENTS):
                raise SystemExit(f'{self.bundles.prefab}: {path} colour over lifetime is not a gradient')
            entry['gradient'] = gradient_keys(over_life['maxGradient'])
            if over_life['minMaxState'] == GRADIENT_TWO_GRADIENTS:
                entry['gradient_low'] = gradient_keys(over_life['minGradient'])
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
                'cycles': round(sheet['cycles'], prefab.VALUE_DIGITS),
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
                'size': prefab.rounded((module['boxX'], module['boxY'], module['boxZ'])),
                'random_direction': bool(module['randomDirection']),
            }
        if kind in (SHAPE_CONE, SHAPE_CONE_SHELL, SHAPE_CONE_VOLUME, SHAPE_CONE_VOLUME_SHELL):
            volume = kind in (SHAPE_CONE_VOLUME, SHAPE_CONE_VOLUME_SHELL)
            return {
                'type': 'cone',
                'angle': round(module['angle'], prefab.VALUE_DIGITS),
                'radius': round(module['radius'], prefab.VALUE_DIGITS),
                'length': round(module['length'], prefab.VALUE_DIGITS) if volume else 0.0,
                'random_direction': bool(module['randomDirection']),
            }
        if kind in (SHAPE_SPHERE, SHAPE_SPHERE_SHELL, SHAPE_HEMISPHERE, SHAPE_HEMISPHERE_SHELL):
            return {
                'type': 'sphere',
                'radius': round(module['radius'], prefab.VALUE_DIGITS),
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
        return {'type': 'points', 'points': prefab.rounded(points), 'normals': prefab.rounded(directions)}

    def legacy(self, transform_id, matrix, path, components):
        emitter = components['EllipsoidParticleEmitter'].read_typetree()
        animator = components['ParticleAnimator'].read_typetree() if 'ParticleAnimator' in components else {}
        renderer = components['ParticleRenderer'].read_typetree()
        if not emitter.get('m_Enabled', True) or not emitter.get('m_Emit', True) or not renderer.get('m_Enabled'):
            return None
        material = self.particle_material(components['ParticleRenderer'], renderer)
        if material is None:
            return None
        ellipsoid = prefab.xyz(emitter['m_Ellipsoid'])
        shaped = prefab.mat_mul(matrix, prefab.trs((0.0, 0.0, 0.0), prefab.NO_ROTATION, ellipsoid))
        placement = self.placement(transform_id, matrix)
        placement['shape_axes'] = prefab.matrix_columns(prefab.mat_mul(prefab.FLIP_Z, shaped))
        lifetime = prefab.rounded((emitter['minEnergy'], emitter['maxEnergy']))
        local = prefab.xyz(emitter['localVelocity'])
        spread = prefab.xyz(emitter['rndVelocity'])
        entry = {'name': path, **material, **placement}
        entry['render'] = 'billboard'
        entry['max_size'] = round(renderer['m_MaxParticleSize'], prefab.VALUE_DIGITS)
        entry['duration'] = round(lifetime[1], prefab.VALUE_DIGITS)
        entry['prewarm'] = True
        entry['rate'] = round((emitter['minEmission'] + emitter['maxEmission']) * 0.5, prefab.VALUE_DIGITS)
        entry['max_particles'] = math.ceil(emitter['maxEmission'] * emitter['maxEnergy'])
        entry['lifetime'] = lifetime
        entry['speed'] = [0.0, 0.0]
        entry['size'] = prefab.rounded((emitter['minSize'], emitter['maxSize']))
        entry['rotation'] = [0.0, round(2.0 * math.pi, prefab.VALUE_DIGITS)] if emitter['rndRotation'] else [0.0, 0.0]
        entry['colors'] = [list(WHITE), list(WHITE)]
        entry['shape'] = {'type': 'sphere', 'radius': LEGACY_SIZE_SPAN, 'hemisphere': False, 'shell': False}
        entry['velocity'] = [
            [prefab.rounded((0.0, local[axis] - spread[axis], local[axis] + spread[axis]))] for axis in range(3)
        ]
        entry['velocity_world'] = False
        if animator.get('Does Animate Color?'):
            keys = [unpack_rgba(animator[f'colorAnimation[{i}]']) for i in range(LEGACY_COLOR_KEYS)]
            times = [i / (LEGACY_COLOR_KEYS - 1) for i in range(LEGACY_COLOR_KEYS)]
            entry['gradient'] = {
                'color': [prefab.rounded((time, *key[:3])) for time, key in zip(times, keys)],
                'alpha': [prefab.rounded((time, key[3])) for time, key in zip(times, keys)],
            }
        grow = animator.get('sizeGrow', 0.0)
        if grow:
            grown = 1.0 + grow * (lifetime[0] + lifetime[1]) * 0.5
            entry['size_curve'] = [[0.0, 1.0, 1.0], prefab.rounded((1.0, grown, grown))]
        tiles = renderer.get('UV Animation', {})
        if tiles.get('x Tile', 1) * tiles.get('y Tile', 1) > 1:
            entry['sheet'] = {
                'tiles': [int(tiles['x Tile']), int(tiles['y Tile'])],
                'frame': [[0.0, 0.0, 1.0]],
                'cycles': round(tiles.get('cycles', 0.0), prefab.VALUE_DIGITS),
            }
        return entry
