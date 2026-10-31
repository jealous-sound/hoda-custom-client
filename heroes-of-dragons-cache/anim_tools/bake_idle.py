import json
import os
import struct
import sys
import zlib

EXTRACT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'extracted')

from hoda_anim import Avatar, MuscleCurves, Xform, qnorm, qmul, qconj, qrot
from muscle_solver import HumanSolver
from bake_generic import promote_skeleton_joints
import glb_writer

CHAR_PACK = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'char_pack')
UNITS = [
    ('hm_sw_fereldan_ashwarrior', 'hm_sw_fereldan_ashwarrior_t4', 'hm_sw_fereldan_ashwarrior_t4_avatar.json', 'sw_ready_ani.json'),
    ('hm_mw_circleapprentice', 'hm_mw_circleapprentice_t4', 'hm_mw_circleapprentice_t4_avatar.json', 'mw_ready06_ani.json'),
    ('df_dw_legionnairescout', 'df_dw_legionnairescout_t4', 'df_dw_legionnairescout_t4_avatar.json', 'dw_ready_ani.json'),
    ('c_hurlockgrunt', 'c_hurlockgrunt_t4', 'c_hurlockgrunt_t4_avatar.json', 'c_hurlocksword_ready_ani.json'),
    ('c_hurlockbolter', 'c_hurlockbolter_t4', 'c_hurlockbolter_t4_avatar.json', 'c_hurlockcrossbow_ready_ani.json'),
    ('c_hurlockemissary', 'c_hurlockemissary_t4', 'c_hurlockemissary_t4_avatar.json', 'mw_ready06_ani.json'),
    ('em_dw_bandit', 'em_dw_bandit_t4', 'em_dw_bandit_t4_avatar.json', 'dw_ready_ani.json'),
    ('c_ladyoftheforest', 'c_ladyoftheforest_t4', 'c_ladyoftheforest_t1_avatar.json', 'hf_idle_ani.json', 'c_ladyoftheforest_t1'),
    ('c_revenant', 'c_revenant_t4', 'c_revenant_t4_avatar.json', 'c_revenant_ready_ani.json', 'c_revenant_t1'),
]
FPS = 30.0
USE_ROOTX = '--rootx' in sys.argv


def glb_hash_to_name(gltf):
    nodes = gltf['nodes']
    children = {i: n.get('children', []) for i, n in enumerate(nodes)}
    roots = [i for i, n in enumerate(nodes) if n.get('name', '').endswith('_Root')]
    h2n = {}

    def walk(i, prefix):
        name = nodes[i].get('name', '')
        path = (prefix + '/' if prefix else '') + name
        h2n[zlib.crc32(path.encode()) & 0xffffffff] = name
        for c in children[i]:
            walk(c, path)
    for r in roots:
        walk(r, '')
    return h2n


def _norm3(v):
    length = sum(c*c for c in v) ** 0.5 or 1.0
    return tuple(c/length for c in v)


def _cross3(a, b):
    return (a[1]*b[2] - a[2]*b[1],
            a[2]*b[0] - a[0]*b[2],
            a[0]*b[1] - a[1]*b[0])


def _dot3(a, b):
    return sum(a[i]*b[i] for i in range(3))


def _basis_quaternion(x, y, z):
    m00, m01, m02 = x[0], y[0], z[0]
    m10, m11, m12 = x[1], y[1], z[1]
    m20, m21, m22 = x[2], y[2], z[2]
    trace = m00 + m11 + m22
    if trace > 0:
        s = (trace + 1.0) ** 0.5 * 2
        return qnorm(((m21-m12)/s, (m02-m20)/s, (m10-m01)/s, s/4))
    if m00 > m11 and m00 > m22:
        s = (1.0 + m00 - m11 - m22) ** 0.5 * 2
        return qnorm((s/4, (m01+m10)/s, (m02+m20)/s, (m21-m12)/s))
    if m11 > m22:
        s = (1.0 + m11 - m00 - m22) ** 0.5 * 2
        return qnorm(((m01+m10)/s, s/4, (m12+m21)/s, (m02-m20)/s))
    s = (1.0 + m22 - m00 - m11) ** 0.5 * 2
    return qnorm(((m02+m20)/s, (m12+m21)/s, s/4, (m10-m01)/s))


