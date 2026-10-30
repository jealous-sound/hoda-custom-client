#!/usr/bin/env python
import argparse
import glob as globmod
import json
import os
import re
import struct
import sys
import time
import traceback

from PIL import Image

TOOLS = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, TOOLS)
CACHE_DIR = os.path.join(TOOLS, 'cache')
CHAR_PACK = '/Users/vladislav/heroes-of-dragons/heroes-of-dragons-cache/char_pack'
SHARED = ('/Users/vladislav/heroes-of-dragons/heroes-of-dragons-cache/'
          'Heroes-of-Dragon-Age-v4-4-0-cache-androidp1/com.ea.game.dragonage_row/'
          'files/UnityCache/Shared')
REPORT_PATH = os.path.join(TOOLS, 'bake_report.json')
SCALES_PATH = os.path.join(TOOLS, 'display_scales.json')
DEFAULT_INVENTORY = os.path.join(TOOLS, 'bake_inventory.json')
SCRATCH = os.path.join(TOOLS, 'generic_tmp')
REFERENCE_FBX = os.path.join(
    os.path.dirname(os.path.dirname(TOOLS)),
    '.agents', 'skills', 'hoda-reference', 'references', 'fbx')

import UnityPy
from UnityPy.helpers import UnityVersion as UV

_orig_from_str = UV.UnityVersion.from_str.__func__
def _patched_from_str(cls, version):
    m = re.search(r"(\d+)\.(\d+)\.(\d+)([a-zA-Z]+)(\d+)", version)
    if m:
        version = m.group(0)
    return _orig_from_str(cls, version)
UV.UnityVersion.from_str = classmethod(_patched_from_str)

import glb_writer
from bake_idle import bake_humanoid
from bake_generic import bake_generic_unit

RACE_PREFIXES = {'hm', 'hf', 'dm', 'df', 'em', 'ef', 'qm', 'qf', 'gm', 'gf'}



def build_cab_paths():
    p = os.path.join(CACHE_DIR, 'cab_paths.json')
    if os.path.exists(p):
        return json.load(open(p))
    paths = {}
    for f in globmod.glob(os.path.join(SHARED, '*', 'CAB-*')):
        paths.setdefault(os.path.basename(f).lower(), f)
    os.makedirs(CACHE_DIR, exist_ok=True)
    json.dump(paths, open(p, 'w'))
    return paths


def cab_lookup(cab_paths, name):
    n = name.lower()
    return cab_paths.get(n) or cab_paths.get('cab-' + n)


def obj_name(obj):
    try:
        raw = obj.get_raw_data()
        ln = struct.unpack_from('<i', raw, 0)[0]
        if 0 <= ln < 512:
            return bytes(raw[4:4+ln]).decode('utf-8', 'replace')
    except Exception:
        pass
    try:
        return obj.read_typetree().get('m_Name', '')
    except Exception:
        return ''


def build_clip_index(cab_paths):
    p = os.path.join(CACHE_DIR, 'clip_index.json')
    if os.path.exists(p):
        return json.load(open(p))
    print(f'building clip index over {len(cab_paths)} CABs (one-time)...')
    idx, t0 = {}, time.time()
    for i, (cab, path) in enumerate(sorted(cab_paths.items())):
        try:
            env = UnityPy.load(path)
            for obj in env.objects:
                if obj.type.name != 'AnimationClip':
                    continue
                name = obj_name(obj)
                if name:
                    idx.setdefault(name, path)
        except Exception:
            pass
        if (i + 1) % 500 == 0:
            print(f'  {i+1}/{len(cab_paths)} cabs, {len(idx)} clips, {time.time()-t0:.0f}s')
    os.makedirs(CACHE_DIR, exist_ok=True)
    json.dump(idx, open(p, 'w'))
    print(f'clip index: {len(idx)} clips in {time.time()-t0:.0f}s')
    return idx



_env_cache = {}

def load_env(path):
    if path not in _env_cache:
        if len(_env_cache) > 12:
            _env_cache.clear()
        _env_cache[path] = UnityPy.load(path)
    return _env_cache[path]


def env_objects(env):
    return list(env.objects)


def env_externals(env):
    objs = env_objects(env)
    if not objs:
        return []
    return [getattr(e, 'path', '') or getattr(e, 'name', '') for e in objs[0].assets_file.externals]


def resolve_pptr(env, pptr, cab_paths):
    fid, pid = pptr.get('m_FileID', 0), pptr.get('m_PathID', 0)
    if pid == 0:
        return None
    if fid == 0:
        return env, pid
    exts = env_externals(env)
    if fid - 1 >= len(exts):
        return None
    path = cab_lookup(cab_paths, os.path.basename(exts[fid - 1]))
    if path is None:
        return None
    return load_env(path), pid


def find_by_pid(env, pid):
    for o in env.objects:
        if o.path_id == pid:
            return o
    return None


_clip_tt_cache = {}

def read_clip_tt(obj):
    key = (obj.assets_file.name if hasattr(obj.assets_file, 'name') else id(obj.assets_file),
           obj.path_id)
    if key not in _clip_tt_cache:
        if len(_clip_tt_cache) > 24:
            _clip_tt_cache.clear()
        _clip_tt_cache[key] = obj.read_typetree()
    return _clip_tt_cache[key]


CLIP_PREDICATES = [
    lambda n: '_ready' in n,
    lambda n: '_rdy' in n,
    lambda n: 'ready' in n.lower(),
    lambda n: '_idle' in n,
    lambda n: 'idle' in n.lower(),
]


