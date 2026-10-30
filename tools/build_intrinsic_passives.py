#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CATALOG = ROOT / "heroes-of-dragons-cache" / "characters.json"
DEFAULT_REFERENCE = ROOT / ".claude" / "skills" / "hoda-reference" / "data" / "characters.json"
DEFAULT_OUTPUT = ROOT / "data" / "intrinsic_passives.json"

TARGET_WEIGHTS: dict[str, tuple[float, str]] = {
    "AURA_TAUNT": (2.0, "more_likely"),
    "AURA_MASSIVE_TAUNT": (4.0, "much_more_likely"),
    "AURA_ELUSIVE": (0.5, "less_likely"),
    "AURA_VERY_ELUSIVE": (0.25, "much_less_likely"),
}

GROUP_ALIASES = {
    "apostate": "Apostate",
    "apostates": "Apostate",
    "avvar": "Avvar",
    "beast": "Beast",
    "beasts": "Beast",
    "creature": "Beast",
    "creatures": "Beast",
    "blight": "Blight",
    "blighted": "Blight",
    "blood mage": "Blood_Mage",
    "blood mages": "Blood_Mage",
    "bulls chargers": "Bulls_Chargers",
    "bull s chargers": "Bulls_Chargers",
    "carta": "Carta",
    "chantry": "Chantry",
    "circle": "Circle",
    "demon": "Demon",
    "demons": "Demon",
    "demonic": "Demon",
    "dragon": "Dragon",
    "dragons": "Dragon",
    "dwarf": "Dwarf",
    "dwarves": "Dwarf",
    "elf": "Elves",
    "elves": "Elves",
    "elven": "Elves",
    "ferelden": "Ferelden",
    "free marches": "FreeMarches",
    "free marcher": "FreeMarches",
    "free marchers": "FreeMarches",
    "golem": "Golem",
    "golems": "Golem",
    "human": "Human",
    "humans": "Human",
    "inquisition": "Inquisition",
    "jaws of hakkon": "Jaws_of_Hakkon",
    "legion of the dead": "LegionOfTheDead",
    "mage": "Mage",
    "mages": "Mage",
    "nobility": "Nobility",
    "nug": "Nug",
    "nugs": "Nug",
    "orlesian": "Orlesian",
    "orlesians": "Orlesian",
    "outlaw": "Outlaw",
    "outlaws": "Outlaw",
    "pet": "Pets",
    "pets": "Pets",
    "qunari": "Qunari",
    "reaver": "Reavers",
    "reavers": "Reavers",
    "red lyrium": "Red_Lyrium",
    "red lyrium heroes": "Red_Lyrium",
    "red lyrium allies": "Red_Lyrium",
    "rogue": "Rogue",
    "rogues": "Rogue",
    "seeker": "Seeker",
    "seekers": "Seeker",
    "spirit": "Spirit",
    "spirits": "Spirit",
    "templar": "Templar",
    "templars": "Templar",
    "tevinter": "Tevinter",
    "triumvirate": "Triumvirate",
    "venatori": "Venatori",
    "warden": "Warden",
    "wardens": "Warden",
    "grey warden": "Warden",
    "grey wardens": "Warden",
    "warrior": "Warrior",
    "warriors": "Warrior",
    "werewolf": "Werewolves",
    "werewolves": "Werewolves",
    "wisp": "Wisp",
    "wisps": "Wisp",
}

FACTION_ALIASES = {
    "black faction": "Black",
    "blue faction": "Blue",
    "red faction": "Red",
    "white faction": "White",
}

SHORT_NAME_ALIASES = {
    "felix alexius": "UNIT_FELIXALEXIUS",
    "king endrin": "UNIT_KINGENDRINAEDUCAN",
    "prince bhelen aeducan": "UNIT_PRINCEBHELEN",
    "regalyan": "UNIT_REGALYAND'MARCALL",
    "ser aaron": "UNIT_SERAARONHAWTHORNE",
}

SPECIALTY_ALIASES = {
    "defender": "DEFENDER",
    "defenders": "DEFENDER",
    "power gainer": "POWERGAIN",
    "power gainers": "POWERGAIN",
    "enemy healers": "HEAL",
    "healers": "HEAL",
    "enemies who power drain": "POWERDRAIN",
}