def _promoted_hips_basis_fix(gltf, avatar, solver, hips_a_node):
    nodes = gltf['nodes']
    source_joints = {nodes[i].get('name') for i in gltf['skins'][0]['joints']}
    hips_name = avatar.node_name[hips_a_node]
    if hips_name in source_joints:
        return (0.0, 0.0, 0.0, 1.0), None

    worlds = avatar.avatar_worlds()

    def bone_position(slot):
        hnode = solver.bone_node[slot]
        return worlds[avatar.h_to_avatar[hnode]].t

    left_arm, right_arm = bone_position(13), bone_position(14)
    left_leg, right_leg = bone_position(1), bone_position(2)
    right = _norm3(tuple((right_arm[i] + right_leg[i]) -
                         (left_arm[i] + left_leg[i]) for i in range(3)))
    up = _norm3(tuple((left_arm[i] + right_arm[i]) -
                      (left_leg[i] + right_leg[i]) for i in range(3)))
    forward = _norm3(_cross3(right, up))
    right = _norm3(_cross3(up, forward))

    inv_root = qconj(worlds[hips_a_node].q)
    local_basis = tuple(qrot(inv_root, axis) for axis in (right, up, forward))

    cardinal = ((1.0, 0.0, 0.0), (-1.0, 0.0, 0.0),
                (0.0, 1.0, 0.0), (0.0, -1.0, 0.0),
                (0.0, 0.0, 1.0), (0.0, 0.0, -1.0))
    best = None
    for snap_right in cardinal:
        for snap_up in cardinal:
            if abs(_dot3(snap_right, snap_up)) > 0.5:
                continue
            snap_forward = _cross3(snap_right, snap_up)
            score = sum(_dot3(a, b) for a, b in
                        zip((snap_right, snap_up, snap_forward), local_basis))
            if best is None or score > best[0]:
                best = (score, snap_right, snap_up, snap_forward)

    _, snap_right, snap_up, snap_forward = best
    local_anatomy = _basis_quaternion(snap_right, snap_up, snap_forward)
    canonical_root = qconj(local_anatomy)
    correction = qnorm(qmul(canonical_root, qconj(worlds[hips_a_node].q)))

    def axis_label(v):
        labels = ('+X', '-X', '+Y', '-Y', '+Z', '-Z')
        return labels[cardinal.index(v)]

    return correction, {
        'right': axis_label(snap_right),
        'up': axis_label(snap_up),
        'forward': axis_label(snap_forward),
    }


