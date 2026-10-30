#!/usr/bin/env python3

from __future__ import annotations

import argparse
import collections
import json
import math
import re
import struct
import sys
from pathlib import Path
from typing import Any


CLI_DESCRIPTION = 'Build the HoDA per-character attack-VFX manifest'
ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CATALOG = ROOT / "heroes-of-dragons-cache" / "characters.json"
DEFAULT_REFERENCE = ROOT / ".agents" / "skills" / "hoda-reference" / "data" / "characters.json"
DEFAULT_CAB_INDEX = ROOT / "heroes-of-dragons-cache" / "anim_tools" / "cache" / "cab_paths.json"
DEFAULT_OUTPUT = ROOT / "data" / "unit_vfx.json"

TIERS = ("I", "II", "III", "IV")
WIRE_FIELDS = (
    "ability_type",
    "vfx_type",
    "ability",
    "impact",
    "muzzle",
    "sustained",
    "projectile_speed",
    "cone_delay",
)
MODEL_OVERRIDES = {
    "UNIT_BEHEMOTH": "unit.c_behemoth_t4_pre",
    "UNIT_GHASTARCHER": "unit.c_ghastarcher_t4_pre",
    "UNIT_GHASTBRUTE": "unit.c_ghastbrute_t4_pre",
    "UNIT_WILDSYLVAN": "unit.c_wildsylvan_t4_pre",
}

UNIT_ACTIONS_SCRIPT = 799
UNIT_VFX_SCRIPT = 914

CREATURE_FAMILY_TOKENS = (
    ("archdemon", "highdragon"),
    ("highdragon", "highdragon"),
    ("dragonling", "dragonling"),
    ("dracolisk", "dracolisk"),
    ("deepstalker", "deepstalker"),
    ("abomination", "abomination"),
    ("nuggalope", "nuggalope"),
    ("spiderling", "spider"),
    ("werewolf", "werewolf"),
    ("darkspawn", "darkspawn"),
    ("skeleton", "skeleton"),
    ("revenant", "revenant"),
    ("behemoth", "behemoth"),
    ("genlock", "genlock"),
    ("hurlock", "hurlock"),
    ("wyvern", "wyvern"),
    ("wyvrin", "wyvern"),
    ("bronto", "bronto"),
    ("mabari", "mabari"),
    ("banner", "banner"),
    ("dragon", "dragon"),
    ("drake", "drake"),
    ("wolf", "wolf"),
    ("dog", "dog"),
    ("bear", "bear"),
    ("nug", "nug"),
    ("halla", "halla"),
    ("hart", "hart"),
    ("spider", "spider"),
    ("ghast", "ghast"),
    ("shriek", "shriek"),
    ("golem", "golem"),
    ("sylvan", "sylvan"),
    ("wisp", "wisp"),
    ("demon", "demon"),
    ("ogre", "ogre"),
)

SEMANTIC_FAMILY_DONORS = {
    "UNIT_HUNTERSHADEDRACOLISK": "UNIT_WYVERN",
    "UNIT_SHARPTAILDRACOLISK": "UNIT_WYVERN",
    "UNIT_PRIMAL-TRAINEDLONGMADRACOLISK": "UNIT_WYVERN",
    "UNIT_MOUNTAINDRACOLISK": "UNIT_WYVERN",
    "UNIT_WILDHART": "UNIT_GOLDENHALLA",
    "UNIT_ZITHER": "UNIT_MARYDEN",
    "UNIT_SPIRITOFSHARTAN": "UNIT_ABELAS",
}

IDENTITY_STOP_WORDS = {
    "and",
    "of",
    "the",
}


class Reader:

    def __init__(self, data: bytes):
        self.data = data
        self.offset = 0

    def need(self, size: int) -> None:
        if self.offset + size > len(self.data):
            raise ValueError("truncated MonoBehaviour")

    def i32(self) -> int:
        self.need(4)
        value = struct.unpack_from("<i", self.data, self.offset)[0]
        self.offset += 4
        return value

    def f32(self) -> float:
        self.need(4)
        value = struct.unpack_from("<f", self.data, self.offset)[0]
        self.offset += 4
        if not math.isfinite(value):
            raise ValueError("non-finite float")
        return float(value)

    def u8(self) -> int:
        self.need(1)
        value = self.data[self.offset]
        self.offset += 1
        return value

    def align(self) -> None:
        self.offset = (self.offset + 3) & ~3

    def string(self) -> str:
        size = self.i32()
        if size < 0 or size > 4096:
            raise ValueError(f"bad string length {size}")
        self.need(size)
        value = self.data[self.offset : self.offset + size].decode("utf-8")
        self.offset += size
        self.align()
        return value

    def pptr(self) -> tuple[int, int]:
        return self.i32(), self.i32()

    def header(self) -> None:
        self.pptr()
        self.u8()
        self.offset += 3
        self.pptr()
        self.string()


