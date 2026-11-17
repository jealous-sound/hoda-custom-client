#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
import sys
from collections.abc import Callable
from pathlib import Path
from typing import Any

import build_gear as gear
import build_intrinsic_passives as intrinsic

CLI_DESCRIPTION = (
    'Build data/hero_auras.json: the battle effects of each hero aura sentence, in the gear effect vocabulary, '
    'leaving out the Health and Power auras data/intrinsic_passives.json already applies'
)
ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / 'client-additional-data/characters.json'
CUSTOM_HEROES = ROOT / 'data/custom_heroes.json'
REFERENCE = ROOT / '.claude/skills/hoda-reference/data/characters.json'
PASSIVES = ROOT / 'data/intrinsic_passives.json'
OUTPUT = ROOT / 'data/hero_auras.json'
COVERAGE_FLOOR = 0.95

QUALITATIVE_PCT = {
    'very small': 5, 'tiny': 5, 'small': 15, 'moderate': 25, 'medium': 35, 'large': 50, 'great': 50, 'high': 50,
    'massive': 75, 'huge': 100, 'very large': 100,
}
QUALITATIVE_DEFENDER_PCT = {'small': 25, 'low': 25, 'medium': 50, 'large': 75}
UNSTATED_DEFENDER_PCT = 25
UNSTATED_AMOUNT = 'small'
ALL_EFFECT_RESISTS = ['stun_resist', 'slow_resist', 'drain_resist', 'curse_power_resist', 'curse_health_resist']
RESIST_PREFIXES = {
    'stun': ['stun_resist'], 'slow': ['slow_resist'], 'power drain': ['drain_resist'], 'drain': ['drain_resist'],
    'curse': ['curse_power_resist', 'curse_health_resist'], 'curse power': ['curse_power_resist'],
    'curse health': ['curse_health_resist'], 'all effect': ALL_EFFECT_RESISTS, 'effect': ALL_EFFECT_RESISTS,
    'damage': ['damage_resist'], 'all': [*ALL_EFFECT_RESISTS, 'damage_resist'],
}
BARE_RESIST_PREFIXES = {'stun', 'slow', 'power drain', 'drain', 'curse', 'curse power', 'curse health', 'all effect',
                        'effect'}
PLAIN_STATS = {
    'power': ['power'], 'health': ['health'], 'bonus damage': ['bonus_damage'], 'damage': ['bonus_damage'],
    '2x damage chance': ['crit'], 'defender': ['defender'],
}
IMMUNITY_STATS = {
    'stun': ['stun_resist'], 'stunned': ['stun_resist'], 'slow': ['slow_resist'], 'slowed': ['slow_resist'],
    'power drain': ['drain_resist'], 'drain': ['drain_resist'], 'curse': ['curse_power_resist', 'curse_health_resist'],
    'curse power': ['curse_power_resist'], 'curse health': ['curse_health_resist'],
}
IMMUNE_PCT = 100
VERSUS_STATS = {'bonus_damage', 'damage_resist', 'raw_damage_resist', 'power'}
SUBJECT_VERBS = r'(?:gain|heal|get|deal|resist)s?\b'
EXTRA_TERMS = {
    'heroes with stealth': [['specialty', 'STEALTH']], 'stealth heroes': [['specialty', 'STEALTH']],
    'power drainers': [['specialty', 'POWERDRAIN']], 'quick': [['speed', 'Quick']],
    'sentinel elves': [['tag', 'SentinelElves']], 'witches of the wild': [['tag', 'WitchOfTheWild']],
}
STAT_ORDER = ['health', 'power']
TRIGGER_PATTERNS = (
    ('any_death', r'(?:on|upon) any (?:hero )?death|when any hero dies|on (?:enemy or ally|ally or enemy) death'
                  r'|when (?:an )?(?:enemy or ally|ally or enemy) dies|when enemies or allies die'),
    ('any_hit', r'when any hero is hit'),
    ('ally_death', r'on ally death|when (?:an? |any )?all(?:y|ies) (?:dies|die|is defeated|are defeated)'),
    ('enemy_death', r'on enemy death|when (?:an? )?enem(?:y|ies) (?:dies|die|is killed|are killed)'),
    ('self_death', r'(?:on|upon) death|when (?:this hero|this unit|this banner|the hero|hero|it) dies'
                   r'|when killed|when it\'s killed'),
    ('ally_hit', r'on ally hit|when (?:an? |any )?all(?:y|ies) (?:is |are )?(?:hit|attacked)'),
    ('enemy_hit', r'on enemy hit|when (?:an? )?enem(?:y|ies) (?:is |are )?(?:hit|attacked)'),
    ('self_hit', r'when (?:this hero |this unit |this banner |the hero |hero )?(?:is )?(?:hit|attacked)|on hit'),
    ('ally_attack', r'when allies attack'),
    ('self_attack', r'on attack|when attacking'),
)
COMPOUND_TRIGGERS = {
    'any_death': ['ally_death', 'enemy_death'],
    'any_hit': ['self_hit', 'ally_hit', 'enemy_hit'],
}
REWRITES = (
    (r'\bvs\.', 'vs'),
    (r'(\d),(\d{3})', r'\1\2'),
    (r'\b(\d+)k damage\b', r'\g<1>000 damage'),
    (r'\bresistances?\b', 'resist'),
    (r'\bresists\b', 'resist'),
    (r'\b(?:all )?(?:status )?effects? resist\b', 'all effect resist'),
    (r'\b(?:critical|crit) chance\b', '2x damage chance'),
    (r'\bpower curse\b', 'curse power'),
    (r'\bhealth curse\b', 'curse health'),
    (r'\b(?:pd|powerdrain)\b', 'power drain'),
    (r'\s*\(self\)', ' for self'),
    (r'(?:an? )?(?:\w+ )?amount \((\d+)%\)', r'+\1%'),
    (r'\bretaliates? for (\d+)% damage', r'reverts \1% of damage received'),
    (r'\bheals and power gains allies (\d+)%', r'allies gain +\1% health and power'),
    (r'\bpower gains allies (\+?\d+)%', r'allies gain \1% power'),
    (r'\bpower drains enemies (\+?\d+)%', r'drains \1% power from enemies'),
    (r'\bdrains enemy power (\d+)%', r'drains \1% enemy power'),
    (r'\benemies lose (\d+)% (.+)$', r'-\1% \2 to enemies'),
    (r'\breduces enemy (.+?) by -?(\d+)%$', r'-\2% \1 to enemies'),
    (r'^enemy (.+?) reduced by (\d+)%$', r'-\2% \1 to enemies'),
    (r'^-(\d+)% enemy (\w+) power$', r'-\1% power to enemy \2'),
)
UNSUPPORTED = (
    r'less likely to attack .+', r'equal chance to attack .+', r'more likely to attack the front row',
    r'(?:much )?more likely to (?:stun|assassinate) .+', r'grants hatred of .+',
    r'(?:\w+ )?(?:allies|werewolves) (?:are )?more likely to (?:attack|target) .+',
    r'.+ for (?:each|every) .+', r'receives .+ per each .+',
)


