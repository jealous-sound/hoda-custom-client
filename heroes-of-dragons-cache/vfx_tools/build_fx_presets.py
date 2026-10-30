#!/usr/bin/env python
import glob
import json
import math
import os
import re
import sys

import UnityPy
from UnityPy.helpers import UnityVersion as UV

_orig = UV.UnityVersion.from_str.__func__
def _patched(cls, version):
    m = re.search(r"(\d+)\.(\d+)\.(\d+)([a-zA-Z]+)(\d+)", version)
    if m:
        version = m.group(0)
    return _orig(cls, version)
UV.UnityVersion.from_str = classmethod(_patched)

CACHE = "/Users/vladislav/heroes-of-dragons/heroes-of-dragons-cache"
SHARED = CACHE + "/Heroes-of-Dragon-Age-v4-4-0-cache-androidp1/com.ea.game.dragonage_row/files/UnityCache/Shared"
PARTICLES = CACHE + "/vfx/particles"
TEXTURES = CACHE + "/vfx/textures"
UNIT_VFX = "/Users/vladislav/heroes-of-dragons/data/unit_vfx.json"
OUT = "/Users/vladislav/heroes-of-dragons/data/fx_presets.json"
REPORT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "build_report.json")

ALIASES = {
    "imp_wintersgrasp_pre": "fx_imp_wintersgrasp_pre",
    "Florianne": "fx_assassinflorianne_prjctl_pre",
    "HallaWinter_pre": "fx_goldenhalla_pre",
    "KC_Meredith_Imp_pre": "fx_RedLyriumMeredith_pre",
    "Sigrun_Imp_Pre": "vfx_greywardensigrun_attack_pre",
    "SpiritofJustinia_Imp_Pre": "fx_SpiritofJustinia_pre",
    "advisorleliana_imp_pre": "fx_Leliana_Imp_pre",
    "apostatesolas_pre": "fx_imp_Solas",
    "Beraht_pre": "fx_atk_lineslash_pre",
    "delrinbarris_pre": "fx_aoe_righteoussmite_pre",
    "dworking_prjctl": "fx_dworking_imp_pre",
    "fx_RedLyriumSamson.pre": "vfx_templarsamson_pre",
    "hall_prjctl": "fx_hall_Mzl_pre",
    "imp_Velanna_pre": "vfx_greywardenvelanna_pre",
    "imp_desire_pre": "vfx_desire_pre",
    "Jarvia_pre": "fx_aoe_righteoussmite_pre",
    "northernhunter_pre": "vfx_dragonfire_pre",
    "Rasaan_pre": "fx_primallightning_pre",
    "redtemplararcher_imp_pre": "fx_redtemplararcher_Mzl_pre",
    "redtemplararcher_prjctl": "fx_hakkonitearcher_prjctl_pre",
    "redtemplararcher_prjctl_pre": "fx_hakkonitearcher_prjctl_pre",
    "scoutharding_prjctl": "fx_scoutharding_imp_pre",
}

MAX_EMITTERS = 24
MAX_BURST = 24
MAX_RATE = 200.0
MAX_LENGTH = 8.0

warnings = []
def warn(bundle, msg):
    warnings.append(f"{bundle}: {msg}")



def mm(node, default=0.0):
    if node is None:
        return (default, default)
    if isinstance(node, (int, float)):
        return (float(node), float(node))
    s = float(node.get("scalar", default))
    st = node.get("minMaxState", 0)
    maxc = (node.get("maxCurve") or {}).get("m_Curve") or []
    minc = (node.get("minCurve") or {}).get("m_Curve") or []
    if st == 0 or not maxc:
        return (s, s)
    if st == 1:
        vals = [k["value"] for k in maxc]
        return tuple(sorted((s * min(vals), s * max(vals))))
    mx = maxc[0]["value"]
    mn = minc[0]["value"] if minc else 0.0
    lo, hi = sorted((s * mn, s * mx))
    return (lo, hi)


def curve_keys(node):
    if not isinstance(node, dict) or int(node.get("minMaxState", 0)) != 1:
        return []
    scalar = float(node.get("scalar", 1.0))
    curve = ((node.get("maxCurve") or {}).get("m_Curve") or [])
    return [[round(float(key["time"]), 6), round(scalar * float(key["value"]), 6)]
            for key in curve]