def pick_clip(names):
    for pred in CLIP_PREDICATES:
        cand = sorted(n for n in names if pred(n))
        if cand:
            return cand[0], cand[1:]
    return None, []


def candidate_prefixes(base):
    family = re.sub(r'_t\d+$', '', base)
    prefixes = [family]
    camel = re.sub(r'(?<=[a-z])[A-Z].*$', '', family)
    if camel != family:
        prefixes.append(camel)
    parts = family.split('_')
    for k in range(len(parts) - 1, 1, -1):
        prefixes.append('_'.join(parts[:k]))
    if parts[0] in RACE_PREFIXES and len(parts) > 1 and len(parts[1]) == 2:
        prefixes.append(parts[1])
    return [p for p in prefixes if len(p) >= 3]


def own_bundle_clips(env):
    own = {}
    for o in env.objects:
        if o.type.name == 'AnimationClip':
            n = obj_name(o)
            if n:
                own[n] = o
    return own


def controller_clips(env, cab_paths, clip_index):
    found, scraped, ctrl_names = {}, set(), []
    for o in env.objects:
        if o.type.name != 'Animator':
            continue
        tt = o.read_typetree()
        hit = resolve_pptr(env, tt.get('m_Controller', {}), cab_paths)
        if hit is None:
            continue
        cenv, cpid = hit
        ctrl_obj = find_by_pid(cenv, cpid)
        if ctrl_obj is None:
            continue
        cname = obj_name(ctrl_obj)
        if cname:
            ctrl_names.append(cname)
        ctt = ctrl_obj.read_typetree()
        pptrs = list(ctt.get('m_AnimationClips', []))
        for pair in ctt.get('m_Clips', []):
            pptrs += [pair.get('m_OriginalClip', {}), pair.get('m_OverrideClip', {})]
        for pp in pptrs:
            h = resolve_pptr(cenv, pp, cab_paths)
            if h is None:
                continue
            e2, pid2 = h
            co = find_by_pid(e2, pid2)
            if co is not None and co.type.name == 'AnimationClip':
                n = obj_name(co)
                if n:
                    found.setdefault(n, co)
        for s in re.findall(rb'[ -~]{4,}', ctrl_obj.get_raw_data()):
            t = s.decode()
            if t in clip_index:
                scraped.add(t)
    return found, scraped, ctrl_names


def discover_clip(env, base, cab_paths, clip_index):
    own = own_bundle_clips(env)
    chosen, alts = pick_clip(own)
    if chosen:
        return read_clip_tt(own[chosen]), chosen, 'own-bundle', alts

    try:
        found, _, _ = controller_clips(env, cab_paths, clip_index)
        chosen, alts = pick_clip(found)
        if chosen:
            return read_clip_tt(found[chosen]), chosen, 'controller', alts
    except Exception:
        pass

    seen = []
    for pref in candidate_prefixes(base):
        for pred in CLIP_PREDICATES:
            cand = sorted(n for n in clip_index if n.startswith(pref) and pred(n))
            if cand:
                chosen, alts = cand[0], cand[1:]
                tt = load_index_clip(chosen, clip_index)
                if tt is not None:
                    return tt, chosen, f'index:{pref}', alts
        seen.append(pref)
    return None, None, 'no-clip', seen



ROLE_ORDER = ('attack', 'attack2', 'hit', 'death', 'cheer')
GLB_CLIP_NAMES = ('idle',) + ROLE_ORDER

FAMILY_BY_CONTROLLER = {'_shield': 'sw', '_dualwield': 'dw', '_mage': 'mw',
                        '_twohanded': 'tw', '_bow_con': 'bw'}
IDLE_TO_FAMILY = {'sw_ready_ani': 'sw', 'dw_ready_ani': 'dw', 'mw_ready06_ani': 'mw',
                  'TwoHanded_Ready': 'tw', 'bw_rdy_ani': 'bw'}
FAMILY_SETS = {
    'sw': {'attack': 'sw_attack01_ani', 'attack2': 'sw_shieldBash_ani',
           'hit': 'TwoHanded_GetHit', 'death': 'TwoHanded_Death'},
    'dw': {'attack': 'dw_attack01_ani', 'attack2': 'dw_scattershot_ani',
           'hit': 'TwoHanded_GetHit', 'death': 'TwoHanded_Death'},
    'mw': {'attack': 'FireMage_Fireball', 'hit': 'TwoHanded_GetHit',
           'death': 'TwoHanded_Death', 'cheer': 'mw_cheer_ani'},
    'tw': {'attack': 'TwoHanded_Attack', 'hit': 'TwoHanded_GetHit',
           'death': 'TwoHanded_Death'},
    'bw': {'attack': 'bw_atkv3_ani', 'attack2': 'bw_atkv5_ani',
           'hit': 'TwoHanded_GetHit', 'death': 'TwoHanded_Death'},
    'cb': {'attack': 'crossbow_attack3_ani', 'hit': 'TwoHanded_GetHit',
           'death': 'TwoHanded_Death'},
}
ODDBALL_FALLBACK = 'tw'

UNIT_CLIP_OVERRIDES = {
    'c_desiredemon': {'attack': 'c_desiredemon_shout_ani',
                      'cheer': 'c_desiredemon_dance_ani'},
}

ATTACK_PRIMARY = ('attack', 'attk', 'atk', 'fireball')
ATTACK_SECONDARY = ('melee', 'flurry', 'chop', 'flame', 'spit', 'slam', 'cast',
                    'bash', 'scattershot', 'smash', 'berzerk', 'blast', 'quake',
                    'stomp', 'spin', 'energyburst', 'flourish', 'rapidfire',
                    'ranged', 'sneak', 'devastation', 'aoe', 'petrify',
                    'conecold', 'rage', 'fire', 'perforate')
