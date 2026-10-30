#!/usr/bin/env python3

from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import json
from pathlib import Path
import re
from typing import Any, Iterable

import build_intrinsic_passives as common


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_RECIPES = ROOT / "data" / "ability_recipes_440.json"
DEFAULT_CATALOG = ROOT / "heroes-of-dragons-cache" / "characters.json"
DEFAULT_REFERENCE = (
    ROOT / ".claude" / "skills" / "hoda-reference" / "data" / "characters.json"
)
DEFAULT_OUTPUT = ROOT / "data" / "intrinsic_passives_440.json"

_INDEPENDENT_CLAUSE = re.compile(
    r",\s*(?=(?:deals?|takes?|gains?|gain|grants?|more|much|very|less|usually|"
    r"immune|cannot|absorbs?|allies|squad|stealth|when|upon|heals?|converts?|"
    r"enemy)\b)",
    re.I,
)
_NEGATIVE_STAT = re.compile(r"-\d+(?:\.\d+)?%\s+(?:health|power)\b", re.I)
_DYNAMIC_FAMILIES: tuple[tuple[str, re.Pattern[str]], ...] = (
    (
        "on_hit",
        re.compile(
            r"\b(?:when|if)\b[^.]{0,80}\b(?:hit|attacked)\b|"
            r"\bdamage received\b",
            re.I,
        ),
    ),
    (
        "on_death",
        re.compile(r"\b(?:death|dies|die|dying|killed)\b", re.I),
    ),
    (
        "bonus_damage",
        re.compile(
            r"\b(?:bonus damage|more damage|damage\s+vs\.?|damage\s+to\b)",
            re.I,
        ),
    ),
    (
        "conditional_damage_resistance",
        re.compile(r"\b(?:damage resist|less damage)\b[^.]*\b(?:vs\.?|from)\b", re.I),
    ),
    (
        "scoped_status_resistance",
        re.compile(
            r"\b(?:stun|slow|power\s*drain)\b[^.]*\b(?:resist|immun)|"
            r"\b(?:resist|immun)[^.]*\b(?:stun|slow|power\s*drain)\b",
            re.I,
        ),
    ),
    (
        "outgoing_target_preference",
        re.compile(
            r"\b(?:more|less|very|usually|much)\s+(?:likely\s+to\s+)?attack\b|"
            r"\bback row\b",
            re.I,
        ),
    ),
)