def rotated_box_extents(rotation, extents):
    axes = [qrot(rotation, axis) for axis in ((1, 0, 0), (0, 1, 0), (0, 0, 1))]
    return tuple(sum(abs(axes[source][axis]) * extents[source] for source in range(3))
                 for axis in range(3))


def qmul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz)


def qrot(q, v):
    qv = (v[0], v[1], v[2], 0.0)
    x, y, z, _w = qmul(qmul(q, qv), (-q[0], -q[1], -q[2], q[3]))
    return (x, y, z)


def rgba(u):
    u = int(u) & 0xFFFFFFFF
    return (u & 0xFF, (u >> 8) & 0xFF, (u >> 16) & 0xFF, (u >> 24) & 0xFF)


def gradient_keys(g):
    ck, ak = [], []
    for i in range(int(g.get("m_NumColorKeys", 0))):
        r, gg, b, _ = rgba(g[f"key{i}"]["rgba"])
        ck.append((g[f"ctime{i}"] / 65535.0, (r, gg, b)))
    for i in range(int(g.get("m_NumAlphaKeys", 0))):
        a = rgba(g[f"key{i}"]["rgba"])[3]
        ak.append((g[f"atime{i}"] / 65535.0, a))
    return ck, ak



def build_cab_index():
    idx = {}
    for root, _dirs, files in os.walk(SHARED):
        for f in files:
            if f.startswith("CAB-"):
                idx[f[4:].lower()] = os.path.join(root, f)
    return idx


class SharedContainer:
    def __init__(self, cab_path):
        self.env = UnityPy.load(cab_path)
        self.tex = {}
        self.shader = {}
        for obj in self.env.objects:
            if obj.type.name == "Texture2D":
                self.tex[obj.path_id] = (obj.read().m_Name, obj)
            elif obj.type.name == "Shader":
                try:
                    self.shader[obj.path_id] = obj.read().m_ParsedForm.m_Name
                except Exception:
                    try:
                        self.shader[obj.path_id] = obj.read().m_Name
                    except Exception:
                        pass


def safe_name(n):
    return n.replace(" ", "_")


def shader_class(name):
    n = name.lower()
    if "add" in n or "premultiply" in n:
        return (1, 0)
    if "multiply" in n:
        return (0, 1)
    if n.endswith("sub") or "-sub" in n:
        return (0, 2)
    return (0, 0)