CHEER_WORDS = ('cheer', 'roar', 'howl', 'rant', 'threaten', 'yell', 'shout',
               'dance', 'twitch', 'scratch', 'headshake', 'barking', 'angry',
               'scream', 'cocked')


def classify_clip(name):
    n = name.lower()
    if 'ready' in n or '_rdy' in n or 'idle' in n:
        return 'idle'
    if 'death' in n or '_die' in n:
        return 'death'
    if 'hit' in n:
        return 'hit'
    if any(w in n for w in ATTACK_PRIMARY) or any(w in n for w in ATTACK_SECONDARY):
        return 'attack'
    if any(w in n for w in CHEER_WORDS):
        return 'cheer'
    return None


def assign_roles(names):
    buckets = {}
    for n in names:
        role = classify_clip(n)
        if role and role != 'idle':
            buckets.setdefault(role, []).append(n)
    roles = {}
    prim = sorted((n for n in buckets.get('attack', ())
                   if any(w in n.lower() for w in ATTACK_PRIMARY)), key=str.lower)
    atk = prim + sorted((n for n in buckets.get('attack', ()) if n not in prim),
                        key=str.lower)
    if atk:
        roles['attack'] = atk[0]
    if len(atk) > 1:
        roles['attack2'] = atk[1]
    for role in ('hit', 'death', 'cheer'):
        if buckets.get(role):
            roles[role] = sorted(buckets[role], key=str.lower)[0]
    return roles


def humanoid_family(base, idle_name=None, controller_names=()):
    for cn in controller_names:
        if cn in FAMILY_BY_CONTROLLER:
            return FAMILY_BY_CONTROLLER[cn]
    fam = IDLE_TO_FAMILY.get(idle_name or '')
    if fam:
        return fam
    parts = re.sub(r'_t\d+$', '', base).split('_')
    if parts[0] in RACE_PREFIXES and len(parts) > 2 and len(parts[1]) <= 5:
        return parts[1]
    return None


def fill_family_roles(roles, family):
    fam = FAMILY_SETS.get(family) or (FAMILY_SETS[ODDBALL_FALLBACK] if family else None)
    if not fam:
        return
    if 'attack' not in roles:
        roles['attack'] = fam['attack']
        if 'attack2' not in roles and 'attack2' in fam:
            roles['attack2'] = fam['attack2']
    for role in ('hit', 'death', 'cheer'):
        if role not in roles and role in fam:
            roles[role] = fam[role]


def load_index_clip(name, clip_index):
    path = clip_index.get(name)
    if not path:
        return None
    cenv = load_env(path)
    for o in cenv.objects:
        if o.type.name == 'AnimationClip' and obj_name(o) == name:
            return read_clip_tt(o)
    return None


def clip_type_ok(clip_tt, rig):
    at = clip_tt.get('m_AnimationType')
    if at is None:
        return True
    return at == (3 if rig == 'humanoid' else 2)


def discover_clip_set(env, base, rig, cab_paths, clip_index, idle_name):
    own = own_bundle_clips(env)
    try:
        ctrl_objs, scraped, ctrl_names = controller_clips(env, cab_paths, clip_index)
    except Exception:
        ctrl_objs, scraped, ctrl_names = {}, set(), []

    names = (set(own) | set(ctrl_objs) | scraped) - {idle_name}
    roles = assign_roles(names)
    source = {}
    for role, name in roles.items():
        source[role] = ('own-bundle' if name in own else
                        'controller' if name in ctrl_objs else 'controller-scrape')

    idle_stem = re.sub(r'_?(ready|rdy|idle)\w*$', '', idle_name or '', flags=re.I)
    for role, name in UNIT_CLIP_OVERRIDES.get(idle_stem, {}).items():
        if name in names or name in clip_index:
            roles[role] = name
            source[role] = 'override'

    if rig == 'humanoid':
        fam = humanoid_family(base, idle_name, ctrl_names)
        if fam:
            before = set(roles)
            fill_family_roles(roles, fam)
            for role in set(roles) - before:
                source[role] = f'family:{fam}'

    prefixes = ([idle_stem] if len(idle_stem) >= 3 else []) + candidate_prefixes(base)
    for role in ('attack', 'hit', 'death', 'cheer'):
        if role in roles:
            continue
        for pref in prefixes:
            cand = sorted((n for n in clip_index
                           if n.startswith(pref) and n != idle_name
                           and classify_clip(n) == role), key=str.lower)
            if cand:
                roles[role] = cand[0]
                source[role] = f'index:{pref}'
                break

    out = {}
    for role, name in roles.items():
        obj = own.get(name) or ctrl_objs.get(name)
        try:
            tt = read_clip_tt(obj) if obj is not None else load_index_clip(name, clip_index)
        except Exception:
            tt = None
        if tt is None or not clip_type_ok(tt, rig):
            continue
        out[role] = (tt, name, source[role])
    if 'attack2' in out and 'attack' not in out:
        out.pop('attack2')
    return out


def read_avatar(env):
    for o in env.objects:
        if o.type.name == 'Avatar':
            tt = o.read_typetree()
            human = tt['m_Avatar'].get('m_Human', {}).get('data', {})
            n_human = len(human.get('m_Skeleton', {}).get('data', {}).get('m_Node', []))
            return tt, ('humanoid' if n_human > 0 else 'generic')
    return None, None