Rows = list[dict[str, Any]]
Recipient = tuple[str, list | None, list | None, bool, bool]


class Resolver:
    def __init__(self, characters: list[dict], reference: dict[str, dict]):
        self.names = intrinsic._name_index(characters)
        self.families = intrinsic._family_index(characters, reference)
        self.gear = gear.TermResolver(characters)

    def part(self, text: str) -> list[list[str]] | str | None:
        key = intrinsic._normal(text)
        if key in {'ally', 'allies', 'all allies', 'squad', 'the squad', 'team', 'any ally'}:
            return 'allies'
        if key in {'self', 'this hero', 'this unit', 'personal'}:
            return 'self'
        if key in {'enemy', 'enemies', 'all enemies', 'the enemy'}:
            return 'enemies'
        if key in EXTRA_TERMS:
            return EXTRA_TERMS[key]
        terms = intrinsic._target_terms(text, self.names, self.families)
        if terms is not None and all(term['kind'] not in {'allies', 'self'} for term in terms):
            return [[term['kind'], term['value']] for term in terms]
        return self.gear.one(text.strip()) or self.gear.one(re.sub(r'e?s$', '', text.strip())) or None

    def recipients(self, text: str) -> tuple[str | None, list[list[str]]] | None:
        text = text.strip()
        enemy = re.match(r'^enemy\s+(.+)$', text) or re.match(r'^(.+?)\s+enemies$', text)
        scope = 'enemies' if enemy else None
        if enemy:
            text = enemy.group(1)
        terms: list[list[str]] = []
        for part in intrinsic._split_targets(text):
            resolved = self.part(part)
            if resolved is None:
                return None
            if isinstance(resolved, str):
                if scope not in (None, resolved):
                    return None
                scope = resolved
            else:
                terms.extend(resolved)
        return scope, terms

    def versus(self, text: str) -> list[list[str]] | None:
        resolved = self.recipients(re.sub(r'\s*\([^)]*\)', '', text))
        if resolved is None or resolved[0] not in (None, 'enemies') or not resolved[1]:
            return None
        return resolved[1]


def row(stat: str, value: int, on: str = 'battle', scope: str = 'self', who=None, vs=None, basis: str = 'base',
        each: bool = False) -> dict[str, Any]:
    vs = vs if stat in VERSUS_STATS else None
    return gear.effect(stat, value, on=on, scope=scope, who=who or None, vs=vs or None, basis=basis, each=each)


def normalize(sentence: str) -> str:
    text = gear.clean(sentence).lower().strip(' .,')
    text = text.replace('\\n', ' ').replace('–', '-')
    for pattern, replacement in REWRITES:
        text = re.sub(pattern, replacement, text)
    return re.sub(r'\s+', ' ', text).strip(' .,')


