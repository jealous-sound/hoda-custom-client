#!/usr/bin/env python
import argparse
import glob
import json
import os
import re
import shutil
import struct
import sys

TOOLS = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, TOOLS)
ROOT = os.path.dirname(os.path.dirname(TOOLS))
CHAR_PACK = os.path.join(ROOT, 'client-additional-data', 'char_pack')
REFERENCE_FBX = os.path.join(
    ROOT, '.agents', 'skills', 'hoda-reference', 'references', 'fbx')
CAB_PATHS = os.path.join(TOOLS, 'cache', 'cab_paths.json')

import UnityPy
from UnityPy.helpers import UnityVersion as UV
_orig_from_str = UV.UnityVersion.from_str.__func__
def _patched_from_str(cls, version):
    m = re.search(r"(\d+)\.(\d+)\.(\d+)([a-zA-Z]+)(\d+)", version)
    if m:
        version = m.group(0)
    return _orig_from_str(cls, version)
UV.UnityVersion.from_str = classmethod(_patched_from_str)

from UnityPy.helpers.MeshHelper import MeshHandler
import glb_writer


def mat_from_tt(m):
    if isinstance(m, dict):
        return [[m[f'e{r}{c}'] for c in range(4)] for r in range(4)]
    return [[m[r*4+c] for c in range(4)] for r in range(4)]


def conj_flip_x(M):
    out = [row[:] for row in M]
    for r in range(4):
        for c in range(4):
            if (r == 0) != (c == 0):
                out[r][c] = -M[r][c]
    return out


def read_prefab(env):
    gos, trs, smrs, meshes = {}, {}, [], {}
    for o in env.objects:
        if o.type.name == 'GameObject':
            gos[o.path_id] = o.read_typetree()
        elif o.type.name == 'Transform':
            trs[o.path_id] = o.read_typetree()
        elif o.type.name == 'SkinnedMeshRenderer':
            smrs.append(o.read_typetree())
        elif o.type.name == 'Mesh':
            meshes[o.path_id] = o

    best = None
    for smr in smrs:
        mo = meshes.get(smr['m_Mesh']['m_PathID'])
        if mo is None:
            continue
        h = MeshHandler(mo.read())
        h.process()
        n = len(h.m_Vertices or [])
        if best is None or n > best[2]:
            best = (smr, mo, n, h)
    if best is None:
        raise ValueError('CAB has no readable skinned mesh')
    return gos, trs, best


def material_texture_name(env, base, mesh_name, cab_paths):
    import bake_all

    material, error = bake_all.renderer_material_for_mesh(
        env, mesh_name, 0, cab_paths, base)
    if material is None:
        return '', error
    texenvs = bake_all.material_texenvs(material.read_typetree())
    main = texenvs.get('_MainTex', {}).get('m_Texture', {})
    return bake_all.texture_name(env, main, cab_paths), None


def copy_reference_textures(base, mesh_name, env, cab_paths, out_dir, reference_fbx):
    import bake_all

    unit_dir = f'unit.{base}_pre'
    candidates = sorted(glob.glob(os.path.join(reference_fbx, '*', unit_dir, '*.png')))
    if not candidates:
        raise FileNotFoundError(f'no reference textures for {unit_dir}')

    texture_name, material_error = material_texture_name(env, base, mesh_name, cab_paths)
    selected = bake_all.pick_reference_texture(base, texture_name, reference_fbx)
    if selected is None:
        raise FileNotFoundError(f'no usable reference texture for {unit_dir}')

    os.makedirs(out_dir, exist_ok=True)
    for source in candidates:
        destination = os.path.join(out_dir, os.path.basename(source))
        if not os.path.exists(destination) or not os.path.samefile(source, destination):
            shutil.copy2(source, destination)
    method = f'CAB _MainTex {texture_name}' if texture_name else 'largest PNG fallback'
    if material_error:
        method += f' ({material_error})'
    return os.path.basename(selected), method, len(candidates)