def avatar_node_count(avatar_tt):
    return len(avatar_tt['m_Avatar']['m_AvatarSkeleton']['data']['m_Node'])


def borrow_family_avatar(base, cab_paths):
    family = re.sub(r'_t\d+$', '', base)
    for tier in ('t1', 't2', 't3', 't4', 't5', 't6'):
        cand = f'{family}_{tier}'
        if cand == base:
            continue
        cab = cab_lookup(cab_paths, f'cab-{cand}_pre')
        if cab is None:
            continue
        tt, rig = read_avatar(load_env(cab))
        if tt is not None and avatar_node_count(tt) > 2:
            return tt, rig, cand
    return None, None, None


def prefab_path_hashes(env):
    import zlib
    gos = {o.path_id: o for o in env.objects if o.type.name == 'GameObject'}
    names, parent = {}, {}
    for o in env.objects:
        if o.type.name != 'Transform':
            continue
        try:
            tt = o.read_typetree()
            go = gos.get(tt['m_GameObject']['m_PathID'])
            names[o.path_id] = go.read_typetree().get('m_Name', '') if go else ''
            parent[o.path_id] = tt['m_Father']['m_PathID']
        except Exception:
            continue
    out = {}
    for pid in names:
        segs, p = [], pid
        while p in names:
            segs.append(names[p])
            p = parent.get(p, 0)
        segs.reverse()
        for k in range(len(segs)):
            h = zlib.crc32('/'.join(segs[k:]).encode()) & 0xffffffff
            out.setdefault(h, segs[-1])
    return out


def display_scaffolding(env):
    gos = {o.path_id: o for o in env.objects if o.type.name == 'GameObject'}
    trs, names = {}, {}
    for o in env.objects:
        if o.type.name != 'Transform':
            continue
        try:
            tt = o.read_typetree()
            trs[o.path_id] = tt
            go = gos.get(tt['m_GameObject']['m_PathID'])
            names[o.path_id] = go.read_typetree().get('m_Name', '') if go else ''
        except Exception:
            continue

    roots = [pid for pid, tt in trs.items()
             if tt['m_Father']['m_PathID'] == 0 and names[pid].startswith('unit.')]
    if not roots:
        return None, None, 'no unit.* root transform'
    root = roots[0]

    def children(pid):
        return [c['m_PathID'] for c in trs[pid]['m_Children'] if c['m_PathID'] in trs]

    def sy(pid):
        return trs[pid]['m_LocalScale']['x'], trs[pid]['m_LocalPosition']['y']

    def n_desc(pid):
        stack, n = children(pid), 0
        while stack:
            n += 1
            stack.extend(children(stack.pop()))
        return n

    frontier = [(c, [c]) for c in children(root)]
    sk_path = None
    while frontier and sk_path is None:
        cand = [(pid, path) for pid, path in frontier if sy(pid)[0] < 0.1 and children(pid)]
        if cand:
            sk_path = max(cand, key=lambda cp: n_desc(cp[0]))[1]
            break
        frontier = [(cc, path + [cc]) for pid, path in frontier for cc in children(pid)]
    if sk_path is None:
        kids = [c for c in children(root) if not names[c].endswith('_loc_marker')]
        fbx = [c for c in kids if names[c].endswith('_fbx')]
        carrier = fbx[0] if fbx else (max(kids, key=n_desc) if kids else None)
        if carrier is None:
            return None, None, 'no cm-folded skeleton root under unit root'
        s, y = sy(carrier)
        return round(s, 6), round(y, 6), 'meter-rig _fbx wrapper (no cm fold)'

    s = 1.0
    for pid in sk_path:
        s *= sy(pid)[0]
    s = round(s * 100.0, 6)

    y = sum(sy(pid)[1] for pid in sk_path[:-1])
    if abs(y) < 1e-6:
        sibs = [c for p in ([root] + sk_path[:-1]) for c in children(p)
                if c not in sk_path and abs(sy(c)[0] - 0.01) < 1e-6 and not children(c)]
        if sibs:
            y = sy(sibs[0])[1]
    note = '' if names[sk_path[0]].endswith('_fbx') else 'no _fbx wrapper'
    return s, round(y, 6), note



def export_texture(env, pptr, cab_paths, dst_png):
    hit = resolve_pptr(env, pptr, cab_paths)
    if hit is None:
        return None
    tenv, pid = hit
    o = find_by_pid(tenv, pid)
    if o is None or o.type.name != 'Texture2D':
        return None
    try:
        img = o.read().image
    except Exception:
        return None
    if dst_png:
        img.save(dst_png)
    return img


def pick_reference_texture(base, texture_name, reference_fbx=REFERENCE_FBX):
    unit_dir = f'unit.{base}_pre'
    candidates = globmod.glob(os.path.join(reference_fbx, '*', unit_dir, '*.png'))
    wanted = texture_name.lower() if texture_name else ''
    match = next((p for p in candidates if wanted and
                  os.path.splitext(os.path.basename(p))[0].lower() == wanted), None)
    if match is None and len(candidates) == 1:
        match = candidates[0]
    if match is None and candidates:
        def image_rank(path):
            try:
                with Image.open(path) as image:
                    pixels = image.width * image.height
            except Exception:
                pixels = 0
            return pixels, os.path.getsize(path), os.path.basename(path).lower()

        match = max(candidates, key=image_rank)
    return match


def reference_texture(base, texture_name, dst_png):
    match = pick_reference_texture(base, texture_name)
    if match is None:
        return None, None
    with Image.open(match) as src:
        img = src.copy()
    img.save(dst_png)
    return img, match


