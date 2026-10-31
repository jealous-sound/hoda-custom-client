import json
import copy
import hashlib
import os
import re
import traceback
import zlib

TOOLS = os.path.dirname(os.path.abspath(__file__))
CHAR_PACK = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'char_pack')
SCRATCH = os.path.join(TOOLS, 'generic_tmp')

RACE_PREFIXES = {'hm', 'hf', 'dm', 'df', 'em', 'ef', 'qm', 'qf', 'gm', 'gf'}
RIG_RACES = {'humanmale': 'hm', 'humanfemale': 'hf', 'elfmale': 'em', 'elffemale': 'ef',
             'dwarfmale': 'dm', 'dwarffemale': 'df', 'qunarimale': 'qm', 'qunarifemale': 'qf'}
GENERIC_TOKENS = {'joint', 'bone', 'root', 'dummy', 'bip', 'bip01', 'base', 'unit'}
MAX_DONOR_TRIES = 60

AUTHORITATIVE_DONORS = {
    'c_theformlessone': ('cab-c_riftwraith_t4_pre', 'c_riftwraith_t1234_fbx'),
    'c_caress': ('cab-c_desiredemon_t1_pre', 'c_desiredemon_t1_tier_1_0'),
    'c_icegiant': ('cab-c_giant_t1_pre', 'c_icegiant_t1_mesh_0'),
}

ABOMINATION_SKIN_BONES = {
    1: (
        'default_LeftWrist', 'default_LeftLowerArm', 'default_LeftLowerLeg',
        'default_LeftAnkle', 'default_Right2', 'default_RightAnkle',
        'default_Chest', 'default_LeftCollar', 'default_LowerBack',
        'default_LeftUpperArm', 'default_RightCollar', 'default_',
        'default_Head', 'default_RightUpperArm', 'default_RightLowerArm',
        'default_RightWrist', 'default_Root', 'default_LeftUpperLeg',
        'default_RightUpperLeg',
    ),
    2: (
        'default_LeftWrist', 'default_LeftLowerArm', 'default_LeftAnkle',
        'default_LeftLowerLeg', 'default_Right2', 'default_RightAnkle',
        'default_RightLowerArm', 'default_RightWrist', 'default_Chest',
        'default_RightCollar', 'default_LeftCollar', 'default_LowerBack',
        'default_Head', 'default_', 'default_LeftUpperArm',
        'default_RightUpperArm', 'default_Root', 'default_LeftUpperLeg',
        'default_RightUpperLeg',
    ),
    3: (
        'default_Chest', 'default_LowerBack', 'default_Root',
        'default_LeftLowerArm', 'default_LeftWrist', 'default_RightCollar',
        'default_LeftCollar', 'default_Head', 'default_',
        'default_LeftUpperArm', 'default_RightUpperArm', 'default_LeftAnkle',
        'default_LeftLowerLeg', 'default_Right2', 'default_RightAnkle',
        'default_LeftUpperLeg', 'default_RightUpperLeg',
        'default_RightLowerArm', 'default_RightWrist',
    ),
    4: (
        'default_LeftLowerArm', 'default_LeftWrist', 'default_Chest',
        'default_RightCollar', 'default_LeftCollar', 'default_LowerBack',
        'default_Head', 'default_', 'default_LeftUpperArm',
        'default_RightUpperArm', 'default_LeftAnkle', 'default_LeftLowerLeg',
        'default_Right2', 'default_RightAnkle', 'default_Root',
        'default_LeftUpperLeg', 'default_RightUpperLeg',
        'default_RightLowerArm', 'default_RightWrist',
    ),
}

WISP_MESH_TIER = {
    'common': 1,
    'uncommon': 2,
    'rare': 3,
    'epic': 4,
    'legendary': 4,
    'mythic': 4,
}
WISP_BASE_RE = re.compile(
    r'c_wisp(?:giant)?_(common|uncommon|rare|epic|legendary|mythic)_t0'
)