def qualitative(text: str) -> str:
    words = '|'.join(sorted(QUALITATIVE_PCT, key=len, reverse=True))
    text = re.sub(rf'\b(?:an? )?({words}) amount(?: of)?\b', lambda m: f'+{QUALITATIVE_PCT[m.group(1)]}%', text)
    text = re.sub(rf'\b(?:an? )?({words})(?= (?:power|health|heal|increase|stun|slow|curse)\b)',
                  lambda m: f'+{QUALITATIVE_PCT[m.group(1)]}%', text)
    text = re.sub(r'(\+\d+%) heal\b', r'heals \1', text)
    text = re.sub(r'^(\+\d+%) (power|health) gain to allies\b', r'allies gain \1 \2', text)
    text = re.sub(r'^(\+\d+%) (power|health|health and power) gain\b', r'gains \1 \2', text)
    text = re.sub(r'^(\+\d+%) increase to ally (.+)$', r'\1 \2 for allies', text)
    text = re.sub(r'^heals and gains (\+\d+%) power\b', r'heals \1 and gains \1 power', text)
    default = QUALITATIVE_PCT[UNSTATED_AMOUNT]
    text = re.sub(r'\b(gains?|allies gain|all allies gain) (power|health|health and power|power and health)\b'
                  r'(?! drain| resist| curse)', rf'\1 +{default}% \2', text)
    text = re.sub(r'\bdeals? bonus damage (to|vs)\b', rf'deals +{default}% bonus damage \1', text)
    return text


def find_trigger(text: str) -> tuple[str, str, bool]:
    for on, pattern in TRIGGER_PATTERNS:
        prefix = re.match(rf'^(?:{pattern}),?\s+(.+)$', text)
        if prefix:
            return on, prefix.group(1), True
        suffix = re.match(rf'^(.+?),?\s+(?:{pattern})$', text)
        if suffix:
            return on, suffix.group(1), False
    return 'battle', text, False


def take_trigger(text: str) -> tuple[str, str]:
    on, body, _ = find_trigger(text)
    return on, body


def expand(rows: Rows) -> Rows:
    return [dict(item, on=on) for item in rows for on in COMPOUND_TRIGGERS.get(item['on'], [item['on']])]


def strip_supplement(text: str) -> str:
    return re.sub(r'^(?:and )?(?:an? )?(?:extra|additional|another)\s+', '', text.strip())