SPEED_ALIASES = {
    "quick round heroes": "Quick",
    "quick heroes": "Quick",
}

TAG_FAMILY_ALIASES = {
    "alistair": "Alistair",
    "aveline": "Aveline",
    "cassandra": "Cassandra",
    "christmas heroes": "Christmas",
    "dace family": "DaceFamily",
    "gamordan stormrider brood": "GamordanStormriderBrood",
    "hawke family": "HawkeFamily",
    "the hawke family": "HawkeFamily",
    "loghain": "Loghain",
    "flemeth": "Flemeth",
    "isabela": "Isabela",
    "sentinels": "SentinelElves",
    "sha brytol": "Sha-Brytol",
    "the sha brytol": "Sha-Brytol",
    "sky cult allies": "SkyCult",
    "the forbidden ones": "ForbiddenOnes",
    "theirin family": "TheirinFamily",
    "varric": "Varric",
    "vinsomer brood": "VinsomerBrood",
    "witches": "WitchOfTheWild",
    "witches of the wilds": "WitchOfTheWild",
    "the witches of the wilds": "WitchOfTheWild",
}

DYNAMIC_WORDS = re.compile(
    r"\b(?:when|whenever|upon|on (?:any |ally |enemy |self |hero )?death|after|per each|"
    r"converts?|gains?|heals?|receives?|"
    r"drains?(?=\s+(?:\d+%|a\b|an\b|the\b|all\b|enemy|enemies|power\b|health\b)))\b",
    re.I,
)
STATIC_STAT_CANDIDATE = re.compile(
    r"(?:\+?\d+%\s+(?:health\b|power\b(?!\s+(?:drain|curse|resist)))|"
    r"(?:health|power)\s*\+?\d+%)",
    re.I,
)
RED_STAT_CANDIDATE = re.compile(
    r"-\d+%\s+(?:health\b|power\b(?!\s*(?:drain|curse|resist)))", re.I
)


def _clean(text: str) -> str:
    return re.sub(r"\s+", " ", text.replace("\\n", " ").replace("’", "'")).strip()


def _colored_sentences(text: str, color: str) -> list[str]:
    chunks = re.findall(rf"<color={color}>(.*?)</color>", text, re.I | re.S)
    if not chunks and color == "green":
        chunks = [re.sub(r"<color=(?:orange|red)>.*?</color>", "", text, flags=re.I | re.S)]
    result: list[str] = []
    for chunk in chunks:
        clean = _clean(re.sub(r"<[^>]+>", "", chunk))
        result.extend(part.strip(" .") for part in re.split(r"[.!?]+", clean) if part.strip(" ."))
    return result


def _green_sentences(text: str) -> list[str]:
    return _colored_sentences(text, "green")


def _normal(value: str) -> str:
    value = value.casefold().replace("_", " ").replace("-", " ")
    value = re.sub(r"[^a-z0-9 ]+", " ", value)
    return re.sub(r"\s+", " ", value).strip()


def _name_index(rows: list[dict[str, Any]]) -> dict[str, list[str]]:
    result: dict[str, list[str]] = {}
    for row in rows:
        result.setdefault(_normal(row["name"]), []).append(row["id"])
    return result