def texture_name(env, pptr, cab_paths):
    hit = resolve_pptr(env, pptr, cab_paths)
    if hit is None:
        return ''
    obj = find_by_pid(*hit)
    return obj_name(obj) if obj is not None else ''


def material_texenvs(material_tt):
    out = {}
    for row in material_tt.get('m_SavedProperties', {}).get('m_TexEnvs', []):
        if isinstance(row, (list, tuple)):
            key, value = row
        else:
            key = row.get('first', row.get('name', ''))
            value = row.get('second', row.get('value', {}))
        if isinstance(key, dict):
            key = key.get('name', '')
        if key:
            out[key] = value
    return out


def renderer_material_for_mesh(env, mesh_name, material_slot, cab_paths, base):
    gos, transforms = {}, {}
    for o in env.objects:
        if o.type.name == 'GameObject':
            try:
                gos[o.path_id] = o.read_typetree().get('m_Name', '')
            except Exception:
                pass
        elif o.type.name == 'Transform':
            try:
                tt = o.read_typetree()
                transforms[o.path_id] = tt
            except Exception:
                pass

    expected_root = f'unit.{base}_pre'
    roots = [pid for pid, tt in transforms.items()
             if gos.get(tt['m_GameObject']['m_PathID']) == expected_root]
    if not roots:
        roots = [pid for pid, tt in transforms.items()
                 if gos.get(tt['m_GameObject']['m_PathID'], '').startswith('unit.')]

    prefab_gos = set()
    stack = list(roots)
    while stack:
        pid = stack.pop()
        if pid in transforms:
            tt = transforms[pid]
            prefab_gos.add(tt['m_GameObject']['m_PathID'])
            stack.extend(c['m_PathID'] for c in tt.get('m_Children', []))

    def mesh_key(name):
        name = name.lower()
        name = re.sub(r'_tier_?\d+(?=$|_)', '_tier', name)
        return re.sub(r'_t\d+(?=_mesh$)', '_t_mesh', name)

    def candidates(prefab_only, allow_tier_fallback=False):
        found = []
        for o in env.objects:
            if o.type.name != 'SkinnedMeshRenderer':
                continue
            try:
                tt = o.read_typetree()
                go_id = tt['m_GameObject']['m_PathID']
                if prefab_only and go_id not in prefab_gos:
                    continue
                hit = resolve_pptr(env, tt.get('m_Mesh', {}), cab_paths)
                if hit is None:
                    continue
                mesh_obj = find_by_pid(*hit)
                source_name = obj_name(mesh_obj) if mesh_obj is not None else ''
                if source_name != mesh_name and not (
                        allow_tier_fallback and mesh_key(source_name) == mesh_key(mesh_name)):
                    continue
                materials = tt.get('m_Materials', [])
                if material_slot >= len(materials):
                    continue
                mat_hit = resolve_pptr(env, materials[material_slot], cab_paths)
                if mat_hit is None:
                    continue
                mat_obj = find_by_pid(*mat_hit)
                if mat_obj is not None and mat_obj.type.name == 'Material':
                    found.append((mat_hit, mat_obj))
            except Exception:
                continue
        return found

    matches = candidates(bool(prefab_gos))
    if not matches and prefab_gos:
        matches = candidates(False)
    if not matches:
        matches = candidates(bool(prefab_gos), allow_tier_fallback=True)
    if not matches and prefab_gos:
        matches = candidates(False, allow_tier_fallback=True)
    if not matches:
        return None, f'no unit renderer material for glTF mesh {mesh_name}'

    unique = {}
    for hit, mat_obj in matches:
        unique[(id(hit[0]), hit[1])] = mat_obj
    if len(unique) > 1:
        names = sorted({obj_name(o) for o in unique.values()})
        return None, f'ambiguous renderer materials for {mesh_name}: {names}'
    return next(iter(unique.values())), None