def rebuild_source(dirname, base, tex_png=None, reference_fbx=REFERENCE_FBX,
                   cab_paths_path=CAB_PATHS):
    with open(cab_paths_path) as source:
        cab_paths = json.load(source)
    cab_key = f'cab-{base}_pre'.lower()
    if cab_key not in cab_paths:
        raise KeyError(f'{cab_key} is absent from {cab_paths_path}')
    env = UnityPy.load(cab_paths[cab_key])
    gos, trs, best = read_prefab(env)
    smr, mesh_obj, nverts, h = best
    mesh_tt = mesh_obj.read_typetree()
    print(f'{base}: using mesh "{mesh_tt["m_Name"]}" with {nverts} verts, '
          f'{len(h.m_IndexBuffer)//3} tris, bones={len(smr["m_Bones"])}')

    out_dir = os.path.join(CHAR_PACK, dirname)
    os.makedirs(out_dir, exist_ok=True)
    texture_note = 'explicit texture'
    texture_count = 1
    if tex_png is None:
        tex_png, texture_note, texture_count = copy_reference_textures(
            base, mesh_tt['m_Name'], env, cab_paths, out_dir, reference_fbx)
    else:
        source_texture = os.path.abspath(tex_png)
        if os.path.isfile(source_texture):
            destination = os.path.join(out_dir, os.path.basename(source_texture))
            if source_texture != destination:
                shutil.copy2(source_texture, destination)
            tex_png = os.path.basename(source_texture)
        elif not os.path.isfile(os.path.join(out_dir, tex_png)):
            raise FileNotFoundError(f'texture not found: {tex_png}')

    name_of = {pid: gos.get(tt['m_GameObject']['m_PathID'], {}).get('m_Name', f'#{pid}')
               for pid, tt in trs.items()}

    bone_pids = [b['m_PathID'] for b in smr['m_Bones']]
    root = bone_pids[0]
    while trs[root]['m_Father']['m_PathID'] in trs:
        root = trs[root]['m_Father']['m_PathID']

    nodes, node_of_pid = [], {}

    def add_node(pid):
        tt = trs[pid]
        t, q, s = tt['m_LocalPosition'], tt['m_LocalRotation'], tt['m_LocalScale']
        idx = len(nodes)
        nodes.append({
            'name': name_of[pid],
            'translation': [-t['x'], t['y'], t['z']],
            'rotation': [q['x'], -q['y'], -q['z'], q['w']],
            'scale': [s['x'], s['y'], s['z']],
        })
        node_of_pid[pid] = idx
        kids = [c['m_PathID'] for c in tt['m_Children'] if c['m_PathID'] in trs]
        if kids:
            nodes[idx]['children'] = [add_node(c) for c in kids]
        return idx

    root_idx = add_node(root)
    assert all(p in node_of_pid for p in bone_pids), 'bone outside the chosen subtree'

    blob = bytearray()
    accessors, buffer_views = [], []

    def push(rows, fmt, comptype, typ, minmax=False):
        off = len(blob)
        for r in rows:
            blob.extend(struct.pack('<' + fmt, *r) if isinstance(r, (tuple, list))
                        else struct.pack('<' + fmt, r))
        while len(blob) % 4:
            blob.append(0)
        buffer_views.append({'buffer': 0, 'byteOffset': off, 'byteLength': len(blob) - off
                             if not (len(blob) - off) % 4 else len(blob) - off})
        acc = {'bufferView': len(buffer_views) - 1, 'componentType': comptype,
               'count': len(rows), 'type': typ}
        if minmax:
            n = len(rows[0])
            acc['min'] = [min(r[i] for r in rows) for i in range(n)]
            acc['max'] = [max(r[i] for r in rows) for i in range(n)]
        accessors.append(acc)
        return len(accessors) - 1

    pos = [(-v[0], v[1], v[2]) for v in h.m_Vertices]
    a_pos = push(pos, 'fff', 5126, 'VEC3', minmax=True)
    a_nrm = None
    if h.m_Normals:
        nrm = [(-v[0], v[1], v[2]) for v in h.m_Normals]
        a_nrm = push(nrm, 'fff', 5126, 'VEC3')
    a_uv = None
    if h.m_UV0:
        uv = [(v[0], 1.0 - v[1]) for v in h.m_UV0]
        a_uv = push(uv, 'ff', 5126, 'VEC2')

    def rows4(flat_or_rows):
        lst = list(flat_or_rows)
        if lst and isinstance(lst[0], (int, float)):
            return [tuple(lst[i*4:i*4+4]) for i in range(len(lst)//4)]
        return [tuple(r) for r in lst]

    jrows = rows4(h.m_BoneIndices)
    wrows = rows4(h.m_BoneWeights)
    a_jnt = push([tuple(int(j) for j in r) for r in jrows], 'HHHH', 5123, 'VEC4')
    a_wgt = push(wrows, 'ffff', 5126, 'VEC4')

    idx = list(h.m_IndexBuffer)
    tris = [(idx[i], idx[i+2], idx[i+1]) for i in range(0, len(idx), 3)]
    flat = [i for t in tris for i in t]
    a_idx = push(flat, 'I', 5125, 'SCALAR')

    bind = mesh_tt['m_BindPose']
    ibms = []
    for m in bind:
        M = conj_flip_x(mat_from_tt(m))
        ibms.append(tuple(M[r][c] for c in range(4) for r in range(4)))
    a_ibm = push(ibms, 'f'*16, 5126, 'MAT4')

    attrs = {'POSITION': a_pos, 'JOINTS_0': a_jnt, 'WEIGHTS_0': a_wgt}
    if a_nrm is not None:
        attrs['NORMAL'] = a_nrm
    if a_uv is not None:
        attrs['TEXCOORD_0'] = a_uv

    mesh_node = len(nodes)
    nodes.append({'name': f'{base}_mesh', 'mesh': 0, 'skin': 0})

    gltf = {
        'asset': {'version': '2.0', 'generator': 'rebuild_stub_src'},
        'scene': 0,
        'scenes': [{'nodes': [root_idx, mesh_node]}],
        'nodes': nodes,
        'meshes': [{'name': mesh_tt['m_Name'], 'primitives': [{
            'attributes': attrs, 'indices': a_idx, 'material': 0, 'mode': 4}]}],
        'skins': [{'inverseBindMatrices': a_ibm, 'skeleton': root_idx,
                   'joints': [node_of_pid[p] for p in bone_pids]}],
        'materials': [{'name': f'{base}_mat', 'pbrMetallicRoughness': {
            'baseColorTexture': {'index': 0}, 'metallicFactor': 0.0}}],
        'textures': [{'source': 0, 'sampler': 0}],
        'samplers': [{'magFilter': 9729, 'minFilter': 9987, 'wrapS': 10497, 'wrapT': 10497}],
        'images': [{'uri': tex_png}],
        'accessors': accessors,
        'bufferViews': buffer_views,
        'buffers': [{'byteLength': len(blob)}],
    }

    out = os.path.join(out_dir, f'{base}.glb')
    if os.path.exists(out) and not os.path.exists(out + '.stub'):
        os.rename(out, out + '.stub')
    glb_writer.save_glb(out, gltf, blob)
    print(f'wrote {out} ({len(blob)} bytes bin, {len(nodes)} nodes, '
          f'{len(bind)} bindposes, joints={len(bone_pids)}; {texture_note})')
    return {
        'base': base,
        'path': out,
        'mesh': mesh_tt['m_Name'],
        'vertices': nverts,
        'texture': tex_png,
        'texture_count': texture_count,
        'texture_method': texture_note,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('dirname')
    parser.add_argument('base')
    parser.add_argument('texture_png', nargs='?')
    parser.add_argument('--reference-fbx', default=REFERENCE_FBX)
    args = parser.parse_args()
    rebuild_source(args.dirname, args.base, args.texture_png, args.reference_fbx)


if __name__ == '__main__':
    main()