def normalize_resource(value: str) -> str | None:

    value = value.strip()
    while value.startswith(("vfx.", "fx.")):
        value = value.split(".", 1)[1]
    return value or None


def parse_vfx(raw: bytes) -> dict[str, Any]:
    reader = Reader(raw)
    reader.header()
    vfx_type = reader.i32()
    ability = reader.string()
    reader.string()
    muzzle = reader.string()
    impact = reader.string()
    sustained = reader.string()
    projectile_speed = reader.f32()
    reader.pptr()
    cone_delay = reader.f32()
    reader.pptr()
    reader.pptr()
    if vfx_type not in range(3):
        raise ValueError(f"invalid VFX type {vfx_type}")
    return {
        "vfx_type": vfx_type,
        "ability": normalize_resource(ability),
        "impact": normalize_resource(impact),
        "muzzle": normalize_resource(muzzle),
        "sustained": normalize_resource(sustained),
        "projectile_speed": round(projectile_speed, 4),
        "cone_delay": round(cone_delay, 4),
    }


def parse_action(raw: bytes) -> int:
    reader = Reader(raw)
    reader.header()
    ability_type = reader.i32()
    if ability_type not in range(8):
        raise ValueError(f"invalid ability type {ability_type}")
    return ability_type


def script_id(raw: bytes) -> int | None:
    if len(raw) < 20:
        return None
    return struct.unpack_from("<i", raw, 16)[0]


def prefab_key(value: str) -> str:
    return value.removeprefix("unit.").strip().casefold()


def requested_prefab(reference: dict[str, Any], tier: str) -> str:
    value = reference["unit_prefab"]
    return value[tier] if isinstance(value, dict) else value


def wire_signature(wire: dict[str, Any], include_action: bool = True) -> str:
    fields = WIRE_FIELDS if include_action else WIRE_FIELDS[1:]
    return json.dumps([wire.get(field) for field in fields], separators=(",", ":"))


def configure_unitypy() -> Any:
    try:
        import UnityPy  # type: ignore
        from UnityPy.helpers import UnityVersion as UV  # type: ignore
    except ImportError as exc:
        raise SystemExit(
            "build_unit_vfx.py requires UnityPy; run it with "
            "heroes-of-dragons-cache/vfx_tools/venv/bin/python"
        ) from exc

    original = UV.UnityVersion.from_str.__func__

    def patched(cls: Any, version: str) -> Any:
        match = re.search(r"(\d+)\.(\d+)\.(\d+)([a-zA-Z]+)(\d+)", version)
        return original(cls, match.group(0) if match else version)

    UV.UnityVersion.from_str = classmethod(patched)
    return UnityPy


def component_object(component: Any) -> Any:

    pointer = component[1] if isinstance(component, tuple) else getattr(component, "component", component)
    if hasattr(pointer, "deref"):
        return pointer.deref()
    if hasattr(pointer, "get_obj"):
        return pointer.get_obj()
    raise ValueError("unsupported UnityPy component pointer")


def extract_prefab(prefab: str, cab_path: str, unitypy: Any) -> dict[str, Any]:

    expected_name = "unit." + prefab_key(prefab)
    environment = unitypy.load(cab_path)
    root = None
    for obj in environment.objects:
        if obj.type.name != "GameObject":
            continue
        game_object = obj.read()
        if game_object.m_Name.casefold() == expected_name:
            root = game_object
            break
    if root is None:
        raise ValueError("root_gameobject_missing")

    vfx_raw = None
    action_raw = None
    for component in root.m_Component:
        obj = component_object(component)
        if obj.type.name != "MonoBehaviour":
            continue
        raw = bytes(obj.get_raw_data())
        sid = script_id(raw)
        if sid == UNIT_VFX_SCRIPT and vfx_raw is None:
            vfx_raw = raw
        elif sid == UNIT_ACTIONS_SCRIPT and action_raw is None:
            action_raw = raw
    if vfx_raw is None:
        raise ValueError("unit_vfx_controller_missing")
    if action_raw is None:
        raise ValueError("unit_actions_controller_missing")

    wire = {"ability_type": parse_action(action_raw)}
    wire.update(parse_vfx(vfx_raw))
    return wire


def semantic_words(value: str) -> list[str]:
    return re.findall(r"[a-z0-9]+", value.casefold())