def _family_index(
    rows: list[dict[str, Any]], reference_rows: dict[str, dict[str, Any]]
) -> dict[str, list[str]]:
    predicates = {
        "advisors": lambda row: row["id"].startswith("UNIT_ADVISOR"),
        "aeducans": lambda row: "AEDUCAN" in row["id"],
        "arcane hawke": lambda row: row["id"].startswith("UNIT_ARCANE") and row["id"].endswith("HAWKE"),
        "divines": lambda row: row["id"].startswith("UNIT_DIVINE"),
        "the divines": lambda row: row["id"].startswith("UNIT_DIVINE"),
        "dracolisks": lambda row: "DRACOLISK" in row["id"],
        "fennecs": lambda row: row["id"].endswith("FENNEC"),
        "goats": lambda row: "GOAT" in row["id"],
        "invisible nugs": lambda row: "INVISIBLENUG" in row["id"],
        "kaltenzahn brood": lambda row: "KALTENZAHN" in row["id"],
        "mistral brood": lambda row: "MISTRAL" in row["id"],
        "nugly sweater nugs": lambda row: "NUGLYSWEATER" in row["id"],
        "sha brytol": lambda row: "SHABRYTOL" in row["id"].replace("-", ""),
        "shale": lambda row: "SHALE" in row["id"],
        "wintersend allies": lambda row: row["id"].startswith("UNIT_WINTERSEND"),
    }
    result: dict[str, list[str]] = {}
    for alias, predicate in predicates.items():
        ids = sorted(row["id"] for row in rows if predicate(row))
        if ids:
            result[alias] = ids
    catalog_ids = {row["id"] for row in rows}
    for alias, tag in TAG_FAMILY_ALIASES.items():
        ids = sorted(
            unit_id
            for unit_id, row in reference_rows.items()
            if unit_id in catalog_ids and tag in row.get("tags", [])
        )
        if ids:
            result[alias] = ids
    return result


def _split_targets(text: str) -> list[str]:
    text = re.sub(r"\([^)]*\)", "", text)
    text = re.sub(r"\b(?:all|the)\s+allies\b", "allies", text, flags=re.I)
    text = text.replace("/", ",")
    text = re.sub(r"\b(?:and|or)\b", ",", text, flags=re.I)
    return [part.strip(" ,") for part in text.split(",") if part.strip(" ,")]


def _target_terms(
    value: str, names: dict[str, list[str]], families: dict[str, list[str]]
) -> list[dict[str, str]] | None:
    key = _normal(value)
    if key in {"ally", "allies", "team", "squad"}:
        return [{"kind": "allies", "value": "*"}]
    if key in {"self", "this hero", "this unit"}:
        return [{"kind": "self", "value": "*"}]
    if key in {"giant", "giants"}:
        return [{"kind": "tag", "value": "GIANT"}]
    if key in families:
        return [{"kind": "character_id", "value": unit_id} for unit_id in families[key]]
    key = re.sub(r"^enemy\s+", "", key)
    key = re.sub(r"\s+enemies?$", "", key)
    key = re.sub(r"\s+allies$", "", key)
    star = re.fullmatch(r"([123])\s*star(?:\s+heroes?)?", key)
    if star:
        return [{"kind": "tag", "value": f"{star.group(1)}STAR"}]
    if key in GROUP_ALIASES:
        return [{"kind": "group", "value": GROUP_ALIASES[key]}]
    if key in FACTION_ALIASES:
        return [{"kind": "faction", "value": FACTION_ALIASES[key]}]
    if key in SPECIALTY_ALIASES:
        return [{"kind": "specialty", "value": SPECIALTY_ALIASES[key]}]
    if key in SPEED_ALIASES:
        return [{"kind": "speed", "value": SPEED_ALIASES[key]}]
    rarity = re.fullmatch(r"(common|uncommon|rare|epic|legendary|mythic)(?: heroes?)?", key)
    if rarity:
        return [{"kind": "rarity", "value": rarity.group(1).title()}]
    if key in SHORT_NAME_ALIASES:
        return [{"kind": "character_id", "value": SHORT_NAME_ALIASES[key]}]
    ids = names.get(key, [])
    if len(ids) == 1:
        return [{"kind": "character_id", "value": ids[0]}]
    return None


