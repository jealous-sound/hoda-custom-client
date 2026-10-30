#!/usr/bin/env python3

from __future__ import annotations

from collections import defaultdict
import argparse
import json
from pathlib import Path
import re
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CATALOG = ROOT / "heroes-of-dragons-cache" / "characters.json"
DEFAULT_OUTPUT = ROOT / "data" / "ability_recipes.json"

TARGETS = {"One": "one", "Row": "row", "Column": "column", "All": "all"}
QUALITATIVE_AMOUNT = {
    "tiny": 5,
    "very small": 10,
    "small": 25,
    "moderate": 50,
    "medium": 50,
    "large": 75,
    "huge": 75,
    "massive": 100,
}
QUALITATIVE_CHANCE = {
    "tiny": 5,
    "very small": 10,
    "small": 25,
    "moderate": 50,
    "medium": 50,
    "high": 75,
    "very high": 85,
}


def _clean(text: str) -> str:
    return re.sub(r"<[^>]+>", "", text).replace("’", "'").strip()


def _pct(patterns: list[str], text: str, key: str, default: int) -> int:
    for pattern in patterns:
        match = re.search(pattern, text, re.I)
        if match:
            return int(match.group(1))
    for pattern in patterns:
        match = re.search(pattern, key, re.I)
        if match:
            return int(match.group(1))
    return default


def _qualitative(text: str, *, chance: bool, default: int) -> int:
    table = QUALITATIVE_CHANCE if chance else QUALITATIVE_AMOUNT
    lower = text.casefold()
    for phrase in sorted(table, key=len, reverse=True):
        if phrase in lower:
            return table[phrase]
    return default


def _chance_for(effect: str, text: str, key: str, default: int = 100) -> int:
    escaped = re.escape(effect)
    patterns = [
        rf"(\d+)%\s+chance[^.]*?\b{escaped}",
        rf"with\s+(?:a\s+)?(\d+)%\s+chance[^.]*?\b{escaped}",
        rf"chance(?:\s+each)?\s+to\s+{escaped}\w*[^.]*?(\d+)%",
        rf"chance(\d+)[a-z_]*{escaped}",
        rf"{escaped}(?:power|health|p|h)?_?chance_?(\d+)",
    ]
    explicit = _pct(patterns, text, key, -1)
    if explicit >= 0:
        return explicit
    window = next((s for s in re.split(r"[.!?]", text) if re.search(rf"\b{escaped}", s, re.I)), text)
    specific_chance = re.search(
        rf"(?:tiny|very small|small|moderate|medium|high|very high)?\s*chance[^.]{{0,35}}\b(?:to\s+)?{escaped}",
        window,
        re.I,
    )
    if specific_chance:
        suffix = r"(?:power|health|p|h)?" if effect == "curse" else ""
        key_match = re.search(rf"{escaped}{suffix}_?(\d+)(?=_|$)", key, re.I)
        if key_match:
            return int(key_match.group(1))
        return _qualitative(window, chance=True, default=25)
    return default


def _amount_for(effect: str, text: str, key: str, default: int) -> int:
    escaped = re.escape(effect)
    patterns = [
        rf"(\d+)%\s+{escaped}",
        rf"\b{escaped}\w*\s+(\d+)%",
        rf"(\d+)%[^.]*?\b{escaped}",
        rf"{escaped}(?:power)?_?(\d+)",
        rf"(\d+)(?:pct)?{escaped}",
    ]
    explicit = _pct(patterns, text, key, -1)
    if explicit >= 0:
        return explicit
    window = next((s for s in re.split(r"[.!?]", text) if re.search(rf"\b{escaped}", s, re.I)), text)
    return _qualitative(window, chance=False, default=default)


def _proc_chance(effect: str, text: str, key: str) -> int:
    escaped = re.escape(effect)
    return _pct(
        [
            rf"(\d+)%\s+chance[^.]*?\b{escaped}",
            rf"chance(\d+)[a-z_]*{escaped}",
            rf"(\d+)chance[a-z_]*{escaped}",
        ],
        text,
        key,
        100,
    )


def _step(
    target: str,
    *,
    damage: int = 100,
    final_damage: int = 0,
    chance: int = 100,
    repeat: int = 1,
    chance_group: int = 0,
    stop_on_fail: bool = False,
    effects_once: bool = False,
    status_only: bool = False,
    hit: dict[str, int] | None = None,
    effects: dict[str, int] | None = None,
) -> dict[str, Any]:
    attack: dict[str, Any] = {
        "target": target,
        "damage_pct": 0 if status_only else damage,
    }
    if status_only:
        attack["status_only"] = True
    attack.update(hit or {})
    result: dict[str, Any] = {"chance_pct": chance, "repeat": repeat, "attack": attack}
    if chance_group:
        result["chance_group"] = chance_group
    if final_damage:
        result["final_damage_pct"] = final_damage
    if stop_on_fail:
        result["stop_on_fail"] = True
    if effects_once:
        result["effects_once"] = True
    if effects:
        result["effects"] = effects
    return result


def _target_after(fragment: str, fallback: str) -> str:
    lower = fragment.casefold()
    if re.search(r"\b(?:all enemies|all targets|attack all|attacks all)\b", lower):
        return "all"
    if "column" in lower:
        return "column"
    if "row" in lower:
        return "row"
    if re.search(r"\b(?:single|one|random|a)\s+(?:enemy|target|hero)\b", lower):
        return "one"
    return fallback


def _damage_after(fragment: str, default: int = 100) -> int:
    match = re.search(r"(?:at|for|dealing)\s+(\d+)%\s+(?:power|damage)", fragment, re.I)
    if match:
        return int(match.group(1))
    if re.search(r"\b(?:triple|3x)\s+(?:power|damage)(?!\s+chance)", fragment, re.I):
        return 300
    if re.search(r"\b(?:double|2x)\s+(?:power|damage)(?!\s+chance)", fragment, re.I):
        return 200
    if re.search(r"\bhalf\s+(?:power|damage)", fragment, re.I):
        return 50
    return default


