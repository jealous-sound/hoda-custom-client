#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

import map_particles
import map_prefab as prefab

CLI_DESCRIPTION = (
    'Build data/nexus_scenes.json, the Nexus event map scenes with their layers and particle emitters, from the '
    'original 4.4.0 map prefabs'
)
ROOT = Path(__file__).resolve().parents[1]
MAPS = ROOT / 'data' / 'nexus_maps.json'
OUTPUT = ROOT / 'data' / 'nexus_scenes.json'
RUN_COMMAND = 'uv run --offline --with UnityPy python tools/build_nexus_scenes.py'
NODE_FRAME = 'NodeMarkers'
FIRST_MARKER = 'step1'
SCENE_VERSION = 1


def material_entry(bundles, material_obj):
    shader = bundles.shader(material_obj)
    if shader not in prefab.SHADER_BLENDS:
        name = material_obj.read_typetree()['m_Name']
        raise SystemExit(f'{bundles.prefab}: no blend rule for shader {shader} ({name})')
    blend, color_key = prefab.SHADER_BLENDS[shader]
    colors, floats = prefab.material_properties(material_obj)
    color = (1.0, 1.0, 1.0, 1.0)
    if color_key:
        color = colors.get(color_key, color)
        if color_key == '_TintColor':
            color = tuple(min(1.0, prefab.PARTICLE_TINT_GAIN * c) for c in color)
    texture, scale, offset = bundles.texture_ref(material_obj)
    if texture is None and color_key is None:
        return None, scale, offset
    entry = {'blend': blend, 'color': prefab.rounded(color, prefab.COLOR_DIGITS), 'texture': texture}
    if blend == prefab.CUTOUT:
        entry['cutoff'] = round(floats.get('_Cutoff', prefab.DEFAULT_CUTOFF), prefab.COLOR_DIGITS)
    return entry, scale, offset


def mesh_geometry(bundles, mesh_handler, filter_obj):
    pointer = filter_obj.read_typetree()['m_Mesh']
    mesh_obj, _ = bundles.resolve(filter_obj, pointer)
    if mesh_obj is None:
        return None
    handler = mesh_handler(mesh_obj.read())
    handler.process()
    return handler.m_Vertices, handler.m_UV0, [list(sum(map(list, sub), [])) for sub in handler.get_triangles()]


class NexusScene(map_particles.ParticleScene):
    def build(self):
        root = self.live_root()
        layers, emitters, camera, frame, marker = [], [], None, None, None

        def visit(transform_id, matrix, path, active):
            nonlocal camera, frame, marker
            game_object = self.game_object(transform_id)
            name = game_object['m_Name']
            parts = self.components(game_object)
            if name == prefab.CAMERA_NODE and 'Camera' in parts:
                camera = (matrix, parts['Camera'].read_typetree())
            elif name == NODE_FRAME:
                frame = matrix
            elif name == FIRST_MARKER:
                marker = self.transforms[transform_id]
            if not active:
                return
            if 'MeshRenderer' in parts and 'MeshFilter' in parts:
                layers.extend(self.layers(matrix, path, parts['MeshRenderer'], parts['MeshFilter']))
            emitters.extend(self.emitters(transform_id, matrix, path, parts))

        self.walk(root, prefab.IDENTITY, '', True, visit)
        if camera is None or frame is None or marker is None:
            raise SystemExit(f'{self.bundles.prefab}: missing {prefab.CAMERA_NODE}, {NODE_FRAME} or {FIRST_MARKER}')
        view = prefab.camera_entry(*camera)
        return {
            'prefab': self.bundles.prefab,
            'camera': view,
            'nodes': nodes_entry(frame, marker),
            'layers': [layer for layer in layers if not prefab.is_side_mask(layer, view)],
            'emitters': emitters,
        }

    def layers(self, matrix, path, renderer_obj, filter_obj):
        renderer = renderer_obj.read_typetree()
        if not renderer.get('m_Enabled', True):
            return []
        geometry = mesh_geometry(self.bundles, self.mesh_handler, filter_obj)
        if geometry is None:
            return []
        positions, uvs, submeshes = geometry
        world = prefab.mat_mul(prefab.FLIP_Z, matrix)
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
            entry['positions'] = prefab.rounded(
                (c for i in used for c in prefab.transform_point(world, positions[i])), prefab.POSITION_DIGITS
            )
            entry['uvs'] = prefab.rounded(
                (c for i in used for c in prefab.image_uv(uvs[i], scale, offset)), prefab.UV_DIGITS
            )
            entry['indices'] = [remap[i] for i in submeshes[index]]
            found.append(entry)
        return found


def nodes_entry(frame, marker):
    marker_scale = prefab.xyz(marker['m_LocalScale'])
    frame_scale = (
        math.sqrt(sum(frame[r][0] ** 2 for r in range(3))),
        math.sqrt(sum(frame[r][2] ** 2 for r in range(3))),
    )
    return {
        'matrix': prefab.column_major(prefab.mat_mul(prefab.FLIP_Z, frame)),
        'unit': prefab.rounded(
            (frame_scale[0] * marker_scale[0], frame_scale[1] * marker_scale[2]), prefab.POSITION_DIGITS
        ),
    }


def prefabs():
    return sorted({event['prefab'] for event in json.loads(MAPS.read_text(encoding='utf-8'))})


def build() -> dict:
    unity, mesh_handler = prefab.unitypy(RUN_COMMAND)
    library = prefab.ApkLibrary(unity)
    scenes = [NexusScene(prefab.Bundles(unity, name, library), mesh_handler).build() for name in prefabs()]
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
