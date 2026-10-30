import json
import math
import re
import struct

from hoda_anim import Xform, to_gltf_t, to_gltf_q

COMP_FMT = {5120: 'b', 5121: 'B', 5122: 'h', 5123: 'H', 5125: 'I', 5126: 'f'}
NCOMP = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}


def load_glb(path):
    with open(path, 'rb') as source:
        d = source.read()
    jlen = struct.unpack('<I', d[12:16])[0]
    gltf = json.loads(d[20:20+jlen])
    off = 20 + jlen
    blen, btype = struct.unpack('<I4s', d[off:off+8])
    assert btype == b'BIN\x00'
    return gltf, bytearray(d[off+8:off+8+blen])


def save_glb(path, gltf, binblob):
    jbytes = json.dumps(gltf, separators=(',', ':')).encode()
    jbytes += b' ' * (-len(jbytes) % 4)
    binblob = bytes(binblob) + b'\x00' * (-len(binblob) % 4)
    total = 12 + 8 + len(jbytes) + 8 + len(binblob)
    with open(path, 'wb') as f:
        f.write(struct.pack('<4sII', b'glTF', 2, total))
        f.write(struct.pack('<I4s', len(jbytes), b'JSON'))
        f.write(jbytes)
        f.write(struct.pack('<I4s', len(binblob), b'BIN\x00'))
        f.write(binblob)


def accessor_span(gltf, idx):
    a = gltf['accessors'][idx]
    bv = gltf['bufferViews'][a['bufferView']]
    start = bv.get('byteOffset', 0) + a.get('byteOffset', 0)
    fmt = COMP_FMT[a['componentType']]
    n = NCOMP[a['type']]
    stride = bv.get('byteStride') or struct.calcsize(fmt) * n
    return a, start, fmt, n, stride


def read_accessor(gltf, blob, idx):
    a, start, fmt, n, stride = accessor_span(gltf, idx)
    return [struct.unpack_from('<' + fmt*n, blob, start + i*stride) for i in range(a['count'])]


def write_accessor(gltf, blob, idx, rows):
    a, start, fmt, n, stride = accessor_span(gltf, idx)
    assert len(rows) == a['count']
    for i, row in enumerate(rows):
        struct.pack_into('<' + fmt*n, blob, start + i*stride, *row)


def scale_glb(path, s):
    gltf, blob = load_glb(path)
    done = set()
    for mesh in gltf['meshes']:
        for prim in mesh['primitives']:
            ai = prim['attributes'].get('POSITION')
            if ai is None or ai in done:
                continue
            done.add(ai)
            rows = [tuple(c*s for c in r) for r in read_accessor(gltf, blob, ai)]
            write_accessor(gltf, blob, ai, rows)
            acc = gltf['accessors'][ai]
            for k in ('min', 'max'):
                if k in acc:
                    acc[k] = [v*s for v in acc[k]]
    for skin in gltf.get('skins', []):
        ai = skin.get('inverseBindMatrices')
        if ai is None or ai in done:
            continue
        done.add(ai)
        rows = []
        for m in read_accessor(gltf, blob, ai):
            m = list(m)
            m[12] *= s
            m[13] *= s
            m[14] *= s
            rows.append(tuple(m))
        write_accessor(gltf, blob, ai, rows)
    for n in gltf['nodes']:
        if 'translation' in n:
            n['translation'] = [v*s for v in n['translation']]
    for anim in gltf.get('animations', []):
        for ch in anim['channels']:
            if ch['target']['path'] != 'translation':
                continue
            ai = anim['samplers'][ch['sampler']]['output']
            if ai in done:
                continue
            done.add(ai)
            rows = [tuple(c*s for c in r) for r in read_accessor(gltf, blob, ai)]
            write_accessor(gltf, blob, ai, rows)
    save_glb(path, gltf, blob)