def _recipient_for(
    target_text: str,
    names: dict[str, list[str]],
    families: dict[str, list[str]],
    *,
    explicit_stacking: bool,
) -> tuple[list[dict[str, Any]], list[str]]:
    parts = _split_targets(target_text)
    terms: list[dict[str, str]] = []
    unknown: list[str] = []
    for part in parts:
        resolved = _target_terms(part, names, families)
        if resolved is None:
            unknown.append(part)
        else:
            terms.extend(resolved)
    if unknown or not terms:
        return [], unknown or [target_text]
    if any(term["kind"] == "self" for term in terms):
        if len(terms) != 1:
            return [], [target_text]
        return [{"scope": "self"}], []

    all_term = any(term["kind"] == "allies" for term in terms)
    filtered = [term for term in terms if term["kind"] != "allies"]
    if explicit_stacking and all_term and filtered:
        recipients: list[dict[str, Any]] = [{"scope": "allies", "include_source": True}]
        recipients.extend(
            {
                "scope": "allies",
                "include_source": True,
                "match": {"mode": "any", "terms": [term]},
            }
            for term in filtered
        )
        return recipients, []
    if all_term:
        return [{"scope": "allies", "include_source": True}], []
    return [
        {
            "scope": "allies",
            "include_source": True,
            "match": {"mode": "each" if explicit_stacking else "any", "terms": filtered},
        }
    ], []


def _stat_values(body: str) -> dict[int, set[str]]:
    found: dict[int, set[str]] = {}
    occupied: list[tuple[int, int]] = []

    def add(match: re.Match[str], stats: tuple[str, ...], values: tuple[int, ...]) -> None:
        occupied.append(match.span())
        for stat, value in zip(stats, values):
            found.setdefault(value, set()).add(stat)

    pair_patterns = [
        r"\+?(\d+)%\s+(?:bonus\s+)?health\s+and\s+(?:bonus\s+)?power\b",
        r"\+?(\d+)%\s+(?:bonus\s+)?power\s+and\s+(?:bonus\s+)?health\b",
        r"\+?(\d+)%\s+(?:bonus\s+)?health\s*,\s*(?:bonus\s+)?power\b(?!\s+(?:drain|curse|resist))",
        r"\+?(\d+)%\s+(?:bonus\s+)?power\s*,\s*(?:bonus\s+)?health\b",
    ]
    for pattern in pair_patterns:
        for match in re.finditer(pattern, body, re.I):
            value = int(match.group(1))
            add(match, ("health", "power"), (value, value))

    separate = re.compile(
        r"\+?(\d+)%\s+(?:bonus\s+)?health\s+and\s+"
        r"\+?(\d+)%\s+(?:bonus\s+)?power\b(?!\s+(?:drain|curse|resist))",
        re.I,
    )
    for match in separate.finditer(body):
        add(match, ("health", "power"), (int(match.group(1)), int(match.group(2))))
    reverse_separate = re.compile(
        r"\+?(\d+)%\s+(?:bonus\s+)?power\s+and\s+"
        r"\+?(\d+)%\s+(?:bonus\s+)?health\b",
        re.I,
    )
    for match in reverse_separate.finditer(body):
        add(match, ("power", "health"), (int(match.group(1)), int(match.group(2))))

    def overlaps(span: tuple[int, int]) -> bool:
        return any(span[0] < end and start < span[1] for start, end in occupied)

    for stat in ("health", "power"):
        suffix = r"(?!\s+(?:drain|curse|resist))" if stat == "power" else ""
        for match in re.finditer(rf"\+?(\d+)%\s+(?:bonus\s+)?{stat}\b{suffix}", body, re.I):
            if not overlaps(match.span()):
                found.setdefault(int(match.group(1)), set()).add(stat)
    return found


def _sentence_clauses(sentence: str) -> list[str]:
    supplement = re.compile(
        r",\s*(?=(?:and\s+)?(?:an\s+)?(?:extra|additional|another|double)\b)", re.I
    )
    pending = [part.strip(" ,") for part in supplement.split(sentence) if part.strip(" ,")]
    parts: list[str] = []
    for part in pending:
        while True:
            split_at: re.Match[str] | None = None
            for candidate in re.finditer(
                r",\s*(?=(?:and\s+)?(?:[+-]?\d+%|allies\b|elven\s+allies\b))", part, re.I
            ):
                before = part[: candidate.start()]
                if re.search(r"\b(?:for|to)\b", before, re.I):
                    split_at = candidate
                    break
            if split_at is None:
                parts.append(part.strip(" ,"))
                break
            parts.append(part[: split_at.start()].strip(" ,"))
            part = part[split_at.end() :].strip(" ,")
    result: list[str] = []
    for part in parts:
        match = re.search(
            r"\s+and\s+(?=(?:an\s+)?(?:extra|additional|another|double)\b|"
            r"[+-]?\d+%[^,.]{0,90}\b(?:for|to)\b)",
            part,
            re.I,
        )
        if match and re.search(r"\b(?:for|to)\b", part[: match.start()], re.I):
            result.extend([part[: match.start()].strip(" ,"), part[match.end() :].strip(" ,")])
        else:
            result.append(part)
    return result