def bake_humanoid(src, dst, avatar_json, clip_json, use_rootx=False, use_tos=False,
                  display_scale=None, extra_clips=None):
    import math
    gltf, _ = glb_writer.load_glb(src)
    h2n = glb_hash_to_name(gltf)
    if use_tos and avatar_json.get('m_TOS'):
        merged = {h: p.split('/')[-1] for h, p in avatar_json['m_TOS']}
        merged.update(h2n)
        h2n = merged

    avatar = Avatar(avatar_json, h2n)
    solver = HumanSolver(avatar)

    avatar_rescale = 1.0
    if solver.scale > 1e-6:
        hips_y = avatar.avatar_worlds()[avatar.h_to_avatar[solver.bone_node[0]]].t[1]
        if hips_y > 1e-9:
            k = round(math.log10(solver.scale / hips_y))
            if k != 0:
                avatar_rescale = 10.0 ** k
                avatar.rescale_units(avatar_rescale)

    unresolved = [n for n in avatar.node_name if n.startswith('#')]


    root_x = solver.root_x if use_rootx else Xform()

    hips_h_node = solver.bone_node[0]
    hips_a_node = avatar.h_to_avatar[hips_h_node]
    hips_name = avatar.node_name[hips_a_node]
    root_a_node = avatar.node_parent[hips_a_node]
    root_local = avatar.default_local[root_a_node]
    hips_basis_fix, hips_basis = _promoted_hips_basis_fix(
        gltf, avatar, solver, hips_a_node)

    def solve_raw(cj):
        curves = MuscleCurves(cj)
        stop = curves.stop_time
        n = int(round(stop * FPS)) + 1
        raw = []
        for f in range(n):
            t = min(f / FPS, stop - 1e-6)
            dof = {}
            for attr in range(len(curves.index_array)):
                v = curves.value(attr, t)
                if v is not None:
                    dof[attr] = v
            node_q, hips_x = solver.solve_frame(dof)
            hips_m = Xform(tuple(c / solver.units_per_meter for c in hips_x.t), hips_x.q, (1, 1, 1))
            raw.append((node_q, root_x.mul(hips_m)))
        return raw

    def shared_clip_fix(reference_raw):
        orientation = (0.0, 0.0, 0.0, 1.0) if hips_basis is None else hips_basis_fix
        hip0 = reference_raw[0][1].t
        centered = qrot(orientation, (hip0[0], 0.0, hip0[2]))
        return Xform(tuple(-c for c in centered), orientation, (1, 1, 1))

    def solve_clip(raw, fix):
        out = []
        for node_q, hips_world in raw:
            frame = {}
            for hnode, q in node_q.items():
                anode = avatar.h_to_avatar[hnode]
                name = avatar.node_name[anode]
                dl = avatar.default_local[anode]
                if q is None:
                    frame[name] = root_local.inv().mul(fix.mul(hips_world))
                else:
                    frame[name] = Xform(dl.t, q, dl.s)
            out.append(frame)
        return out

    idle_raw = solve_raw(clip_json)
    fix = shared_clip_fix(idle_raw)
    frames = solve_clip(idle_raw, fix)
    clips = [('idle', frames)]
    clip_errors = {}
    for cname, cj in (extra_clips or []):
        try:
            clips.append((cname, solve_clip(solve_raw(cj), fix)))
        except (Exception, SystemExit) as e:
            clip_errors[cname] = f'{type(e).__name__}: {e}'
    n_frames = len(frames)

    hx = frames[0][hips_name]
    promoted_tmp = dst + '.promoted.tmp.glb'
    bake_src, promoted, promote_skipped = promote_skeleton_joints(
        src, promoted_tmp, avatar, ancestors_only=True)
    try:
        baked_gltf, _ = glb_writer.load_glb(bake_src)
        skin_names = {baked_gltf['nodes'][j].get('name') for j in baked_gltf['skins'][0]['joints']}
        name_to_idx = {name: i for i, name in enumerate(avatar.node_name)}
        joint_indices = {name_to_idx[name] for name in skin_names if name in name_to_idx}

        def has_joint_ancestor(index):
            parent = avatar.node_parent[index]
            while parent >= 0:
                if parent in joint_indices:
                    return True
                parent = avatar.node_parent[parent]
            return False

        top_joints = [i for i in joint_indices if not has_joint_ancestor(i)]
        default_worlds = avatar.avatar_worlds()

        def ground_clip(clip_frames):
            foot_indices = []
            for slot in (5, 6):
                human_node = solver.bone_node[slot]
                if human_node >= 0:
                    foot_indices.append(avatar.h_to_avatar[human_node])
            if not foot_indices:
                return 0.0, None
            foot_min = float('inf')
            for frame in clip_frames:
                overrides = {name_to_idx[name]: xform for name, xform in frame.items() if name in name_to_idx}
                worlds = avatar.avatar_worlds(overrides)
                foot_min = min(foot_min, *(worlds[index].t[1] for index in foot_indices))
            shift = -0.02 - foot_min if foot_min < -0.08 else 0.0
            if shift == 0.0:
                return 0.0, foot_min
            for index in top_joints:
                delta = (0.0, shift, 0.0)
                parent = avatar.node_parent[index]
                if parent >= 0:
                    parent_world = default_worlds[parent]
                    delta = qrot(qconj(parent_world.q), delta)
                    delta = tuple(delta[i] / parent_world.s[i] for i in range(3))
                name = avatar.node_name[index]
                for frame in clip_frames:
                    xform = frame.get(name, avatar.default_local[index])
                    frame[name] = Xform(
                        tuple(xform.t[i] + delta[i] for i in range(3)), xform.q, xform.s)
            return shift, foot_min

        ground_shift, foot_min_y = ground_clip(frames)
        extra_ground = {}
        for clip_name, clip_frames in clips[1:]:
            extra_ground[clip_name] = ground_clip(clip_frames)[0]
        glb_writer.rebuild_glb(bake_src, dst, avatar, clips, FPS)
    finally:
        if bake_src == promoted_tmp and os.path.exists(promoted_tmp):
            os.remove(promoted_tmp)
    post_scale = glb_writer.normalize_units(dst, display_scale)
    return {
        'neutral_check': solver.neutral_check(), 'units_per_meter': solver.units_per_meter,
        'unresolved': unresolved, 'frames': n_frames, 'hips0': hx.t, 'post_scale': post_scale,
        'avatar_rescale': avatar_rescale, 'clips': [c[0] for c in clips],
        'bind_unit_correction': getattr(avatar, 'last_bind_unit_correction', 1.0),
        'stripped_root_helpers': getattr(avatar, 'last_stripped_root_helpers', 0),
        'clip_errors': clip_errors,
        'promoted': promoted, 'promote_skipped': promote_skipped,
        'hips_basis': hips_basis,
        'hips_basis_fix_degrees': (2*math.degrees(math.acos(
            min(1.0, abs(hips_basis_fix[3])))) if hips_basis is not None else 0.0),
        'hips_default': avatar.default_local[hips_a_node].t, 'dst': dst,
        'foot_min_y': foot_min_y, 'ground_shift': ground_shift,
        'extra_ground_shift': extra_ground,
    }


def main():
    for row in UNITS:
        dirname, base, avatar_file, clip_file = row[:4]
        out_base = row[4] if len(row) > 4 else base
        src = f'{CHAR_PACK}/{dirname}/{base}.glb'
        dst = f'{CHAR_PACK}/{dirname}/{out_base}_rl.glb'
        avatar_json = json.load(open(os.path.join(EXTRACT_DIR, avatar_file)))
        clip_json = json.load(open(os.path.join(EXTRACT_DIR, clip_file)))
        info = bake_humanoid(src, dst, avatar_json, clip_json, use_rootx=USE_ROOTX)
        print(f"== {base}: neutral_check={info['neutral_check']:.6f} "
              f"units/m={info['units_per_meter']} unresolved_nodes={info['unresolved']}")
        hx, hd = info['hips0'], info['hips_default']
        print(f"   frames={info['frames']} hips0 local t=({hx[0]:.2f},{hx[1]:.2f},{hx[2]:.2f}) "
              f"(default {hd})")
        print(f"   wrote {dst}")


if __name__ == '__main__':
    main()