def fix_textures(env, dst_glb, out_dir, base, cab_paths):
    notes = []
    gltf, blob = glb_writer.load_glb(dst_glb)
    meshes = gltf.get('meshes') or []
    if len(meshes) != 1 or len(meshes[0].get('primitives', [])) != 1:
        notes.append(f'texture repair expects one mesh/primitive, got {len(meshes)} mesh(es)')
        return notes
    mesh = meshes[0]
    primitive = mesh['primitives'][0]
    material_slot = 0
    material_obj, error = renderer_material_for_mesh(
        env, mesh.get('name', ''), material_slot, cab_paths, base)
    if material_obj is None:
        notes.append(error)
        return notes

    try:
        texenvs = material_texenvs(material_obj.read_typetree())
    except Exception as e:
        notes.append(f'renderer material read failed: {type(e).__name__}')
        return notes
    main_pptr = texenvs.get('_MainTex', {}).get('m_Texture', {})
    alpha_pptr = texenvs.get('_AlphaTex', {}).get('m_Texture', {})
    if not main_pptr.get('m_PathID'):
        notes.append(f'{obj_name(material_obj)} has no _MainTex')
        return notes

    os.makedirs(out_dir, exist_ok=True)
    main_name = f'{base}_main.png'
    main_path = os.path.join(out_dir, main_name)
    main_img = export_texture(env, main_pptr, cab_paths, main_path)
    if main_img is None:
        main_img, fallback = reference_texture(
            base, texture_name(env, main_pptr, cab_paths), main_path)
        if main_img is None:
            notes.append('renderer _MainTex export failed')
            return notes
        notes.append(f'used reference FBX texture {os.path.basename(fallback)} -> {main_name}')
    else:
        notes.append(f'exported renderer _MainTex -> {main_name}')

    material_index = primitive.get('material')
    materials = gltf.get('materials') or []
    try:
        material = materials[material_index]
    except (IndexError, TypeError):
        notes.append('glTF primitive has no material')
        return notes
    images = gltf.get('images')
    if not isinstance(images, list):
        images = []
        gltf['images'] = images
    textures = gltf.get('textures')
    if not isinstance(textures, list):
        textures = []
        gltf['textures'] = textures
    pbr = material.setdefault('pbrMetallicRoughness', {})
    try:
        texture_index = pbr['baseColorTexture']['index']
        image_index = textures[texture_index]['source']
        images[image_index]['uri'] = main_name
    except (KeyError, IndexError, TypeError):
        image_index = len(images)
        images.append({'uri': main_name})
        texture_index = len(textures)
        textures.append({'source': image_index})
        pbr['baseColorTexture'] = {'index': texture_index}
        notes.append('created missing glTF base-color image')

    if alpha_pptr.get('m_PathID'):
        alpha_img = export_texture(env, alpha_pptr, cab_paths, None)
        if alpha_img is not None:
            rgb = main_img.convert('RGB')
            a = alpha_img.convert('RGB').getchannel('R').resize(rgb.size)
            rgba = rgb.copy()
            rgba.putalpha(a)
            rgba_name = f'{base}_rgba.png'
            rgba.save(os.path.join(out_dir, rgba_name))
            images[image_index]['uri'] = rgba_name
            notes.append(f'split-alpha merged -> {rgba_name}')
        else:
            notes.append('renderer _AlphaTex export failed')

    glb_writer.save_glb(dst_glb, gltf, blob)
    return notes



def check_glb(path, display_scale=None):
    gltf, blob = glb_writer.load_glb(path)
    out = {}
    anims = gltf.get('animations', [])
    out['anims'] = [a.get('name') for a in anims]
    durs, bad_scale = {}, 0.0
    for a in anims:
        tacc = gltf['accessors'][a['samplers'][0]['input']]
        durs[a.get('name')] = round(tacc['max'][0], 4)
        for ch in a['channels']:
            if ch['target']['path'] != 'scale':
                continue
            rows = glb_writer.read_accessor(gltf, blob, a['samplers'][ch['sampler']]['output'])
            for r in rows:
                bad_scale = max(bad_scale, max(abs(c - 1.0) for c in r))
    if anims:
        out['duration'] = durs.get('idle', durs[out['anims'][0]])
        out['max_scale_dev'] = round(bad_scale, 6)
        if len(anims) > 1:
            out['clip_durations'] = durs
    joints = gltf['skins'][0]['joints'] if gltf.get('skins') else []
    out['joint_node_scale_dev'] = round(max(
        (max(abs(c - 1.0) for c in gltf['nodes'][j].get('scale', [1, 1, 1])) for j in joints),
        default=0.0), 6)
    ys = [gltf['accessors'][p['attributes']['POSITION']]['min'][1]
          for m in gltf['meshes'] for p in m['primitives'] if 'JOINTS_0' in p['attributes']]
    out['mesh_min_y'] = round(min(ys), 4) if ys else None
    out['mesh_height'] = round(glb_writer.skinned_height(gltf), 4)
    height_ok = True
    if display_scale:
        eff = out['mesh_height'] * display_scale
        out['effective_height'] = round(eff, 3)
        if not (glb_writer.EFF_LO <= eff <= glb_writer.EFF_HI):
            if glb_writer.EFF_WARN_LO <= eff <= glb_writer.EFF_WARN_HI:
                out['height_warn'] = (f'effective height {eff:.2f}m outside '
                                      f'[{glb_writer.EFF_LO}, {glb_writer.EFF_HI}]')
            else:
                height_ok = False
    names_ok = (out['anims'][:1] == ['idle']
                and len(set(out['anims'])) == len(out['anims'])
                and all(n in GLB_CLIP_NAMES for n in out['anims']))
    out['ground_ok'] = out['mesh_min_y'] is not None and -0.1 <= out['mesh_min_y'] <= 0.2
    out['ok'] = (names_ok and min(durs.values(), default=0) > 0
                 and out.get('max_scale_dev', 1) < 0.01 and out['joint_node_scale_dev'] < 0.01
                 and height_ok)
    return out



def ensure_src_glb(dirname, base):
    p = f'{CHAR_PACK}/{dirname}/{base}.glb'
    if os.path.exists(p):
        return p
    g = f'{CHAR_PACK}/{dirname}/{base}.gltf'
    if not os.path.exists(g):
        m = re.match(r'(.*)_t\d$', base)
        if not m:
            return None
        for tier in ('t4', 't3', 't2', 't1'):
            sib = f'{m.group(1)}_{tier}'
            if sib == base:
                continue
            if os.path.exists(f'{CHAR_PACK}/{dirname}/{sib}.glb'):
                return f'{CHAR_PACK}/{dirname}/{sib}.glb'
            if os.path.exists(f'{CHAR_PACK}/{dirname}/{sib}.gltf'):
                g = f'{CHAR_PACK}/{dirname}/{sib}.gltf'
                break
        else:
            return None
    gltf = json.load(open(g))
    buf = gltf['buffers'][0]
    blob = open(os.path.join(os.path.dirname(g), buf['uri']), 'rb').read()
    buf.pop('uri')
    os.makedirs(SCRATCH, exist_ok=True)
    tmp = os.path.join(SCRATCH, base + '_packed.glb')
    glb_writer.save_glb(tmp, gltf, blob)
    return tmp