class Parser:
    def __init__(self, resolver: Resolver):
        self.resolver = resolver

    def recipient(self, tail: str, value: int) -> Recipient | None:
        each = bool(re.search(r'\((?:all )?(?:stacking|bonuses stack)\)', tail))
        tail = re.sub(r'\s*\([^)]*\)', '', tail).strip(' ,')
        versus = None
        match = re.match(r'^(.+?) (?:vs|against) (.+)$', tail)
        if match:
            tail, versus = match.group(1), self.resolver.versus(match.group(2))
            if versus is None:
                return None
        resolved = self.resolver.recipients(tail)
        if resolved is None:
            return None
        scope, who = resolved
        mixed = scope in ('allies', 'self') and bool(who)
        if scope is None:
            scope = 'enemies' if value < 0 else 'allies'
        return scope, who or None, versus, each, mixed

    def heads(self, head: str) -> list[tuple[int, list[str]]] | None:
        parts = [part.strip() for part in re.split(r',\s*(?:and\s+)?|\s+and\s+', head) if part.strip()]
        flat: list[list[Any]] = []
        for part in parts:
            immunity = re.fullmatch(r'(.+?) immunity', part)
            if immunity and immunity.group(1) in IMMUNITY_STATS:
                part = f'+{IMMUNE_PCT}% {immunity.group(1)} resist'
            match = re.fullmatch(r'([+-]?\d+)(%?)(?![x\d])\s*(.*)', part)
            if match:
                flat.append([int(match.group(1)), match.group(2) == '%', match.group(3).strip(), True])
            elif flat:
                flat.append([flat[-1][0], flat[-1][1], part, False])
            else:
                return None
        resist_seen = False
        pending = False
        stats_of: list[list[str]] = []
        for value, percent, word, opens in reversed(flat):
            word = re.sub(r'^(?:ally |bonus (?=health|power))', '', word).strip()
            resist = re.fullmatch(r'(.+?) resist', word)
            if resist and resist.group(1) in RESIST_PREFIXES:
                found = RESIST_PREFIXES[resist.group(1)]
                resist_seen = pending = True
            elif resist_seen and word in BARE_RESIST_PREFIXES:
                found = RESIST_PREFIXES[word]
            elif pending and word == 'damage':
                found = RESIST_PREFIXES[word]
            elif word in PLAIN_STATS:
                found = PLAIN_STATS[word]
            else:
                return None
            if not percent:
                if found != ['damage_resist']:
                    return None
                found = ['raw_damage_resist']
            stats_of.insert(0, found)
            if opens:
                pending = False
        result: list[tuple[int, list[str]]] = []
        for (value, _, _, opens), stats in zip(flat, stats_of):
            if opens or not result:
                result.append((value, list(stats)))
            else:
                result[-1][1].extend(stats)
        seen: set[str] = set()
        for _, stats in result:
            if seen & set(stats):
                return None
            seen |= set(stats)
        return result

    def stat_list(self, text: str, on: str, scope: str = 'self', who=None) -> Rows | None:
        text = text.strip()
        split = re.search(r'\s(?:for|to|vs|against)\s', text)
        head, tail = (text[:split.start()], text[split.start() + 1:]) if split else (text, None)
        each = bool(re.search(r'\((?:all )?stacking\)', head))
        head = re.sub(r'\s*\((?:all )?stacking\)', '', head)
        if re.search(r'\bally\b', head):
            scope = 'allies'
        values = self.heads(head)
        if not values or len({value < 0 for value, _ in values}) > 1:
            return None
        vs = None
        mixed = False
        if tail:
            tail = re.sub(r'^(?:for|to)\s+', '', tail)
            against = re.fullmatch(r'(?:vs|against) (.+?)(?: for (.+))?', tail)
            if against:
                vs = self.resolver.versus(against.group(1))
                if vs is None:
                    return None
                each = each or 'stacking' in tail
                if against.group(2):
                    found = self.recipient(against.group(2), values[0][0])
                    if found is None:
                        return None
                    scope, who, _, each_tail, mixed = found
                    each = each or each_tail
            else:
                found = self.recipient(tail, values[0][0])
                if found is None:
                    return None
                scope, who, vs, each_tail, mixed = found
                each = each or each_tail
        elif scope == 'self' and values[0][0] < 0:
            scope = 'enemies'
        rows: Rows = []
        for value, stats in values:
            for stat in stats:
                if mixed:
                    rows.append(row(stat, value, on=on, scope=scope, vs=vs))
                    rows.append(row(stat, value, on=on, scope='allies', who=who, vs=vs, each=each))
                else:
                    rows.append(row(stat, value, on=on, scope=scope, who=who, vs=vs, each=each))
        return rows

    def grouped(self, group: str, body: str, on: str) -> Rows | None:
        resolved = self.resolver.recipients(group)
        if resolved is None or resolved[0] is not None or not resolved[1]:
            return None
        rows = self.stat_list(body, on, 'allies', resolved[1])
        if rows is None or any(item.get('who') != resolved[1] for item in rows):
            return None
        return rows

    def group_heal(self, group: str, value: str, on: str) -> Rows | None:
        resolved = self.resolver.recipients(group)
        if resolved is None or resolved[0] is not None or not resolved[1]:
            return None
        return [row('health', int(value), on=on, scope='allies', who=resolved[1])]

    def subject(self, text: str, on: str) -> Rows | None:
        forms: list[tuple[str, Callable[[re.Match[str]], Rows | None]]] = [
            (r'(?:all )?allies (?:gain|get|receive) (.+)', lambda m: self.stat_list(m.group(1), on, 'allies')),
            (r'(?:grants?|gives?) (?:all )?allies (.+)', lambda m: self.stat_list(m.group(1), on, 'allies')),
            (r'squad gets (.+)', lambda m: self.stat_list(m.group(1), on, 'allies')),
            (r'(?:gains?|receives?|gets?|personal) (.+)', lambda m: self.stat_list(m.group(1), on)),
            (r'heals? (?:all )?(?:living )?allies (?:for )?\+?(\d+)%',
             lambda m: [row('health', int(m.group(1)), on=on, scope='allies')]),
            (r'(?:all )?allies heal \+?(\d+)%', lambda m: [row('health', int(m.group(1)), on=on, scope='allies')]),
            (r'heals? (?:for )?\+?(\d+)%', lambda m: [row('health', int(m.group(1)), on=on)]),
            (r'heals? (.+?) (?:allies )?\+?(\d+)%', lambda m: self.group_heal(m.group(1), m.group(2), on)),
            (r'(.+?) (?:allies )?heal \+?(\d+)%', lambda m: self.group_heal(m.group(1), m.group(2), on)),
            (r'(?:grants?|gives?) (.+?) allies (.+)', lambda m: self.grouped(m.group(1), m.group(2), on)),
            (r'(?:grants?|gives?) (.+?) (?:to|for) (.+)',
             lambda m: self.stat_list(f'{m.group(1)} for {m.group(2)}', on)),
            (r'(.+?) (?:allies )?(?:gains?|gets?) (.+)', lambda m: self.grouped(m.group(1), m.group(2), on)),
        ]
        for pattern, build in forms:
            match = re.fullmatch(pattern, text)
            if match:
                rows = build(match)
                if rows is not None:
                    return rows
        return None

    def defender(self, text: str) -> Rows | None:
        condition = r'(?: (?:when|if) (?:also )?attacked(?: also)?)?'
        match = re.fullmatch(rf'(?:\+?(\d+)% defender|absorbs \+?(\d+)% (?:of )?(?:aoe|area) damage(?: for allies)?{condition}'
                             rf'|absorbs (?:(small|low|medium|large) )?aoe damage for allies{condition}|defender)', text)
        if not match:
            return None
        amount = match.group(1) or match.group(2)
        if amount is None:
            amount = QUALITATIVE_DEFENDER_PCT[match.group(3)] if match.group(3) else UNSTATED_DEFENDER_PCT
        return [row('defender', int(amount))]

    def immunity(self, words: str, scope: str, who, on: str) -> Rows | None:
        rows: Rows = []
        for word in re.split(r',\s*(?:and\s+)?|\s+and\s+', words):
            if word.strip() not in IMMUNITY_STATS:
                return None
            rows.extend(row(stat, IMMUNE_PCT, on=on, scope=scope, who=who) for stat in IMMUNITY_STATS[word.strip()])
        return rows

    def immunities(self, text: str, on: str) -> Rows | None:
        match = re.fullmatch(r'(?:immune to|immunity to) (.+)', text) or re.fullmatch(r'cannot be (stunned|slowed)', text)
        if match:
            return self.immunity(match.group(1), 'self', None, on)
        match = re.fullmatch(r'(?:grants? |gives? )?(.+?) immunity (?:for|to) (.+)', text)
        if match:
            target = self.recipient(match.group(2), 1)
            if target is None:
                return None
            scope, who, _, _, mixed = target
            rows = self.immunity(match.group(1), scope if not mixed else 'allies', who, on)
            if rows is not None and mixed:
                rows += self.immunity(match.group(1), scope, None, on) or []
            return rows
        match = re.fullmatch(r'(?:grants? |gives? )?(.+?) immunity', text)
        return self.immunity(match.group(1), 'self', None, on) if match else None

    def damage_resists(self, text: str, on: str) -> Rows | None:
        match = re.fullmatch(r'(?:(.+?) )?(?:resists?|resist) (?:the first )?(\d+) (?:points of )?damage'
                             r'(?: per hit)?(, double for self)?', text)
        if match:
            amount = int(match.group(2))
            if not match.group(1):
                rows = [row('raw_damage_resist', amount)]
            else:
                target = self.recipient(match.group(1), 1)
                if target is None:
                    return None
                scope, who, _, _, mixed = target
                rows = [row('raw_damage_resist', amount, scope='allies' if mixed else scope, who=who)]
                if mixed:
                    rows.append(row('raw_damage_resist', amount, scope=scope))
            if match.group(3):
                rows.append(row('raw_damage_resist', amount))
            return rows
        match = re.fullmatch(r'takes \+?(\d+)% less damage(?: (?:from|vs) (.+))?', text)
        if match:
            versus = self.resolver.versus(match.group(2)) if match.group(2) else []
            return None if versus is None else [row('damage_resist', int(match.group(1)), on=on, vs=versus)]
        match = re.fullmatch(r'(?:(.+?) )?allies take \+?(\d+)% less damage', text)
        if match:
            who = None
            if match.group(1):
                resolved = self.resolver.recipients(match.group(1))
                if resolved is None or resolved[0] is not None:
                    return None
                who = resolved[1]
            return [row('damage_resist', int(match.group(2)), on=on, scope='allies', who=who)]
        match = re.fullmatch(r'(.+?) take \+?(\d+)% less damage', text)
        if match:
            target = self.recipient(match.group(1), 1)
            return None if target is None else [
                row('damage_resist', int(match.group(2)), on=on, scope=target[0], who=target[1])]
        match = re.fullmatch(r'deals? (\d+)% more and takes (\d+)% less damage vs (.+)', text)
        if match:
            versus = self.resolver.versus(match.group(3))
            return None if versus is None else [row('bonus_damage', int(match.group(1)), vs=versus),
                                                 row('damage_resist', int(match.group(2)), vs=versus)]
        return None

    def bonus_damage(self, text: str, on: str) -> Rows | None:
        match = re.fullmatch(r'(?:all )?allies deal \+?(\d+)% (?:bonus )?damage (?:vs|to|against) (.+)', text)
        if match:
            return self.stat_list(f'+{match.group(1)}% bonus damage vs {match.group(2)} for allies', on)
        match = re.fullmatch(r'deals? \+?(\d+)% (?:bonus |more )?damage (?:vs|to|against) (.+)', text)
        if match:
            return self.stat_list(f'+{match.group(1)}% bonus damage vs {match.group(2)}', on)
        match = re.fullmatch(r'(.+?) deal \+?(\d+)% (?:bonus )?damage (?:vs|to|against) (.+)', text)
        if match:
            return self.stat_list(f'+{match.group(2)}% bonus damage vs {match.group(3)} for {match.group(1)}', on)
        match = re.fullmatch(r'(\+?\d+% (?:bonus )?damage) for, and against, (.+)', text)
        if match:
            first = self.stat_list(f'{match.group(1)} for {match.group(2)}', on)
            second = self.stat_list(f'{match.group(1)} vs {match.group(2)} for allies', on)
            return None if first is None or second is None else first + second
        return None

    def reaction(self, text: str, on: str) -> Rows | None:
        match = re.fullmatch(r'reverts? \+?(\d+)%(?: (?:of )?damage(?: (?:received|taken))?)?'
                             r'(?: by (allies|any hero))?(?: to (?:all )?(.+))?', text)
        if match:
            trigger = on if on != 'battle' else {'allies': 'ally_hit', 'any hero': 'any_hit'}.get(
                match.group(2), 'self_hit')
            who = None
            if match.group(3) and match.group(3) not in ('enemies', 'enemy'):
                target = self.recipient(match.group(3), -1)
                if target is None or target[0] != 'enemies':
                    return None
                who = target[1]
            return [row('damage', int(match.group(1)), on=trigger, scope='enemies', who=who, basis='damage')]
        match = re.fullmatch(r'(?:power )?drains? \+?(\d+)% (?:enemy power|power(?: from (?:all )?enemies)?)', text)
        if match:
            return [row('drain', int(match.group(1)), on=on, scope='enemies')]
        match = re.fullmatch(r'(?:all )?(enemies|allies) (?:take|receive) \+?(\d+)% (?:of )?damage (?:dealt|received)',
                             text)
        if match:
            return [row('damage', int(match.group(2)), on=on, scope=match.group(1), basis='damage')]
        return None

    def conversion(self, text: str, on: str) -> Rows | None:
        match = re.fullmatch(
            r'(?:(allies) )?converts? \+?(\d+)% (?:of )?(all )?damage (taken|received|dealt or received|dealt|to allies)'
            r'(?: (?:by|to) (allies|any hero|self|enemies by self|enemies))?(?: into| to| as) (ally |self |allies )?'
            r'(power and health|health and power|power|health)(?: for (.+))?', text)
        if not match:
            return None
        whose, value, everyone, kind, by, owner, stats, target = match.groups()
        triggers = {
            ('taken', None): 'self_hit', ('received', None): 'self_hit', ('taken', 'allies'): 'ally_hit',
            ('received', 'allies'): 'ally_hit', ('to allies', None): 'ally_hit', ('received', 'any hero'): 'any_hit',
            ('dealt', 'allies'): 'ally_attack', ('dealt', 'enemies by self'): 'self_attack',
            ('dealt', 'enemies'): 'self_attack', ('dealt', None): 'ally_attack' if everyone else 'self_attack',
            ('dealt or received', 'allies'): 'ally_attack+ally_hit', ('dealt or received', None): 'self_attack+self_hit',
        }
        trigger = triggers.get((kind, by))
        if trigger is None:
            return None
        if whose == 'allies':
            trigger = trigger.replace('self_', 'ally_')
        scope, who = ('allies' if owner in ('ally ', 'allies ') else 'self'), None
        if target:
            resolved = self.recipient(target, 1)
            if resolved is None:
                return None
            scope, who = resolved[0], resolved[1]
        rows = [row(stat, int(value), on=on_each, scope=scope, who=who, basis='damage')
                for on_each in trigger.split('+')
                for stat in sorted(re.findall(r'power|health', stats), key=STAT_ORDER.index)]
        return expand(rows)

    def targeting(self, text: str) -> Rows | None:
        scope = 'self'
        squad = re.match(r'^(?:squad|allies) ((?:more likely to attack|usually attack).*)$', text)
        if squad:
            scope, text = 'allies', squad.group(1)
        text = re.sub(r'^(?:much |very )?(?:more )?likely to (?:attack|target)', 'more likely to attack', text)
        fixed = [
            (r'(?:more likely to attack|usually attacks?) (?:the )?back row', 'target_back_row'),
            (r'more likely to attack (?:the )?(?:highest power|strongest|most powerful) (?:enemy|target)',
             'target_highest_power'),
            (r'more likely to attack (?:the )?(?:enemy with )?lowest (?:% )?health(?: target| enemy)?',
             'target_lowest_health'),
            (r'more likely to attack (?:the )?quickest enemy', 'target_quick'),
            (r'more likely to attack (?:the )?slowest enemy', 'target_slowest'),
            (r'more likely to attack first(?: within initiative)?', 'haste'),
            (r'(?:blocks enemies from flanking (?:the )?back row|reduces enemy chance to flank)', 'flank_reduction'),
        ]
        for pattern, stat in fixed:
            if re.fullmatch(pattern, text):
                return [row(stat, 1, scope='allies' if stat == 'flank_reduction' else scope)]
        match = re.fullmatch(r'more likely to attack (.+)', text)
        if match and scope == 'self':
            versus = self.resolver.versus(re.sub(r'\s+heroes$', '', match.group(1)))
            if versus:
                return [row('target_match', 1, who=versus)]
        return None

    def special(self, text: str, on: str) -> Rows | None:
        if re.fullmatch(r'perceptive(?::.*)?', text):
            return [row('perceptive', 1)]
        if re.fullmatch(r'stealth(?::.*)?', text):
            return [row('stealth', 1)]
        for handler in (self.defender,):
            rows = handler(text)
            if rows is not None:
                return rows
        for handler in (self.immunities, self.damage_resists, self.bonus_damage, self.reaction, self.conversion):
            rows = handler(text, on)
            if rows is not None:
                return rows
        return self.targeting(text)

    def clause(self, text: str, on: str) -> Rows | None:
        text = strip_supplement(text.strip(' ,'))
        for attempt in (self.special, self.subject, self.stat_list):
            rows = attempt(text, on)
            if rows is not None:
                return expand(rows)
        return None

    def modifier(self, text: str, previous: Rows, on: str | None) -> Rows | None:
        text = strip_supplement(text)
        immune = re.fullmatch(r'immunity (?:for|to) (.+)', text)
        double = re.fullmatch(r'double (?:for|to|vs) (.+)', text)
        if immune or double:
            value = IMMUNE_PCT if immune else None
            tail = (immune or double).group(1)
            versus = bool(double) and text.startswith('double vs')
        else:
            match = re.fullmatch(r'([+-]?\d+)%(?: (?:extra|each|bonus))*(?: ((?:for|to|vs) .+))?', text)
            if not match:
                return None
            value = int(match.group(1))
            tail = re.sub(r'^(?:for|to) ', '', match.group(2) or '')
            versus = tail.startswith('vs ')
            tail = re.sub(r'^vs ', '', tail)
        base = previous[0]
        scope, who, vs, each = base['scope'], base.get('who'), base.get('vs'), bool(base.get('each'))
        if versus:
            vs = self.resolver.versus(tail)
            if vs is None:
                return None
        elif tail:
            target = self.recipient(tail, -1 if base['scope'] == 'enemies' else 1)
            if target is None:
                return None
            scope, who, vs_tail, each, _ = target
            vs = vs_tail or base.get('vs')
            if base['scope'] == 'enemies' and who:
                scope = 'enemies'
        seen: set[tuple] = set()
        rows: Rows = []
        for item in previous:
            key = (item['stat'], item['on'], item['basis'])
            if key in seen:
                continue
            seen.add(key)
            amount = item['value'] if value is None else (value if item['value'] > 0 else -abs(value))
            rows.append(row(item['stat'], amount, on=on or item['on'], scope=scope, who=who, vs=vs,
                            basis=item['basis'], each=each))
        return rows

    def continued(self, text: str, on: str, previous: Rows | None, subject: str) -> Rows | None:
        if subject and re.match(rf'^{SUBJECT_VERBS}', text):
            text = subject + text
        if (previous and re.match(r'^[+-]?\d+%', text) and not re.search(r'\s(?:for|to|vs|against)\s', text)
                and not re.search(r'\ball(?:y|ies)\b', text)):
            base = previous[0]
            if base['scope'] != 'self' or base.get('who'):
                rows = self.stat_list(text, on, base['scope'], base.get('who'))
                if rows is not None:
                    return expand(rows)
        found = self.clause(text, on)
        if found is None and previous:
            found = self.modifier(text, previous, on if on != 'battle' else None)
        return found

    def piece(self, text: str, on: str, previous: Rows | None, subject: str) -> Rows | None:
        found = self.continued(text, on, previous, subject)
        if found is None:
            sub_on, sub_text = take_trigger(text)
            if sub_on != 'battle':
                found = self.continued(sub_text, sub_on, previous, subject)
                if found is not None:
                    found = expand(found)
        return found

    def body(self, text: str, on: str) -> Rows | None:
        rows = self.clause(text, on)
        if rows is not None:
            return rows
        subject_match = re.match(rf'^((?:all )?allies |(?!(?:gain|heal|get|deal|resist))[a-z\' ]+? (?={SUBJECT_VERBS}))',
                                 text)
        subject = subject_match.group(1) if subject_match else ''
        pieces = re.split(r'(,\s*(?:and\s+)?|\s+and\s+)', text)
        tokens, joins = pieces[0::2], pieces[1::2] + ['']
        result: Rows = []
        previous: Rows | None = None
        start = 0
        while start < len(tokens):
            for end in range(len(tokens), start, -1):
                piece = ''.join(tokens[i] + (joins[i] if i < end - 1 else '') for i in range(start, end))
                found = self.piece(piece, on, previous, subject)
                if found is not None:
                    result.extend(found)
                    previous, start = found, end
                    break
            else:
                return None
        return result

    def sentence(self, sentence: str) -> Rows | None:
        text = qualitative(normalize(sentence))
        if not text:
            return []
        on, body, leading = find_trigger(text)
        if on == 'battle':
            return self.body(text, 'battle')
        attempts = [(body, on), (text, 'battle')] if leading or ',' not in body else [(text, 'battle'), (body, on)]
        for attempt_text, attempt_on in attempts:
            rows = self.body(attempt_text, attempt_on)
            if rows is not None:
                return rows
        return None