def family_key(prefab: str, *hints: str) -> str:

    key = prefab_key(prefab)
    parts = semantic_words(key.replace("_", " "))
    race = parts[0] if parts else "unit"
    weapon_codes = {"mw", "sw", "tw", "dw", "bw", "sp", "bg", "bd", "bow", "spear", "advisor", "nc"}
    if race in {"hm", "hf", "em", "ef", "dm", "df", "qm", "qf", "mnt"}:
        weapon = next((part for part in parts[1:3] if part in weapon_codes), "generic")
        return f"humanoid:{race}:{weapon}"

    encoded = parts[1:]
    hint_words = [word for hint in hints for word in semantic_words(hint)]
    semantic = set(encoded) | set(hint_words)
    semantic.update(left + right for left, right in zip(hint_words, hint_words[1:]))
    matches = [
        (len(token), token, family)
        for token, family in CREATURE_FAMILY_TOKENS
        if token in semantic or any(part.startswith(token) or part.endswith(token) for part in encoded)
    ]
    if matches:
        return f"creature:{max(matches)[2]}"

    large_markers = ("large", "giant", "brute")
    is_large = any(
        marker in semantic or any(part.startswith(marker) or part.endswith(marker) for part in encoded)
        for marker in large_markers
    )
    return "creature:large" if is_large else "creature:generic"


def character_family(character: dict[str, Any], tier: str) -> str:
    return family_key(character["models"][tier], character.get("name", ""))


def identity_tokens(character: dict[str, Any]) -> set[str]:

    return {
        word
        for word in semantic_words(character.get("name", ""))
        if len(word) >= 3 and word not in IDENTITY_STOP_WORDS
    }


def wire_resources(wire: dict[str, Any]) -> set[str]:
    return {
        value.casefold()
        for field in ("ability", "impact", "muzzle", "sustained")
        if isinstance((value := wire.get(field)), str) and value
    }


def choose_family_donors(
    profiles: dict[str, dict[str, Any]], profile_families: dict[str, str]
) -> dict[str, str]:
    by_family: dict[str, list[str]] = collections.defaultdict(list)
    for prefab in profiles:
        if prefab in profile_families:
            by_family[profile_families[prefab]].append(prefab)

    donors: dict[str, str] = {}
    for family, prefabs in sorted(by_family.items()):
        counts = collections.Counter(wire_signature(profiles[prefab], include_action=False) for prefab in prefabs)
        prefabs.sort(
            key=lambda prefab: (
                counts[wire_signature(profiles[prefab], include_action=False)],
                bool(profiles[prefab].get("ability")),
                prefab,
            )
        )
        donors[family] = prefabs[-1]
    return donors


def catalog_hint_donor(
    character: dict[str, Any],
    tier: str,
    family: str,
    profiles: dict[str, dict[str, Any]],
    profile_owners: dict[str, list[tuple[str, str]]],
    characters: dict[str, dict[str, Any]],
) -> tuple[str, str, str, tuple[str, ...]] | None:

    hints = {value.casefold() for value in character.get("vfx", []) if isinstance(value, str)}
    if not hints:
        return None
    target_identity = identity_tokens(character)
    candidates: list[tuple[tuple[int, int, int, int, int, int], str, str, str, tuple[str, ...]]] = []
    for prefab, wire in profiles.items():
        matched = tuple(sorted(hints & wire_resources(wire)))
        if not matched:
            continue
        for source_unit, source_tier in profile_owners.get(prefab, []):
            source_character = characters[source_unit]
            source_identity = identity_tokens(source_character)
            shared_identity = target_identity & source_identity
            if not shared_identity:
                continue
            source_family = character_family(source_character, source_tier)
            score = (
                len(matched),
                int(isinstance(wire.get("ability"), str) and wire["ability"].casefold() in hints),
                len(shared_identity),
                -len(source_identity - target_identity),
                int(source_family == family),
                int(source_tier == tier),
            )
            candidates.append((score, prefab, source_unit, source_tier, matched))
    if not candidates:
        return None
    candidates.sort(
        key=lambda row: (
            *(-value for value in row[0]),
            row[1],
            row[2],
            TIERS.index(row[3]),
        )
    )
    _score, prefab, source_unit, source_tier, matched = candidates[0]
    return prefab, source_unit, source_tier, matched