def _later_attack_steps(text: str, initial_target: str, uncertain: list[str]) -> list[dict[str, Any]]:
    lower = text.casefold()
    starts = [m.start() for m in re.finditer(r"\battack(?:s|ed|ing)?\b", lower)]
    result: list[dict[str, Any]] = []
    for index, start in enumerate(starts[1:], 1):
        before = lower[max(0, start - 70) : start]
        end = starts[index + 1] if index + 1 < len(starts) else min(len(text), start + 135)
        after = text[start:end]
        if re.search(r"(?:each|every)\s+$", before) or re.match(
            r"attack(?:s|ed|ing)?\s+(?:self|(?:all\s+)?allies)\b", after, re.I
        ):
            continue
        if "mark" in before and "chance" not in before and "then" not in before:
            continue
        if re.search(r"(?:2nd|3rd|4th|second|third|fourth)\s+$", before) and not re.search(
            r"chance\s+(?:at|for|to)\s+(?:the\s+)?(?:2nd|3rd|4th|second|third|fourth)", before
        ):
            continue
        chance_match = re.search(r"(\d+)%\s+chance(?:\s+each)?\s+(?:at|for|to)?[^.]{0,35}$", before)
        if not chance_match:
            chance_match = re.search(r"with\s+(?:a\s+)?(\d+)%\s+chance[^.]{0,35}$", before)
        chance = int(chance_match.group(1)) if chance_match else 100
        if "chance" in before and chance == 100:
            chance = _qualitative(before, chance=True, default=25)
            uncertain.append("follow-up chance is qualitative or omitted")
        target = _target_after(after, initial_target)
        damage = _damage_after(after)
        repeat = 1
        repeat_match = re.search(r"up to\s+([2-9]|10)\s+(?:more|extra)\s+$", before)
        if repeat_match:
            repeat = int(repeat_match.group(1))
        elif re.search(r"(?:2nd|second)\s+and\s+(?:3rd|third)\s+$", before):
            repeat = 2
        result.append(_step(target, damage=damage, chance=chance, repeat=repeat, stop_on_fail=repeat > 1))
    return result


def _primary_repeat(text: str, target: str, uncertain: list[str]) -> tuple[int, list[dict[str, Any]]]:
    lower = text.casefold()
    up_to = re.search(r"\battacks?[^.]{0,75}?\bup to\s+([2-9]|10)\s+times\b", lower)
    if not up_to:
        up_to = re.search(r"\battacks?\s+up to\s+([2-9]|10)(?:\s+(?:random\s+)?(?:targets|enemies))?\b", lower)
    if up_to:
        total = int(up_to.group(1))
        uncertain.append("per-hit chance for 'up to' attacks is not present in the client data")
        return 1, [_step(target, chance=50, repeat=total - 1, stop_on_fail=True)]

    exact = re.search(r"\battacks?[^.]{0,75}?\b([2-9]|10)\s*(?:x(?!\s+damage)|times)\b", lower)
    if not exact and "twice" in lower.split(".", 1)[0]:
        return 2, []
    if not exact:
        words = {"two": 2, "three": 3, "four": 4, "five": 5, "six": 6, "eight": 8, "ten": 10}
        match = re.search(r"\battacks?[^.]{0,75}?\b(" + "|".join(words) + r")\s+times\b", lower)
        if match:
            return words[match.group(1)], []
    if exact:
        return int(exact.group(1)), []
    random_targets = re.search(r"\battacks?\s+(?:the\s+)?(two|three|[2-9])\s+random\s+(?:targets|enemies)\b", lower)
    if random_targets:
        words = {"two": 2, "three": 3}
        return words.get(random_targets.group(1), int(random_targets.group(1)) if random_targets.group(1).isdigit() else 1), []
    return 1, []


def _effect_values(text: str, key: str, mechanics: set[str], uncertain: list[str]) -> dict[str, int]:
    lower, kl = text.casefold(), key.casefold()
    effects: dict[str, int] = {}

    if "HEAL_ALLIES" in mechanics:
        value = _pct(
            [
                r"(?:heals?|health)[^.]{0,35}?(\d+)%[^.]{0,25}allies",
                r"heals?\s+(?:all\s+)?allies[^.]{0,20}?(\d+)%",
                r"allies[^.]{0,35}?(?:gain|grants?)[^.]{0,20}?(\d+)%\s+health",
                r"grants?\s+allies\s+(\d+)%\s+health",
                r"grants?\s+\+?(\d+)%\s+health[^.]{0,35}allies",
                r"heals?\s+and\s+power\s+gains?\s+allies\s+(\d+)%",
                r"heal(?:allies)?_?(\d+)",
                r"(\d+)heal(?:all|allies)",
                r"health(\d+)[^_]*(?:all|allies)",
            ],
            text,
            key,
            -1,
        )
        if value < 0:
            value = _qualitative(text, chance=False, default=25)
        effects["ally_heal_pct"] = value

    if "GAIN_POWER_ALLIES" in mechanics:
        value = _pct(
            [
                r"allies[^.]{0,45}?(?:gain|grants?)[^.]{0,20}?(\d+)%\s+power",
                r"allies\s+gain[^.]{0,20}?(\d+)%\s+power",
                r"grants?\s+(?:all\s+)?allies\s+(\d+)%\s+power",
                r"(?:gain|increase)[^.]{0,25}?(\d+)%\s+power[^.]{0,30}allies",
                r"grants?\s+\+?(\d+)%[^.]{0,25}power[^.]{0,35}allies",
                r"heals?\s+and\s+power\s+gains?\s+allies\s+(\d+)%",
                r"(?:powergain|gainpower|gain)_?(\d+)_?(?:all|allies)",
                r"(\d+)(?:pwr|power)(?:all|allies)",
                r"power(\d+)[^_]*(?:all|allies)",
            ],
            text,
            key,
            -1,
        )
        if value < 0:
            value = _qualitative(text, chance=False, default=25)
        effects["ally_power_pct"] = value

    if "HEAL_SELF" in mechanics:
        value = _pct(
            [
                r"(?:self[- ]?heal|heal(?:s|ed)?\s+(?:self|himself|herself|itself))[^.]{0,25}?(\d+)%",
                r"(?:gains?|heals?)[^.]{0,20}?(\d+)%\s+health",
                r"(\d+)%\s+(?:self\s+)?heal",
                r"selfheal_?(\d+)",
                r"heal(\d+)self",
                r"gainhealth(\d+)",
            ],
            text,
            key,
            -1,
        )
        if value < 0:
            value = _qualitative(text, chance=False, default=25)
        effects["self_heal_pct"] = value
        effects["self_heal_chance"] = _proc_chance("heal", text, key)

    if "GAIN_POWER_SELF" in mechanics:
        value = _pct(
            [
                r"gains?\s+(?:a\s+)?(?:small|medium|large|massive)?\s*(\d+)%\s+power",
                r"gains?\s+\+?(\d+)%\s+power",
                r"(?:self_?)?gainpower_?(\d+)",
                r"gain(\d+)self",
                r"powergain(\d+)(?!_?(?:all|allies))",
            ],
            text,
            key,
            -1,
        )
        if value < 0:
            value = _qualitative(text, chance=False, default=25)
        effects["self_power_pct"] = value
        effects["self_power_chance"] = _proc_chance("gain", text, key)

    if "GAIN_CRIT_ALLIES" in mechanics:
        value = _pct(
            [
                r"allies[^.]{0,55}?\+?(\d+)%\s+(?:2x damage|crit)",
                r"(?:grants?|increases?)[^.]{0,45}?allies[^.]{0,30}?(\d+)%\s+(?:2x damage|crit)",
                r"(?:2x damage|crit)\s+chance[^.]{0,30}?allies\s+\+?(\d+)%",
                r"allies\s+2x damage chance\s+by\s+(\d+)%",
                r"(?:critgain|crit|critchance)(\d+)[^a-z0-9]*(?:all|allies)",
                r"(\d+)crit(?:gain)?(?:all|allies)",
                r"(\d+)critchance(?:all|allies)",
            ],
            text,
            key,
            5,
        )
        effects["ally_crit_pct"] = value

    if "GAIN_DAMAGE_ALLIES" in mechanics:
        effects["ally_damage_bonus_pct"] = _pct(
            [
                r"allies[^.]{0,55}?\+?(\d+)%\s+bonus damage",
                r"(\d+)(?:bns|bonus)?dmg(?:all|allies)",
            ],
            text,
            key,
            5,
        )
    if "GAIN_DAMAGE_SELF" in mechanics:
        effects["self_damage_bonus_pct"] = _pct(
            [r"(?:gains?|gain)\s+\+?(\d+)%\s+bonus damage", r"gain(\d+)(?:bns|bonus)dmg"], text, key, 5
        )

    if "DAMAGE_SELF" in mechanics:
        value = _pct(
            [r"(?:self|himself|herself|itself)\s+for\s+(\d+)%", r"self(?:damage|dmg)?_?(\d+)", r"attackself_?(\d+)"],
            text,
            key,
            -1,
        )
        if value < 0:
            value = 5
            uncertain.append("self-damage amount is not stated")
        effects["self_damage_pct"] = -value
    if "LOSE_POWER_SELF" in mechanics:
        value = _pct([r"self\s+power[^.]{0,15}?(\d+)%", r"drain(?:power)?(\d+)self", r"drain(\d+)self"], text, key, 10)
        effects["self_power_pct"] = -value
    if "SLOW_ALLIES" in mechanics:
        effects["ally_slow_amt"] = -50
    if "ATTACK_FRIENDLY" in mechanics:
        value = _pct(
            [r"allies\s+(?:for|at)\s+(\d+)%", r"allies?[^.]{0,20}?(\d+)%\s+(?:damage|power)", r"ally_?(\d+)"],
            text,
            key,
            5,
        )
        effects["ally_damage_pct"] = -value

    if "GAIN_CRIT_ALLIES" not in mechanics and re.search(
        r"\b(?:gains?|increas(?:e|es|ing))[^.]{0,25}(?:2x damage|crit) chance", lower
    ):
        effects["self_crit_pct"] = _pct(
            [
                r"(?:gains?|increas(?:e|es|ing))[^.]{0,25}?(\d+)%\s+(?:2x damage|crit)",
                r"(?:2x damage|crit)\s+chance\s+by\s+(\d+)%",
                r"plus(\d+)crit",
            ],
            text,
            key,
            5,
        )
    return effects