def unsupported(sentence: str) -> bool:
    text = normalize(sentence)
    return any(re.fullmatch(pattern, text) for pattern in UNSUPPORTED)


def sentences_of(text: str) -> list[str]:
    text = re.sub(r'\bvs\.', 'vs', text or '', flags=re.I)
    if not re.search(r'<color=', text, re.I):
        return [part.strip(' .') for part in re.split(r'[.!?]+', gear.clean(text)) if part.strip(' .')]
    return [sentence for color in ('green', 'red', 'orange') for sentence in intrinsic._colored_sentences(text, color)]


def applied_passives(passives: list[dict]) -> dict[str, set]:
    applied: dict[str, set] = {'stat': set(), 'bonus_damage': set(), 'kinds': set()}
    for effect in passives:
        applied['kinds'].add(effect['kind'])
        if effect['kind'] in ('stat_percent', 'stat_percent_per_matching_ally'):
            applied['stat'].update((stat, abs(int(effect['value_pct']))) for stat in effect['stats'])
        if effect['kind'] == 'bonus_damage_percent':
            applied['bonus_damage'].add(abs(int(effect['value_pct'])))
    return applied


def already_applied(item: dict, applied: dict[str, set]) -> bool:
    stat, value, on = item['stat'], abs(item['value']), item['on']
    if on == 'battle' and stat in ('health', 'power') and not item.get('vs'):
        return (stat, value) in applied['stat']
    if on == 'battle' and stat in ('power', 'bonus_damage') and item.get('vs'):
        return value in applied['bonus_damage']
    if stat == 'health' and on == 'enemy_death' and item['scope'] == 'self':
        return 'heal_percent_max_health' in applied['kinds']
    if stat == 'target_lowest_health':
        return 'outgoing_target_preference' in applied['kinds']
    return False