def skinned_height(gltf):
    lo, hi = float('inf'), float('-inf')
    for skinned_only in (True, False):
        for mesh in gltf['meshes']:
            for prim in mesh['primitives']:
                attrs = prim['attributes']
                if skinned_only and 'JOINTS_0' not in attrs:
                    continue
                acc = gltf['accessors'][attrs['POSITION']]
                lo = min(lo, acc['min'][1])
                hi = max(hi, acc['max'][1])
        if hi > lo:
            break
    return hi - lo


EFF_LO, EFF_HI = 0.4, 7.5
EFF_WARN_LO, EFF_WARN_HI = 0.25, 12.0


def normalize_units(path, display_scale=None):
    import math
    gltf, _ = load_glb(path)
    eff = skinned_height(gltf) * (display_scale if display_scale else 1.0)
    if EFF_WARN_LO <= eff <= EFF_WARN_HI:
        return 1.0
    best = None
    for k in (2, 1, -1, -2):
        s = 10.0 ** k
        if EFF_LO <= eff*s <= EFF_HI:
            d = abs(math.log10(eff*s / (EFF_LO*EFF_HI) ** 0.5))
            if best is None or d < best[0]:
                best = (d, s)
    if best is None:
        raise SystemExit(f'{path}: effective height {eff:.3f}m not fixable by a decade rescale')
    scale_glb(path, best[1])
    return best[1]


def xform_world_map(avatar):
    worlds = avatar.avatar_worlds()
    return {avatar.node_name[i]: worlds[i] for i in range(len(worlds))}


