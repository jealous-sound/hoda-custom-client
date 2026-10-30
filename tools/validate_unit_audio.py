#!/usr/bin/env python3

from __future__ import annotations

import argparse
import collections
import json
from pathlib import Path
from typing import Any

import original_cache


CLI_DESCRIPTION = 'Validate per-unit battle audio coverage against the original 4.4.0 cache'
ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "data/unit_audio.json"
DEFAULT_CATALOG = ROOT / "heroes-of-dragons-cache/characters.json"
SLOTS = (
    "attack",
    "impact",
    "vocal",
    "death",
    "move_out",
    "move_back",
    "land",
    "animation",
    "critical",
    "ui",
)
TIERS = ("I", "II", "III", "IV")


def fail(message: str) -> None:
    raise ValueError(message)


def validate_profile(key: str, profile: dict[str, Any], audio_sources: dict[str, str]) -> set[str]:
    referenced: set[str] = set()
    for slot in SLOTS:
        value = profile.get(slot)
        if not isinstance(value, dict):
            fail(f"profile {key}: missing {slot} object")
        clips = value.get("clips")
        volume = value.get("volume")
        if not isinstance(clips, list) or any(not isinstance(clip, str) or not clip for clip in clips):
            fail(f"profile {key}: {slot}.clips must be non-empty strings")
        if not isinstance(volume, (int, float)) or not 0.0 <= volume <= 4.0:
            fail(f"profile {key}: {slot}.volume out of range")
        for clip in clips:
            normalized = clip.casefold()
            referenced.add(normalized)
            if normalized not in audio_sources:
                fail(f"profile {key}: clip {clip!r} is absent from audio_sources")
    limit = profile.get("impact_time_limit")
    if not isinstance(limit, (int, float)) or not 0.0 <= limit <= 10.0:
        fail(f"profile {key}: impact_time_limit out of range")
    return referenced


def validate(manifest_path: Path, catalog_path: Path, check_assets: bool) -> dict[str, int]:
    manifest = json.loads(manifest_path.read_text())
    catalog = json.loads(catalog_path.read_text())["characters"]
    if manifest.get("version") != 1:
        fail("unit_audio.json: unsupported version")

    profiles = manifest.get("profiles")
    characters = manifest.get("characters")
    families = manifest.get("families")
    audio_sources = manifest.get("audio_sources")
    if not all(isinstance(value, dict) for value in (profiles, characters, families, audio_sources)):
        fail("unit_audio.json: profiles/characters/families/audio_sources must be objects")

    for key, ref in audio_sources.items():
        if key != ref.rpartition(":")[2].rpartition("/")[2].casefold():
            fail(f"audio_sources: key {key!r} does not match {ref!r}")
        if check_assets and not original_cache.resolves(ref, original_cache.AUDIO_CLIP):
            fail(f"audio_sources: {ref} is not an AudioClip in the original cache")

    referenced: set[str] = set()
    for key, profile in profiles.items():
        if key != key.casefold():
            fail(f"profile key is not normalized: {key}")
        referenced.update(validate_profile(key, profile, audio_sources))

    catalog_by_id = {character["id"]: character for character in catalog}
    if set(characters) != set(catalog_by_id):
        missing = sorted(set(catalog_by_id) - set(characters))
        extra = sorted(set(characters) - set(catalog_by_id))
        fail(f"character key mismatch; missing={missing[:5]} extra={extra[:5]}")

    counts: collections.Counter[str] = collections.Counter()
    attack_capable_tiers = 0
    attack_capable_tiers_with_audio = 0
    used_families: set[str] = set()
    for character_id, binding in characters.items():
        source = catalog_by_id[character_id]
        expected_attack = "ATTACK_NONE" not in source.get("ability", {}).get("mechanics", [])
        if binding.get("attack_capable") is not expected_attack:
            fail(f"{character_id}: attack_capable disagrees with catalog")
        tiers = binding.get("tiers")
        if not isinstance(tiers, dict):
            fail(f"{character_id}: tiers must be an object")
        expected_tiers = {tier for tier in TIERS if source.get("models", {}).get(tier)}
        if set(tiers) != expected_tiers:
            fail(f"{character_id}: tier bindings disagree with catalog")
        for tier, value in tiers.items():
            expected_prefab = source["models"][tier].removeprefix("unit.").casefold()
            if value.get("prefab") != expected_prefab:
                fail(f"{character_id} {tier}: prefab mismatch")
            family = value.get("family")
            resolution = value.get("resolution")
            profile_key = value.get("profile")
            if not isinstance(family, str) or family not in families:
                fail(f"{character_id} {tier}: undefined family {family!r}")
            used_families.add(family)
            if resolution not in {"exact", "family_fallback", "silent_noncombat"}:
                fail(f"{character_id} {tier}: invalid resolution {resolution!r}")
            if resolution == "exact" and profile_key != expected_prefab:
                fail(f"{character_id} {tier}: exact profile does not match prefab")
            if resolution == "family_fallback" and not value.get("reason"):
                fail(f"{character_id} {tier}: fallback has no reason")
            if profile_key is not None and profile_key not in profiles:
                fail(f"{character_id} {tier}: unknown profile {profile_key!r}")
            counts[resolution] += 1
            if expected_attack:
                attack_capable_tiers += 1
                if profile_key and profiles[profile_key]["attack"]["clips"]:
                    attack_capable_tiers_with_audio += 1
                else:
                    fail(f"{character_id} {tier}: attack-capable unit has no attack cue")

    for family, value in families.items():
        if family != family.casefold():
            fail(f"family key is not normalized: {family}")
        profile_key = value.get("profile")
        if profile_key is not None and profile_key not in profiles:
            fail(f"family {family}: unknown profile {profile_key!r}")
    if not used_families.issubset(families):
        fail("some character families are undefined")

    coverage = manifest.get("coverage", {})
    expected_coverage = {
        "characters": len(characters),
        "tier_bindings": sum(len(value["tiers"]) for value in characters.values()),
        "exact_profiles": len(profiles),
        "families": len(families),
        "resolution": dict(sorted(counts.items())),
        "attack_capable_tiers": attack_capable_tiers,
        "attack_capable_tiers_with_audio": attack_capable_tiers_with_audio,
        "referenced_clips": len(referenced),
        "missing_clips": [],
    }
    if coverage != expected_coverage:
        fail(f"coverage summary is stale: expected {expected_coverage}, got {coverage}")
    return {
        "characters": len(characters),
        "profiles": len(profiles),
        "families": len(families),
        "attack_tiers": attack_capable_tiers,
        "clips": len(referenced),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--skip-assets", action="store_true", help="do not require the original 4.4.0 cache")
    args = parser.parse_args()
    counts = validate(args.manifest, args.catalog, not args.skip_assets)
    print(
        "unit audio: ok "
        f"({counts['characters']} characters, {counts['profiles']} exact profiles, "
        f"{counts['families']} families, {counts['attack_tiers']} attack tiers, {counts['clips']} clips)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