def approved_family_donor(
    character: dict[str, Any],
    tier: str,
    profiles: dict[str, dict[str, Any]],
    profile_owners: dict[str, list[tuple[str, str]]],
) -> tuple[str, str, str]:

    source_unit = SEMANTIC_FAMILY_DONORS.get(character["id"])
    if not source_unit:
        raise RuntimeError(f"no approved serialized VFX donor for {character['id']}")
    wanted_index = TIERS.index(tier)
    candidates: list[tuple[int, int, str, str]] = []
    for prefab, owners in profile_owners.items():
        if prefab not in profiles:
            continue
        for owner_unit, source_tier in owners:
            if owner_unit == source_unit:
                candidates.append(
                    (
                        abs(TIERS.index(source_tier) - wanted_index),
                        -TIERS.index(source_tier),
                        prefab,
                        source_tier,
                    )
                )
    if not candidates:
        raise RuntimeError(f"approved VFX donor {source_unit} has no serialized profile")
    _distance, _higher_tier, prefab, source_tier = min(candidates)
    return prefab, source_unit, source_tier


def fallback_action_type(vfx_type: int, target: str) -> int:
    if vfx_type == 2:
        return 6
    if vfx_type == 0:
        return 5 if target == "All" else 1
    return 4 if target == "All" else 2


def noncombat_fallback_wire(target: str) -> dict[str, Any]:

    return {
        "ability_type": fallback_action_type(1, target),
        "vfx_type": 1,
        "ability": "fx_null_pre",
        "impact": None,
        "muzzle": None,
        "sustained": None,
        "projectile_speed": 0.0,
        "cone_delay": 0.0,
    }


def with_metadata(
    wire: dict[str, Any],
    *,
    provenance: str,
    requested: str,
    source_prefab: str,
    source_tier: str | None,
    family: str,
    source_unit: str | None = None,
    reason: str | None = None,
) -> dict[str, Any]:
    result = {field: wire.get(field) for field in WIRE_FIELDS}
    result.update(
        {
            "provenance": provenance,
            "requested_prefab": requested,
            "source_prefab": source_prefab,
            "source_tier": source_tier,
            "family": family,
        }
    )
    if source_unit:
        result["source_unit"] = source_unit
    if reason:
        result["reason"] = reason
    return result