def probe_bundle(bundle, cab_path, shared_res, shared_shd, tex_files):
    env = UnityPy.load(cab_path)
    exts = []
    for _name, f in env.files.items():
        e = getattr(f, "externals", None)
        if e is not None:
            exts = [getattr(x, "path", "") for x in e]
            break

    transforms, go_tr, mats, texs, shaders, renderers = {}, {}, {}, {}, {}, {}
    for obj in env.objects:
        t = obj.type.name
        if t == "Transform":
            tr = obj.read()
            transforms[obj.path_id] = (
                tr.m_GameObject.path_id, tr.m_Father.path_id,
                (tr.m_LocalPosition.x, tr.m_LocalPosition.y, tr.m_LocalPosition.z),
                (tr.m_LocalRotation.x, tr.m_LocalRotation.y,
                 tr.m_LocalRotation.z, tr.m_LocalRotation.w),
                tr.m_LocalScale.x)
            go_tr[tr.m_GameObject.path_id] = obj.path_id
        elif t == "Material":
            mats[obj.path_id] = obj.read()
        elif t == "Texture2D":
            texs[obj.path_id] = (obj.read().m_Name, obj)
        elif t == "Shader":
            try:
                shaders[obj.path_id] = obj.read().m_ParsedForm.m_Name
            except Exception:
                try:
                    shaders[obj.path_id] = obj.read().m_Name
                except Exception:
                    pass
        elif t == "ParticleSystemRenderer":
            r = obj.read()
            m0 = r.m_Materials[0] if r.m_Materials else None
            renderers[r.m_GameObject.path_id] = {
                "mode": getattr(r, "m_RenderMode", 0),
                "material": (m0.file_id, m0.path_id) if m0 is not None else None,
                "max_screen_size": float(getattr(r, "m_MaxParticleSize", 0.0)),
                "length_scale": float(getattr(r, "m_LengthScale", 0.0)),
                "velocity_scale": float(getattr(r, "m_VelocityScale", 0.0)),
            }

    def resolve_ext(file_id):
        return exts[file_id - 1].lower() if 0 < file_id <= len(exts) else ""

    def compose(go_pid):
        chain = []
        tr_pid = go_tr.get(go_pid)
        while tr_pid in transforms:
            chain.append(tr_pid)
            tr_pid = transforms[tr_pid][1]
        chain.reverse()
        pos = (0.0, 0.0, 0.0)
        rot = (0.0, 0.0, 0.0, 1.0)
        scale = 1.0
        for i, pid in enumerate(chain):
            _go, _father, p, q, s = transforms[pid]
            if i > 0:
                wp = qrot(rot, (p[0] * scale, p[1] * scale, p[2] * scale))
                pos = tuple(pos[j] + wp[j] for j in range(3))
            rot = qmul(rot, q)
            scale *= s
        return pos, rot, scale

    def texture_file(tref):
        fid, pid = tref
        if fid == 0:
            if pid in texs:
                fn = f"{bundle}__{safe_name(texs[pid][0])}.png"
                if fn in tex_files:
                    return fn, None
                try:
                    texs[pid][1].read().image.save(os.path.join(TEXTURES, fn))
                    tex_files.add(fn)
                    return fn, f"exported in-bundle tex {fn}"
                except Exception as e:
                    return None, f"in-bundle tex export failed: {e}"
            return None, f"tex path_id {pid} not in bundle"
        path = resolve_ext(fid)
        if "shared_resourcecontainer" in path:
            if pid in shared_res.tex:
                name = safe_name(shared_res.tex[pid][0])
                hits = [f for f in tex_files if f.endswith(f"__{name}.png")]
                if hits:
                    return sorted(hits)[0], None
                fn = f"shared__{name}.png"
                try:
                    shared_res.tex[pid][1].read().image.save(os.path.join(TEXTURES, fn))
                    tex_files.add(fn)
                    return fn, f"exported shared tex {fn}"
                except Exception as e:
                    return None, f"shared tex export failed: {e}"
            return None, f"shared tex p{pid} unknown"
        hits = [f for f in tex_files if f.endswith("__Default-Particle.png")]
        return (sorted(hits)[0], None) if hits else (None, "no Default-Particle")

    out = {}
    for go_pid, renderer in renderers.items():
        mode, mref = renderer["mode"], renderer["material"]
        additive = keyout = 0
        tex_fn, note = None, "no material"
        if mref is not None:
            fid, pid = mref
            mat = None
            if fid == 0 and pid in mats:
                mat = mats[pid]
            if mat is not None:
                sh = mat.m_Shader
                if sh.file_id == 0:
                    sh_name = shaders.get(sh.path_id, "")
                else:
                    path = resolve_ext(sh.file_id)
                    if "shared_shadercontainer" in path:
                        sh_name = shared_shd.shader.get(sh.path_id, "")
                    else:
                        sh_name = "builtin-additive"
                additive, keyout = shader_class(sh_name)
                tref = None
                try:
                    envs = mat.m_SavedProperties.m_TexEnvs
                    items = envs.items() if hasattr(envs, "items") else envs
                    for k, v in items:
                        kn = k if isinstance(k, str) else getattr(k, "string", str(k))
                        if "_MainTex" in str(kn):
                            tref = (v.m_Texture.file_id, v.m_Texture.path_id)
                except Exception as e:
                    note = f"texenv error: {e}"
                if tref is not None:
                    tex_fn, note = texture_file(tref)
            else:
                path = resolve_ext(fid) if fid else ""
                if "unity default resources" in path.lower() and pid == 10754:
                    hits = [f for f in tex_files if f.endswith("__Default-Particle.png")]
                    tex_fn = sorted(hits)[0] if hits else None
                    note = None if tex_fn else "no Default-Particle"
                else:
                    note = f"material (f{fid},p{pid}) unresolved"
        offset, rot, scale = compose(go_pid)
        out[go_pid] = {"offset": (offset[0], offset[1], -offset[2]),
                       "rot": rot, "scale": scale, "mode": mode,
                       "max_screen_size": renderer["max_screen_size"],
                       "length_scale": renderer["length_scale"],
                       "velocity_scale": renderer["velocity_scale"],
                       "additive": additive, "keyout": keyout,
                       "tex": tex_fn, "note": note}
    return out