def _generic_recipe(row: dict[str, Any]) -> dict[str, Any]:
    ability = row["ability"]
    key, text = ability["key"], _clean(ability["text"])
    target = TARGETS[ability["target"]]
    mechanics = set(ability["mechanics"])
    lower, kl = text.casefold(), key.casefold()
    uncertain: list[str] = []
    unsupported: list[str] = []

    if key in {"WISP_ATTACK_ABILITY_KEY", "SUPER_WISP_ATTACK_ABILITY_KEY"}:
        return {
            "key": key,
            "text": text,
            "target": target,
            "mechanics": ability["mechanics"],
            "steps": [],
            "noncombat": True,
            "unsupported": ["consumable boost has no battle action"],
        }

    hit: dict[str, int] = {}
    if "POWER_DRAIN" in mechanics:
        hit["drain_pct"] = _amount_for("drain", text, key, 25 if target == "all" else 50)
        hit["drain_chance"] = _pct(
            [
                r"(\d+)%\s+chance[^.]*?\bdrain",
                r"chance(\d+)[a-z_]*drain",
                r"(\d+)chance[a-z_]*drain",
            ],
            text,
            key,
            0,
        )
    if "STUN" in mechanics:
        hit["stun_pct"] = _chance_for("stun", text, key)
    if "SLOW" in mechanics:
        hit["slow_pct"] = _chance_for("slow", text, key)
        hit["slow_amt"] = -100 if re.search(r"\b(?:heavy|heavily|greatly|a lot)\b", lower) or "init100" in kl else -50
    if "CURSE_POWER" in mechanics:
        hit["curse_power_pct"] = _chance_for("curse", text, key)
    if "CURSE_HEALTH" in mechanics:
        hit["curse_health_pct"] = _chance_for("curse", text, key)
    if "CURSE_TWO_ROUNDS" in mechanics:
        hit["curse_turns"] = 2
    if "LEVY" in mechanics:
        hit["levy_pct"] = _amount_for("levy", text, key, 15)
    if "MARK" in mechanics:
        hit["mark_pct"] = _pct(
            [r"mark[^.]{0,75}?\+?(\d+)%\s+(?:bonus\s+)?damage", r"mark_?(\d+)", r"mark(\d+)"], text, key, 20
        )
    if "ASSASSINATE" in mechanics:
        hit["assassinate_pct"] = _chance_for("assassinate", text, key, 5)

    damage = 100
    first_sentence = text.split(".", 1)[0]
    attack_words = list(re.finditer(r"\battack(?:s|ed|ing)?\b", first_sentence, re.I))
    primary_fragment = first_sentence[: attack_words[1].start()] if len(attack_words) > 1 else first_sentence
    damage = _damage_after(primary_fragment, damage)
    if re.search(r"\bheavily damages?\b", lower):
        damage = 150
        uncertain.append("'heavily damages' has no numeric server value")
    if re.search(r"\bmassively damages?\b", lower):
        damage = 200
        uncertain.append("'massively damages' has no numeric server value")

    effects = _effect_values(text, key, mechanics, uncertain)
    repeat, repeated = _primary_repeat(text, target, uncertain)
    no_attack = "ATTACK_NONE" in mechanics
    per_hit_effects = bool(re.search(r"\bwith each (?:hit|attack)\b|\beach (?:hit|attack)\b", lower))
    steps = [
        _step(
            target,
            damage=damage,
            repeat=repeat,
            status_only=no_attack,
            hit=hit,
            effects=effects,
            effects_once=repeat > 1 and not per_hit_effects,
        )
    ]
    steps.extend(repeated)
    steps.extend(_later_attack_steps(text, target, uncertain))

    if "MULTIATTACK" in mechanics and len(steps) == 1 and repeat == 1:
        chance = _pct(
            [
                r"(\d+)%\s+chance[^.]{0,45}(?:attack again|second attack|2nd|3rd)",
                r"followup_?(\d+)",
                r"followup(\d+)",
            ],
            text,
            key,
            -1,
        )
        if chance < 0:
            chance = _qualitative(text, chance=True, default=25)
            uncertain.append("follow-up chance is not numeric in the client data")
        extra_count = 2 if re.search(r"(?:2nd|second)\s+and\s+(?:3rd|third)\s+attack", lower) else 1
        followup_fragment = next(
            (sentence for sentence in re.split(r"[.!?]", text) if re.search(r"(?:2nd|3rd|second attack|attack again)", sentence, re.I)),
            text,
        )
        steps.append(
            _step(
                target,
                damage=_damage_after(followup_fragment),
                chance=chance,
                repeat=extra_count,
                stop_on_fail=extra_count > 1,
            )
        )

    status_specs = (
        ("drain", "drain_pct", "drain_chance"),
        ("stun", "stun_pct", None),
        ("slow", "slow_pct", "slow_amt"),
    )
    for word, value_field, aux_field in status_specs:
        match = re.search(
            rf"\b{word}\w*[^.]*?\b(?:from\s+|of\s+)?(all enemies|a row|the row|a column|the column)\b",
            text,
            re.I,
        )
        if not match:
            continue
        other = _target_after(match.group(1), target)
        if other == target:
            continue
        moved: dict[str, int] = {}
        if value_field in steps[0]["attack"]:
            moved[value_field] = steps[0]["attack"].pop(value_field)
        if aux_field and aux_field in steps[0]["attack"]:
            moved[aux_field] = steps[0]["attack"].pop(aux_field)
        if moved:
            steps.insert(1, _step(other, status_only=True, hit=moved))

    if "damage resist" in lower:
        unsupported.append("ally damage resistance is not represented by the current wire deltas")
    if "stealth" in lower:
        unsupported.append("stealth is a passive rune/specialty, not an active spell delta")
    if re.search(r"drains?\s+\d+%\s+2x damage chance", lower):
        unsupported.append("enemy critical-chance drain is absent from the recovered wire protocol")
    if "glances longingly" in lower:
        unsupported.append("flavor text has no combat effect")

    result: dict[str, Any] = {
        "key": key,
        "text": text,
        "target": target,
        "mechanics": ability["mechanics"],
        "steps": steps,
    }
    if uncertain:
        result["ambiguous"] = sorted(set(uncertain))
    if unsupported:
        result["unsupported"] = unsupported
    return result


