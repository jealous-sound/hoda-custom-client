#!/usr/bin/env python3

from __future__ import annotations

import argparse
import collections
import json
import math
import os
import re
import struct
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CATALOG = ROOT / "heroes-of-dragons-cache/characters.json"
DEFAULT_CAB_INDEX = ROOT / "heroes-of-dragons-cache/anim_tools/cache/cab_paths.json"
DEFAULT_AUDIO_DIR = ROOT / "heroes-of-dragons-cache/vfx/audio"
DEFAULT_OUTPUT = ROOT / "data/unit_audio.json"

TIERS = ("I", "II", "III", "IV")
ARRAY_FIELDS = (
    ("attack", "AbilityStartAudioClip", "VolAbilityStart"),
    ("impact", "AbilityHitTargetAudioClip", "VolAbilityHitTarget"),
    ("vocal", "AbilityStartVocalAudioClip", "VolAbilityStartVocal"),
    ("death", "UnitDeathAudioClip", "VolUnitDeath"),
    ("move_out", "MoveToCenterAudioClips", "VolMoveToCenter"),
    ("move_back", "MoveBackAudioClips", "VolMoveBack"),
    ("land", "LandInCenterAudioClips", "VolLandInCenter"),
    ("animation", "AnimationStartAudioClips", "VolAnimationStart"),
)
SINGLE_FIELDS = (
    ("critical", "AbilityHitCriticalAudioClip", "VolAbilityHitCritical"),
    ("ui", "UIActionAudioClip", "VolUIAction"),
)


class Reader:

    def __init__(self, data: bytes):
        self.data = data
        self.offset = 0

    def _need(self, size: int) -> None:
        if self.offset + size > len(self.data):
            raise ValueError("truncated MonoBehaviour")

    def i32(self) -> int:
        self._need(4)
        value = struct.unpack_from("<i", self.data, self.offset)[0]
        self.offset += 4
        return value

    def f32(self) -> float:
        self._need(4)
        value = struct.unpack_from("<f", self.data, self.offset)[0]
        self.offset += 4
        if not math.isfinite(value):
            raise ValueError("non-finite float")
        return float(value)

    def u8(self) -> int:
        self._need(1)
        value = self.data[self.offset]
        self.offset += 1
        return value

    def align(self) -> None:
        self.offset = (self.offset + 3) & ~3

    def string(self) -> str:
        size = self.i32()
        if size < 0 or size > 4096:
            raise ValueError(f"bad string length {size}")
        self._need(size)
        value = self.data[self.offset : self.offset + size].decode("utf-8")
        self.offset += size
        self.align()
        return value

    def strings(self) -> list[str]:
        count = self.i32()
        if count < 0 or count > 64:
            raise ValueError(f"bad array length {count}")
        return [self.string() for _ in range(count)]

    def pptr(self) -> tuple[int, int]:
        return self.i32(), self.i32()

    def header(self) -> str:
        self.pptr()
        self.u8()
        self.offset += 3
        self.pptr()
        return self.string()


def parse_unit_audio(raw: bytes, known_clips: set[str]) -> dict[str, Any] | None:

    try:
        reader = Reader(raw)
        reader.header()
        out: dict[str, Any] = {}
        out["AbilityStartAudioClip"] = reader.strings()
        out["VolAbilityStart"] = reader.f32()
        out["AbilityHitTargetAudioClip"] = reader.strings()
        out["VolAbilityHitTarget"] = reader.f32()
        out["AbilityHitTargetTimeLimit"] = reader.f32()
        out["AbilityStartVocalAudioClip"] = reader.strings()
        out["VolAbilityStartVocal"] = reader.f32()
        out["AbilityHitCriticalAudioClip"] = reader.string()
        out["VolAbilityHitCritical"] = reader.f32()
        out["UnitDeathAudioClip"] = reader.strings()
        out["VolUnitDeath"] = reader.f32()
        out["UIActionAudioClip"] = reader.string()
        out["VolUIAction"] = reader.f32()
        out["MoveToCenterAudioClips"] = reader.strings()
        out["VolMoveToCenter"] = reader.f32()
        out["MoveBackAudioClips"] = reader.strings()
        out["VolMoveBack"] = reader.f32()
        out["LandInCenterAudioClips"] = reader.strings()
        out["VolLandInCenter"] = reader.f32()
        out["AnimationStartAudioClips"] = reader.strings()
        out["VolAnimationStart"] = reader.f32()
    except (UnicodeDecodeError, ValueError, struct.error):
        return None

    clips: list[str] = []
    for _slot, field, _volume in ARRAY_FIELDS:
        clips.extend(out[field])
    for _slot, field, _volume in SINGLE_FIELDS:
        if out[field]:
            clips.append(out[field])
    if (
        not clips
        or not any(clip.casefold() in known_clips for clip in clips)
        or any(not re.fullmatch(r"[A-Za-z0-9 _.-]+", clip) for clip in clips)
    ):
        return None

    volumes = [out[field] for _slot, _clips, field in ARRAY_FIELDS + SINGLE_FIELDS]
    if any(volume < 0.0 or volume > 4.0 for volume in volumes):
        return None
    if out["AbilityHitTargetTimeLimit"] < 0.0 or out["AbilityHitTargetTimeLimit"] > 10.0:
        return None
    return out