ORIENT = {0: 0, 1: 2, 2: 1, 3: 0, 4: 1}

def decode_ps(bundle, psid, dump, probe):
    ini = dump.get("InitialModule") or {}
    emi = dump.get("EmissionModule") or {}
    go_pid = (dump.get("m_GameObject") or {}).get("m_PathID", 0)
    rec = probe.get(go_pid)
    meta = {
        "psid": psid,
        "go_pid": go_pid,
        "play": bool(dump.get("playOnAwake", True)),
        "looping": bool(dump.get("looping", False)),
        "subs": [],
        "first_time": float(dump.get("startDelay", 0.0)),
    }
    sub = dump.get("SubModule") or {}
    if sub.get("enabled"):
        for k in ("subEmitterBirth", "subEmitterBirth1"):
            pid = (sub.get(k) or {}).get("m_PathID", 0)
            if pid:
                meta["subs"].append(pid)

    if rec is None or rec["tex"] is None:
        if rec is not None and rec["note"]:
            warn(bundle, f"ps {psid}: {rec['note']}")
        return [], meta
    if int(ini.get("maxNumParticles", 0)) <= 0 or not emi.get("enabled", True):
        return [], meta

    scale = rec["scale"] if rec["scale"] > 0.0001 else 1.0
    life = mm(ini.get("startLifetime"), 1.0)
    speed = mm(ini.get("startSpeed"))
    size = mm(ini.get("startSize"), 1.0)

    base = {
        "tex": rec["tex"],
        "additive": rec["additive"],
        "keyout": rec["keyout"],
        "orient": ORIENT.get(rec["mode"], 0),
        "delay": round(float(dump.get("startDelay", 0.0)), 3),
        "duration": round(float(dump.get("lengthInSec", 0.0)), 3),
        "life0": round(max(life[0], 0.03), 3),
        "life1": round(max(life[1], 0.03), 3),
        "speed0": round(speed[0], 3),
        "speed1": round(speed[1], 3),
        "offset": [round(v, 3) + 0.0 for v in rec["offset"]],
        "period": round(float(dump.get("lengthInSec", 0.0)), 3),
        "one_shot": int(not meta["looping"]),
        "prewarm": int(bool(dump.get("prewarm", False))),
        "max_particles": int(ini.get("maxNumParticles", 0)),
    }
    if rec["max_screen_size"] > 0:
        base["max_screen_size"] = round(rec["max_screen_size"], 4)
    if base["orient"] == 2:
        if rec["length_scale"] > 0:
            base["stretch_length"] = round(rec["length_scale"], 4)
        if rec["velocity_scale"] > 0:
            base["stretch_velocity"] = round(rec["velocity_scale"], 4)

    shp = dump.get("ShapeModule") or {}
    if shp.get("enabled"):
        st = int(shp.get("type", 0))
        if st in (4, 7, 8, 9):
            d = qrot(rec["rot"], (0.0, 0.0, 1.0))
            base["dir"] = [round(d[0], 3), round(d[1], 3), round(-d[2], 3)]
            base["cone_deg"] = round(float(shp.get("angle", 0.0)), 1)
            base["radius"] = round(float(shp.get("radius", 0.0)) * scale, 3)
        elif st == 5:
            extents = rotated_box_extents(
                rec["rot"],
                tuple(float(shp.get(axis, 0.0)) * scale for axis in ("boxX", "boxY", "boxZ")),
            )
            base["box"] = [round(extents[0], 3), round(extents[1], 3), round(extents[2], 3)]
            if speed[1] > 0.01 and not shp.get("randomDirection", False):
                d = qrot(rec["rot"], (0.0, 0.0, 1.0))
                base["dir"] = [round(d[0], 3), round(d[1], 3), round(-d[2], 3)]
        else:
            base["radius"] = round(float(shp.get("radius", 0.0)) * scale, 3)

    grav = float(ini.get("gravityModifier", 0.0)) * 9.81
    frc = dump.get("ForceModule") or {}
    if frc.get("enabled"):
        fy = mm(frc.get("y"))
        grav -= (fy[0] + fy[1]) * 0.5
    if abs(grav) > 0.01:
        base["gravity"] = round(grav, 2)

    vel = dump.get("VelocityModule") or {}
    if vel.get("enabled"):
        v = [(lo + hi) * 0.5 for lo, hi in (mm(vel.get(a)) for a in "xyz")]
        mag = math.sqrt(sum(c * c for c in v))
        if mag > 0.05:
            if not vel.get("inWorldSpace"):
                v = list(qrot(rec["rot"], v))
            v[2] = -v[2]
            if base["speed1"] < 0.05:
                base["dir"] = [round(c / mag, 3) for c in v]
                base["speed0"], base["speed1"] = round(mag * 0.5, 2), round(mag, 2)
            else:
                base["speed1"] = round(base["speed1"] + mag, 2)

    clv = dump.get("ClampVelocityModule") or {}
    if clv.get("enabled"):
        base["drag"] = round(float(clv.get("dampen", 0.0)) * 10.0, 1)

    szm = dump.get("SizeModule") or {}
    if szm.get("enabled"):
        size_curve = szm.get("curve") or {}
        curve = curve_keys(size_curve)
        if size[1] <= 1e-4:
            curve_range = mm(size_curve)
            curve_peak = max(abs(curve_range[0]), abs(curve_range[1]))
            if curve_peak > 1e-4:
                size = (1.0, 1.0)
                warn(bundle, f"ps {psid}: zero start size uses SizeModule as absolute size")
        if curve:
            base["size_curve"] = curve

    sc = ini.get("startColor") or {}
    start = rgba((sc.get("maxColor") or {}).get("rgba", 0xFFFFFFFF))
    colm = dump.get("ColorModule") or {}
    have_alpha = False
    have_color = False
    if colm.get("enabled"):
        gradient = (colm.get("gradient") or {}).get("maxGradient")
        if gradient:
            color_keys, alpha_keys = gradient_keys(gradient)
            if color_keys:
                have_color = True
                base["color_curve"] = [
                    [round(time, 6),
                     min(255, round(start[0] * color[0] / 255)),
                     min(255, round(start[1] * color[1] / 255)),
                     min(255, round(start[2] * color[2] / 255)),
                     start[3]]
                    for time, color in color_keys
                ]
            if alpha_keys:
                have_alpha = True
                base["alpha_curve"] = [[round(time, 6), round(alpha / 255.0, 6)]
                                       for time, alpha in alpha_keys]
    if not have_color and start != (255, 255, 255, 255):
        base["tint"] = list(start)
    if not have_alpha:
        base["alpha_hold"] = 1.0

    rot = mm(ini.get("startRotation"))
    rot_bias = math.degrees((rot[0] + rot[1]) * 0.5)
    rot_span = math.degrees((rot[1] - rot[0]) * 0.5)
    if abs(rot_bias) > 1:
        base["rot_bias_deg"] = round(rot_bias, 2)
    if rot_span > 1:
        base["rot_deg"] = round(rot_span, 2)
    rotm = dump.get("RotationModule") or {}
    if rotm.get("enabled"):
        spin_node = rotm.get("curve")
        spin_keys = curve_keys(spin_node)
        if spin_keys:
            base["spin_curve"] = [[time, round(math.degrees(value), 6)] for time, value in spin_keys]
        else:
            spin = mm(spin_node)
            spin_bias = math.degrees((spin[0] + spin[1]) * 0.5)
            spin_span = math.degrees((spin[1] - spin[0]) * 0.5)
            if abs(spin_bias) > 1:
                base["spin_bias_deg"] = round(spin_bias, 2)
            if spin_span > 1:
                base["spin_deg"] = round(spin_span, 2)

    uv = dump.get("UVModule") or {}
    if uv.get("enabled"):
        base["cols"] = int(uv.get("tilesX", 1))
        base["rows"] = int(uv.get("tilesY", 1))
        fot = uv.get("frameOverTime") or {}
        cycles = float(uv.get("cycles", 0.0))
        if cycles > 0:
            base["sheet_cycles"] = round(cycles, 3)
        if fot.get("minMaxState", 0) == 1 and int(uv.get("animationType", 0)) == 0:
            base["sheet_once"] = 1
    base["size0"], base["size1"] = round(size[0], 3), round(size[1], 3)

    ems = []
    max_live = int(ini.get("maxNumParticles", 1000))
    rate_node = emi.get("rate")
    rate = mm(rate_node)
    exact_rate = curve_keys(rate_node)
    n_burst = int(emi.get("m_BurstCount", 0))
    bursts = {}
    for i in range(min(n_burst, 4)):
        t = round(float(emi.get(f"time{i}", 0.0)), 3)
        bursts[t] = bursts.get(t, 0) + int(emi.get(f"cnt{i}", 0))
    expected = rate[1] * max(base["duration"], 0.05)
    if int(emi.get("m_Type", 0)) == 0 and rate[1] >= 0.5 and \
            (meta["looping"] or expected >= (2.0 if bursts else 0.75)):
        e = dict(base)
        life_avg = max((base["life0"] + base["life1"]) * 0.5, 0.05)
        e["rate"] = round(min(rate[1], max_live / life_avg, MAX_RATE), 1)
        if len(exact_rate) >= 2 and rate[1] > 1e-6:
            e["rate_curve"] = [[time, round(value / rate[1], 6)] for time, value in exact_rate]
        ems.append(e)
    for t, cnt in sorted(bursts.items()):
        if cnt <= 0:
            continue
        e = dict(base)
        e["count"] = min(cnt, max_live, MAX_BURST)
        e["delay"] = round(base["delay"] + t, 3)
        e["duration"] = 0.0
        ems.append(e)
    meta["first_time"] = base["delay"] + (min(bursts) if bursts and rate[1] < 0.5 else 0.0)
    return ems, meta