def _inherited_clause(clause: str, values: dict[int, set[str]] | None) -> str:
    if not values:
        return clause
    clean = re.sub(r"^(?:and\s+)?(?:an\s+)?(?:extra|additional|another)\s+", "", clause, flags=re.I)
    double = re.fullmatch(r"double\s+for\s+(.+)", clean, re.I)
    if double:
        chunks: list[str] = []
        for value, stats in sorted(values.items()):
            for stat in sorted(stats):
                chunks.append(f"+{value}% {stat.title()}")
        return " and ".join(chunks) + f" for {double.group(1)}"
    implicit = re.fullmatch(r"\+?(\d+)%\s+(?:extra\s+)?(?:each\s+)?for\s+(.+)", clean, re.I)
    if implicit:
        value = int(implicit.group(1))
        stats = sorted({stat for group in values.values() for stat in group})
        return f"+{value}% {' and '.join(stat.title() for stat in stats)} for {implicit.group(2)}"
    return clean


def _static_effects(
    sentence: str,
    names: dict[str, list[str]],
    families: dict[str, list[str]],
) -> tuple[list[dict[str, Any]], str | None, dict[int, set[str]] | None]:
    if not STATIC_STAT_CANDIDATE.search(sentence) or DYNAMIC_WORDS.search(sentence):
        return [], None, None
    prefix_target = re.fullmatch(
        r"(.+?)\s+allies\b.*?\+?(\d+)%\s+(?:bonus\s+)?(health|power)", sentence, re.I
    )
    if prefix_target:
        value = int(prefix_target.group(2))
        values = {value: {prefix_target.group(3).casefold()}}
        recipients, unknown = _recipient_for(
            prefix_target.group(1), names, families, explicit_stacking="stacking" in sentence.casefold()
        )
        if unknown:
            return [], "unsupported recipient: " + ", ".join(unknown), values
        return (
            [
                {
                    "kind": "stat_percent",
                    "trigger": "battle_start",
                    "stats": sorted(values[value]),
                    "value_pct": value,
                    "recipient": recipient,
                    "stacking": "additive",
                    "confidence": "explicit",
                    "source_text": sentence,
                }
                for recipient in recipients
            ],
            None,
            values,
        )
    delimiter = re.search(r"\b(?:for|to)\b", sentence, re.I)
    if not delimiter:
        ally = re.fullmatch(r"\+?(\d+)%\s+ally\s+(health|power)", sentence, re.I)
        if not ally:
            return [], "unsupported static Health/Power grammar", None
        values = {int(ally.group(1)): {ally.group(2).casefold()}}
        recipients = [{"scope": "allies", "include_source": True}]
    else:
        body = sentence[: delimiter.start()].strip(" ,")
        target_text = sentence[delimiter.end() :].strip(" ,")
        values = _stat_values(body)
        if not values:
            return [], "unsupported static Health/Power grammar", None
        each_ally = re.fullmatch(r"each\s+(.+?)\s+ally", target_text, re.I)
        if each_ally:
            count_terms = _target_terms(each_ally.group(1), names, families)
            if not count_terms or any(term["kind"] in {"allies", "self"} for term in count_terms):
                return [], "unsupported counted ally: " + each_ally.group(1), values
            return (
                [
                    {
                        "kind": "stat_percent_per_matching_ally",
                        "trigger": "battle_start",
                        "stats": sorted(values[value]),
                        "value_pct": value,
                        "recipient": {"scope": "self"},
                        "count": {
                            "scope": "allies",
                            "include_source": False,
                            "match": {"mode": "any", "terms": count_terms},
                        },
                        "stacking": "additive",
                        "confidence": "explicit",
                        "source_text": sentence,
                    }
                    for value in sorted(values)
                ],
                None,
                values,
            )
        recipients, unknown = _recipient_for(
            target_text,
            names,
            families,
            explicit_stacking="stacking" in sentence.casefold() or "stacks" in sentence.casefold(),
        )
        if unknown:
            return [], "unsupported recipient: " + ", ".join(unknown), values

    result: list[dict[str, Any]] = []
    for value in sorted(values):
        for recipient in recipients:
            result.append(
                {
                    "kind": "stat_percent",
                    "trigger": "battle_start",
                    "stats": sorted(values[value]),
                    "value_pct": value,
                    "recipient": recipient,
                    "stacking": "additive",
                    "confidence": "explicit",
                    "source_text": sentence,
                }
            )
    return result, None, values