def infer_bind_unit_correction(gltf, avatar):
    if not gltf.get('skins'):
        return 1.0
    nodes = gltf.get('nodes', [])
    avatar_index = {name: i for i, name in enumerate(avatar.node_name)}
    logs = []
    for ni in gltf['skins'][0].get('joints', []):
        node = nodes[ni]
        name = node.get('name')
        ai = avatar_index.get(name)
        if ai is None:
            ai = avatar_index.get(re.sub(r' \d+$', '', name or ''))
        if ai is None:
            continue
        src = node.get('translation', [0.0, 0.0, 0.0])
        src_len = math.sqrt(sum(float(c) * float(c) for c in src))
        dst = avatar.default_local[ai].t
        dst_len = math.sqrt(sum(float(c) * float(c) for c in dst))
        current = abs(float(avatar.acc_scale[ai]))
        if src_len < 1e-5 or dst_len < 1e-5 or current < 1e-8:
            continue
        ratio = dst_len / (src_len * current)
        if math.isfinite(ratio) and ratio > 0:
            logs.append(math.log10(ratio))
    if len(logs) < 3:
        return 1.0
    logs.sort()
    median = logs[len(logs) // 2]
    decade = int(round(median))
    if decade == 0 or abs(median - decade) > 0.32 or abs(decade) > 2:
        return 1.0
    return 10.0 ** decade


def repair_float_skin_weights(rows):
    fixed = []
    for row in rows:
        values = [float(value) for value in row]
        if len(values) == 4 and values[3] < -0.001 and all(-0.001 <= value <= 1.001 for value in values[:3]):
            values[3] = max(0.0, 1.0 - sum(values[:3]))
        values = [max(0.0, value) for value in values]
        total = sum(values)
        if total <= 1e-8:
            values = [1.0] + [0.0] * (len(values) - 1)
        else:
            values = [value / total for value in values]
        fixed.append(tuple(values))
    return fixed


def strip_root_helper_meshes(gltf):
    stripped = 0
    candidates = set()
    for node in gltf.get('nodes', []):
        mesh_index = node.get('mesh')
        if mesh_index is None or 'skin' in node or not re.search(r'_Root_0$', node.get('name', '')):
            continue
        primitives = gltf['meshes'][mesh_index].get('primitives', [])
        helper = bool(primitives)
        for prim in primitives:
            attrs = prim.get('attributes', {})
            if 'JOINTS_0' in attrs or 'TEXCOORD_0' in attrs:
                helper = False
                break
            accessor = gltf['accessors'][attrs['POSITION']]
            extents = [accessor['max'][axis] - accessor['min'][axis] for axis in range(3)]
            if accessor.get('count') != 24 or max(extents, default=1.0) > 0.15:
                helper = False
                break
        if helper:
            candidates.add(mesh_index)
            node.pop('mesh')
            stripped += 1
    used = {node['mesh'] for node in gltf.get('nodes', []) if 'mesh' in node}
    remove = candidates - used
    if remove:
        remap = {}
        meshes = []
        for old, mesh in enumerate(gltf['meshes']):
            if old in remove:
                continue
            remap[old] = len(meshes)
            meshes.append(mesh)
        gltf['meshes'] = meshes
        for node in gltf.get('nodes', []):
            if 'mesh' in node:
                node['mesh'] = remap[node['mesh']]
    return stripped


def rebuild_glb(src_path, dst_path, avatar, anim_locals, anim_fps, anim_name='idle'):
    if anim_locals and isinstance(anim_locals[0], (tuple, list)):
        clips = [(name, frames) for name, frames in anim_locals]
    else:
        clips = [(anim_name, anim_locals)]
    gltf, blob = load_glb(src_path)
    stripped_root_helpers = strip_root_helper_meshes(gltf)
    skin = gltf['skins'][0]
    nodes = gltf['nodes']

    parent = {}
    for i, n in enumerate(nodes):
        for c in n.get('children', []):
            parent[c] = i

    skeleton_nodes = set()
    for ni in skin['joints']:
        while ni is not None and ni not in skeleton_nodes:
            skeleton_nodes.add(ni)
            ni = parent.get(ni)
    name_to_node = {}
    for i in sorted(skeleton_nodes):
        name_to_node.setdefault(nodes[i].get('name'), i)

    worlds_by_name = xform_world_map(avatar)
    avatar_index = {name: i for i, name in enumerate(avatar.node_name)}

    residual = getattr(avatar, 'residual_scale', None)
    acc_by_name = {}
    for i, name in enumerate(avatar.node_name):
        a = avatar.acc_scale[i]
        r = residual[i] if residual else (1.0, 1.0, 1.0)
        acc_by_name[name] = (a*r[0], a*r[1], a*r[2])

    bind_unit_correction = infer_bind_unit_correction(gltf, avatar)
    if bind_unit_correction != 1.0:
        acc_by_name = {
            name: tuple(c * bind_unit_correction for c in scale)
            for name, scale in acc_by_name.items()
        }

    def unity_local(ni):
        nd = nodes[ni]
        t = nd.get('translation', [0, 0, 0])
        q = nd.get('rotation', [0, 0, 0, 1])
        s = nd.get('scale', [1, 1, 1])
        return Xform((-t[0], t[1], t[2]), (q[0], -q[1], -q[2], q[3]), tuple(s))

    alias = {}
    synth_local = {}
    for ni in skin['joints']:
        name = nodes[ni].get('name')
        if name in worlds_by_name:
            continue
        p = parent.get(ni)
        pname = nodes[p].get('name') if p is not None else None
        assert pname in worlds_by_name, f'joint {name}: avatar-unknown with unknown parent {pname}'
        pacc = acc_by_name[pname]
        x = unity_local(ni)
        local_m = Xform((x.t[0]*pacc[0], x.t[1]*pacc[1], x.t[2]*pacc[2]), x.q, (1.0, 1.0, 1.0))
        stripped = re.sub(r' \d+$', '', name)
        ai = avatar_index.get(stripped)
        if ai is not None:
            ax = avatar.default_local[ai]
            if all(abs(local_m.t[c] - ax.t[c]) < 0.02 + 0.1*abs(ax.t[c]) for c in range(3)):
                alias[name] = stripped
                continue
        synth_local[name] = local_m
        worlds_by_name[name] = worlds_by_name[pname].mul(local_m)
        acc_by_name[name] = pacc

    def resolve(name):
        return alias.get(name, name)

    def xform_to_gltf_mat(x):
        q = to_gltf_q(x.q)
        t = to_gltf_t(x.t)
        qx, qy, qz, qw = q
        R = [
            [1-2*(qy*qy+qz*qz), 2*(qx*qy-qz*qw), 2*(qx*qz+qy*qw)],
            [2*(qx*qy+qz*qw), 1-2*(qx*qx+qz*qz), 2*(qy*qz-qx*qw)],
            [2*(qx*qz-qy*qw), 2*(qy*qz+qx*qw), 1-2*(qx*qx+qy*qy)],
        ]
        return [[R[r][c]*x.s[c] for c in range(3)] + [t[r]] for r in range(3)] + [[0, 0, 0, 1]]

    def matmul4(A, B):
        return [[sum(A[i][k]*B[k][j] for k in range(4)) for j in range(4)] for i in range(4)]

    ibm_orig = read_accessor(gltf, blob, skin['inverseBindMatrices'])
    skin_mats = []
    for k, ni in enumerate(skin['joints']):
        cm = ibm_orig[k]
        ibm = [[cm[c*4 + r] for c in range(4)] for r in range(4)]
        name = resolve(nodes[ni]['name'])
        w = worlds_by_name[name]
        sv = acc_by_name.get(name, (1.0, 1.0, 1.0))
        W = xform_to_gltf_mat(Xform(w.t, w.q, sv))
        skin_mats.append(matmul4(W, ibm))

    for mesh in gltf['meshes']:
        for prim in mesh['primitives']:
            attrs = prim['attributes']
            if 'JOINTS_0' not in attrs:
                continue
            pos = read_accessor(gltf, blob, attrs['POSITION'])
            jnt = read_accessor(gltf, blob, attrs['JOINTS_0'])
            wgt = read_accessor(gltf, blob, attrs['WEIGHTS_0'])
            wacc = gltf['accessors'][attrs['WEIGHTS_0']]
            wscale = {5121: 1/255.0, 5123: 1/65535.0}.get(wacc['componentType'], 1.0)
            if wacc['componentType'] == 5126:
                wgt = repair_float_skin_weights(wgt)
                write_accessor(gltf, blob, attrs['WEIGHTS_0'], wgt)
            else:
                wgt = [tuple(max(0.0, value * wscale) for value in row) for row in wgt]
            newp = []
            for v, jj, ww in zip(pos, jnt, wgt):
                total = sum(ww) or 1.0
                x = y = z = 0.0
                for jk, wk in zip(jj, ww):
                    if wk == 0.0:
                        continue
                    M = skin_mats[jk]
                    x += wk*(M[0][0]*v[0] + M[0][1]*v[1] + M[0][2]*v[2] + M[0][3])
                    y += wk*(M[1][0]*v[0] + M[1][1]*v[1] + M[1][2]*v[2] + M[1][3])
                    z += wk*(M[2][0]*v[0] + M[2][1]*v[1] + M[2][2]*v[2] + M[2][3])
                newp.append((x/total, y/total, z/total))
            write_accessor(gltf, blob, attrs['POSITION'], newp)
            pacc = gltf['accessors'][attrs['POSITION']]
            pacc['min'] = [min(p[i] for p in newp) for i in range(3)]
            pacc['max'] = [max(p[i] for p in newp) for i in range(3)]
            if 'NORMAL' in attrs:
                nrm = read_accessor(gltf, blob, attrs['NORMAL'])
                newn = []
                for v, jj, ww in zip(nrm, jnt, wgt):
                    x = y = z = 0.0
                    for jk, wk in zip(jj, ww):
                        if wk == 0.0:
                            continue
                        M = skin_mats[jk]
                        x += wk*(M[0][0]*v[0] + M[0][1]*v[1] + M[0][2]*v[2])
                        y += wk*(M[1][0]*v[0] + M[1][1]*v[1] + M[1][2]*v[2])
                        z += wk*(M[2][0]*v[0] + M[2][1]*v[1] + M[2][2]*v[2])
                    ln = (x*x + y*y + z*z) ** 0.5 or 1.0
                    newn.append((x/ln, y/ln, z/ln))
                write_accessor(gltf, blob, attrs['NORMAL'], newn)
    for name, ai in avatar_index.items():
        ni = name_to_node.get(name)
        if ni is None:
            continue
        x = avatar.default_local[ai]
        nodes[ni]['translation'] = list(to_gltf_t(x.t))
        nodes[ni]['rotation'] = list(to_gltf_q(x.q))
        nodes[ni]['scale'] = list(x.s)
    for ni in skin['joints']:
        name = nodes[ni].get('name')
        x = (avatar.default_local[avatar_index[alias[name]]] if name in alias
             else synth_local.get(name))
        if x is not None:
            nodes[ni]['translation'] = list(to_gltf_t(x.t))
            nodes[ni]['rotation'] = list(to_gltf_q(x.q))
            nodes[ni]['scale'] = [1.0, 1.0, 1.0]

    neutralized = set()
    for ni, n in enumerate(nodes):
        if 'mesh' in n and 'skin' in n:
            for k in ('matrix', 'translation', 'rotation', 'scale'):
                n.pop(k, None)
            neutralized.add(ni)

    neutral = set(skeleton_nodes)
    for i, n in enumerate(nodes):
        if 'mesh' in n:
            j = parent.get(i)
            while j is not None:
                neutral.add(j)
                j = parent.get(j)
    for ni in neutral:
        if 'skin' in nodes[ni] or 'mesh' in nodes[ni]:
            continue
        name = nodes[ni].get('name')
        known = name in avatar_index or name in alias or name in synth_local
        if known and ni in skin['joints']:
            continue
        for k in ('matrix', 'translation', 'rotation', 'scale'):
            nodes[ni].pop(k, None)
        neutralized.add(ni)

    joints = skin['joints']
    jset = set(joints)

    def joint_parent(ni):
        p = parent.get(ni)
        while p is not None and p not in jset:
            p = parent.get(p)
        return p

    ordered, seen = [], set()

    def add(ni):
        if ni in seen:
            return
        p = joint_parent(ni)
        if p is not None:
            add(p)
        seen.add(ni)
        ordered.append(ni)
    for ni in joints:
        add(ni)
    old_to_new = {old: ordered.index(old) for old in joints}
    perm = [joints.index(ni) for ni in ordered]
    skin['joints'] = ordered

    new_ibms = []
    for ni in ordered:
        w = worlds_by_name[resolve(nodes[ni]['name'])]
        inv = w.inv()
        q = to_gltf_q(inv.q)
        t = to_gltf_t(inv.t)
        x, y, z, ww = q
        R = [
            [1-2*(y*y+z*z), 2*(x*y-z*ww), 2*(x*z+y*ww)],
            [2*(x*y+z*ww), 1-2*(x*x+z*z), 2*(y*z-x*ww)],
            [2*(x*z-y*ww), 2*(y*z+x*ww), 1-2*(x*x+y*y)],
        ]
        M = [[R[r][c]*inv.s[c] for c in range(3)] + [t[r]] for r in range(3)] + [[0, 0, 0, 1]]
        new_ibms.append(tuple(M[r][c] for c in range(4) for r in range(4)))
    write_accessor(gltf, blob, skin['inverseBindMatrices'], new_ibms)

    for mesh in gltf['meshes']:
        for prim in mesh['primitives']:
            attrs = prim['attributes']
            if 'JOINTS_0' not in attrs:
                continue
            jnt = read_accessor(gltf, blob, attrs['JOINTS_0'])
            wgt = read_accessor(gltf, blob, attrs['WEIGHTS_0'])
            remapped = []
            for row, ws in zip(jnt, wgt):
                remapped.append(tuple(
                    old_to_new[joints[j]] if w != 0 else 0
                    for j, w in zip(row, ws)
                ))
            write_accessor(gltf, blob, attrs['JOINTS_0'], remapped)

    def fold_chain(ni):
        chain = []
        p = parent.get(ni)
        while p is not None:
            if p not in jset:
                chain.append(p)
            p = parent.get(p)
        return list(reversed(chain))

    def node_local_xform(ni, frame):
        if ni in neutralized:
            return Xform()
        name = nodes[ni].get('name')
        if name in alias:
            name = alias[name]
        if name in frame:
            return frame[name]
        if name in synth_local:
            return synth_local[name]
        ai = avatar_index.get(name)
        if ai is not None:
            return avatar.default_local[ai]
        n = nodes[ni]
        t = n.get('translation', [0, 0, 0])
        q = n.get('rotation', [0, 0, 0, 1])
        s = n.get('scale', [1, 1, 1])
        return Xform((-t[0], t[1], t[2]), (q[0], -q[1], -q[2], q[3]), s)

    binext = bytearray()
    clip_specs = []
    for clip_name, frames_list in clips:
        frame_count = len(frames_list)
        times = [f/anim_fps for f in range(frame_count)]
        time_off = len(binext)
        for t in times:
            binext += struct.pack('<f', t)

        channel_data = []
        for ni in ordered:
            jp = joint_parent(ni)
            tvals, qvals, svals = [], [], []
            for frame in frames_list:
                if jp is None:
                    x = Xform()
                    for anc in fold_chain(ni):
                        x = x.mul(node_local_xform(anc, frame))
                    x = x.mul(node_local_xform(ni, frame))
                else:
                    x = node_local_xform(ni, frame)
                tvals.append(to_gltf_t(x.t))
                qvals.append(to_gltf_q(x.q))
                svals.append(x.s)
            for path, vals, ncomp in (('translation', tvals, 3), ('rotation', qvals, 4), ('scale', svals, 3)):
                off = len(binext)
                for v in vals:
                    binext += struct.pack('<' + 'f'*ncomp, *v)
                channel_data.append((ni, path, off, ncomp))
        clip_specs.append((clip_name, times, time_off, channel_data, frame_count))

    base = len(blob)
    blob += binext
    gltf['buffers'][0]['byteLength'] = len(blob)

    gltf['animations'] = []
    for clip_name, times, time_off, channel_data, frame_count in clip_specs:
        gltf['bufferViews'].append({'buffer': 0, 'byteOffset': base + time_off, 'byteLength': 4*frame_count})
        acc_time = len(gltf['accessors'])
        gltf['accessors'].append({
            'bufferView': len(gltf['bufferViews'])-1, 'componentType': 5126, 'count': frame_count,
            'type': 'SCALAR', 'min': [times[0]], 'max': [times[-1]],
        })
        samplers, channels = [], []
        for ni, path, off, ncomp in channel_data:
            gltf['bufferViews'].append({'buffer': 0, 'byteOffset': base + off, 'byteLength': 4*ncomp*frame_count})
            gltf['accessors'].append({
                'bufferView': len(gltf['bufferViews'])-1, 'componentType': 5126,
                'count': frame_count, 'type': 'VEC4' if ncomp == 4 else 'VEC3',
            })
            samplers.append({'input': acc_time, 'output': len(gltf['accessors'])-1, 'interpolation': 'LINEAR'})
            channels.append({'sampler': len(samplers)-1, 'target': {'node': ni, 'path': path}})
        gltf['animations'].append({'name': clip_name, 'samplers': samplers, 'channels': channels})
    save_glb(dst_path, gltf, blob)
    avatar.last_bind_unit_correction = bind_unit_correction
    avatar.last_stripped_root_helpers = stripped_root_helpers
    return dst_path