def unit_ratio(gltf, avatar_tt, g2a, par):
    tos_names, av_par = avatar_skeleton(avatar_tt)
    pose = avatar_tt['m_Avatar']['m_DefaultPose']['data']['m_X']
    acc = []
    for i, x in enumerate(pose):
        s = (x['s']['x'] + x['s']['y'] + x['s']['z']) / 3.0
        acc.append((acc[av_par[i]] if av_par[i] >= 0 else 1.0) * s)
    gacc = {}

    def g_acc(i):
        if i not in gacc:
            n = gltf['nodes'][i]
            s = sum(n.get('scale', [1, 1, 1])) / 3.0
            p = par.get(i)
            gacc[i] = (g_acc(p) if p is not None else 1.0) * s
        return gacc[i]

    ratios = []
    for gi, ai in g2a.items():
        t = gltf['nodes'][gi].get('translation', [0, 0, 0])
        p = par.get(gi)
        glb_len = sum(c * c for c in t) ** 0.5 * (g_acc(p) if p is not None else 1.0)
        x = pose[ai]['t']
        pacc = acc[av_par[ai]] if av_par[ai] >= 0 else 1.0
        av_len = (x['x']**2 + x['y']**2 + x['z']**2) ** 0.5 * pacc
        if glb_len > 0.05 and av_len > 1e-6:
            ratios.append(av_len / glb_len)
    if not ratios:
        return 1.0
    ratios.sort()
    return ratios[len(ratios) // 2]


def norm_bone(n):
    return re.sub(r'\d+$', '', n or '').lower()


def load_src_json(dirname, base):
    g = f'{CHAR_PACK}/{dirname}/{base}.gltf'
    if os.path.exists(g):
        return json.load(open(g))
    p = f'{CHAR_PACK}/{dirname}/{base}.glb'
    if os.path.exists(p):
        import glb_writer
        return glb_writer.load_glb(p)[0]
    return None


def parent_map(gltf):
    par = {}
    for i, n in enumerate(gltf['nodes']):
        for c in n.get('children', []):
            par[c] = i
    return par


def repair_wisp_skin(gltf, base):
    match = WISP_BASE_RE.fullmatch(base.lower())
    if match is None:
        return None
    tier = WISP_MESH_TIER[match.group(1)]
    expected_mesh = f'c_abomination_t{tier}'
    expected_names = ABOMINATION_SKIN_BONES[tier]

    skins = gltf.get('skins', [])
    nodes = gltf.get('nodes', [])
    meshes = gltf.get('meshes', [])
    accessors = gltf.get('accessors', [])
    if len(skins) != 1:
        return None
    skin = skins[0]
    try:
        bind_count = accessors[skin['inverseBindMatrices']]['count']
    except (KeyError, IndexError, TypeError):
        return None
    if bind_count != len(expected_names):
        return None

    skinned = [
        (i, node) for i, node in enumerate(nodes)
        if node.get('skin') == 0 and 'mesh' in node
        and 0 <= node['mesh'] < len(meshes)
        and meshes[node['mesh']].get('name') == expected_mesh
    ]
    if len(skinned) != 1:
        return None
    mesh_node, _ = skinned[0]
    par = parent_map(gltf)
    wrapper = par.get(mesh_node)
    if wrapper is None or nodes[wrapper].get('name') != expected_mesh + '_fbx':
        return None

    current = skin.get('joints', [])
    try:
        current_names = [nodes[i].get('name', '') for i in current]
    except (IndexError, TypeError):
        return None
    if len(current) == len(expected_names):
        return None
    if len(current) != 3 or set(current_names) != {
        'default_Root', 'default_LowerBack', 'default_Chest'
    }:
        return None

    descendants, stack = set(), list(nodes[wrapper].get('children', []))
    while stack:
        node_index = stack.pop()
        if node_index in descendants or not 0 <= node_index < len(nodes):
            return None
        descendants.add(node_index)
        stack.extend(nodes[node_index].get('children', []))
    by_name = {}
    for node_index in descendants:
        by_name.setdefault(nodes[node_index].get('name', ''), []).append(node_index)
    if any(len(by_name.get(name, [])) != 1 for name in expected_names):
        return None

    repaired = [by_name[name][0] for name in expected_names]
    if any(node_index in descendants for node_index in current):
        return None
    skin['joints'] = repaired
    return f'repaired {expected_mesh} skin joints 3->{len(repaired)}'


def skeleton_nodes(gltf):
    nodes = gltf['nodes']
    par = parent_map(gltf)
    joints = list(gltf['skins'][0]['joints'])
    keep = set()
    for j in joints:
        ni = j
        while ni is not None and ni not in keep:
            keep.add(ni)
            ni = par.get(ni)
    stack = [c for j in joints for c in nodes[j].get('children', [])]
    while stack:
        c = stack.pop()
        name = nodes[c].get('name', '')
        if c in keep or 'mesh' in nodes[c] or name.startswith('vfx.') or name == 'Particle System':
            continue
        keep.add(c)
        stack.extend(nodes[c].get('children', []))
    return joints, keep, par


def gltf_scaffolding(gltf, base, joints, par):
    nodes = gltf['nodes']
    anc = set()
    ni = joints[0] if joints else None
    while ni is not None:
        anc.add(ni)
        ni = par.get(ni)
    cands = [i for i, n in enumerate(nodes) if n.get('name', '').endswith('_fbx')]
    best = ([i for i in cands if i in anc]
            or [i for i in cands if nodes[i].get('name') == f'{base}_fbx']
            or cands)
    if not best:
        best = [i for i in anc
                if not nodes[i].get('name', '').startswith('unit')
                and (par.get(i) is None or nodes[par[i]].get('name', '').startswith('unit'))]
    if not best:
        return None, None, None
    n = nodes[best[0]]
    s = n.get('scale', [1.0, 1.0, 1.0])[0]
    y = n.get('translation', [0.0, 0.0, 0.0])[1]
    return round(s, 6), round(y, 6), n.get('name')


def _skinned_surface_binding(gltf, base, mesh_name=None):
    skinned = [(i, n) for i, n in enumerate(gltf.get('nodes', []))
               if 'mesh' in n and 'skin' in n]
    if mesh_name is not None:
        skinned = [(i, n) for i, n in skinned
                   if gltf['meshes'][n['mesh']].get('name') == mesh_name]
    if len(skinned) != 1:
        return None
    mesh = gltf['meshes'][skinned[0][1]['mesh']]
    primitives = mesh.get('primitives', [])
    if len(primitives) != 1:
        return None
    try:
        material = gltf['materials'][primitives[0]['material']]
        if material.get('name', '').lower() != base.lower():
            return None
        texture = material['pbrMetallicRoughness']['baseColorTexture']['index']
        image = gltf['textures'][texture]['source']
        gltf['images'][image]
    except (KeyError, IndexError, TypeError):
        return None
    return mesh.get('name', ''), image


def _file_sha1(path):
    with open(path, 'rb') as stream:
        return hashlib.sha1(stream.read()).hexdigest()


def repair_donor_texture(source_gltf, dst_glb, dirname, base):
    import glb_writer

    source_binding = _skinned_surface_binding(source_gltf, base)
    match = re.search(r'_t([1-4])$', base.lower())
    if source_binding is None or match is None:
        return []
    _, source_image = source_binding
    current_uri = source_gltf['images'][source_image].get('uri', '')
    tier = match.group(1)
    family = base[:match.start()]
    out_dir = os.path.join(CHAR_PACK, dirname)

    from PIL import Image
    candidates = []
    pattern = re.compile(
        re.escape(family) + r'_t([1234]+)(?:_tex)?(?:_\d+)?\.png$', re.IGNORECASE)
    for name in os.listdir(out_dir):
        candidate = pattern.fullmatch(name)
        if candidate is None or tier not in candidate.group(1):
            continue
        path = os.path.join(out_dir, name)
        try:
            with Image.open(path) as image:
                if image.width != image.height or image.width < 512:
                    continue
            digest = _file_sha1(path)
        except (OSError, ValueError):
            continue
        rank = 0 if candidate.group(1) == tier else 1
        candidates.append((rank, digest, name))
    if not candidates:
        return []

    best_rank = min(row[0] for row in candidates)
    best = [row for row in candidates if row[0] == best_rank]
    hashes = {row[1] for row in best}
    if len(hashes) != 1:
        return [f'donor albedo ambiguous for {base}']
    expected_hash = next(iter(hashes))
    expected_uri = min(
        (row[2] for row in best),
        key=lambda name: (bool(re.search(r'_\d+\.png$', name, re.IGNORECASE)), len(name), name))

    current_path = os.path.join(out_dir, current_uri)
    try:
        current_hash = _file_sha1(current_path)
    except OSError:
        current_hash = None
    if current_hash == expected_hash:
        return []

    output_gltf, blob = glb_writer.load_glb(dst_glb)
    output_binding = _skinned_surface_binding(output_gltf, base, mesh_name=source_binding[0])
    if output_binding is None:
        return [f'donor texture repair could not match output mesh {source_binding[0]}']
    output_gltf['images'][output_binding[1]]['uri'] = expected_uri
    glb_writer.save_glb(dst_glb, output_gltf, blob)
    return [f'donor albedo {current_uri} -> {expected_uri}']


def avatar_skeleton(avatar_tt):
    tos = {h: p for h, p in avatar_tt.get('m_TOS', [])}
    sk = avatar_tt['m_Avatar']['m_AvatarSkeleton']['data']
    parents = [n['m_ParentId'] for n in sk['m_Node']]
    names = [tos.get(h, '').split('/')[-1] for h in sk['m_ID']]
    return names, parents


def match_rig(gltf, avatar_tt, joints, keep, par):
    nodes = gltf['nodes']
    av_names, av_par = avatar_skeleton(avatar_tt)

    def gdepth(i):
        d = 0
        while i in par:
            i = par[i]
            d += 1
        return d

    def adepth(i):
        d = 0
        while av_par[i] >= 0:
            i = av_par[i]
            d += 1
        return d

    g2a, used = {}, set()
    for key in (lambda n: n, norm_bone):
        gg, aa = {}, {}
        for i in sorted(keep):
            if i not in g2a:
                gg.setdefault(key(nodes[i].get('name', '')), []).append(i)
        for i, nm in enumerate(av_names):
            if i not in used and nm:
                aa.setdefault(key(nm), []).append(i)
        for k, gs in gg.items():
            if not k or len(gs) != len(aa.get(k, [])):
                continue
            for gi, ai in zip(sorted(gs, key=gdepth), sorted(aa[k], key=adepth)):
                g2a[gi] = ai
                used.add(ai)
    changed = True
    while changed:
        changed = False
        for gi in sorted(keep):
            if gi in g2a or par.get(gi) not in g2a:
                continue
            pai = g2a[par[gi]]
            mapped_kids = {g2a[c] for c in nodes[gi].get('children', []) if c in g2a}
            cands = [ai for ai in range(len(av_names))
                     if ai not in used and av_par[ai] == pai
                     and all(av_par[m] == ai for m in mapped_kids)]
            if len(cands) == 1:
                g2a[gi] = cands[0]
                used.add(cands[0])
                changed = True
    if any(j not in g2a for j in joints):
        return None
    for gi, ai in g2a.items():
        pgi = par.get(gi)
        if pgi in g2a and g2a[pgi] != av_par[ai]:
            return None
    return g2a


def authoritative_donor(base, gltf):
    family = re.sub(r'_t\d+$', '', base).lower()
    override = AUTHORITATIVE_DONORS.get(family)
    if override is None:
        return None
    donor, mesh_name = override
    names = {mesh.get('name', '').lower() for mesh in gltf.get('meshes', [])}
    return donor if mesh_name in names else None


def donor_candidates(base, gltf, joints, clip_path, cab_paths, clip_name=None):
    nodes = gltf['nodes']
    fam = re.sub(r'_t\d+$', '', base)
    tiers = ('t4', 't3', 't2', 't1')
    preferred = authoritative_donor(base, gltf)
    cands = ([preferred] if preferred else []) + [f'cab-{fam}_{t}_pre' for t in tiers]
    if clip_name:
        for tok in re.sub(r'_(ready|rdy|idle|ani|v?\d+)\b', '_', clip_name).split('_'):
            if len(tok) >= 4 and tok.lower() not in GENERIC_TOKENS:
                cands += sorted(k for k in cab_paths
                                if tok.lower() in k and k.endswith('_pre'))[:10]
    for n in nodes:
        nm = n.get('name', '')
        if nm.endswith('_fbx'):
            stem = nm[:-4].lower()
            cands.append(f'cab-{stem}_pre')
            sfam = re.sub(r'_t\d+$', '', stem)
            cands += [f'cab-{sfam}_{t}_pre' for t in tiers]
    if clip_path:
        cands.append(os.path.basename(clip_path).lower())
    parts0 = fam.split('_')
    last = parts0[-1]
    for i in range(1, len(last) - 4):
        sub = last[i:]
        hits = sorted(k for k in cab_paths if f'_{sub}_t' in k and k.endswith('_pre'))
        if hits:
            cands += hits[:8]
            break
    toks = {}
    for j in joints:
        tok = nodes[j].get('name', '').split('_')[0]
        if len(norm_bone(tok)) >= 3 and norm_bone(tok) not in GENERIC_TOKENS:
            toks[tok] = toks.get(tok, 0) + 1
    top_toks = [t for t, _ in sorted(toks.items(), key=lambda kv: -kv[1])[:2]]
    parts = fam.split('_')
    races = {parts[0]} if parts[0] in RACE_PREFIXES else set()
    races |= {RIG_RACES[norm_bone(t)] for t in top_toks if norm_bone(t) in RIG_RACES}
    for race in sorted(races):
        fams_seen = set()
        for k in sorted(k for k in cab_paths
                        if k.startswith(f'cab-{race}_') and k.endswith('_pre')):
            f = re.sub(r'_t\d+_pre$', '', k)
            if f in fams_seen:
                continue
            fams_seen.add(f)
            cands.append(k)
            if len(fams_seen) >= 20:
                break
    for tok in top_toks:
        subs = {tok.lower()} | {s.lower() for s in re.findall(r'[A-Z][a-z]+|^[a-z]+', tok)}
        for sub in sorted(subs):
            if len(sub) >= 3 and sub not in GENERIC_TOKENS:
                cands += sorted(k for k in cab_paths if sub in k and k.endswith('_pre'))[:10]
    seen, out = set(), []
    for c in cands:
        if c not in seen and c in cab_paths:
            seen.add(c)
            out.append(c)
    return out[:MAX_DONOR_TRIES]


def donor_matches(base, gltf, joints, keep, par, clip_path, cab_paths, H, tried, clip_name=None):
    preferred = authoritative_donor(base, gltf)
    for cabname in donor_candidates(base, gltf, joints, clip_path, cab_paths, clip_name):
        try:
            env = H.load_env(cab_paths[cabname])
            avatar_tt, rig = H.read_avatar(env)
            if avatar_tt is None:
                continue
            g2a = match_rig(gltf, avatar_tt, joints, keep, par)
            if g2a is None:
                tried.append(cabname)
                continue
            r = unit_ratio(gltf, avatar_tt, g2a, par)
            if cabname != preferred and not (0.5 <= r <= 2.0):
                tried.append(f'{cabname}!unit-ratio={r:.2f}')
                continue
            yield cabname, avatar_tt, rig, g2a
        except Exception:
            tried.append(cabname + '!err')


def _xj(t, q, s):
    return {'t': {'x': t[0], 'y': t[1], 'z': t[2]},
            'q': {'x': q[0], 'y': q[1], 'z': q[2], 'w': q[3]},
            's': {'x': s[0], 'y': s[1], 'z': s[2]}}


def _build_synth(nodes, keep, par, root):
    order, stack = [root], [c for c in nodes[root].get('children', []) if c in keep]
    while stack:
        gi = stack.pop(0)
        order.append(gi)
        stack.extend(c for c in nodes[gi].get('children', []) if c in keep)
    idx = {gi: k for k, gi in enumerate(order)}
    paths = {root: ''}
    m_node, m_id, m_x, tos = [], [], [], []
    for gi in order:
        n = nodes[gi]
        if gi == root:
            pk, path = -1, ''
            t, q, s = (0.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0), (1.0, 1.0, 1.0)
        else:
            pgi = par[gi]
            pk = idx[pgi]
            name = n.get('name', '')
            path = name if pgi == root else paths[pgi] + '/' + name
            t = n.get('translation', [0.0, 0.0, 0.0])
            q = n.get('rotation', [0.0, 0.0, 0.0, 1.0])
            s = n.get('scale', [1.0, 1.0, 1.0])
            t = (-t[0], t[1], t[2])
            q = (q[0], -q[1], -q[2], q[3])
        paths[gi] = path
        m_node.append({'m_ParentId': pk, 'm_AxesId': -1})
        m_id.append(zlib.crc32(path.encode()) & 0xffffffff if path else 0)
        m_x.append(_xj(t, q, s))
        tos.append([m_id[-1], path])
    return {'m_Avatar': {'m_AvatarSkeleton': {'data': {'m_Node': m_node, 'm_ID': m_id}},
                         'm_DefaultPose': {'data': {'m_X': m_x}},
                         'm_Human': {'data': {'m_Skeleton': {'data': {'m_Node': []}}}}},
            'm_TOS': tos}


def synth_avatar(gltf, joints, keep, par, clip_tt):
    bindings = clip_tt['m_ClipBindingConstant']['genericBindings']
    want = {b['path'] for b in bindings if b.get('classID', b.get('typeID')) == 4}
    nodes = gltf['nodes']
    chain, ni = [], joints[0]
    while ni is not None:
        chain.append(ni)
        ni = par.get(ni)
    best = None
    for root in chain[1:] or chain:
        av = _build_synth(nodes, keep, par, root)
        got = len(want & set(av['m_Avatar']['m_AvatarSkeleton']['data']['m_ID']))
        if best is None or got > best[0]:
            best = (got, av, len(want))
    if best and best[0] > 0:
        return best[1], f'{best[0]}/{best[2]} bindings resolved'
    return None, f'0/{len(want)} bindings resolved from glb paths'


def rename_glb(src, names, tmp):
    import glb_writer
    gltf, blob = glb_writer.load_glb(src)
    changed = 0
    for gi, new in names.items():
        if gltf['nodes'][gi].get('name') != new:
            gltf['nodes'][gi]['name'] = new
            changed += 1
    if not changed:
        return src, 0
    glb_writer.save_glb(tmp, gltf, blob)
    return tmp, changed


def take_manifest(dirname, base):
    if not base:
        return {}
    path = os.path.join(CHAR_PACK, dirname, f'{base}_fbx_takes.json')
    if not os.path.exists(path):
        return {}
    with open(path) as stream:
        return json.load(stream)


def sidecar_clips(dirname, base=None):
    out = {}
    d = f'{CHAR_PACK}/{dirname}'
    for f in sorted(os.listdir(d)):
        if f.startswith('anim_') and f.endswith('.json'):
            stem = re.sub(r'_\d+$', '', f[5:-5])
            out.setdefault(stem, os.path.join(d, f))
    data = take_manifest(dirname, base)
    for stem in data.get('animation_stacks', []):
        if isinstance(stem, str) and stem:
            out.setdefault(stem, None)
    for stem in data.get('clip_roles', {}).values():
        if isinstance(stem, str) and stem:
            out.setdefault(stem, None)
    return out


def bake_embedded(dst, gltf, blob, manifest, row, scale, notes, force, H):
    roles = manifest.get('embedded_clip_roles')
    if not roles:
        return None
    animations = gltf.get('animations', [])
    expected_names = manifest.get('embedded_source_names', [])
    if len(animations) != len(roles) or len(expected_names) != len(roles):
        raise ValueError(
            f'embedded clip count {len(animations)} does not match mapping {len(roles)}')
    actual_names = [animation.get('name', '') for animation in animations]
    if actual_names != expected_names:
        raise ValueError(f'embedded clip names changed: {actual_names} != {expected_names}')

    source_names = dict(zip(roles, actual_names))
    for animation, role in zip(animations, roles):
        animation['name'] = role
    by_role = {animation['name']: animation for animation in animations}
    for role, source_role in manifest.get('duplicate_embedded_clips', {}).items():
        if source_role not in by_role:
            raise ValueError(f'embedded fallback {role} refers to missing {source_role}')
        duplicate = copy.deepcopy(by_role[source_role])
        duplicate['name'] = role
        animations.append(duplicate)
        source_names[role] = source_names[source_role]

    parent = parent_map(gltf)
    skin_joints = {joint for skin in gltf.get('skins', []) for joint in skin.get('joints', [])}
    neutral = set()
    for node_index, node in enumerate(gltf['nodes']):
        if 'mesh' not in node:
            continue
        ancestor = parent.get(node_index)
        while ancestor is not None:
            if ancestor not in skin_joints:
                neutral.add(ancestor)
            ancestor = parent.get(ancestor)
    for node_index in neutral:
        for key in ('matrix', 'translation', 'rotation', 'scale'):
            gltf['nodes'][node_index].pop(key, None)

    if not os.path.exists(dst) or force:
        import glb_writer
        glb_writer.save_glb(dst, gltf, blob)
    check = H.check_glb(dst, display_scale=scale)
    expected_durations = manifest.get('embedded_durations', [])
    for role, expected in zip(roles, expected_durations):
        actual = check.get('clip_durations', {}).get(role)
        if actual is None or abs(actual - expected) > 0.02:
            raise ValueError(f'{role} duration {actual} != expected {expected}')
    if not check.get('ok') or not check.get('ground_ok'):
        raise ValueError(f'embedded GLB validation failed: {check}')
    row['status'] = 'ok'
    row['rig'] = 'embedded-fbx'
    row['clip'] = source_names['idle']
    row['clip_method'] = 'embedded-fbx'
    row['avatar_donor'] = None
    row['clips'] = source_names
    row['clip_sources'] = {role: 'embedded-fbx' for role in source_names}
    absent = manifest.get('intentionally_absent_roles', [])
    if absent:
        row['clips_intentionally_absent'] = absent
    row['check'] = check
    row['detail'] = '; '.join(notes + [manifest.get('mapping', 'embedded FBX clips')])
    return row


def load_clip(stem, sidecar_path, cab_paths, clip_index, H):
    p = clip_index.get(stem)
    if p:
        env = H.load_env(p)
        for o in env.objects:
            if o.type.name == 'AnimationClip' and H.obj_name(o) == stem:
                return H.read_clip_tt(o), 'obb', p
    if sidecar_path is None:
        raise FileNotFoundError(f'clip {stem!r} is named by the FBX but absent from the OBB')
    tt = json.load(open(sidecar_path))
    for b in tt['m_ClipBindingConstant']['genericBindings']:
        if 'classID' not in b:
            b['classID'] = b.get('typeID')
    return tt, 'sidecar-5.x', None


def process(dirname, base, row, cab_paths, clip_index, force, H):
    notes = ['donor-mode: no own CAB bundle']
    dst = f'{CHAR_PACK}/{dirname}/{base}_rl.glb'
    scale_entry = None
    try:
        gltf = load_src_json(dirname, base)
        if gltf is None or not gltf.get('skins'):
            row['status'] = 'no-bundle'
            row['detail'] = 'no own bundle and no skinned source gltf'
            return row, None
        skin_note = repair_wisp_skin(gltf, base)
        if skin_note:
            notes.append(skin_note)
        joints, keep, par = skeleton_nodes(gltf)
        scale, y, fbx_name = gltf_scaffolding(gltf, base, joints, par)
        row['scale'] = scale
        scale_entry = {'scale': scale, 'y': y} if scale is not None else None
        if fbx_name:
            notes.append(f'scaffold from gltf node {fbx_name}')

        manifest = take_manifest(dirname, base)
        if manifest.get('embedded_clip_roles'):
            import glb_writer
            source_gltf, source_blob = glb_writer.load_glb(f'{CHAR_PACK}/{dirname}/{base}.glb')
            embedded = bake_embedded(
                dst, source_gltf, source_blob,
                manifest, row, scale, notes, force, H,
            )
            return embedded, scale_entry

        clips = sidecar_clips(dirname, base)
        role_overrides = manifest.get('clip_roles', {})
        stem = role_overrides.get('idle')
        alts = []
        if stem is None:
            stem, alts = H.pick_clip(clips)
        if stem is None:
            wind = sorted(n for n in clips if 'wind' in n.lower())
            if wind:
                stem, alts = wind[0], wind[1:]
        if stem is None:
            row['status'] = 'no-clip'
            row['detail'] = '; '.join(notes + [f'sidecar clips {sorted(clips)}'])
            return row, scale_entry
        row['clip'] = stem
        clip_tt, csrc, clip_path = load_clip(stem, clips[stem], cab_paths, clip_index, H)
        row['clip_method'] = f'donor:{csrc}'
        if alts:
            row['clip_alternatives'] = alts[:8]

        sidecar_roles = H.assign_roles(set(clips) - {stem})
        idle_stem = re.sub(r'_?(ready|rdy|idle)\w*$', '', stem or '', flags=re.I)
        for role, name in H.UNIT_CLIP_OVERRIDES.get(idle_stem, {}).items():
            if name in clips or name in clip_index:
                sidecar_roles[role] = name
        for role, name in role_overrides.items():
            if role != 'idle':
                sidecar_roles[role] = name
        _extras_cache = {}

        def build_extras(rig):
            if rig in _extras_cache:
                return _extras_cache[rig]
            roles = dict(sidecar_roles)
            if rig == 'humanoid':
                fam = H.humanoid_family(base, stem)
                if fam:
                    H.fill_family_roles(roles, fam)
            extras, srcs = [], {}
            for role in H.ROLE_ORDER:
                nm = roles.get(role)
                if not nm:
                    continue
                try:
                    if nm in clips:
                        tt, esrc, _ = load_clip(nm, clips[nm], cab_paths, clip_index, H)
                        esrc = f'donor:{esrc}'
                    else:
                        tt, esrc = H.load_index_clip(nm, clip_index), 'family-index'
                except Exception:
                    continue
                if tt is None or not H.clip_type_ok(tt, rig):
                    continue
                extras.append((role, tt))
                srcs[role] = (nm, esrc)
            _extras_cache[rig] = (extras, srcs)
            return extras, srcs

        def record_existing(rig, donor_label):
            _, srcs = build_extras(rig)
            check = H.check_glb(dst, display_scale=scale)
            row['rig'] = rig
            row['avatar_donor'] = donor_label
            row['check'] = check
            row['clips'] = {'idle': stem}
            row['clip_sources'] = {'idle': f'donor:{csrc}'}
            for role in check.get('anims', [])[1:]:
                name, source = srcs.get(role, (role, 'existing-glb'))
                row['clips'][role] = name
                row['clip_sources'][role] = source
            missing = [role for role in ('attack', 'hit', 'death')
                       if role not in row['clips']]
            if missing:
                row['clips_missing'] = missing
            notes.extend(repair_donor_texture(gltf, dst, dirname, base))
            row['detail'] = '; '.join(
                notes + [f'avatar donor {donor_label}', 'exists, bake skipped'])
            return row, scale_entry

        tried = []

        def bake_with(avatar_tt, rig, mapping):
            src = H.ensure_src_glb(dirname, base)
            if src is None:
                raise FileNotFoundError('no source glb/gltf')
            bnotes = []
            if WISP_BASE_RE.fullmatch(base.lower()):
                import glb_writer
                source_gltf, source_blob = glb_writer.load_glb(src)
                if repair_wisp_skin(source_gltf, base):
                    os.makedirs(SCRATCH, exist_ok=True)
                    repaired_src = os.path.join(SCRATCH, base + '_wisp_skin.glb')
                    glb_writer.save_glb(repaired_src, source_gltf, source_blob)
                    src = repaired_src
            if mapping:
                av_names, _ = avatar_skeleton(avatar_tt)
                names = {gi: av_names[ai] for gi, ai in mapping.items() if av_names[ai]}
                os.makedirs(SCRATCH, exist_ok=True)
                src, changed = rename_glb(src, names, os.path.join(SCRATCH, base + '_donor.glb'))
                if changed:
                    bnotes.append(f'renamed {changed} nodes to donor rig')
            extras, srcs = build_extras(rig)
            if rig == 'humanoid':
                info = H.bake_humanoid(src, dst, avatar_tt, clip_tt, use_tos=True,
                                       display_scale=scale, extra_clips=extras)
                bnotes.append(f"frames={info['frames']} neutral={info['neutral_check']:.4f}")
                if info['unresolved']:
                    bnotes.append(f"unresolved_nodes={len(info['unresolved'])}")
                if info.get('ground_shift'):
                    bnotes.append(f"grounded feet {info['ground_shift']:+.3f}")
            else:
                info = H.bake_generic_unit(src, dst, base, avatar_tt, clip_tt,
                                           display_scale=scale, extra_clips=extras)
                bnotes.append(f"frames={info['frames']} animated={info['animated']}/{info['total']} "
                              f"shift={info['shift']:+.3f} posed_minY={info['posed_min_y']:.3f}")
                if info['promoted']:
                    bnotes.append(f"promoted={info['promoted']}")
                if info.get('promote_skipped'):
                    bnotes.append(f"promote_skipped={info['promote_skipped']}")
            if info.get('avatar_rescale', 1.0) != 1.0:
                bnotes.append(f"stale decade fold: avatar repaired x{info['avatar_rescale']:g}")
            if info.get('post_scale', 1.0) != 1.0:
                bnotes.append(f"unit normalization: post-scaled x{info['post_scale']:g}")
            chk = H.check_glb(dst, display_scale=scale)
            if chk.get('height_warn'):
                bnotes.append(chk['height_warn'])
            if not chk.get('ground_ok', False):
                bnotes.append(f"bind bounds off ground (minY={chk.get('mesh_min_y')})")
            return bnotes, chk, info, srcs

        def ground_err(chk):
            y = chk.get('mesh_min_y')
            if y is None:
                return 1e9
            return max(0.0, -0.1 - y, y - 0.2)

        preferred = authoritative_donor(base, gltf)
        options = donor_matches(base, gltf, joints, keep, par, clip_path, cab_paths, H,
                                 tried, clip_name=stem)
        best = None
        attempts = 0
        bad_fams = set()
        last_label = None
        for cabname, avatar_tt, rig, mapping in options:
            is_preferred = cabname == preferred
            fam = re.sub(r'_t\d+_pre$', '', cabname)
            if fam in bad_fams:
                continue
            if os.path.exists(dst) and not force:
                return record_existing(rig, cabname)
            attempts += 1
            try:
                bnotes, chk, info, srcs = bake_with(avatar_tt, rig, mapping)
            except (Exception, SystemExit) as e:
                tried.append(f'{cabname}!{type(e).__name__}')
                bad_fams.add(fam)
                if attempts >= 16:
                    break
                continue
            last_label = cabname
            if best is None or is_preferred or ground_err(chk) < ground_err(best[1]):
                best = ([f'avatar donor {cabname}'] + bnotes, chk, cabname, rig,
                        (avatar_tt, rig, mapping), info, srcs)
            if (chk['ok'] and (is_preferred or chk.get('ground_ok', False))) or attempts >= 16:
                break
            tried.append(f'{cabname}!check-fail(minY={chk["mesh_min_y"]})')
            bad_fams.add(fam)
        if best is not None and last_label != best[2]:
            bake_with(*best[4])

        if best is None:
            avatar_tt, snote = synth_avatar(gltf, joints, keep, par, clip_tt)
            if avatar_tt is None:
                row['status'] = 'no-donor'
                row['detail'] = '; '.join(notes + [snote, f'no avatar match (tried {tried[:8]})'])
                return row, scale_entry
            if os.path.exists(dst) and not force:
                return record_existing('generic', 'synthesized-from-glb')
            bnotes, chk, info, srcs = bake_with(avatar_tt, 'generic', None)
            best = ([f'avatar synthesized from glb skeleton ({snote})'] + bnotes,
                    chk, 'synthesized-from-glb', 'generic', None, info, srcs)

        bnotes, chk, donor_label, rig, _, info, srcs = best
        row['rig'] = rig
        row['avatar_donor'] = donor_label
        row['check'] = chk
        row['clips'] = {'idle': stem}
        row['clip_sources'] = {'idle': f'donor:{csrc}'}
        for role, (nm, esrc) in srcs.items():
            row['clips'][role] = nm
            row['clip_sources'][role] = esrc
        if info.get('clip_errors'):
            row['clip_errors'] = {k: v[:160] for k, v in info['clip_errors'].items()}
            for role in info['clip_errors']:
                row['clips'].pop(role, None)
            notes.append(f"clip_errors={sorted(info['clip_errors'])}")
        missing = [r for r in ('attack', 'hit', 'death') if r not in row['clips']]
        if missing:
            row['clips_missing'] = missing
        notes += bnotes
        notes += repair_donor_texture(gltf, dst, dirname, base)
        if not chk['ok']:
            notes.append('CHECK-FAIL')
        imgs = gltf.get('images') or []
        if imgs and not os.path.exists(f"{CHAR_PACK}/{dirname}/{imgs[0].get('uri', '')}"):
            notes.append('image uri missing (no OBB material to export from)')
        row['detail'] = '; '.join(notes)
        return row, scale_entry
    except (Exception, SystemExit) as e:
        row['status'] = 'bake-error'
        row['detail'] = '; '.join(notes + [f'{type(e).__name__}: {e}'])
        row['trace'] = traceback.format_exc(limit=6)
        return row, scale_entry