def normalized_profile(raw: dict[str, Any]) -> dict[str, Any]:
    profile: dict[str, Any] = {}
    for slot, field, volume_field in ARRAY_FIELDS:
        profile[slot] = {
            "clips": raw[field],
            "volume": round(raw[volume_field], 4),
        }
    for slot, field, volume_field in SINGLE_FIELDS:
        profile[slot] = {
            "clips": [raw[field]] if raw[field] else [],
            "volume": round(raw[volume_field], 4),
        }
    profile["impact_time_limit"] = round(raw["AbilityHitTargetTimeLimit"], 4)
    return profile


def prefab_key(value: str) -> str:
    value = value.removeprefix("unit.").strip().casefold()
    return re.sub(r"[^a-z0-9_]+", "_", value).strip("_")


def family_key(prefab: str) -> str:

    key = prefab_key(prefab)
    parts = key.split("_")
    race = parts[0] if parts else "unit"
    weapon_codes = {"mw", "sw", "tw", "dw", "bw", "sp", "bg", "bow", "spear", "advisor"}
    if race in {"hm", "hf", "em", "ef", "dm", "df", "qm", "qf", "mnt"}:
        weapon = next((part for part in parts[1:3] if part in weapon_codes), "generic")
        return f"humanoid:{race}:{weapon}"

    species = (
        "highdragon",
        "dragon",
        "drake",
        "wyvern",
        "werewolf",
        "wolf",
        "dog",
        "bear",
        "nug",
        "halla",
        "hart",
        "spider",
        "deepstalker",
        "ghast",
        "shriek",
        "genlock",
        "hurlock",
        "darkspawn",
        "skeleton",
        "revenant",
        "golem",
        "sylvan",
        "wisp",
        "demon",
        "abomination",
        "ogre",
        "behemoth",
        "qunari",
    )
    joined = "_".join(parts[1:])
    found = next((name for name in species if name in joined), None)
    if found:
        return f"creature:{found}"
    if race in {"b", "banner"} or "banner" in joined:
        return "noncombat:banner"
    return "creature:large" if any(word in joined for word in ("large", "giant", "brute")) else "creature:generic"


def donor_score(target: str, candidate: str) -> tuple[int, int, int, str]:

    tf, cf = family_key(target), family_key(candidate)
    tparts, cparts = set(prefab_key(target).split("_")), set(prefab_key(candidate).split("_"))
    shared = len(tparts & cparts)
    tier = 0
    match = re.search(r"_t([1-4])(?:_|$)", candidate)
    if match:
        tier = int(match.group(1))
    return (int(tf == cf), shared, tier, candidate)


def choose_family_donors(profiles: dict[str, dict[str, Any]]) -> dict[str, str]:
    by_family: dict[str, list[str]] = collections.defaultdict(list)
    for key in profiles:
        if profiles[key]["attack"]["clips"]:
            by_family[family_key(key)].append(key)

    donors: dict[str, str] = {}
    for family, keys in sorted(by_family.items()):
        signatures = collections.Counter(
            json.dumps(profiles[key], sort_keys=True, separators=(",", ":")) for key in keys
        )
        keys.sort(
            key=lambda key: (
                bool(profiles[key]["vocal"]["clips"]),
                bool(profiles[key]["impact"]["clips"]),
                signatures[json.dumps(profiles[key], sort_keys=True, separators=(",", ":"))],
                key,
            )
        )
        donors[family] = keys[-1]
    return donors