def runtime_effect(item: dict) -> dict:
    if item['stat'] == 'power' and item.get('vs'):
        return dict(item, stat='bonus_damage')
    return item


def build() -> tuple[dict, list[str], int]:
    characters = json.loads(CATALOG.read_text(encoding='utf-8'))['characters']
    custom = json.loads(CUSTOM_HEROES.read_text(encoding='utf-8'))['characters']
    reference = json.loads(REFERENCE.read_text(encoding='utf-8'))
    passives = {row['id']: row['effects'] for row in json.loads(PASSIVES.read_text(encoding='utf-8'))['characters']}
    parser = Parser(Resolver(characters, reference))
    items: list[dict] = []
    unparsed: list[str] = []
    skipped: list[str] = []
    counted = 0
    for character in sorted([*characters, *custom], key=lambda c: c['id']):
        applied = applied_passives(passives.get(character.get('based_on') or character['id'], []))
        effects: Rows = []
        for sentence in sentences_of(character.get('equipped_rune_text', '')):
            if unsupported(sentence):
                skipped.append(f'{character["id"]}: {sentence}')
                continue
            counted += 1
            rows = parser.sentence(sentence)
            if rows is None:
                unparsed.append(f'{character["id"]}: {sentence}')
                continue
            effects.extend(runtime_effect(item) for item in rows if not already_applied(item, applied))
        if effects:
            items.append({'id': character['id'], 'aura_key': character.get('equipped_rune', ''), 'effects': effects})
    return {
        'version': 1,
        'source': 'equipped_rune_text of client-additional-data/characters.json and data/custom_heroes.json',
        'conventions': {
            'effects': 'data/gear.json effect rows; the battle sim fires them as one more gear source on the hero',
            'left_to_intrinsic_passives': 'battle-start Health/Power and Power-vs clauses it already applies',
            'qualitative_pct': QUALITATIVE_PCT,
            'qualitative_defender_pct': QUALITATIVE_DEFENDER_PCT,
            'unstated_defender_pct': UNSTATED_DEFENDER_PCT,
            'unscoped_negative': 'applies to enemies, as data/intrinsic_passives.json reads red aura text',
        },
        'unsupported': skipped,
        'unparsed': unparsed,
        'items': items,
    }, unparsed, counted


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument('--check', action='store_true', help='fail when data/hero_auras.json is out of date')
    parser.add_argument('--verbose', action='store_true', help='list every sentence left unparsed')
    args = parser.parse_args()
    catalog, unparsed, counted = build()
    text = json.dumps(catalog, ensure_ascii=False, indent=1) + '\n'
    coverage = 1.0 - len(unparsed) / counted if counted else 1.0
    if args.verbose:
        for line in unparsed:
            print(line)
    print(f'{len(catalog["items"])} heroes, {counted} sentences, {coverage:.1%} parsed, '
          f'{len(catalog["unsupported"])} beyond the battle sim')
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding='utf-8') != text:
            print(f'{OUTPUT.relative_to(ROOT)} is out of date; run tools/build_hero_auras.py', file=sys.stderr)
            return 1
    else:
        OUTPUT.write_text(text, encoding='utf-8')
    return 0 if coverage >= COVERAGE_FLOOR else 1


if __name__ == '__main__':
    sys.exit(main())