def _read_object(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise SystemExit(f"expected a JSON object: {path}")
    return value


def _clean(text: str) -> str:
    return re.sub(r"\s+", " ", text.replace("\\n", " ")).strip()


def _sentences(text: str) -> list[str]:
    return [
        value.strip(" ,")
        for value in re.split(r"[.!?]+", _clean(text))
        if value.strip(" ,")
    ]


def _independent_clauses(sentence: str) -> list[str]:
    result: list[str] = []
    for value in _INDEPENDENT_CLAUSE.split(sentence):
        parts = re.split(
            r"\s+(?=-\d+(?:\.\d+)?%\s+(?:health|power)\b)", value, flags=re.I
        )
        result.extend(part.strip(" ,") for part in parts if part.strip(" ,"))
    return result


def _dedupe_effects(effects: Iterable[dict[str, Any]]) -> list[dict[str, Any]]:
    result: list[dict[str, Any]] = []
    seen: set[str] = set()
    for effect in effects:
        signature = json.dumps(effect, sort_keys=True, ensure_ascii=False)
        if signature in seen:
            continue
        seen.add(signature)
        result.append(effect)
    return result


def _static_effects(
    text: str,
    names: dict[str, list[str]],
    families: dict[str, list[str]],
) -> tuple[list[dict[str, Any]], list[dict[str, str]]]:
    effects: list[dict[str, Any]] = []
    unresolved: list[dict[str, str]] = []
    for sentence in _sentences(text):
        for independent in _independent_clauses(sentence):
            inherited: dict[int, set[str]] | None = None
            for raw_clause in common._sentence_clauses(independent):
                clause = raw_clause.strip(" ,")
                if not clause:
                    continue
                if re.search(r"\bpower\s+vs\.?\b", clause, re.I):
                    unresolved.append(
                        {
                            "kind": "power_vs_target",
                            "source_text": raw_clause,
                            "reason": "combat Power-vs-target wording is not a base-stat aura",
                        }
                    )
                    continue
                if _NEGATIVE_STAT.search(clause):
                    parsed, reason, values = common._red_static_effects(
                        common._inherited_red_clause(clause, inherited),
                        names,
                        families,
                    )
                else:
                    parsed, reason, values = common._static_effects(
                        common._inherited_clause(clause, inherited),
                        names,
                        families,
                    )
                if values:
                    inherited = values
                if parsed:
                    effects.extend(parsed)
                elif reason:
                    unresolved.append(
                        {
                            "kind": "battle_start_stat",
                            "source_text": raw_clause,
                            "reason": reason,
                        }
                    )
    return _dedupe_effects(effects), unresolved


def _status_names(value: str) -> list[str]:
    value = value.casefold().replace("powerdrain", "power drain")
    result: list[str] = []
    for label, token in (
        ("stun", "stun"),
        ("slow", "slow"),
        ("power_drain", "power drain"),
    ):
        if token in value:
            result.append(label)
    return result


def _personal_resistance_effects(text: str) -> list[dict[str, Any]]:

    effects: list[dict[str, Any]] = []
    for sentence in _sentences(text):
        lower = sentence.casefold().replace("powerdrain", "power drain")
        statuses: list[str] = []
        value: float | None = None

        immune = re.fullmatch(r"(?:is\s+)?immune to (.+)", lower)
        cannot = re.fullmatch(r"cannot be (stunned|slowed)", lower)
        resist_and_immune = re.fullmatch(
            r"resists? (power drain|stun|slow)(?: and is immune to (power drain|stun|slow))?",
            lower,
        )
        bare = re.fullmatch(
            r"\+?(\d+(?:\.\d+)?)%\s+(.+?)\s+(?:resist|resistance)",
            lower,
        )
        negative_self = re.fullmatch(
            r"-(\d+(?:\.\d+)?)%\s+(.+?)\s+(?:resist|resistance)\s+to self",
            lower,
        )

        if immune:
            statuses = _status_names(immune.group(1))
            value = 100.0
        elif cannot:
            statuses = ["stun" if cannot.group(1) == "stunned" else "slow"]
            value = 100.0
        elif resist_and_immune and resist_and_immune.group(2):
            statuses = _status_names(resist_and_immune.group(2))
            value = 100.0
        elif bare and not re.search(
            r"\b(?:for|to|vs\.?|from|ally|allies|squad|enemy)\b", lower
        ):
            statuses = _status_names(bare.group(2))
            value = float(bare.group(1))
        elif negative_self:
            statuses = _status_names(negative_self.group(2))
            value = -float(negative_self.group(1))

        if value is None:
            continue
        for status in statuses:
            effects.append(
                {
                    "kind": "resistance_percent",
                    "trigger": "before_status",
                    "status": status,
                    "value_pct": int(value) if value.is_integer() else value,
                    "recipient": {"scope": "self"},
                    "stacking": "additive",
                    "confidence": "explicit",
                    "source_text": sentence,
                }
            )
    return _dedupe_effects(effects)


def _personal_damage_resistance_effects(text: str) -> list[dict[str, Any]]:
    effects: list[dict[str, Any]] = []
    for sentence in _sentences(text):
        lower = sentence.casefold()
        if re.search(r"\b(?:for|to|vs\.?|from|allies|squad|enemy)\b", lower):
            continue
        match = re.fullmatch(
            r"(?:personal\s+)?\+?(\d+(?:\.\d+)?)%\s+damage resist",
            lower,
        ) or re.fullmatch(r"takes?\s+(\d+(?:\.\d+)?)%\s+less damage", lower)
        if not match:
            continue
        value = float(match.group(1))
        effects.append(
            {
                "kind": "damage_resist_percent",
                "trigger": "before_damage",
                "value_pct": int(value) if value.is_integer() else value,
                "recipient": {"scope": "self"},
                "stacking": "additive",
                "confidence": "explicit",
                "source_text": sentence,
            }
        )
    return _dedupe_effects(effects)


def _defender_effect(key: str, text: str) -> dict[str, Any] | None:
    if not re.search(r"(?:defender|damage_?absorb)", key, re.I) and not re.search(
        r"\babsorb", text, re.I
    ):
        return None

    values: list[int] = []
    for pattern in (
        r"defender_?(\d+)",
        r"(?:ally|allies|aura_ally)_damage_?absorb_?(\d+)",
        r"absorbpower(\d+)",
    ):
        values.extend(int(value) for value in re.findall(pattern, key, re.I))
    values.extend(int(value) for value in re.findall(r"absorbs?\s+(\d+)%", text, re.I))
    if values:
        value = max(values)
        confidence = "key_or_text_encoded"
    elif re.search(r"\bmedium\b", text, re.I):
        value = 50
        confidence = "qualitative_convention"
    elif re.search(r"\bsmall\b", text, re.I):
        value = 25
        confidence = "qualitative_convention"
    else:
        return None
    return {
        "kind": "defender_percent",
        "trigger": "before_damage",
        "value_pct": value,
        "recipient": {"scope": "self"},
        "condition": "source_is_also_targeted_by_aoe",
        "confidence": confidence,
        "source_text": next(
            (
                sentence
                for sentence in _sentences(text)
                if re.search(r"\babsorb", sentence, re.I)
            ),
            text,
        ),
    }


def _stealth_effect(text: str) -> dict[str, Any] | None:
    sentence = next(
        (
            sentence
            for sentence in _sentences(text)
            if re.search(r"\bstealth\b", sentence, re.I)
        ),
        "",
    )
    if not re.search(
        r"\bstealth:\s*attacks without triggering on-hit effects\b", sentence, re.I
    ):
        return None
    return {
        "kind": "stealth",
        "trigger": "on_attack",
        "blocks": "on_hit_triggers",
        "recipient": {"scope": "self"},
        "confidence": "explicit",
        "source_text": sentence,
    }


def _dynamic_unresolved(
    text: str,
    typed_kinds: set[str],
) -> list[dict[str, str]]:
    unresolved: list[dict[str, str]] = []
    for sentence in _sentences(text):
        for kind, pattern in _DYNAMIC_FAMILIES:
            if not pattern.search(sentence):
                continue
            if (
                kind == "scoped_status_resistance"
                and "resistance_percent" in typed_kinds
            ):
                if not re.search(r"\b(?:for|to|allies|squad|enemy)\b", sentence, re.I):
                    continue
            if (
                kind == "conditional_damage_resistance"
                and "damage_resist_percent" in typed_kinds
            ):
                continue
            unresolved.append(
                {
                    "kind": kind,
                    "source_text": sentence,
                    "reason": "not emitted without a version-proven recipient, trigger amount, or target rule",
                }
            )
    return unresolved


def _target_weight_record(recipe: dict[str, Any]) -> dict[str, Any] | None:
    value = recipe.get("target_weight", {})
    if not isinstance(value, dict) or not value.get("key"):
        return None
    key = str(value["key"])
    if key not in common.TARGET_WEIGHTS:
        raise SystemExit(
            f"promote group {recipe.get('promote_group')}: unknown target weight {key!r}"
        )
    multiplier, strength = common.TARGET_WEIGHTS[key]
    return {
        "semantic_recipe_key": str(recipe["semantic_recipe_key"]),
        "promote_group": int(recipe["promote_group"]),
        "definition_ids": [int(value) for value in recipe.get("definition_ids", [])],
        "character_id": str(recipe.get("character_id", "")),
        "key": key,
        "text": str(value.get("text", "")),
        "value_multiplier": multiplier,
        "strength": strength,
        "confidence": "reconstructed_multiplier",
    }


def build(
    recipes_path: Path = DEFAULT_RECIPES,
    catalog_path: Path = DEFAULT_CATALOG,
    reference_path: Path = DEFAULT_REFERENCE,
) -> dict[str, Any]:
    recipes_root = _read_object(recipes_path)
    recipes = recipes_root.get("recipes", [])
    if not isinstance(recipes, list):
        raise SystemExit(f"invalid recipes list: {recipes_path}")
    catalog = _read_object(catalog_path).get("characters", [])
    reference = _read_object(reference_path)
    if not isinstance(catalog, list):
        raise SystemExit(f"invalid character catalog: {catalog_path}")
    names = common._name_index(catalog)
    families = common._family_index(catalog, reference)

    occurrences: dict[str, list[dict[str, Any]]] = defaultdict(list)
    texts: dict[str, tuple[str, str]] = {}
    promote_to_passive: dict[str, str] = {}
    definition_to_passive: dict[str, str] = {}
    target_weights: list[dict[str, Any]] = []

    for recipe in recipes:
        if not isinstance(recipe, dict):
            continue
        passive = recipe.get("passive", {})
        if not isinstance(passive, dict):
            passive = {}
        key = str(passive.get("key", ""))
        if key:
            pair = (str(passive.get("text", "")), str(passive.get("text_ru", "")))
            if key in texts and texts[key] != pair:
                raise SystemExit(
                    f"4.4 passive key {key!r} has multiple localized texts"
                )
            texts[key] = pair
            occurrence = {
                "semantic_recipe_key": str(recipe["semantic_recipe_key"]),
                "promote_group": int(recipe["promote_group"]),
                "definition_ids": [
                    int(value) for value in recipe.get("definition_ids", [])
                ],
                "character_id": str(recipe.get("character_id", "")),
            }
            occurrences[key].append(occurrence)
            promote_to_passive[str(recipe["promote_group"])] = key
            for definition_id in recipe.get("definition_ids", []):
                definition_to_passive[str(definition_id)] = key
        target = _target_weight_record(recipe)
        if target is not None:
            target_weights.append(target)

    passive_records: list[dict[str, Any]] = []
    for key in sorted(texts):
        text, text_ru = texts[key]
        effects, unresolved = _static_effects(text, names, families)
        effects.extend(_personal_resistance_effects(text))
        effects.extend(_personal_damage_resistance_effects(text))
        defender = _defender_effect(key, text)
        if defender is not None:
            effects.append(defender)
        stealth = _stealth_effect(text)
        if stealth is not None:
            effects.append(stealth)
        effects = _dedupe_effects(effects)
        typed_kinds = {str(effect.get("kind", "")) for effect in effects}
        unresolved.extend(_dynamic_unresolved(text, typed_kinds))
        if (
            re.search(r"(?:defender|damage_?absorb)", key, re.I)
            or re.search(r"\babsorb", text, re.I)
        ) and defender is None:
            unresolved.append(
                {
                    "kind": "defender_percent",
                    "source_text": next(
                        (
                            sentence
                            for sentence in _sentences(text)
                            if re.search(r"\babsorb", sentence, re.I)
                        ),
                        text,
                    ),
                    "reason": "defender magnitude is not explicit in the key or 4.4 text",
                }
            )
        passive_records.append(
            {
                "key": key,
                "text": text,
                "text_ru": text_ru,
                "occurrences": sorted(
                    occurrences[key], key=lambda value: value["promote_group"]
                ),
                "effects": effects,
                "unresolved": _dedupe_effects(unresolved),
            }
        )

    effect_counts = Counter(
        str(effect.get("kind", ""))
        for record in passive_records
        for effect in record["effects"]
    )
    unresolved_counts = Counter(
        str(value.get("kind", ""))
        for record in passive_records
        for value in record["unresolved"]
    )
    return {
        "version": 1,
        "client_version": "4.4.0",
        "source": str(recipes_path.relative_to(ROOT)),
        "catalog_source": str(catalog_path.relative_to(ROOT)),
        "reference_source": str(reference_path.relative_to(ROOT)),
        "conventions": {
            "percent_unit": "whole percentage points",
            "hero_aura_stacking": "base * (1 + sum(hero aura percentages) / 100)",
            "match_any": "apply once when at least one term matches",
            "match_each": "apply once for every matching term; only for explicit stacking wording",
            "target_weight_multipliers": {
                key: value[0] for key, value in common.TARGET_WEIGHTS.items()
            },
            "target_weight_note": "keys/text are exact 4.4; multipliers are reconstructed because the retired server values are unavailable",
            "qualitative_defender": {"small": 25, "medium": 50},
            "unresolved_policy": "never execute an unresolved clause at runtime",
        },
        "audit": {
            "recipe_entries_with_passive": sum(
                len(value) for value in occurrences.values()
            ),
            "unique_passive_keys": len(passive_records),
            "character_ids_with_passive": len(
                {
                    value["character_id"]
                    for values in occurrences.values()
                    for value in values
                }
            ),
            "passive_keys_with_typed_effects": sum(
                bool(record["effects"]) for record in passive_records
            ),
            "passive_keys_with_unresolved": sum(
                bool(record["unresolved"]) for record in passive_records
            ),
            "target_weight_recipe_count": len(target_weights),
            "effect_counts": dict(sorted(effect_counts.items())),
            "unresolved_counts": dict(sorted(unresolved_counts.items())),
        },
        "promote_group_to_passive_key": dict(
            sorted(promote_to_passive.items(), key=lambda value: int(value[0]))
        ),
        "definition_to_passive_key": dict(
            sorted(definition_to_passive.items(), key=lambda value: int(value[0]))
        ),
        "target_weights": sorted(
            target_weights, key=lambda value: value["promote_group"]
        ),
        "passives": passive_records,
    }


def _validate(result: dict[str, Any]) -> None:
    audit = result["audit"]
    if audit["unique_passive_keys"] != 325:
        raise SystemExit(
            f"expected 325 exact 4.4 passive keys, got {audit['unique_passive_keys']}"
        )
    if audit["recipe_entries_with_passive"] != 412:
        raise SystemExit(
            "expected 412 4.4 recipe/passive associations, got "
            f"{audit['recipe_entries_with_passive']}"
        )
    if audit["target_weight_recipe_count"] != 115:
        raise SystemExit(
            "expected 115 exact 4.4 target weights, got "
            f"{audit['target_weight_recipe_count']}"
        )
    passives = result["passives"]
    if len({record["key"] for record in passives}) != len(passives):
        raise SystemExit("duplicate passive key in generated catalog")
    promote_groups = [value["promote_group"] for value in result["target_weights"]]
    if len(promote_groups) != len(set(promote_groups)):
        raise SystemExit("duplicate promote-group target weight in generated catalog")

    target_by_group = {
        value["promote_group"]: value["key"] for value in result["target_weights"]
    }
    if 322 in target_by_group:
        raise SystemExit("4.4 Bear incorrectly acquired a target-weight override")
    if target_by_group.get(330) != "AURA_ELUSIVE":
        raise SystemExit("4.4 Isabela must retain AURA_ELUSIVE")
    if target_by_group.get(1838) != "AURA_TAUNT":
        raise SystemExit("4.4 Chevalier must retain AURA_TAUNT")

    chantry_key = (
        "aura_plus35health_to_chantry_and_plus15health_to_seekers_and_"
        "plus25power_to_regalyan_and_defender15_and_flank_and_minus25power_to_mages"
    )
    chantry = next(record for record in passives if record["key"] == chantry_key)
    stale_power = any(
        effect.get("kind") == "stat_percent"
        and effect.get("value_pct") == 35
        and "power" in effect.get("stats", [])
        for effect in chantry["effects"]
    )
    if stale_power:
        raise SystemExit("4.4 Chantry passive incorrectly acquired later +35% Power")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--recipes", type=Path, default=DEFAULT_RECIPES)
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--reference", type=Path, default=DEFAULT_REFERENCE)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument(
        "--check",
        action="store_true",
        help="verify that --output is current instead of rewriting it",
    )
    args = parser.parse_args()
    result = build(
        args.recipes.resolve(),
        args.catalog.resolve(),
        args.reference.resolve(),
    )
    _validate(result)
    rendered = json.dumps(result, indent=2, ensure_ascii=False) + "\n"
    output = args.output.resolve()
    if args.check:
        if not output.exists() or output.read_text(encoding="utf-8") != rendered:
            raise SystemExit(f"generated passive catalog is stale: {output}")
        print(f"validated {output}: {result['audit']}")
        return
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(rendered, encoding="utf-8")
    print(f"wrote {output}: {result['audit']}")


if __name__ == "__main__":
    main()