def fallback_donor(prefab: str, profiles: dict[str, dict[str, Any]], family_donors: dict[str, str]) -> tuple[str, str]:
    family = family_key(prefab)
    if family in family_donors:
        return family, family_donors[family]

    bits = family.split(":")
    alternatives: list[tuple[int, str]] = []
    for known in family_donors:
        kb = known.split(":")
        score = 0
        if bits[0] == kb[0]:
            score += 10
        if len(bits) > 2 and len(kb) > 2 and bits[2] == kb[2]:
            score += 6
        if len(bits) > 1 and len(kb) > 1 and bits[1] == kb[1]:
            score += 4
        alternatives.append((score, known))
    alternatives.sort(key=lambda item: (item[0], item[1]))
    if alternatives and alternatives[-1][0] > 0:
        known = alternatives[-1][1]
        return family, family_donors[known]

    candidates = [key for key, profile in profiles.items() if profile["attack"]["clips"]]
    if not candidates:
        raise RuntimeError("no attack-audio donor profiles were extracted")
    return family, max(candidates, key=lambda key: donor_score(prefab, key))


def configure_unitypy() -> Any:
    try:
        import UnityPy  # type: ignore
        from UnityPy.helpers import UnityVersion as UV  # type: ignore
    except ImportError as exc:
        raise SystemExit(
            "build_unit_audio.py requires UnityPy; run it with the project's UnityPy venv. "
            "Validation alone uses tools/validate_unit_audio.py."
        ) from exc

    original = UV.UnityVersion.from_str.__func__

    def patched(cls: Any, version: str) -> Any:
        match = re.search(r"(\d+)\.(\d+)\.(\d+)([a-zA-Z]+)(\d+)", version)
        return original(cls, match.group(0) if match else version)

    UV.UnityVersion.from_str = classmethod(patched)
    return UnityPy


def extract_profiles(
    prefabs: list[str], cab_index: dict[str, str], known_clips: set[str], unitypy: Any
) -> tuple[dict[str, dict[str, Any]], dict[str, str]]:
    profiles: dict[str, dict[str, Any]] = {}
    unavailable: dict[str, str] = {}
    for index, prefab in enumerate(prefabs, 1):
        cab_path = cab_index.get("cab-" + prefab.casefold())
        if not cab_path or not Path(cab_path).is_file():
            unavailable[prefab] = "cab_missing"
            continue
        try:
            env = unitypy.load(cab_path)
            found = None
            for obj in env.objects:
                if obj.type.name != "MonoBehaviour":
                    continue
                found = parse_unit_audio(bytes(obj.get_raw_data()), known_clips)
                if found:
                    break
            if found:
                profiles[prefab] = normalized_profile(found)
            else:
                unavailable[prefab] = "unit_audio_missing"
        except Exception as exc:
            unavailable[prefab] = f"cab_error:{type(exc).__name__}"
        if index % 100 == 0:
            print(f"audio extraction: {index}/{len(prefabs)}", file=sys.stderr)
    return profiles, unavailable