def _enemy_recipient(
    target_text: str,
    names: dict[str, list[str]],
    families: dict[str, list[str]],
) -> tuple[dict[str, Any] | None, list[str]]:
    target_text = re.sub(
        r"\b(black|blue|red|white)\s*/\s*(black|blue|red|white)\s+faction\b",
        lambda match: f"{match.group(1)} faction, {match.group(2)} faction",
        target_text,
        flags=re.I,
    )
    if _normal(target_text) in {"", "enemy", "enemies", "all enemies"}:
        return {"scope": "enemies"}, []
    parts = _split_targets(target_text)
    terms: list[dict[str, str]] = []
    unknown: list[str] = []
    for part in parts:
        resolved = _target_terms(part, names, families)
        if resolved is None:
            unknown.append(part)
        else:
            terms.extend(term for term in resolved if term["kind"] not in {"allies", "self"})
    if unknown or not terms:
        return None, unknown or [target_text]
    return {"scope": "enemies", "match": {"mode": "any", "terms": terms}}, []


def _power_vs_effects(
    clause: str,
    names: dict[str, list[str]],
    families: dict[str, list[str]],
) -> tuple[list[dict[str, Any]], str | None]:
    match = re.fullmatch(r"\+?(\d+)%\s+power\s+vs\s+(.+)", clause, re.I)
    if not match:
        return [], None
    target, unknown = _enemy_recipient(match.group(2), names, families)
    if unknown or target is None:
        return [], "unsupported Power-vs target: " + ", ".join(unknown)
    return (
        [
            {
                "kind": "bonus_damage_percent",
                "trigger": "before_damage",
                "value_pct": int(match.group(1)),
                "recipient": {"scope": "self"},
                "target": target,
                "stacking": "additive",
                "confidence": "explicit",
                "source_text": clause,
            }
        ],
        None,
    )


def _inherited_red_clause(clause: str, values: dict[int, set[str]] | None) -> str:
    if not values:
        return clause
    clean = re.sub(r"^(?:and\s+)?(?:an\s+)?(?:extra|additional|another)\s+", "", clause, flags=re.I)
    clean = re.sub(r"^and\s+", "", clean, flags=re.I)
    double = re.fullmatch(r"double\s+(?:for|to)\s+(.+)", clean, re.I)
    if double:
        chunks = [
            f"-{value}% {stat.title()}"
            for value, stats in sorted(values.items())
            for stat in sorted(stats)
        ]
        return " and ".join(chunks) + f" to {double.group(1)}"
    implicit = re.fullmatch(r"-?(\d+)%\s+(?:extra\s+)?(?:each\s+)?(?:for|to)\s+(.+)", clean, re.I)
    if implicit:
        value = int(implicit.group(1))
        stats = sorted({stat for group in values.values() for stat in group})
        return " and ".join(f"-{value}% {stat.title()}" for stat in stats) + f" to {implicit.group(2)}"
    return clean