def process_target(dirname, base, cab_paths, clip_index, force=False):
    row = {'dir': dirname, 'base': base, 'status': 'ok', 'rig': None, 'clip': None,
           'scale': None, 'detail': ''}
    dst = f'{CHAR_PACK}/{dirname}/{base}_rl.glb'
    notes = []
    try:
        cab = cab_lookup(cab_paths, f'cab-{base}_pre')
        if cab is None:
            import donor_bake
            return donor_bake.process(dirname, base, row, cab_paths, clip_index,
                                      force, sys.modules[__name__])
        env = load_env(cab)

        scale, y, snote = display_scaffolding(env)
        if snote:
            notes.append(snote)
        row['scale'] = scale
        scale_entry = {'scale': scale, 'y': y} if scale is not None else None

        avatar_tt, rig = read_avatar(env)
        if avatar_tt is not None and avatar_node_count(avatar_tt) <= 2:
            b_tt, b_rig, b_from = borrow_family_avatar(base, cab_paths)
            if b_tt is not None:
                avatar_tt, rig = b_tt, b_rig
                notes.append(f'degenerate avatar, borrowed from {b_from}')
        if avatar_tt is None:
            row['status'] = 'no-avatar'
            row['detail'] = '; '.join(notes)
            return row, None
        row['rig'] = rig
        if rig == 'humanoid' and scale is not None and abs(scale - 1.2885) > 0.02:
            notes.append(f'humanoid scale outlier {scale}')

        if os.path.exists(dst) and not force:
            notes.append('exists, bake skipped')
            row['detail'] = '; '.join(notes)
            return row, scale_entry

        src = ensure_src_glb(dirname, base)
        if src is None:
            row['status'] = 'no-src'
            row['detail'] = '; '.join(notes)
            return row, None
        if src.endswith('_packed.glb'):
            notes.append('packed from gltf+bin')

        clip_tt, clip_name, method, alts = discover_clip(env, base, cab_paths, clip_index)
        if clip_tt is None:
            row['status'] = 'no-clip'
            row['detail'] = '; '.join(notes + [f'tried prefixes {alts}'])
            return row, None
        if rig == 'humanoid' and clip_tt.get('m_AnimationType') == 2:
            rig = 'generic'
            row['rig'] = rig
            notes.append('generic transform clip overrides stale humanoid avatar flag')
        row['clip'] = clip_name
        row['clip_method'] = method
        if alts:
            row['clip_alternatives'] = alts[:8]

        combat = discover_clip_set(env, base, rig, cab_paths, clip_index, clip_name)
        extra_clips = [(role, combat[role][0]) for role in ROLE_ORDER if role in combat]
        row['clips'] = {'idle': clip_name}
        row['clip_sources'] = {'idle': method}
        for role in ROLE_ORDER:
            if role in combat:
                row['clips'][role] = combat[role][1]
                row['clip_sources'][role] = combat[role][2]
        missing = [r for r in ('attack', 'hit', 'death') if r not in combat]
        if missing:
            row['clips_missing'] = missing

        if rig == 'humanoid':
            info = bake_humanoid(src, dst, avatar_tt, clip_tt, use_tos=True,
                                 display_scale=scale, extra_clips=extra_clips)
            notes.append(f"frames={info['frames']} neutral={info['neutral_check']:.4f}")
            if info['unresolved']:
                notes.append(f"unresolved_nodes={len(info['unresolved'])}")
            if info.get('promoted'):
                notes.append(f"promoted={info['promoted']}")
            if info.get('promote_skipped'):
                notes.append(f"promote_skipped={info['promote_skipped']}")
            if info.get('hips_basis'):
                basis = info['hips_basis']
                notes.append('hips basis '
                             f"R={basis['right']} U={basis['up']} F={basis['forward']} "
                             f"fix={info['hips_basis_fix_degrees']:.2f}deg")
            if info.get('ground_shift'):
                notes.append(f"grounded feet {info['ground_shift']:+.3f}")
        else:
            info = bake_generic_unit(src, dst, base, avatar_tt, clip_tt,
                                     hash_to_name=prefab_path_hashes(env),
                                     display_scale=scale, extra_clips=extra_clips)
            notes.append(f"frames={info['frames']} animated={info['animated']}/{info['total']} "
                         f"shift={info['shift']:+.3f} posed_minY={info['posed_min_y']:.3f}")
            if info['promoted']:
                notes.append(f"promoted={info['promoted']}")
            if info.get('promote_skipped'):
                notes.append(f"promote_skipped={info['promote_skipped']}")
            if info.get('scale_overrides'):
                notes.append(f"scale_overrides={info['scale_overrides']}")
            if info.get('scale_ignored'):
                notes.append(f"scale_ignored={info['scale_ignored']}")
        if info.get('avatar_rescale', 1.0) != 1.0:
            notes.append(f"stale decade fold: avatar repaired x{info['avatar_rescale']:g}")
        if info.get('post_scale', 1.0) != 1.0:
            notes.append(f"unit normalization: post-scaled x{info['post_scale']:g}")
        if info.get('clip_errors'):
            row['clip_errors'] = {k: v[:160] for k, v in info['clip_errors'].items()}
            for role in info['clip_errors']:
                row['clips'].pop(role, None)
            notes.append(f"clip_errors={sorted(info['clip_errors'])}")

        notes += fix_textures(env, dst, f'{CHAR_PACK}/{dirname}', base, cab_paths)
        chk = check_glb(dst, display_scale=scale)
        row['check'] = chk
        if chk.get('height_warn'):
            notes.append(chk['height_warn'])
        if not chk.get('ground_ok', False):
            notes.append(f"bind bounds off ground (minY={chk.get('mesh_min_y')})")
        if not chk['ok']:
            notes.append('CHECK-FAIL')
        row['detail'] = '; '.join(notes)
        return row, scale_entry
    except (Exception, SystemExit) as e:
        row['status'] = 'bake-error'
        row['detail'] = '; '.join(notes + [f'{type(e).__name__}: {e}'])
        row['trace'] = traceback.format_exc(limit=6)
        return row, None