def build_manifest(
    catalog_path: Path, cab_index_path: Path, audio_dir: Path, unitypy: Any
) -> dict[str, Any]:
    catalog_root = json.loads(catalog_path.read_text())
    characters = catalog_root["characters"]
    cab_index = json.loads(cab_index_path.read_text())
    audio_files = {
        path.stem.casefold(): path.name
        for path in audio_dir.iterdir()
        if path.is_file() and path.suffix.casefold() in {".mp3", ".wav", ".ogg"}
    }
    prefabs = sorted(
        {
            prefab_key(prefab)
            for character in characters
            for prefab in character.get("models", {}).values()
            if prefab
        }
    )
    profiles, unavailable = extract_profiles(prefabs, cab_index, set(audio_files), unitypy)
    family_donors = choose_family_donors(profiles)

    character_map: dict[str, Any] = {}
    resolution_counts: collections.Counter[str] = collections.Counter()
    for character in characters:
        tiers: dict[str, Any] = {}
        attack_capable = "ATTACK_NONE" not in character.get("ability", {}).get("mechanics", [])
        for tier in TIERS:
            raw_prefab = character.get("models", {}).get(tier)
            if not raw_prefab:
                continue
            prefab = prefab_key(raw_prefab)
            family = family_key(prefab)
            if prefab in profiles:
                donor = prefab
                resolution = "exact"
            elif not attack_capable and family.startswith("noncombat:"):
                donor = ""
                resolution = "silent_noncombat"
            else:
                family, donor = fallback_donor(prefab, profiles, family_donors)
                resolution = "family_fallback"
            resolution_counts[resolution] += 1
            tiers[tier] = {
                "prefab": prefab,
                "family": family,
                "profile": donor or None,
                "resolution": resolution,
                "reason": unavailable.get(prefab) if resolution == "family_fallback" else None,
            }
        character_map[character["id"]] = {
            "attack_capable": attack_capable,
            "tiers": tiers,
        }

    family_bindings: dict[str, dict[str, Any]] = {
        family: {"profile": profile, "source": "exact_profile_donor"}
        for family, profile in sorted(family_donors.items())
    }
    for character in character_map.values():
        for tier in character["tiers"].values():
            family = tier["family"]
            if family not in family_bindings:
                family_bindings[family] = {
                    "profile": tier["profile"],
                    "source": "nearest_family_donor" if tier["profile"] else "silent_noncombat",
                }

    referenced = {
        clip.casefold()
        for profile in profiles.values()
        for slot in (*[item[0] for item in ARRAY_FIELDS], *[item[0] for item in SINGLE_FIELDS])
        for clip in profile[slot]["clips"]
    }
    missing_clips = sorted(referenced - set(audio_files))
    attack_capable_tiers = [
        tier
        for character in character_map.values()
        if character["attack_capable"]
        for tier in character["tiers"].values()
    ]

    return {
        "version": 1,
        "source": {
            "catalog": os.path.relpath(catalog_path, ROOT),
            "reference": "UnitAudio MonoBehaviour on each HoDA tier prefab",
            "audio_pool": os.path.relpath(audio_dir, ROOT),
        },
        "semantics": {
            "vocal": "attack animation start; random vocal plus random animation cue",
            "animation": "attack animation start; layered with vocal",
            "attack": "ability release / ProjectileLaunch marker",
            "impact": "projectile contact; reference rate limit is impact_time_limit seconds",
            "critical": "critical hit",
            "death": "unit death",
            "move_out": "movement toward the battle stage",
            "move_back": "movement back to the grid",
            "land": "arrival on the battle stage",
            "ui": "unit UI action",
        },
        "audio_files": dict(sorted(audio_files.items())),
        "profiles": dict(sorted(profiles.items())),
        "families": dict(sorted(family_bindings.items())),
        "characters": character_map,
        "unavailable_prefabs": dict(sorted(unavailable.items())),
        "coverage": {
            "characters": len(character_map),
            "tier_bindings": sum(len(character["tiers"]) for character in character_map.values()),
            "exact_profiles": len(profiles),
            "families": len(family_bindings),
            "resolution": dict(sorted(resolution_counts.items())),
            "attack_capable_tiers": len(attack_capable_tiers),
            "attack_capable_tiers_with_audio": sum(bool(tier["profile"]) for tier in attack_capable_tiers),
            "referenced_clips": len(referenced),
            "missing_clips": missing_clips,
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--cab-index", type=Path, default=DEFAULT_CAB_INDEX)
    parser.add_argument("--audio-dir", type=Path, default=DEFAULT_AUDIO_DIR)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()

    unitypy = configure_unitypy()
    manifest = build_manifest(args.catalog, args.cab_index, args.audio_dir, unitypy)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")
    coverage = manifest["coverage"]
    print(
        "unit audio: "
        f"{coverage['characters']} characters, {coverage['exact_profiles']} exact prefab profiles, "
        f"{coverage['attack_capable_tiers_with_audio']}/{coverage['attack_capable_tiers']} "
        "attack-capable tier bindings with audio"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