def convert_bundle(bundle, cab_path, shared_res, shared_shd, tex_files):
    dumps = {}
    for path in glob.glob(f"{PARTICLES}/{glob.escape(bundle)}__*.json"):
        psid = int(os.path.basename(path)[len(bundle) + 2:-5])
        dumps[psid] = json.load(open(path))
    if not dumps:
        warn(bundle, "no particle dumps")
        return None
    probe = probe_bundle(bundle, cab_path, shared_res, shared_shd, tex_files)
    if probe is None:
        warn(bundle, "CAB probe failed")
        return None

    decoded = {}
    for psid, dump in dumps.items():
        decoded[psid] = decode_ps(bundle, psid, dump, probe)

    include = {}
    for psid, (ems, meta) in decoded.items():
        if meta["play"] and ems:
            include[psid] = 0.0
    for psid, (_ems, meta) in decoded.items():
        if psid not in include:
            continue
        for sub in decoded[psid][1]["subs"]:
            if sub in decoded and sub not in include and decoded[sub][0]:
                include[sub] = decoded[psid][1]["first_time"]

    emitters, length = [], 0.0
    for psid, extra_delay in include.items():
        for e in decoded[psid][0]:
            if extra_delay > 0:
                e["delay"] = round(e["delay"] + extra_delay, 3)
            emitters.append(e)
    if not emitters:
        warn(bundle, "no usable emitters")
        return None
    if len(emitters) > MAX_EMITTERS:
        emitters.sort(key=lambda e: -(e.get("count", 0) + e.get("rate", 0) *
                                      max(e.get("duration", 0), 0.3)) * e["size1"] ** 2)
        warn(bundle, f"{len(emitters)} emitters trimmed to {MAX_EMITTERS}")
        emitters = emitters[:MAX_EMITTERS]
    for e in emitters:
        length = max(length, e["delay"] + (e["duration"] if e.get("rate") else 0) + e["life1"])
    return {"name": bundle, "length": round(min(length + 0.1, MAX_LENGTH), 2),
            "emitters": [compact(e) for e in emitters]}


