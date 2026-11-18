#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

import build_campaign_node_layout as node_layout
import map_particles as particles
import map_prefab as prefab

CLI_DESCRIPTION = (
    'Build data/campaign_scenes.json, the campaign map scenes with their layers, wind meshes, particle emitters and '
    'node markers, from the original 4.4.0 map prefabs'
)
ROOT = Path(__file__).resolve().parents[1]
MAPS = ROOT / 'data' / 'campaign_maps.json'
OUTPUT = ROOT / 'data' / 'campaign_scenes.json'
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
    **prefab.SHADER_BLENDS,
    'Mobile/Diffuse': (prefab.OPAQUE, None),
    'Unlit/Transparent_+100': (prefab.ALPHA, None),
    'EA/Mobile/Particles/Additive Color': (prefab.ADDITIVE, '_TintColor'),
}
COLOR_BYTE = 255.0
UNIFORM_SCALE_TOLERANCE = 1e-4


class MapScene(particles.ParticleScene):
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
            if name == prefab.CAMERA_NODE and 'Camera' in components:
                camera = (matrix, components['Camera'].read_typetree())
            if name.startswith(NODE_MARKER_PREFIX) and self.father_name(transform_id) == node_layout.NODE_MARKERS:
                markers[int(name[len(NODE_MARKER_PREFIX) :])] = matrix
            if name == TRANSIT_MARKER:
                transit_marks.append((matrix, self.rotation_chain(transform_id)))
            if active:
                self.collect(transform_id, matrix, path, components, layers, winds, emitters)

        self.walk(root, prefab.IDENTITY, '', True, visit)
        if camera is None or not markers:
            raise SystemExit(f'{self.bundles.prefab}: missing {prefab.CAMERA_NODE} or node markers')
        if sorted(markers) != list(range(1, len(markers) + 1)):
            raise SystemExit(f'{self.bundles.prefab}: node markers are not numbered 1..{len(markers)}')
        for matrix, rotation in transit_marks:
            self.transit.place(matrix, rotation, layers, winds, emitters)
        view = prefab.camera_entry(*camera)
        return {
            'prefab': self.bundles.prefab,
            'camera': view,
            'nodes': [node_entry(markers[step], parts) for step in sorted(markers)],
            'layers': [layer for layer in layers if not prefab.is_side_mask(layer, view)],
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
        emitters.extend(self.emitters(transform_id, matrix, path, components))

    def mesh_layers(self, transform_id, matrix, path, renderer_obj, filter_obj, layers, winds):
        renderer = renderer_obj.read_typetree()
        if not renderer.get('m_Enabled', True):
            return
        mesh = self.mesh(filter_obj)
        if mesh is None:
            return
        world = prefab.mat_mul(prefab.FLIP_Z, matrix)
        for index, pointer in enumerate(renderer['m_Materials']):
            if index >= len(mesh['submeshes']):
                break
            material_obj, _ = self.bundles.resolve(renderer_obj, pointer)
            if material_obj is None:
                continue
            shader = self.bundles.shader(material_obj)
            texture, scale, offset = self.bundles.texture_ref(material_obj)
            colors, floats = prefab.material_properties(material_obj)
            triangles = mesh['submeshes'][index]
            used = sorted(set(triangles))
            remap = {old: new for new, old in enumerate(used)}
            uvs = prefab.rounded(
                (c for i in used for c in prefab.image_uv(mesh['uvs'][i], scale, offset)), prefab.UV_DIGITS
            )
            indices = [remap[i] for i in triangles]
            if shader == WIND_SHADER:
                placed = renderer_placement(matrix, self.rotation_chain(transform_id))
                winds.append(wind_entry(path, placed, mesh, used, uvs, indices, texture, colors, floats))
                continue
            if shader not in MESH_BLENDS:
                raise SystemExit(f'{self.bundles.prefab}: no blend rule for shader {shader} on {path}')
            blend, color_key = MESH_BLENDS[shader]
            if texture is None and color_key is None:
                continue
            gain = prefab.PARTICLE_TINT_GAIN if color_key == '_TintColor' else 1.0
            color = tuple(min(1.0, c) for c in prefab.material_tint(colors, color_key, gain))
            entry = {
                'name': path,
                'blend': blend,
                'color': prefab.rounded(color, prefab.COLOR_DIGITS),
                'texture': texture,
            }
            if blend == prefab.CUTOUT:
                entry['cutoff'] = round(floats.get('_Cutoff', prefab.DEFAULT_CUTOFF), prefab.COLOR_DIGITS)
            if shader == LATE_TRANSPARENT_SHADER:
                entry['queue'] = LATE_TRANSPARENT_QUEUE
            entry['positions'] = prefab.rounded(
                (c for i in used for c in prefab.transform_point(world, mesh['positions'][i])), prefab.POSITION_DIGITS
            )
            entry['uvs'] = uvs
            entry['indices'] = indices
            layers.append(entry)

    def mesh(self, filter_obj):
        pointer = filter_obj.read_typetree()['m_Mesh']
        mesh_obj, _ = self.bundles.resolve(filter_obj, pointer)
        if mesh_obj is None:
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
        return matrix, prefab.IDENTITY
    origin = prefab.transform_point(matrix, (0.0, 0.0, 0.0))
    basis = prefab.rotation_matrix(rotation)
    model = tuple(tuple(basis[row][:3]) + (origin[row],) for row in range(3)) + ((0.0, 0.0, 0.0, 1.0),)
    return model, prefab.mat_mul(node_layout.invert_affine(model), matrix)


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
    origin = prefab.transform_point(model, (0.0, 0.0, 0.0))
    vertex_colors = [
        tuple(channel / COLOR_BYTE for channel in color) for color in mesh['colors']
    ] or [(1.0, 1.0, 1.0, 1.0)] * len(mesh['positions'])
    normals = mesh['normals'] or [(0.0, 1.0, 0.0)] * len(mesh['positions'])
    return {
        'name': path,
        'texture': texture,
        'matrix': prefab.column_major(prefab.mat_mul(prefab.FLIP_Z, model)),
        'positions': prefab.rounded(
            (c for i in used for c in prefab.transform_point(baked, mesh['positions'][i])), prefab.POSITION_DIGITS
        ),
        'normals': prefab.rounded(
            (c for i in used for c in transformed_normal(baked, normals[i])), prefab.POSITION_DIGITS
        ),
        'colors': prefab.rounded((c for i in used for c in vertex_colors[i]), prefab.COLOR_DIGITS),
        'uvs': uvs,
        'indices': indices,
        'wind': prefab.rounded((*local_wind, wind[3])),
        'phase': round(sum(origin), prefab.VALUE_DIGITS),
        'flutter': round(floats.get('_WindEdgeFlutter', 0.5), prefab.VALUE_DIGITS),
        'flutter_frequency': round(floats.get('_WindEdgeFlutterFreqScale', 0.5), prefab.VALUE_DIGITS),
        'color_scale': round(floats.get('_VertColorScale', 1.0), prefab.VALUE_DIGITS),
        'color_shift': round(floats.get('_VertColorShift', 0.0), prefab.VALUE_DIGITS),
    }


def node_entry(marker, parts):
    centre = prefab.transform_point(prefab.mat_mul(prefab.FLIP_Z, marker), (0.0, 0.0, 0.0))
    entry = {'position': prefab.rounded(centre, prefab.POSITION_DIGITS)}
    for key, part in parts.items():
        world = prefab.mat_mul(prefab.FLIP_Z, prefab.mat_mul(marker, part))
        if key == 'text':
            point = prefab.transform_point(world, (0.0, 0.0, 0.0))
            entry[key] = prefab.rounded((point[i] - centre[i] for i in range(3)), prefab.POSITION_DIGITS)
            continue
        corners = [
            prefab.transform_point(
                world, (sx * node_layout.UNITY_PLANE_HALF_EXTENT, 0.0, sz * node_layout.UNITY_PLANE_HALF_EXTENT)
            )
            for sx, sz in NODE_PART_CORNERS
        ]
        entry[key] = prefab.rounded(
            (corner[i] - centre[i] for corner in corners for i in range(3)), prefab.POSITION_DIGITS
        )
    return entry


class Transit:
    def __init__(self, unity, mesh_handler, library):
        self.scene = MapScene(prefab.Bundles(unity, TRANSIT_PREFAB, library), mesh_handler, None)
        self.root = self.scene.live_root()
        root = self.scene.transforms[self.root]
        self.root_scale = prefab.xyz(root['m_LocalScale'])

    def place(self, marker, marker_rotation, layers, winds, emitters):
        start = prefab.mat_mul(marker, prefab.trs((0.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0), self.root_scale))

        def visit(transform_id, matrix, path, active):
            if not active:
                return
            components = self.scene.components(self.scene.game_object(transform_id))
            found = []
            self.scene.collect(transform_id, matrix, f'{TRANSIT_PREFAB}/{path}', components, layers, winds, found)
            for emitter in found:
                rotation = prefab.quat_mul(marker_rotation, self.scene.rotation_chain(transform_id, self.root))
                axes = prefab.mat_mul(prefab.FLIP_Z, prefab.rotation_matrix(rotation))
                emitter['axes'] = prefab.matrix_columns(axes)
                emitters.append(emitter)

        self.scene.walk(self.root, start, '', True, visit)


def build() -> dict:
    unity, mesh_handler = prefab.unitypy(RUN_COMMAND)
    library = prefab.ApkLibrary(unity)
    transit = Transit(unity, mesh_handler, library)
    parts = node_layout.node_parts(node_layout.Scene(node_layout.Bundle(node_layout.NODE_PREFAB)))
    scenes = []
    for chapter in json.loads(MAPS.read_text(encoding='utf-8')):
        name = node_layout.map_prefab(chapter['num'])
        scene = MapScene(prefab.Bundles(unity, name, library), mesh_handler, transit).build_campaign(parts)
        if len(scene['nodes']) != chapter['node_count']:
            raise SystemExit(f"{chapter['id']}: {name} has {len(scene['nodes'])} node markers, not {chapter['node_count']}")
        scenes.append({'map': chapter['id'], **scene})
    return {'version': SCENE_VERSION, 'scenes': scenes}


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument('--check', action='store_true', help='fail when the committed scenes are stale')
    args = parser.parse_args()
    text = prefab.serialize(build())
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding='utf-8') != text:
            print(f'{OUTPUT.relative_to(ROOT)} is stale; rerun {RUN_COMMAND}', file=sys.stderr)
            return 1
        return 0
    OUTPUT.write_text(text, encoding='utf-8')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
