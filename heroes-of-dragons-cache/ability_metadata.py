#!/usr/bin/env python3

from __future__ import annotations

import re


MECHANIC_ORDER = (
    "ATTACK",
    "ATTACK_NONE",
    "POWER_DRAIN",
    "STUN",
    "SLOW",
    "CURSE_POWER",
    "CURSE_HEALTH",
    "HEAL_ALLIES",
    "HEAL_SELF",
    "GAIN_POWER_ALLIES",
    "GAIN_POWER_SELF",
    "MULTIATTACK",
    "MARK",
    "LEVY",
    "ASSASSINATE",
    "ATTACK_FRIENDLY",
    "DAMAGE_SELF",
    "LOSE_POWER_SELF",
    "SLOW_ALLIES",
    "GAIN_CRIT_ALLIES",
    "CURSE_TWO_ROUNDS",
    "GAIN_DAMAGE_ALLIES",
    "GAIN_DAMAGE_SELF",
)


def normalize_text(value: str | None) -> str:
    text = (value or "").replace("\\n", " ").replace("\u00a0", " ")
    return re.sub(r"\s+", " ", text).strip()


def infer_ability_mechanics(text: str, source_specialties: list[str] | None = None) -> list[str]:
    prose = normalize_text(text).casefold()
    specialties = set(source_specialties or ())
    found: set[str] = set()

    says_attack = bool(re.search(r"\battack(?:s|ed|ing)?\b", prose))
    if (
        "does not attack" in prose
        or re.search(r"\bno attack\b", prose)
        or ("ATTACKNONE" in specialties and not says_attack)
    ):
        found.add("ATTACK_NONE")
    else:
        found.add("ATTACK")
    self_power_loss = bool(
        re.search(r"\bdrain(?:s|ed|ing)?\b[^.]{0,35}\bself power\b", prose)
        or re.search(r"\bdrain(?:s|ed|ing)?\b[^.]{0,25}\bpower from (?:himself|herself|itself|self)\b", prose)
    )
    hostile_prose = re.sub(
        r"\bdrain(?:s|ed|ing)?\b[^.]{0,35}\bself power\b|"
        r"\bdrain(?:s|ed|ing)?\b[^.]{0,25}\bpower from (?:himself|herself|itself|self)\b",
        "",
        prose,
    )
    if re.search(r"\bdrain(?:s|ed|ing)?\b[^.]{0,45}\bpower\b|\bpower[- ]drain", hostile_prose):
        found.add("POWER_DRAIN")
    if self_power_loss:
        found.add("LOSE_POWER_SELF")
    if re.search(r"\bstun(?:s|ned|ning)?\b", prose):
        found.add("STUN")
    if re.search(r"\bslow(?:s|ed|ing)?\b", prose):
        found.add("SLOW")
    if re.search(
        r"\bslow(?:s|ed|ing)?\s+(?:all\s+)?all(?:y|ies)\b|"
        r"\ball(?:y|ies)\b[^.]{0,20}\b(?:are|get|become)\s+slow(?:ed)?\b",
        prose,
    ):
        found.add("SLOW_ALLIES")
    if (
        re.search(r"\ballies (?:gain|gains|get|receive)[^.]{0,55}\b(?:crit|2x damage)\b", prose)
        or re.search(r"\bgrants? allies\b[^.]{0,55}\b(?:crit|2x damage)\b", prose)
        or re.search(r"\bincreases? allies\b[^.]{0,55}\b(?:crit|2x damage)\b", prose)
        or re.search(r"\bincreases?\b[^.]{0,55}\b(?:crit|2x damage)\b[^.]{0,25}\bfor allies\b", prose)
        or re.search(r"\bgranting\b[^.]{0,55}\b(?:crit|2x damage)\b[^.]{0,45}\bto allies\b", prose)
    ):
        found.add("GAIN_CRIT_ALLIES")
    bonus_damage_sentences = [sentence.strip() for sentence in re.split(r"[.!?]+", prose) if sentence.strip()]
    if any(
        "bonus damage" in sentence
        and "allies" in sentence
        and re.search(r"\b(?:gain|gains|grant|grants)\b", sentence)
        for sentence in bonus_damage_sentences
    ):
        found.add("GAIN_DAMAGE_ALLIES")
    if any(
        "bonus damage" in sentence and "allies" not in sentence and "mark" not in sentence
        for sentence in bonus_damage_sentences
    ):
        found.add("GAIN_DAMAGE_SELF")
    if re.search(r"\bstatus (?:ailment|effect)s?\b", prose):
        for specialty, mechanic in (
            ("POWERDRAIN", "POWER_DRAIN"),
            ("STUN", "STUN"),
            ("SLOW", "SLOW"),
            ("CURSEPOWER", "CURSE_POWER"),
            ("CURSEHEALTH", "CURSE_HEALTH"),
        ):
            if specialty in specialties:
                found.add(mechanic)

    curse_power = bool(
        re.search(
            r"\bcurs(?:e|es|ed|ing)\b"
            r"(?:(?!\b(?:drain(?:s|ed|ing)?|gains|gained|gaining|grant(?:s|ed|ing)?|"
            r"heal(?:s|ed|ing)?|stun(?:s|ned|ning)?|slow(?:s|ed|ing)?|attack(?:s|ed|ing)?)\b)[^.])"
            r"{0,35}\bpower\b",
            prose,
        )
        or re.search(r"prevent(?:s|ing)?[^.]{0,35}\bpower (?:gain|gains)\b", prose)
    )
    curse_health = bool(
        re.search(
            r"\bcurs(?:e|es|ed|ing)\b"
            r"(?:(?!\b(?:drain(?:s|ed|ing)?|gains|gained|gaining|grant(?:s|ed|ing)?|"
            r"heal(?:s|ed|ing)?|stun(?:s|ned|ning)?|slow(?:s|ed|ing)?|attack(?:s|ed|ing)?)\b)[^.])"
            r"{0,35}\bhealth\b",
            prose,
        )
        or re.search(r"prevent(?:s|ing)?[^.]{0,35}\bheal(?:ing|s)?\b", prose)
    )
    has_curse = bool(re.search(r"\bcurs(?:e|es|ed|ing)\b", prose))
    if has_curse and re.search(r"\b(?:power and health|health and power) (?:gain|gains)\b", prose):
        curse_power = curse_health = True
    has_unqualified_curse = has_curse and not (curse_power or curse_health)
    if has_unqualified_curse:
        curse_power = curse_power or "CURSEPOWER" in specialties
        curse_health = curse_health or "CURSEHEALTH" in specialties
    if curse_power:
        found.add("CURSE_POWER")
    if curse_health:
        found.add("CURSE_HEALTH")
    if has_curse and re.search(r"\b(?:2|two) rounds\b", prose):
        found.add("CURSE_TWO_ROUNDS")

    ally_clause = bool(
        re.search(r"\bheal(?:s|ed|ing)?\b[^.]{0,40}\ball(?:ies|y)\b", prose)
        or re.search(r"\ball(?:ies|y)\b[^.]{0,45}\bheal(?:s|ed|ing)?\b", prose)
        or re.search(r"\b(?:grants?|gives?)\b[^.]{0,25}\ballies\b[^.]{0,30}\bhealth\b", prose)
        or re.search(r"\ballies (?:gain|gains|get|receive)[^.]{0,80}\bhealth\b", prose)
        or re.search(
            r"\b(?:grants?|gives?|gain(?:s|ed|ing)?)\b[^.]{0,80}\bhealth\b"
            r"[^.]{0,50}\b(?:to|for)?\s*(?:all )?allies\b",
            prose,
        )
    )
    if ally_clause:
        found.add("HEAL_ALLIES")
    sentences = [sentence.strip() for sentence in re.split(r"[.!?]+", prose) if sentence.strip()]
    first_ally_health = next(
        (
            i
            for i, sentence in enumerate(sentences)
            if re.search(r"\ball(?:y|ies)\b", sentence)
            and re.search(r"\b(?:heal|health)\w*\b", sentence)
        ),
        len(sentences),
    )
    self_heal = bool(
        re.search(r"\bself[- ]?heal|\bheals? (?:himself|herself|itself|self)\b", prose)
        or any(
            re.search(r"\bgain(?:s|ed|ing)?\b[^.]{0,30}\bhealth\b", sentence)
            and "allies" not in sentence
            for sentence in sentences
        )
        or any(
            re.search(r"\bheal(?:s|ed|ing)?\b", sentence)
            and not re.search(r"\ball(?:y|ies)\b", sentence)
            and not re.search(r"\b(?:prevent(?:s|ed|ing)?|negat(?:e|es|ed|ing))\b[^.]{0,30}\bheal", sentence)
            and (not ally_clause or i < first_ally_health)
            for i, sentence in enumerate(sentences)
        )
    )
    if self_heal:
        found.add("HEAL_SELF")

    ally_power = bool(
        re.search(r"\b(?:grants?|gives?)\b[^.]{0,30}\ballies\b[^.]{0,35}\bpower\b", prose)
        or re.search(
            r"\b(?:grants?|gives?)\b[^.]{0,30}\bpower\b[^.]{0,20}"
            r"\b(?:to|for|on|of)\s+(?:all )?allies\b",
            prose,
        )
        or re.search(r"\ballies (?:gain|gains|get|receive)[^.]{0,35}\bpower\b", prose)
        or re.search(r"\b(?:increases?|raises?)\b[^.]{0,30}\bpower of (?:all )?allies\b", prose)
        or re.search(r"\bpower gains? (?:all )?allies\b", prose)
        or re.search(
            r"\b(?:grants?|gives?|increases?|raises?|gain(?:s|ed|ing)?)\b[^.]{0,80}\bpower\b"
            r"[^.]{0,50}\b(?:to|for|on|of)\s+(?:all )?allies\b",
            prose,
        )
        or re.search(
            r"\b(?:all )?allies\b[^.]{0,80}\b(?:gain(?:s|ed|ing)?|receive(?:s|d)?|gets?|grants?|gives?)\b"
            r"[^.]{0,40}\bpower\b",
            prose,
        )
    )
    if ally_power:
        found.add("GAIN_POWER_ALLIES")
    self_power = any(
        re.search(
            r"(?:^|[,;]\s*|\band\s+)(?:then\s+)?"
            r"(?:gain(?:s|ed|ing)?|increas(?:e|es|ed|ing))\b[^.]{0,35}\bpower\b"
            r"(?!\s+(?:of|to|for|on)\s+(?:all\s+)?allies\b)|"
            r"\bchance[^.]{0,20}\bto gain\b[^.]{0,25}\bpower\b",
            sentence,
        )
        for sentence in sentences
    )
    if self_power:
        found.add("GAIN_POWER_SELF")

    enemy_attacks = re.findall(
        r"\battacks?\b(?!\s+(?:(?:all\s+)?allies|an ally|self)\b)", prose
    )
    if len(enemy_attacks) >= 2 or re.search(
        r"\battack(?:s|ed|ing)?\b[^.]{0,35}\bagain\b|\badditional attacks?\b|\bextra attacks?\b|"
        r"\bsecond attack\b|\bsecond time\b|"
        r"\battacks?[^.]{0,45}\b(?:twice|(?:[2-9]|10|two|three|four|five|six|eight|ten) times)\b|"
        r"\battacks?(?:\s+(?:[2-9]|10)x|[^.]{0,35}\b(?:enemies|target|row|column)\s+(?:[2-9]|10)x)\b|"
        r"\battacks?\s+(?:[2-9]|10|two|three|four|five|six|eight|ten)\s+"
        r"(?:random\s+)?(?:enemies|targets)\b|"
        r"\b(?:2nd|3rd|[4-9]th|second|third|fourth|fifth|sixth)\b[^.]{0,20}\battacks?\b|"
        r"\battacks?\s+up to (?:[2-9]|10)\b|\bup to (?:[2-9]|10)[^.]{0,30}\battacks?\b|"
        r"\bthen attacks?\b|"
        r"\battacks?[^.]{0,60}\bthen\s+(?:a|an|one|all|the|random)\b|"
        r"\bchance[^.]{0,35}\b(?:attack|hit)\b[^.]{0,25}\badditional target\b|"
        r"\battacks?\b[^.]*\.\s*[^.]{0,30}\bchance to attack\b|"
        r"\bchance (?:each )?to hit\b",
        prose,
    ):
        found.add("MULTIATTACK")
    if re.search(r"\bmark(?:s|ed|ing)?\b", prose):
        found.add("MARK")
    if re.search(r"\blevy\b", prose):
        found.add("LEVY")
    if re.search(r"\bassassinate\b", prose):
        found.add("ASSASSINATE")
    if re.search(
        r"\battacks? (?:(?:an|all) )?all(?:y|ies)\b|\bharms? (?:all )?allies\b|"
        r"\bdamages\s+(?:all\s+)?allies\b|"
        r"\bdamages\b[^.]{0,55}\band (?:all )?allies (?:for|at)\b|"
        r"\bhit\b[^.]{0,30}\ball(?:y|ies)\b|"
        r"\battack\b[^.]{0,45}\bincluding allies\b",
        prose,
    ):
        found.add("ATTACK_FRIENDLY")
    if re.search(
        r"\b(?:attacks?|damages?|harms?)\s+(?:himself|herself|itself|self)\b|"
        r"\bharms?\s+(?:himself|herself|itself|self)\b",
        prose,
    ):
        found.add("DAMAGE_SELF")

    return [name for name in MECHANIC_ORDER if name in found]