def load_json(path, default):
    if os.path.exists(path):
        return json.load(open(path))
    return default


def dump_json_atomic(path, data):
    tmp = path + '.tmp'
    with open(tmp, 'w') as f:
        json.dump(data, f, indent=1)
    os.replace(tmp, path)



_W = {}

def _winit(cab_paths, clip_index):
    _W['cab'] = cab_paths
    _W['clip'] = clip_index


def _wrun(args):
    dirname, base, force = args
    return process_target(dirname, base, _W['cab'], _W['clip'], force)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--only', default=None, help='comma list of bases')
    ap.add_argument('--force', action='store_true')
    ap.add_argument('--upgrade', action='store_true',
                    help='force-rebake only units whose report row is not yet '
                         'multi-clip (restartable combat-clip rollout)')
    ap.add_argument('--max-seconds', type=float, default=None)
    ap.add_argument('--workers', type=int, default=1)
    ap.add_argument('--inventory', default=DEFAULT_INVENTORY)
    ap.add_argument('--check', default=None, help='comma list of bases: verify only')
    args = ap.parse_args()

    if os.path.exists(args.inventory):
        inv = json.load(open(args.inventory))
        todo = [(d, b) for d, b in inv['todo']]
    else:
        todo = [(row['dir'], row['base']) for row in load_json(REPORT_PATH, [])]
        if not todo:
            raise SystemExit(f'missing bake inventory and report: {args.inventory}')

    if args.check:
        scales = load_json(SCALES_PATH, {})
        for b in args.check.split(','):
            d = next((d for d, bb in todo if bb == b), None)
            if d is None:
                d = next((dd for dd in os.listdir(CHAR_PACK)
                          if os.path.exists(f'{CHAR_PACK}/{dd}/{b}_rl.glb')), None)
            e = scales.get(b)
            s = (e.get('scale') if isinstance(e, dict) else e) if e else None
            print(b, json.dumps(check_glb(f'{CHAR_PACK}/{d}/{b}_rl.glb', display_scale=s)))
        return

    cab_paths = build_cab_paths()
    clip_index = build_clip_index(cab_paths)

    if args.only:
        only = set(args.only.split(','))
        todo = [(d, b) for d, b in todo if b in only]
        for b in sorted(only - {b for _, b in todo}):
            hits = [d for d in os.listdir(CHAR_PACK)
                    if any(os.path.exists(f'{CHAR_PACK}/{d}/{b}{sfx}')
                           for sfx in ('_rl.glb', '.glb', '.gltf'))]
            if hits:
                todo.append((hits[0], b))
            else:
                print(f'WARNING: {b} not in inventory or char_pack, skipped')

    report = load_json(REPORT_PATH, [])
    by_base = {r['base']: i for i, r in enumerate(report)}
    scales = load_json(SCALES_PATH, {})

    if args.upgrade:
        done_multi = {r['base'] for r in report if r.get('status') == 'ok' and 'clips' in r}
        todo = [(d, b) for d, b in todo if b not in done_multi]
        args.force = True
        print(f'upgrade: {len(todo)} targets still single-clip')

    def emit(row, scale_entry):
        if row['base'] in by_base:
            report[by_base[row['base']]] = row
        else:
            by_base[row['base']] = len(report)
            report.append(row)
        if scale_entry is not None:
            scales[row['base']] = scale_entry
        dump_json_atomic(REPORT_PATH, report)
        dump_json_atomic(SCALES_PATH, scales)

    t_start = time.time()
    done = 0
    jobs = [(d, b, args.force) for d, b in todo]

    if args.workers > 1:
        import multiprocessing as mp
        with mp.Pool(args.workers, initializer=_winit, initargs=(cab_paths, clip_index)) as pool:
            for row, scale_entry in pool.imap_unordered(_wrun, jobs, chunksize=1):
                emit(row, scale_entry)
                done += 1
                print(f"[{done}/{len(jobs)}] {row['base']}: {row['status']} "
                      f"({row.get('clip')}) {row['detail'][:100]}")
                if args.max_seconds and time.time() - t_start > args.max_seconds:
                    print('max-seconds reached, stopping')
                    pool.terminate()
                    break
    else:
        for job in jobs:
            t0 = time.time()
            row, scale_entry = process_target(job[0], job[1], cab_paths, clip_index, job[2])
            emit(row, scale_entry)
            done += 1
            print(f"[{done}/{len(jobs)}] {row['base']}: {row['status']} ({row.get('clip')}) "
                  f"{time.time()-t0:.1f}s {row['detail'][:110]}")
            if args.max_seconds and time.time() - t_start > args.max_seconds:
                print('max-seconds reached, stopping')
                break

    counts = {}
    for r in report:
        counts[r['status']] = counts.get(r['status'], 0) + 1
    print('report totals:', counts)


if __name__ == '__main__':
    main()