def _override_steps(recipe: dict[str, Any]) -> None:
    key = recipe["key"]
    overrides: dict[str, list[dict[str, Any]]] = {
        "damage_all_and_drainpower_and_10pctchance_attackagain": [
            _step("all", hit={"drain_pct": 50}),
            _step("all", chance=10, hit={"drain_pct": 50}),
        ],
        "attack_one_levy25_drain75_followup35_column_stun": [
            _step("one", hit={"levy_pct": 25, "drain_pct": 75}),
            _step("column", chance=35, hit={"stun_pct": 100}),
        ],
        "attack_row_stun75_followup30_single200_drain40": [
            _step("row", hit={"stun_pct": 75}),
            _step("one", damage=200, chance=30, hit={"drain_pct": 40}),
        ],
        "damage_one_row_column_all_plus50crit": [
            _step("one"),
            _step("row"),
            _step("column"),
            _step("all", effects={"self_crit_pct": 50}),
        ],
        "attack1_damage_all_health25_andpower25_to_allies_attack2_damage_plus100": [
            _step("all", effects={"ally_heal_pct": 25, "ally_power_pct": 25}),
            _step("one", damage=200),
        ],
        "attack1_damage_column_chance50_stun_attack2_all": [
            _step("column", hit={"stun_pct": 50}),
            _step("all"),
        ],
        "attack_all75_attack_single300_mark30": [
            _step("all", damage=75),
            _step("one", damage=300, hit={"mark_pct": 30}),
        ],
        "attack_one50_drain40all_followup_row250_cursepower": [
            _step("one", damage=50),
            _step("all", status_only=True, hit={"drain_pct": 40}),
            _step("row", damage=250, hit={"curse_power_pct": 100}),
        ],
        "attack_one_columncursehealth_rowcursepower_chance20_attackallcursehealthpower": [
            _step("one"),
            _step("column", status_only=True, hit={"curse_health_pct": 100}),
            _step("row", status_only=True, hit={"curse_power_pct": 100}),
            _step("all", chance=20, hit={"curse_health_pct": 100, "curse_power_pct": 100}),
        ],
        "attack_single_explode50enemy10ally_slowall": [
            _step("one"),
            _step("all", damage=50, hit={"slow_pct": 100, "slow_amt": -50}, effects={"ally_damage_pct": -10}),
        ],
        "damage_single_init_100_column": [
            _step("one"),
            _step("column", status_only=True, hit={"slow_pct": 100, "slow_amt": -100}),
        ],
        "attack_single_stun_gain25_2x_followup40_250x": [
            _step("one", repeat=2, hit={"stun_pct": 100}, effects={"self_power_pct": 25}),
            _step("one", damage=250, chance=40, hit={"stun_pct": 100}, effects={"self_power_pct": 25}),
        ],
        "attackrow_levy20_stuncolumn_chance65_attackally_2x_2dmg": [
            _step("row", hit={"levy_pct": 20}),
            _step("column", status_only=True, hit={"stun_pct": 100}),
            _step("one", chance=65, repeat=2, status_only=True, effects={"ally_damage_pct": -2}),
        ],
        "attack1_damage_single_self_gainpower25_attack2_chance_damage_single_self_gainpower25_attack3_chance_damage_single_self_gainpower25": [
            _step("one", effects={"self_power_pct": 25}),
            _step("one", chance=33, repeat=2, stop_on_fail=True, effects={"self_power_pct": 25}),
        ],
        "attack1_drainpower60_4critgainallies": [
            _step("one", effects={"ally_crit_pct": 4}),
            _step("one", status_only=True, hit={"drain_pct": 60}),
        ],
        "attack_all_chance30singlelevy50_heal25gain25all": [
            _step("all", effects={"ally_heal_pct": 25, "ally_power_pct": 25}),
            _step("one", chance=30, hit={"levy_pct": 50}),
        ],
        "attack_all_curse_followup50_single_slow_all_chance25_heal50": [
            _step("all", hit={"curse_power_pct": 100, "curse_health_pct": 100}),
            _step("one", chance=50, chance_group=1),
            _step("all", chance=50, chance_group=1, status_only=True, hit={"slow_pct": 100, "slow_amt": -50}),
            _step("one", chance=25, status_only=True, effects={"self_heal_pct": 50}),
        ],
        "attack_all_curse_power_followup40_stun_healall25": [
            _step("all", hit={"curse_power_pct": 100}, effects={"ally_heal_pct": 25}),
            _step("all", chance=40, hit={"stun_pct": 100}),
        ],
        "attack_all_cursep_drain50_selfheal100_followup25_row_stun": [
            _step(
                "all",
                hit={"curse_power_pct": 100, "drain_pct": 50},
                effects={"self_heal_pct": 100},
            ),
            _step("row", chance=25, hit={"stun_pct": 100}),
        ],
        "attack_all_slow_stun75_chance25_attack_column_curse": [
            _step("all", hit={"slow_pct": 100, "slow_amt": -50, "stun_pct": 75}),
            _step("column", chance=25, hit={"curse_power_pct": 100, "curse_health_pct": 100}),
        ],
        "attack_all_stun25_followup30levy20_mark15": [
            _step("all", hit={"stun_pct": 25, "mark_pct": 15}),
            _step("all", chance=30, hit={"levy_pct": 20}),
        ],
        "attack_all_stun75_followup25_single200_followup25_any10": [
            _step("all", hit={"stun_pct": 75}),
            _step("one", damage=200, chance=25),
            _step("any", damage=10, chance=25),
        ],
        "attack_column50_cursepower_heavyslowrow_chance75_attackall140": [
            _step("column", damage=50, hit={"curse_power_pct": 100}),
            _step("row", status_only=True, hit={"slow_pct": 100, "slow_amt": -100}),
            _step("all", damage=140, chance=75),
        ],
        "attack_column_curse_2rounds_plus5critall_plus5dmgresall_chance50drain30all": [
            _step(
                "column",
                hit={"curse_power_pct": 100, "curse_health_pct": 100, "curse_turns": 2},
                effects={"ally_crit_pct": 5},
            ),
            _step("all", chance=50, status_only=True, hit={"drain_pct": 30}),
        ],
        "attack_column_levy40_slow_followup_row_stun50": [
            _step("column", hit={"levy_pct": 40, "slow_pct": 100, "slow_amt": -50}),
            _step("row", chance=50, hit={"stun_pct": 100}),
        ],
        "attack_column_slow50all_followup50row_drain25": [
            _step("column"),
            _step("all", status_only=True, hit={"slow_pct": 50, "slow_amt": -50}),
            _step("row", chance=50, hit={"drain_pct": 25}),
        ],
        "attack_column_stun_followup_drain40_allypowergain25": [
            _step("column", hit={"stun_pct": 100}, effects={"ally_power_pct": 25}),
            _step("column", hit={"drain_pct": 40}),
        ],
        "attack_one_4x_final200_chance25_stunall": [
            _step("one"),
            _step("one", chance=50, repeat=3, stop_on_fail=True, final_damage=200),
            _step("all", status_only=True, hit={"stun_pct": 25}),
        ],
        "attack_one_75_drain50_gain50_followup75_attack_all_levy15": [
            _step("one", damage=75, hit={"drain_pct": 50}, effects={"self_power_pct": 50}),
            _step("all", chance=75, hit={"levy_pct": 15}),
        ],
        "attack_one_attack_all30_heavyslow25all": [
            _step("one"),
            _step("all", damage=30, hit={"slow_pct": 25, "slow_amt": -100}),
        ],
        "attack_one_chance25_followup300_selfdamage_chance30heal50": [
            _step("one"),
            _step("one", damage=300, chance=25, effects={"self_damage_pct": -5}),
            _step("one", chance=30, status_only=True, effects={"self_heal_pct": 50}),
        ],
        "attack_one_cursedrain30all_stun35all_gain45heal45all": [
            _step("one", effects={"ally_heal_pct": 45, "ally_power_pct": 45}),
            _step(
                "all",
                status_only=True,
                hit={
                    "curse_power_pct": 100,
                    "curse_health_pct": 100,
                    "drain_pct": 30,
                    "stun_pct": 35,
                },
            ),
        ],
        "attack_one_drain40all_gain20all_mark25": [
            _step("one", hit={"mark_pct": 25}, effects={"ally_power_pct": 20}),
            _step("all", status_only=True, hit={"drain_pct": 40}),
        ],
        "attack_one_gain15all_7x_slowall": [
            _step("one", effects={"ally_power_pct": 15}),
            _step("one", chance=50, repeat=6, stop_on_fail=True, effects={"ally_power_pct": 15}),
            _step("all", status_only=True, hit={"slow_pct": 100, "slow_amt": -50}),
        ],
        "attack_one_levy20_followup35_drain50": [
            _step("one", hit={"levy_pct": 20}),
            _step("one", chance=35, hit={"drain_pct": 50}),
        ],
        "attack_one_levy25_cursehealth_followup50_cursepower": [
            _step("one", hit={"levy_pct": 25, "curse_health_pct": 100}),
            _step("one", chance=50, hit={"curse_power_pct": 100}),
        ],
        "attack_one_levy25_drain50all_chance50curserow_gain50self": [
            _step("one", hit={"levy_pct": 25}, effects={"self_power_pct": 50}),
            _step("all", status_only=True, hit={"drain_pct": 50}),
            _step("row", chance=50, status_only=True, hit={"curse_power_pct": 100, "curse_health_pct": 100}),
        ],
        "attack_one_levy35_cursehealth_all_heal25gain25allies": [
            _step("one", hit={"levy_pct": 35}, effects={"ally_heal_pct": 25, "ally_power_pct": 25}),
            _step("all", status_only=True, hit={"curse_health_pct": 100}),
        ],
        "attack_one_stun_followup50_drain50_gain35all_heal35all": [
            _step("one", hit={"stun_pct": 100}, effects={"ally_heal_pct": 35, "ally_power_pct": 35}),
            _step("one", chance=50, hit={"drain_pct": 50}),
        ],
        "attack_one_stun_followup_attack_all_drain40_chance30_drain40all_x2assassinate10": [
            _step("one", hit={"stun_pct": 100}),
            _step("all", hit={"drain_pct": 40}),
            _step("all", chance=30, status_only=True, hit={"drain_pct": 40}),
            _step("one", repeat=2, status_only=True, hit={"assassinate_pct": 10}),
        ],
        "attack_row75_followup_all_levy15_cursepower_heal30": [
            _step("row", damage=75, effects={"ally_heal_pct": 30}),
            _step("all", hit={"levy_pct": 15, "curse_power_pct": 100}),
        ],
        "attack_row_attack_all_slow_crit2_stealth": [
            _step("row", effects={"self_crit_pct": 2}),
            _step("all", hit={"slow_pct": 100, "slow_amt": -50}, effects={"self_crit_pct": 2}),
        ],
        "attack_row_curse_gain50_attackcolumn_levy25_attackself_3dmg": [
            _step(
                "row",
                hit={"curse_power_pct": 100, "curse_health_pct": 100},
                effects={"self_power_pct": 50},
            ),
            _step("column", hit={"levy_pct": 25}, effects={"self_damage_pct": -3}),
        ],
        "attack_row_curse_power_chance35_attack_all_levy10": [
            _step("row", hit={"curse_power_pct": 100}),
            _step("all", chance=35, hit={"levy_pct": 10}),
        ],
        "attack_row_followup20_chance25stunslowall_gain5crit": [
            _step("row", effects={"self_crit_pct": 5}),
            _step("row", chance=20),
            _step("all", status_only=True, hit={"stun_pct": 25, "slow_pct": 25, "slow_amt": -50}),
        ],
        "attack_row_followup_all75_drain35": [
            _step("row"),
            _step("all", damage=75, hit={"drain_pct": 35}),
        ],
        "attack_row_followup_all_cursehealth_gain25all_gain5dmgresall": [
            _step("row", effects={"ally_power_pct": 25}),
            _step("all", hit={"curse_health_pct": 100}),
        ],
        "attack_row_followup_column_levy25_stun50_slow50_attack_ally5": [
            _step("row", hit={"stun_pct": 50, "slow_pct": 50, "slow_amt": -50}),
            _step("column", hit={"levy_pct": 25}, effects={"ally_damage_pct": -5}),
        ],
        "attack_row_levy15_slow50_chance25_stun_column": [
            _step("row", hit={"levy_pct": 15, "slow_pct": 50, "slow_amt": -50}),
            _step("column", status_only=True, hit={"stun_pct": 50}),
        ],
        "attack_row_levy15_stun50all_attackallies5": [
            _step("row", hit={"levy_pct": 15}, effects={"ally_damage_pct": -5}),
            _step("all", status_only=True, hit={"stun_pct": 50}),
        ],
        "attack_row_levy20_followup50_column_stun_followup50_column_slow_gain40": [
            _step("row", hit={"levy_pct": 20}, effects={"self_power_pct": 40}),
            _step("column", chance=50, hit={"stun_pct": 100}),
            _step("column", chance=50, hit={"slow_pct": 100, "slow_amt": -50}),
        ],
        "attack_row_slow75_curse75_heal20all_gain20all": [
            _step("row", effects={"ally_heal_pct": 20, "ally_power_pct": 20}),
            _step(
                "all",
                status_only=True,
                hit={"slow_pct": 75, "slow_amt": -50, "curse_health_pct": 75},
            ),
        ],
        "attack_row_slow_3critall_followup25_200x_drain60_stealth": [
            _step("row", hit={"slow_pct": 100, "slow_amt": -50}, effects={"ally_crit_pct": 3}),
            _step("row", damage=200, chance=25, hit={"drain_pct": 60}),
        ],
        "attack_row_stun50_attackall_cursepower50": [
            _step("row", hit={"stun_pct": 50}),
            _step("all", hit={"curse_power_pct": 50}),
        ],
        "attack_row_stun50_heal50_all_followup50_single_slow": [
            _step("row", hit={"stun_pct": 50}, effects={"ally_heal_pct": 50}),
            _step("one", chance=50, hit={"slow_pct": 100, "slow_amt": -50}),
        ],
        "attack_row_stun_chance25_stunall_crit10self": [
            _step("row", hit={"stun_pct": 100}, effects={"self_crit_pct": 10}),
            _step("all", status_only=True, hit={"stun_pct": 25}),
        ],
        "attack_row_stun_cursecolumn_followup50all_levy15": [
            _step("row", hit={"stun_pct": 100}),
            _step("column", status_only=True, hit={"curse_power_pct": 100, "curse_health_pct": 100}),
            _step("all", chance=50, hit={"levy_pct": 15}),
        ],
        "attack_single6x_gain2critall_gain2dmgresall": [
            _step("one", effects={"ally_crit_pct": 2}),
            _step("one", chance=50, repeat=5, stop_on_fail=True, effects={"ally_crit_pct": 2}),
        ],
        "attack_single_2x_slow50_assassinate25": [
            _step("one", repeat=2),
            _step("all", status_only=True, hit={"slow_pct": 50, "slow_amt": -50}),
            _step("one", status_only=True, hit={"assassinate_pct": 25}),
        ],
        "attack_single_5x_chance25_attack_all_stun_chance25_heal200": [
            _step("one"),
            _step("one", chance=50, repeat=4, stop_on_fail=True),
            _step("all", damage=200, chance=25, hit={"stun_pct": 100}),
            _step("one", chance=25, status_only=True, effects={"self_heal_pct": 200}),
        ],
        "attack_single_assassinate_chance50_powergain200_chance10": [
            _step("one"),
            _step("one", chance=50, hit={"assassinate_pct": 100}),
            _step("one", chance=10, status_only=True, effects={"self_power_pct": 200}),
        ],
        "attack_single_chance5x_gain20_statuseffect_attackself10": [
            _step("one", hit={"drain_pct": 20}, effects={"self_power_pct": 20}),
            _step("one", chance=50, hit={"stun_pct": 100}, effects={"self_power_pct": 20}),
            _step("one", chance=50, hit={"slow_pct": 100, "slow_amt": -50}, effects={"self_power_pct": 20}),
            _step("one", chance=50, hit={"curse_power_pct": 100}, effects={"self_power_pct": 20}),
            _step("one", chance=50, hit={"curse_health_pct": 100}, effects={"self_power_pct": 20}),
            _step("one", status_only=True, effects={"self_damage_pct": -10}),
        ],
        "attack_single_cursehealth30_heavyslow25all_mark10": [
            _step("one", hit={"curse_health_pct": 30, "mark_pct": 10}),
            _step("all", status_only=True, hit={"slow_pct": 50, "slow_amt": -100}),
        ],
        "attack_single_levy20_gain25_attack_row_stun_assassinate25": [
            _step("one", hit={"levy_pct": 20}, effects={"self_power_pct": 25}),
            _step("row", hit={"stun_pct": 100}),
            _step("one", status_only=True, hit={"assassinate_pct": 25}),
        ],
        "attack_single_levy25_cursepower_drain35all_gain20all": [
            _step("one", hit={"levy_pct": 25, "curse_power_pct": 100}, effects={"ally_power_pct": 20}),
            _step("all", status_only=True, hit={"drain_pct": 35}),
        ],
        "attack_single_levy30_stun50all_followup_row200": [
            _step("one", hit={"levy_pct": 30}),
            _step("all", status_only=True, hit={"stun_pct": 50}),
            _step("row", damage=200),
        ],
        "attack_single_levy30_stunrow_gain20_followup50_attackall50": [
            _step("one", hit={"levy_pct": 30}, effects={"self_power_pct": 20}),
            _step("row", status_only=True, hit={"stun_pct": 100}),
            _step("all", damage=50, chance=50),
        ],
        "attack_single_slowall_drain50all_mark20_assassinate5_4x": [
            _step("one", hit={"mark_pct": 20}),
            _step("all", status_only=True, hit={"slow_pct": 100, "slow_amt": -50, "drain_pct": 50}),
            _step("one", repeat=4, status_only=True, hit={"assassinate_pct": 5}),
        ],
        "attack_single_slowrow_attackall_levy10_stun50": [
            _step("one"),
            _step("row", status_only=True, hit={"slow_pct": 100, "slow_amt": -50}),
            _step("all", hit={"levy_pct": 10, "stun_pct": 50}),
        ],
        "attack_single_stun_slowall_gain5crit": [
            _step("one", hit={"stun_pct": 100}, effects={"self_crit_pct": 5}),
            _step("all", status_only=True, hit={"slow_pct": 100, "slow_amt": -50}),
        ],
        "attackall_25stun_secondattack50_crit10": [
            _step("all", hit={"stun_pct": 25}, effects={"ally_crit_pct": 10}),
            _step("all", chance=50, hit={"stun_pct": 25}),
        ],
        "attackall_75drain35pwr_50stunrow_assassinate15": [
            _step("all", hit={"drain_pct": 35}),
            _step("row", status_only=True, hit={"stun_pct": 50}),
            _step("one", status_only=True, hit={"assassinate_pct": 15}),
        ],
        "attackrow_secondattack40strun_pwrgain20all": [
            _step("row", effects={"ally_power_pct": 20}),
            _step("row", chance=40, hit={"stun_pct": 100}),
        ],
        "damage_3x_increase_power": [
            _step("one", effects={"self_power_pct": 25}),
            _step("one", chance=50, repeat=2, stop_on_fail=True, effects={"self_power_pct": 25}),
        ],
        "damage_all_damage_single_drainpower_heal_allies_25": [
            _step("all", effects={"ally_heal_pct": 25}),
            _step("one", hit={"drain_pct": 75}),
        ],
        "damage_all_gain_power_2x": [
            _step("all", effects={"ally_power_pct": 25}),
            _step("all", chance=25, status_only=True, effects={"ally_power_pct": 25}),
        ],
        "damage_column_heal_all_50chancedrains_25power": [
            _step("column", effects={"ally_heal_pct": 25}),
            _step("all", chance=50, status_only=True, hit={"drain_pct": 25}),
        ],
        "damage_column_levy20_cursepowerall_heal50all": [
            _step("column", hit={"levy_pct": 20}, effects={"ally_heal_pct": 50}),
            _step("all", status_only=True, hit={"curse_power_pct": 100}),
        ],
        "damage_one_65healall_35powerdrainall": [
            _step("one", effects={"ally_heal_pct": 65}),
            _step("all", status_only=True, hit={"drain_pct": 35}),
        ],
        "damage_one_gain_power_big_attack2x_and_3x_halfpower_and_heal_all": [
            _step("one", effects={"self_power_pct": 100}),
            _step("one", damage=50, chance=50, repeat=2, stop_on_fail=True, effects={"ally_heal_pct": 25}),
        ],
        "damage_one_gain_power_damage_all_init_50": [
            _step("one", effects={"self_power_pct": 25}),
            _step("all", hit={"slow_pct": 100, "slow_amt": -50}),
        ],
        "damage_one_heal_all_large_slow4x": [
            _step("one", effects={"ally_heal_pct": 75}),
            _step("one", chance=25, repeat=4, status_only=True, hit={"slow_pct": 100, "slow_amt": -50}),
        ],
        "damage_one_increase_power_followup_stun_slow_curse_power_health_chance33": [
            _step("one", effects={"self_power_pct": 25}),
            _step(
                "one",
                damage=50,
                hit={
                    "stun_pct": 33,
                    "slow_pct": 33,
                    "slow_amt": -50,
                    "curse_power_pct": 33,
                    "curse_health_pct": 33,
                },
            ),
        ],
        "damage_one_slow_all_followup3x_drain_and_curse_power": [
            _step("one"),
            _step("all", status_only=True, hit={"slow_pct": 100, "slow_amt": -50}),
            _step(
                "one",
                damage=25,
                chance=50,
                repeat=3,
                stop_on_fail=True,
                hit={"drain_pct": 25, "curse_power_pct": 100},
            ),
        ],
        "damage_one_stun50_followup_slow_row_quart_power": [
            _step("one", hit={"stun_pct": 50}),
            _step("row", status_only=True, hit={"slow_pct": 100, "slow_amt": -50}),
            _step("row", damage=25),
        ],
        "damage_one_stun_attack_row25power": [
            _step("one", hit={"stun_pct": 100}),
            _step("row", damage=25),
        ],
        "damage_row_3x_and_drainpower50": [
            _step("row", repeat=3),
            _step("row", status_only=True, hit={"drain_pct": 50}),
        ],
        "damage_row_3x_drainpower_slow_chance40": [
            _step("row", repeat=3, hit={"slow_pct": 40, "slow_amt": -50}),
            _step("row", status_only=True, hit={"drain_pct": 75}),
        ],
        "damage_row_curse_health_grantallies25power_and_25pctchance_attackagain": [
            _step("row", hit={"curse_health_pct": 100}, effects={"ally_power_pct": 25}),
            _step("row", chance=20, hit={"curse_health_pct": 100}),
        ],
        "damage_row_damage_all_allies_heal_all": [
            _step("row", effects={"ally_damage_pct": -5, "ally_heal_pct": 25}),
        ],
        "damage_row_heal35_followup40_heal_allies25": [
            _step("row", effects={"ally_heal_pct": 35}),
            _step("row", chance=40, effects={"ally_heal_pct": 25}),
        ],
        "damage_row_stun50_damage_column_init_50": [
            _step("row", hit={"stun_pct": 50}),
            _step("column", hit={"slow_pct": 100, "slow_amt": -50}),
        ],
        "multiple_3attacks_single_self_gainhealth25_chance_column_self_gainpower25_chance_all_self_gainpower25": [
            _step("one", effects={"self_heal_pct": 25, "self_power_pct": 25}),
            _step("column", damage=150, chance=25, effects={"self_power_pct": 25}),
            _step("all", damage=150, chance=25, effects={"self_power_pct": 25}),
        ],
        "attackcolumn_20pwrdrn_secondattack30stun_heal20all": [
            _step("column", hit={"drain_pct": 20}, effects={"ally_heal_pct": 20}),
            _step("column", chance=30, hit={"stun_pct": 100}),
        ],
        "attack_column50_cursestunrow_followup_attackone300_assassinate20": [
            _step("column", damage=50),
            _step(
                "row",
                status_only=True,
                hit={"stun_pct": 100, "curse_power_pct": 100, "curse_health_pct": 100},
            ),
            _step("one", damage=300, hit={"assassinate_pct": 20}),
        ],
        "attack_single_curse75all_mark30": [
            _step("one", hit={"mark_pct": 30}),
            _step(
                "all",
                chance=75,
                status_only=True,
                hit={"curse_power_pct": 100, "curse_health_pct": 100},
            ),
        ],
        "damage_all_stun_50_and_curse_power_and_health_all": [
            _step("all", hit={"stun_pct": 50, "curse_power_pct": 100, "curse_health_pct": 100}),
        ],
        "damage_all_stun_25_heal_curse_all": [
            _step(
                "all",
                hit={"stun_pct": 25, "curse_power_pct": 100},
                effects={"ally_heal_pct": 25},
            ),
        ],
        "damage_one_10x_heal5_crit2_small_chance_neg_ailment": [
            _step(
                "one",
                hit={
                    "stun_pct": 20,
                    "slow_pct": 20,
                    "slow_amt": -50,
                    "drain_pct": 20,
                    "drain_chance": 20,
                    "curse_power_pct": 20,
                    "curse_health_pct": 20,
                    "self_heal_pct": 5,
                    "self_crit_pct": 2,
                },
            ),
            _step(
                "one",
                chance=50,
                repeat=9,
                stop_on_fail=True,
                hit={
                    "stun_pct": 20,
                    "slow_pct": 20,
                    "slow_amt": -50,
                    "drain_pct": 20,
                    "drain_chance": 20,
                    "curse_power_pct": 20,
                    "curse_health_pct": 20,
                    "self_heal_pct": 5,
                    "self_crit_pct": 2,
                },
            ),
        ],
        "ABILITY_DAMAGE_COLUMN_STUN25_CURSEHEALTH": [
            _step("column", hit={"stun_pct": 25, "curse_health_pct": 100}),
        ],
        "damage_all_cursehealth_stun20": [
            _step("all", hit={"stun_pct": 25, "curse_health_pct": 100}),
        ],
        "damage_row_cursehealth_slow50": [
            _step("row", hit={"slow_pct": 50, "slow_amt": -50, "curse_health_pct": 100}),
        ],
        "attack_all_curse_health_chance50_stun_drain50": [
            _step(
                "all",
                hit={"stun_pct": 50, "drain_pct": 50, "drain_chance": 50, "curse_health_pct": 100},
            ),
        ],
        "damage_gainpower_drainpower_cursepower": [
            _step("one", hit={"drain_pct": 30}, effects={"self_power_pct": 25}),
            _step("column", status_only=True, hit={"curse_power_pct": 100}),
        ],
        "slows_all_curse_power_backrow_stun_column": [
            _step("all", status_only=True, hit={"slow_pct": 100, "slow_amt": -50}),
            _step("back_row", status_only=True, hit={"curse_power_pct": 100}),
            _step("column", status_only=True, hit={"stun_pct": 85}),
        ],
        "ABILITY_DAMAGE_INIT_50_STUN25_COLUMN": [
            _step("column", hit={"slow_pct": 100, "slow_amt": -50, "stun_pct": 25}),
        ],
        "ABILITY_DAMAGE_ROW_SLOWINIT100_FOLLOWUP25": [
            _step("row", hit={"slow_pct": 100, "slow_amt": -100}),
            _step("row", chance=25),
        ],
        "attack_all_slow_25chancecursehealth": [
            _step("all", hit={"slow_pct": 100, "slow_amt": -50, "curse_health_pct": 25}),
        ],
        "attack_all_slow_50curse2rounds_attack_allies_3": [
            _step(
                "all",
                hit={
                    "slow_pct": 100,
                    "slow_amt": -50,
                    "curse_power_pct": 50,
                    "curse_health_pct": 50,
                    "curse_turns": 2,
                },
                effects={"ally_damage_pct": -3},
            ),
        ],
        "damage_all_slow_2rounds_heal50_all": [
            _step(
                "all",
                hit={"slow_pct": 100, "slow_amt": -100},
                effects={"ally_heal_pct": 50},
            ),
        ],
        "damage_one_slow_5x": [
            _step("one", hit={"slow_pct": 25, "slow_amt": -50}),
            _step(
                "one",
                chance=50,
                repeat=4,
                stop_on_fail=True,
                hit={"slow_pct": 25, "slow_amt": -50},
            ),
        ],
        "damage_row_drainpower_25_and_slow_50": [
            _step("row", hit={"drain_pct": 25, "slow_pct": 100, "slow_amt": -50}),
        ],
        "damage_row_slow_25stun": [
            _step("row", hit={"slow_pct": 100, "slow_amt": -50, "stun_pct": 25}),
        ],
        "damage_column_allies_gain_medium_power_health": [
            _step("column", effects={"ally_heal_pct": 50, "ally_power_pct": 50}),
        ],
        "attack_row_all_gain_power10_and_health10": [
            _step("row", effects={"ally_heal_pct": 10, "ally_power_pct": 10}),
        ],
        "damage_one_power_all_30": [
            _step("one", effects={"ally_power_pct": 30}),
        ],
        "damage_row_gain_power_all_10": [
            _step("row", effects={"ally_power_pct": 10}),
        ],
        "ABILITY_DAMAGE_ONE_POWERGAIN50_ALL": [
            _step("one", effects={"ally_power_pct": 25, "ally_power_chance": 50}),
        ],
        "grant_allies_100power_no_attack": [
            _step("all", status_only=True, effects={"ally_power_from_actor_pct": 100}),
        ],
        "noattack_heal100all_gain100all": [
            _step(
                "all",
                status_only=True,
                effects={"ally_heal_pct": 100, "ally_power_from_actor_pct": 100},
            ),
        ],
        "attack_all_alliesgain15power20health_slow50": [
            _step(
                "all",
                hit={"slow_pct": 50, "slow_amt": -50},
                effects={"ally_heal_pct": 20, "ally_power_pct": 15},
            ),
        ],
        "damage_column_drainpower_2x": [
            _step("column", hit={"drain_pct": 50}),
            _step("column", chance=25, hit={"drain_pct": 50}),
        ],
        "damage_row_slow_cursepower_2x": [
            _step("row", hit={"curse_power_pct": 100}),
            _step("row", chance=25, hit={"curse_power_pct": 100}),
        ],
        "ABILITY_DAMAGE_AND_HALF_POWER_FOLLOWUP25": [
            _step("one", hit={"drain_pct": 50}),
            _step("one", chance=25, hit={"drain_pct": 50}),
        ],
        "ABILITY_DAMAGE_ROW_CURSEPOWER_DESCRIPTION_FOLLOWUP25": [
            _step("row", hit={"curse_power_pct": 100}),
            _step("row", chance=25, hit={"curse_power_pct": 100}),
        ],
        "attack_row_levy20_stun_gain20all_attack_ally5": [
            _step(
                "row",
                hit={"levy_pct": 20, "stun_pct": 100},
                effects={"ally_power_pct": 20, "ally_damage_pct": -5},
            ),
        ],
    }
    if key in overrides:
        recipe["steps"] = overrides[key]
    if key == "damage_all_slow_2rounds_heal50_all":
        recipe.setdefault("ambiguous", []).append(
            "the key advertises a two-round slow, but the recovered battle delta has no slow-duration field"
        )