DEFAULTS = {"additive": 0, "keyout": 0, "orient": 0, "cols": 0, "rows": 0,
            "sheet_once": 0, "sheet_cycles": 0, "max_screen_size": 0,
            "render_scale": 0, "stretch_length": 0, "stretch_velocity": 0,
            "one_shot": 0, "prewarm": 0, "count": 0, "rate": 0, "delay": 0, "duration": 0,
            "speed0": 0, "speed1": 0, "cone_deg": 0, "radius": 0, "drag": 0,
            "gravity": 0, "size_end": 0, "alpha_in": 0, "alpha_hold": 0,
            "rot_deg": 0, "spin_deg": 0, "rot_bias_deg": 0, "spin_bias_deg": 0}

def compact(e):
    out = {}
    for k, v in e.items():
        if k in DEFAULTS and (v == DEFAULTS[k] or v == 0):
            continue
        if k == "offset" and v == [0, 0, 0]:
            continue
        if k == "dir" and v == [0, 0, 0]:
            continue
        out[k] = v
    return out


def validate_presets(presets):
    errors = []
    for preset in presets:
        for index, emitter in enumerate(preset["emitters"]):
            live = emitter.get("count", 0) > 0 or emitter.get("rate", 0) > 0
            if live and emitter.get("size0", 0) <= 0 and emitter.get("size1", 0) <= 0:
                errors.append(f"{preset['name']} emitter {index}: zero size")
            if emitter.get("size_end", 0) > 8:
                errors.append(f"{preset['name']} emitter {index}: pathological size_end")
            for field in ("size_curve", "alpha_curve", "rate_curve", "spin_curve", "color_curve"):
                keys = emitter.get(field, [])
                if len(keys) > 16:
                    errors.append(f"{preset['name']} emitter {index}: {field} has {len(keys)} keys")
                times = [key[0] for key in keys]
                if times != sorted(times) or any(not math.isfinite(float(value)) for key in keys for value in key):
                    errors.append(f"{preset['name']} emitter {index}: invalid {field}")
    if errors:
        raise ValueError("invalid generated emitters: " + "; ".join(errors))