def _red_static_effects(
    clause: str,
    names: dict[str, list[str]],
    families: dict[str, list[str]],
) -> tuple[list[dict[str, Any]], str | None, dict[int, set[str]] | None]:
    if not RED_STAT_CANDIDATE.search(clause) or DYNAMIC_WORDS.search(clause):
        return [], None, None
    if re.search(r"\bpower\s+vs\b", clause, re.I):
        return [], "combat Power-vs-target wording is not a base-stat aura", None
    delimiter = re.search(r"\b(?:for|to)\b", clause, re.I)
    body = clause[: delimiter.start()].strip(" ,") if delimiter else clause
    target_text = clause[delimiter.end() :].strip(" ,") if delimiter else "enemies"
    values = _stat_values(body)
    if not values:
        return [], "unsupported enemy Health/Power grammar", None
    recipient, unknown = _enemy_recipient(target_text, names, families)
    if unknown or recipient is None:
        return [], "unsupported enemy recipient: " + ", ".join(unknown), values
    effects = [
        {
            "kind": "stat_percent",
            "trigger": "battle_start",
            "stats": sorted(stats),
            "value_pct": -value,
            "recipient": recipient,
            "stacking": "additive",
            "confidence": "explicit",
            "source_text": clause,
        }
        for value, stats in sorted(values.items())
    ]
    return effects, None, values


def _envy_special_effects(row: dict[str, Any]) -> list[dict[str, Any]]:
    if row["id"] != "UNIT_ENVYDEMON":
        return []
    return [
        {
            "kind": "heal_percent_max_health",
            "trigger": "enemy_death",
            "value_pct": 25,
            "recipient": {"scope": "self"},
            "confidence": "key_encoded",
            "source_text": "Heals a small amount on enemy death.",
        },
        {
            "kind": "bonus_damage_percent",
            "trigger": "before_damage",
            "value_pct": 35,
            "recipient": {"scope": "self"},
            "target": {
                "scope": "enemies",
                "match": {"mode": "any", "terms": [{"kind": "specialty", "value": "POWERGAIN"}]},
            },
            "stacking": "additive",
            "confidence": "explicit",
            "source_text": "+35% bonus damage vs Power gainers.",
        },
        {
            "kind": "outgoing_target_preference",
            "trigger": "target_selection",
            "selector": "lowest_health_percent",
            "value_multiplier": 2.0,
            "recipient": {"scope": "enemies"},
            "confidence": "reconstructed",
            "source_text": "More likely to attack the lowest Health target.",
        },
    ]


def _target_weight_effect(row: dict[str, Any]) -> dict[str, Any] | None:
    key = row.get("target_weight", "")
    if not key:
        return None
    if key not in TARGET_WEIGHTS:
        raise SystemExit(f"{row['id']}: unsupported target weight key {key!r}")
    multiplier, strength = TARGET_WEIGHTS[key]
    return {
        "kind": "incoming_target_weight",
        "trigger": "target_selection",
        "strength": strength,
        "value_multiplier": multiplier,
        "recipient": {"scope": "self"},
        "confidence": "reconstructed",
        "source_text": _clean(re.sub(r"<[^>]+>", "", row.get("target_weight_text", ""))),
    }