def build(catalog_path: Path) -> dict[str, Any]:
    root = json.loads(catalog_path.read_text())
    grouped: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for row in root["characters"]:
        grouped[row["ability"]["key"]].append(row)

    recipes = []
    for key in sorted(grouped):
        rows = grouped[key]
        signatures = {
            (row["ability"]["text"], row["ability"]["target"], tuple(row["ability"]["mechanics"])) for row in rows
        }
        if len(signatures) != 1:
            raise SystemExit(f"{key}: shared ability key has conflicting descriptions")
        recipe = _generic_recipe(rows[0])
        _override_steps(recipe)
        recipe["character_ids"] = [row["id"] for row in rows]
        recipes.append(recipe)

    return {
        "version": 1,
        "source": str(catalog_path.relative_to(ROOT)),
        "conventions": {
            "qualitative_amount_pct": QUALITATIVE_AMOUNT,
            "qualitative_chance_pct": QUALITATIVE_CHANCE,
            "note": "Exact numeric prose/key values win; qualitative server-only values use these documented bands.",
        },
        "recipe_count": len(recipes),
        "recipes": recipes,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    result = build(args.catalog.resolve())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n")
    ambiguous = sum(bool(row.get("ambiguous")) for row in result["recipes"])
    unsupported = sum(bool(row.get("unsupported")) for row in result["recipes"])
    print(f"wrote {args.output}: {result['recipe_count']} recipes, {ambiguous} ambiguous, {unsupported} unsupported")


if __name__ == "__main__":
    main()