def main():
    only = None
    for i, a in enumerate(sys.argv):
        if a == "--only":
            only = sys.argv[i + 1].split(",")

    wires = json.load(open(UNIT_VFX))
    names = set()
    for character in wires.values():
        bindings = [character, *(character.get("tiers") or {}).values()]
        for wire in bindings:
            for role in ("ability", "impact", "muzzle", "sustained"):
                if wire.get(role):
                    names.add(wire[role])
    names.update({
        "fx_eff_faction_charge_pre",
        "fx_eff_redfaction_charge_pre",
        "fx_eff_bluefaction_charge_pre",
        "fx_eff_blackfaction_charge_pre",
    })
    targets = {}
    for n in sorted(names):
        targets.setdefault(ALIASES.get(n, n), []).append(n)
    if only:
        targets = {ALIASES.get(n, n): [n] for n in only}

    cab_idx = build_cab_index()
    shared_res = SharedContainer(cab_idx["shared_resourcecontainer"])
    shared_shd = SharedContainer(cab_idx["shared_shadercontainer"])
    tex_files = set(os.listdir(TEXTURES))

    presets, aliases, missing = [], {}, []
    for bundle, wire_names in sorted(targets.items()):
        cab = cab_idx.get(bundle.lower())
        if cab is None:
            missing.append(bundle)
            continue
        p = convert_bundle(bundle, cab, shared_res, shared_shd, tex_files)
        if p is None:
            missing.append(bundle)
            continue
        presets.append(p)
        for n in wire_names:
            if n != bundle:
                aliases[n] = bundle

    if only:
        print(json.dumps(presets, indent=1))
        print("ALIASES:", aliases, "MISSING:", missing)
        for w in warnings:
            print("WARN", w)
        return

    presets.sort(key=lambda p: p["name"])
    validate_presets(presets)
    with open(OUT, "w") as f:
        json.dump({"version": 1, "presets": presets, "aliases": aliases}, f, indent=0)
    with open(REPORT, "w") as f:
        json.dump({"presets": len(presets), "aliases": aliases, "missing": missing,
                   "warnings": warnings}, f, indent=1)
    total_em = sum(len(p["emitters"]) for p in presets)
    print(f"{len(presets)} presets, {total_em} emitters, {len(aliases)} aliases, "
          f"{len(missing)} missing -> {OUT}")
    print(f"report -> {REPORT} ({len(warnings)} warnings)")


if __name__ == "__main__":
    main()