def build(catalog_path: Path, reference_path: Path = DEFAULT_REFERENCE) -> dict[str, Any]:
    catalog = json.loads(catalog_path.read_text())
    reference_rows = json.loads(reference_path.read_text())
    rows = catalog["characters"]
    names = _name_index(rows)
    families = _family_index(rows, reference_rows)
    output_rows: list[dict[str, Any]] = []
    supported_static = 0
    unresolved_static = 0
    supported_power_vs = 0
    unresolved_power_vs = 0
    supported_enemy_static = 0
    unresolved_enemy_static = 0
    target_count = 0

    for row in sorted(rows, key=lambda item: item["id"]):
        effects: list[dict[str, Any]] = []
        unresolved: list[dict[str, str]] = []
        for sentence in _green_sentences(row.get("equipped_rune_text", "")):
            inherited: dict[int, set[str]] | None = None
            for raw_clause in _sentence_clauses(sentence):
                clause = _inherited_clause(raw_clause, inherited)
                power_vs, power_vs_reason = _power_vs_effects(clause, names, families)
                if power_vs:
                    effects.extend(power_vs)
                    supported_power_vs += 1
                    continue
                if power_vs_reason:
                    unresolved.append(
                        {"kind": "power_vs_target", "source_text": raw_clause, "reason": power_vs_reason}
                    )
                    unresolved_power_vs += 1
                    continue
                parsed, reason, values = _static_effects(clause, names, families)
                if values:
                    inherited = values
                if parsed:
                    effects.extend(parsed)
                    supported_static += 1
                elif reason:
                    unresolved.append({"kind": "battle_start_stat", "source_text": raw_clause, "reason": reason})
                    unresolved_static += 1
        for sentence in _colored_sentences(row.get("equipped_rune_text", ""), "red"):
            inherited = None
            for raw_clause in _sentence_clauses(sentence):
                clause = _inherited_red_clause(raw_clause, inherited)
                parsed, reason, values = _red_static_effects(clause, names, families)
                if values:
                    inherited = values
                if parsed:
                    effects.extend(parsed)
                    supported_enemy_static += 1
                elif reason:
                    unresolved.append(
                        {"kind": "battle_start_enemy_stat", "source_text": raw_clause, "reason": reason}
                    )
                    unresolved_enemy_static += 1

        target = _target_weight_effect(row)
        if target:
            effects.append(target)
            target_count += 1
        effects.extend(_envy_special_effects(row))
        if not effects and not unresolved:
            continue
        for index, effect in enumerate(effects, 1):
            effect["id"] = f"{row['id'].casefold()}_{index:02d}"
        output_rows.append(
            {
                "id": row["id"],
                "source": {
                    "equipped_rune_key": row.get("equipped_rune", ""),
                    "equipped_rune_text": row.get("equipped_rune_text", ""),
                    "target_weight_key": row.get("target_weight", ""),
                    "target_weight_text": row.get("target_weight_text", ""),
                },
                "effects": effects,
                "unresolved": unresolved,
            }
        )

    return {
        "version": 1,
        "source": str(catalog_path.relative_to(ROOT)),
        "reference_source": str(reference_path.relative_to(ROOT)),
        "conventions": {
            "percent_unit": "whole percentage points",
            "hero_aura_stacking": "base * (1 + sum(hero aura percentages) / 100)",
            "squad_rune_stacking": "apply after hero auras as a separate multiplicative layer",
            "match_any": "apply once when at least one term matches",
            "match_each": "apply once for every matching term; emitted only for explicit stacking wording",
            "composition_count": "count matching allied units other than the source, then add value_pct per match",
            "power_vs_target": "apply as additive outgoing damage only when the attacked enemy matches; do not alter preview Power",
            "target_weight_note": "Multipliers are reconstructed because exact retired-server weights are unavailable.",
        },
        "coverage": {
            "catalog_characters": len(rows),
            "manifest_characters": len(output_rows),
            "supported_battle_start_stat_clauses": supported_static,
            "unresolved_battle_start_stat_clauses": unresolved_static,
            "supported_power_vs_target_clauses": supported_power_vs,
            "unresolved_power_vs_target_clauses": unresolved_power_vs,
            "supported_enemy_battle_start_stat_clauses": supported_enemy_static,
            "unresolved_enemy_battle_start_stat_clauses": unresolved_enemy_static,
            "target_weight_descriptions": target_count,
            "target_weight_effects": target_count,
        },
        "characters": output_rows,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--catalog", type=Path, default=DEFAULT_CATALOG)
    parser.add_argument("--reference", type=Path, default=DEFAULT_REFERENCE)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    result = build(args.catalog.resolve(), args.reference.resolve())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n")
    coverage = result["coverage"]
    print(
        f"wrote {args.output}: {coverage['manifest_characters']} characters, "
        f"{coverage['supported_battle_start_stat_clauses']} supported static clauses, "
        f"{coverage['unresolved_battle_start_stat_clauses']} unresolved, "
        f"{coverage['supported_power_vs_target_clauses']} Power-vs clauses, "
        f"{coverage['unresolved_power_vs_target_clauses']} Power-vs unresolved, "
        f"{coverage['supported_enemy_battle_start_stat_clauses']} enemy stat clauses, "
        f"{coverage['unresolved_enemy_battle_start_stat_clauses']} enemy unresolved, "
        f"{coverage['target_weight_effects']} target weights"
    )


if __name__ == "__main__":
    main()