def build_manifest(
    catalog_path: Path,
    reference_path: Path,
    cab_index_path: Path,
    unitypy: Any,
) -> dict[str, Any]:
    characters = json.loads(catalog_path.read_text())["characters"]
    characters_by_id = {character["id"]: character for character in characters}
    references = json.loads(reference_path.read_text())
    cab_index = json.loads(cab_index_path.read_text())

    requested = {
        prefab_key(requested_prefab(references[character["id"]], tier))
        for character in characters
        for tier in TIERS
    }
    requested.update(prefab_key(value) for value in MODEL_OVERRIDES.values())

    profiles: dict[str, dict[str, Any]] = {}
    unavailable: dict[str, str] = {}
    for index, prefab in enumerate(sorted(requested), 1):
        path = cab_index.get("cab-" + prefab)
        if not path or not Path(path).is_file():
            unavailable[prefab] = "cab_missing"
            continue
        try:
            profiles[prefab] = extract_prefab(prefab, path, unitypy)
        except Exception as exc:
            unavailable[prefab] = str(exc) or type(exc).__name__
        if index % 100 == 0:
            print(f"VFX extraction: {index}/{len(requested)}", file=sys.stderr)

    profile_owners: dict[str, list[tuple[str, str]]] = collections.defaultdict(list)
    for character in characters:
        reference = references[character["id"]]
        for tier in TIERS:
            profile_owners[prefab_key(requested_prefab(reference, tier))].append((character["id"], tier))
    profile_families: dict[str, str] = {}
    for prefab in profiles:
        owners = sorted(profile_owners.get(prefab, []))
        if owners:
            owner_unit, owner_tier = owners[0]
            profile_families[prefab] = character_family(characters_by_id[owner_unit], owner_tier)
    donors = choose_family_donors(profiles, profile_families)

    manifest: dict[str, Any] = {}
    counts: collections.Counter[str] = collections.Counter()
    stable_characters = 0
    attack_tiers = 0
    for character in characters:
        unit_id = character["id"]
        reference = references[unit_id]
        noncombat = "ATTACK" not in character["ability"]["mechanics"]
        if not noncombat:
            attack_tiers += len(TIERS)

        exact_by_tier: dict[str, tuple[str, dict[str, Any]]] = {}
        for tier in TIERS:
            prefab = prefab_key(requested_prefab(reference, tier))
            if prefab in profiles:
                exact_by_tier[tier] = (prefab, profiles[prefab])

        tier_bindings: dict[str, dict[str, Any]] = {}
        for tier in TIERS:
            wanted = prefab_key(requested_prefab(reference, tier))
            family = character_family(character, tier)
            if tier in exact_by_tier:
                source, wire = exact_by_tier[tier]
                binding = with_metadata(
                    wire,
                    provenance="serialized_exact",
                    requested=wanted,
                    source_prefab=source,
                    source_tier=tier,
                    family=family,
                )
            elif exact_by_tier:
                wanted_index = TIERS.index(tier)
                source_tier = min(
                    exact_by_tier,
                    key=lambda value: (abs(TIERS.index(value) - wanted_index), -TIERS.index(value)),
                )
                source, wire = exact_by_tier[source_tier]
                binding = with_metadata(
                    wire,
                    provenance="tier_fallback",
                    requested=wanted,
                    source_prefab=source,
                    source_tier=source_tier,
                    family=family,
                    reason=unavailable.get(wanted, "tier_prefab_unavailable"),
                )
            else:
                override = prefab_key(MODEL_OVERRIDES[unit_id]) if unit_id in MODEL_OVERRIDES else ""
                if override and override in profiles:
                    binding = with_metadata(
                        profiles[override],
                        provenance="model_override",
                        requested=wanted,
                        source_prefab=override,
                        source_tier=None,
                        family=family,
                        reason="reference prefab was renamed; using the exported model family",
                    )
                else:
                    hinted = catalog_hint_donor(
                        character,
                        tier,
                        family,
                        profiles,
                        profile_owners,
                        characters_by_id,
                    )
                    if hinted:
                        donor, source_unit, source_tier, matched = hinted
                        wire = dict(profiles[donor])
                        wire["ability_type"] = fallback_action_type(
                            wire["vfx_type"], character["ability"]["target"]
                        )
                        hint_names = ", ".join(matched)
                        reason = unavailable.get(wanted, "serialized prefab unavailable")
                        binding = with_metadata(
                            wire,
                            provenance="family_fallback",
                            requested=wanted,
                            source_prefab=donor,
                            source_tier=source_tier,
                            source_unit=source_unit,
                            family=family,
                            reason=f"{reason}; catalog VFX identity match: {hint_names}",
                        )
                    elif noncombat:
                        binding = with_metadata(
                            noncombat_fallback_wire(character["ability"]["target"]),
                            provenance="noncombat_fallback",
                            requested=wanted,
                            source_prefab=wanted,
                            source_tier=tier,
                            family=family,
                            reason=(
                                f"{unavailable.get(wanted, 'serialized prefab unavailable')}; "
                                "no identity-supported catalog VFX donor"
                            ),
                        )
                    else:
                        if family in donors:
                            donor = donors[family]
                            source_unit, source_tier = sorted(profile_owners[donor])[0]
                        else:
                            donor, source_unit, source_tier = approved_family_donor(
                                character,
                                tier,
                                profiles,
                                profile_owners,
                            )
                        wire = dict(profiles[donor])
                        wire["ability_type"] = fallback_action_type(
                            wire["vfx_type"], character["ability"]["target"]
                        )
                        binding = with_metadata(
                            wire,
                            provenance="family_fallback",
                            requested=wanted,
                            source_prefab=donor,
                            source_tier=source_tier,
                            source_unit=source_unit,
                            family=family,
                            reason=unavailable.get(wanted, "serialized family prefab unavailable"),
                        )
            counts[binding["provenance"]] += 1
            tier_bindings[tier] = binding

        signatures = {wire_signature(binding) for binding in tier_bindings.values()}
        tier_stable = len(signatures) == 1
        stable_characters += int(tier_stable)
        entry = {field: tier_bindings["IV"][field] for field in WIRE_FIELDS}
        entry.update(
            {
                "manifest_version": 2,
                "selected_tier": "IV",
                "provenance": tier_bindings["IV"]["provenance"],
                "noncombat": noncombat,
                "tier_stable": tier_stable,
                "tiers": tier_bindings,
            }
        )
        manifest[unit_id] = entry

    print(
        "VFX coverage: "
        f"{len(manifest)} characters, {attack_tiers} attack-capable tier bindings, "
        f"{stable_characters} tier-stable; "
        + ", ".join(f"{key}={value}" for key, value in sorted(counts.items())),
        file=sys.stderr,
    )
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--reference", type=Path, default=DEFAULT_REFERENCE)
    parser.add_argument("--cab-index", type=Path, default=DEFAULT_CAB_INDEX)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()

    unitypy = configure_unitypy()
    manifest = build_manifest(args.catalog, args.reference, args.cab_index, unitypy)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")
    print(f"wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
