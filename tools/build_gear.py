#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
from pathlib import Path

CLI_DESCRIPTION = 'Build data/gear.json from the 5.4.4 gear reference and copy gear icons into extra/'
ROOT = Path(__file__).resolve().parents[1]
GEAR_DEFS = ROOT / 'client-additional-data/HoDA_character_info/GearData.json'
LOCALIZATION = ROOT / 'client-additional-data/HoDA_character_info/Localization_ENG.json'
CHARACTERS = ROOT / 'client-additional-data/characters.json'
REFERENCE = ROOT / '.claude/skills/hoda-reference'
REFERENCE_GEAR = REFERENCE / 'data/gear.json'
REFERENCE_ICONS = REFERENCE / 'references/gears'
OUTPUT = ROOT / 'data/gear.json'
EXTRA_ICON_DIR = ROOT / 'client-additional-data/extra/gui/gear'
EXTRA_ICON_REF = 'gui/gear/'
CATEGORY_TYPES = {0: 'faction', 1: 'group', 4: 'specialty', 5: 'initiative'}
FACTION_NAMES = {'WHITE': 'White', 'BLACK': 'Black', 'RED': 'Red', 'BLUE': 'Blue'}
INITIATIVE_SPEEDS = {'FAST': 'Quick', 'MEDIUM': 'Normal', 'SLOW': 'Slow'}
RARITY_GOLD_VALUES = [50, 200, 800, 3000, 10000, 30000]
PRESTIGE_SLOT_GEM_COST = 100
COVERAGE_FLOOR = 0.99
RUSSIAN_LOCALIZATION = REFERENCE / 'data/localization/Loc_RUS_RU.json'
TEXT_KEYS = [
    'GEAR_EQUIP_EMPTY_CONFIRM', 'GEAR_EQUIP_FILLED_CONFIRM', 'GEAR_EQUIP_FAIL_CATEGORY_LIMIT_CONFIRM',
    'GEAR_EQUIP_FAIL_DUPLICATE_CONFIRM', 'GEAR_EQUIP_FAIL_DEFENDER_CONFIRM', 'GEAR_EQUIP_FAIL_INVALID_PRESTIGE_GEAR',
    'GEAR_SELL_BUTTON_TEXT', 'GEAR_EQUIP_BUTTON_TEXT', 'GEAR_SELL_MAIN_CONFIRM', 'GEAR_SELL_RECEIVE_CONFIRM',
    'GEAR_PRESTIGE_MESSAGE', 'GEAR_PRESTIGE_UNLOCK', 'GEAR_EMPTY_INVENTORY_MESSAGE', 'GEAR_FILTER_ALL_TITLE', 'GEAR_FILTER_EQUIPPABLE_TITLE',
    'GEAR_NO_LIMITATIONS', 'GEAR_LIMITED_TITLE', 'GEAR_REVEAL_WINDOW_TITLE', 'GEAR_SELECT_HERO_HELP', 'GEAR_INVENTORY_FOOTER_TEXT', 'GEAR_RARITY_COMMON', 'GEAR_RARITY_UNCOMMON',
    'GEAR_RARITY_RARE', 'GEAR_RARITY_EPIC', 'GEAR_RARITY_LEGENDARY', 'GEAR_RARITY_MYTHIC',
]
TOME_LOCAL_NAME = 'Том вознесения'
UI_TEXTURE_DIR = REFERENCE / 'references/game-assets/textures'
UI_TEXTURES = (
    'tex.tomeofascension', 'tex.gear_frame_common', 'tex.gear_frame_uncommon', 'tex.gear_frame_rare',
    'tex.gear_frame_epic', 'tex.gear_frame_legendary', 'tex.gear_frame_mythic', 'Gear_Details_Box',
    'Gear_Slot_Add_Deselect', 'Gear_Empty_Slot', 'tex.Ico_Gear_Slot_Deselected',
    'tex.Ico_Gear_TomeofAscension_Deselected',
)
LOCAL_FILTER_NAMES = {
    'GEAR_SHOW_PREVIOUS': 'Предыдущие {0}', 'GEAR_SHOW_NEXT': 'Следующие {0}',
    'EVO_TOME_SLOT_LOCKED_MESSAGE': 'Заполните все ячейки снаряжения, чтобы открыть том вознесения.',
    'EVO_TOME_REQUIRED_MESSAGE': 'Нужен том вознесения. Его можно купить в магазине.',
    'EVO_TOME_USE_CONFIRM': 'Использовать том вознесения, чтобы повысить героя до мифической редкости?',
    'EVO_TOME_EXPLAIN_DESC': 'Мифические герои получают <color=#ffa500ff>+5 к макс. уровню</color> и '
                             '<color=#ffa500ff>доп. ячейку снаряжения</color>!',
    'EVO_TOME_COUNT': 'Томов: {0}',
    'RARITY_MYTHIC': 'Мифический',
    'ROSTER_FILTER_COMMON': 'Обычные', 'ROSTER_FILTER_UNCOMMON': 'Необычные', 'ROSTER_FILTER_RARE': 'Редкие',
    'ROSTER_FILTER_EPIC': 'Эпические', 'ROSTER_FILTER_LEGENDARY': 'Легендарные', 'ROSTER_FILTER_MYTHIC': 'Мифические',
}
GEAR_PACKS = [
    {'id': 'peasant', 'loc': 'PEASANT_GEAR_PACK_NAME', 'currency': 'gold', 'price': 2000, 'count': 1, 'min_rarity': 1,
     'max_rarity': 2},
    {'id': 'novice', 'loc': 'NOVICE_GEAR_PACK_NAME', 'currency': 'gold', 'price': 6000, 'count': 1, 'min_rarity': 1,
     'max_rarity': 3},
    {'id': 'journeyman', 'loc': 'JOURNEYMAN_GEAR_PACK_NAME', 'currency': 'gold', 'price': 20000, 'count': 2,
     'min_rarity': 2, 'max_rarity': 3},
    {'id': 'merchant', 'loc': 'MERCHANT_GEAR_PACK_NAME', 'currency': 'gems', 'price': 50, 'count': 3, 'min_rarity': 2,
     'max_rarity': 4},
    {'id': 'masterwork', 'loc': 'MASTERWORK_GEAR_PACK_NAME', 'currency': 'gems', 'price': 150, 'count': 3,
     'min_rarity': 3, 'max_rarity': 5},
    {'id': 'armory', 'loc': 'ARMORY_GEAR_PACK_NAME', 'currency': 'gems', 'price': 400, 'count': 5, 'min_rarity': 4,
     'max_rarity': 6},
    {'id': 'tome', 'loc': 'TOME_OF_ASCENSION_LOCAL_NAME', 'currency': 'gems', 'price': 500, 'count': 1, 'min_rarity': 6,
     'max_rarity': 6, 'grants_tome': True},
]

GROUP_NAMES = {
    'apostate': 'Apostate', 'apostates': 'Apostate', 'avvar': 'Avvar', 'beast': 'Beast', 'beasts': 'Beast',
    'creature': 'Beast', 'creatures': 'Beast', 'blight': 'Blight', 'blighted': 'Blight', 'blood mage': 'Blood_Mage',
    'blood mages': 'Blood_Mage', 'chantry': 'Chantry', 'circle': 'Circle', 'demon': 'Demon', 'demons': 'Demon',
    'dragon': 'Dragon', 'dragons': 'Dragon', 'dwarf': 'Dwarf', 'dwarves': 'Dwarf', 'elf': 'Elves', 'elves': 'Elves',
    'elven': 'Elves', 'ferelden': 'Ferelden', 'golem': 'Golem', 'golems': 'Golem', 'inquisition': 'Inquisition',
    'legion of the dead': 'LegionOfTheDead', 'mage': 'Mage', 'mages': 'Mage', 'nobility': 'Nobility',
    'orlesian': 'Orlesian', 'orlesians': 'Orlesian', 'outlaw': 'Outlaw', 'outlaws': 'Outlaw', 'qunari': 'Qunari',
    'red lyrium': 'Red_Lyrium', 'rogue': 'Rogue', 'rogues': 'Rogue', 'spirit': 'Spirit', 'spirits': 'Spirit',
    'templar': 'Templar', 'templars': 'Templar', 'tevinter': 'Tevinter', 'venatori': 'Venatori', 'warden': 'Warden',
    'wardens': 'Warden', 'warrior': 'Warrior', 'warriors': 'Warrior', 'reavers': 'Reavers', 'elusive': None,
}
FACTION_TERMS = {
    name: color.capitalize() for color in ('black', 'blue', 'red', 'white') for name in (color, f'{color} faction')
}
SPECIAL_TERMS = {
    'large creatures': ('tag', 'GIANT'), 'quick round': ('speed', 'Quick'), 'defenders': ('specialty', 'DEFENDER'),
    'healers': ('specialty', 'HEAL'), 'healer': ('specialty', 'HEAL'), 'flankers': ('specialty', 'FLANK'),
    'flanker': ('specialty', 'FLANK'), 'power gainers': ('specialty', 'POWERGAIN'), 'mythic': ('rarity', 'Mythic'),
    'epic': ('rarity', 'Epic'), 'common': ('rarity', 'Common'), 'uncommon': ('rarity', 'Uncommon'),
    'elusive': ('specialty', 'ELUSIVE'),
}
FAMILY_NAMES = {'hawke family': 'hawke', 'theirin family': 'theirin', 'guerrin family': 'guerrin', 'dog': 'dog|mabari',
                'cat': 'cat'}
STAT_WORDS = {
    'power': ['power'], 'health': ['health'], 'bonus damage': ['bonus_damage'], 'damage': ['bonus_damage'],
    '2x damage chance': ['crit'], 'damage resist': ['damage_resist'], 'stun resist': ['stun_resist'],
    'slow resist': ['slow_resist'], 'power drain resist': ['drain_resist'], 'drain resist': ['drain_resist'],
    'curse power resist': ['curse_power_resist'], 'curse health resist': ['curse_health_resist'],
    'curse resist': ['curse_power_resist', 'curse_health_resist'],
    'all effect resists': ['stun_resist', 'slow_resist', 'drain_resist', 'curse_power_resist', 'curse_health_resist'],
    'all effect resist': ['stun_resist', 'slow_resist', 'drain_resist', 'curse_power_resist', 'curse_health_resist'],
}
RESIST_SUFFIX_WORDS = {'stun': 'stun_resist', 'slow': 'slow_resist', 'power drain': 'drain_resist',
                       'curse power': 'curse_power_resist', 'curse health': 'curse_health_resist',
                       'damage': 'damage_resist', 'curse': None, 'all effect': None}
TRIGGER_PHRASES = [
    (r'^when hit,? ', 'self_hit'), (r' when hit$', 'self_hit'), (r'^on attack,? ', 'self_attack'),
    (r' on attack$', 'self_attack'), (r' when attacking$', 'self_attack'),
    (r'^when (?:an ally is|allies are) hit,? ', 'ally_hit'), (r' when (?:an ally is|allies are) hit$', 'ally_hit'),
    (r'^when (?:an )?enem(?:y is|ies are) hit,? ', 'enemy_hit'), (r' when (?:an )?enem(?:y is|ies are) hit$', 'enemy_hit'),
    (r'^on enemy death,? ', 'enemy_death'), (r' (?:on enemy death|when (?:an )?enemy dies)$', 'enemy_death'),
    (r'^(?:on ally death|when allies die),? ', 'ally_death'), (r' (?:on ally death|when an ally dies)$', 'ally_death'),
    (r'^on death,? ', 'self_death'), (r' when this hero dies$', 'self_death'),
]
TARGET_SENTENCES = {
    'more likely to attack the strongest enemy': ('target_highest_power', []),
    'more likely to attack enemy with lowest % health': ('target_lowest_health', []),
    'more likely to attack the back row': ('target_back_row', []),
    'much more likely to attack quick round heroes': ('target_quick', []),
    'more likely to attack the slowest enemy': ('target_slowest', []),
    'more likely to attack first': ('haste', []),
    'reduces enemy chance to flank': ('flank_reduction', []),
}
UNSUPPORTED_SENTENCES = {
    '+5% bonus damage vs non-spirits': 'filters cannot negate a term',
}


def clean(text: str) -> str:
    text = re.sub(r'<[^>]+>', '', text or '').replace('\u200b', '').replace('\u2019', "'")
    return re.sub(r'\s+', ' ', text).strip()


def rich(text: str) -> str:
    return re.sub(r'\s+', ' ', (text or '').replace('\u200b', '')).strip()


def effective_text(gear: dict, table: dict) -> str:
    if gear['RuneOverride']:
        return clean(table.get(gear['RuneOverride'].upper(), ''))
    return ' '.join(clean(table.get(key.upper(), '')) for key in gear['RuneKeys']).strip()


def sentences_of(text: str) -> list[str]:
    return [part.strip(' .') for part in re.split(r'(?<=[.!])\s+', text) if part.strip(' .')]


class TermResolver:
    def __init__(self, characters: list[dict]):
        self.characters = characters

    def character_ids(self, name: str) -> list[list[str]]:
        pattern = re.compile(rf'\b(?:{FAMILY_NAMES.get(name, re.escape(name))})\b', re.IGNORECASE)
        return [['character_id', c['id']] for c in self.characters if pattern.search(c['name'])]

    def one(self, name: str) -> list[list[str]] | None:
        name = re.sub(r'^(?:the|enemy) ', '', name.strip())
        name = re.sub(r' (?:allies|ally|heroes)$', '', name).strip()
        if name in FACTION_TERMS:
            return [['faction', FACTION_TERMS[name]]]
        if name in SPECIAL_TERMS:
            kind, value = SPECIAL_TERMS[name]
            return [[kind, value]]
        if GROUP_NAMES.get(name):
            return [['group', GROUP_NAMES[name]]]
        ids = self.character_ids(name)
        return ids or None

    def many(self, names: str) -> list[list[str]] | None:
        names = re.sub(r'^(?:only )', '', names.strip())
        terms: list[list[str]] = []
        for name in re.split(r',\s*(?:and\s+)?|\s+and\s+', names):
            if not name.strip():
                continue
            resolved = self.one(name)
            if resolved is None:
                return None
            terms.extend(resolved)
        return terms


def split_stats(words: str) -> list[str] | None:
    words = words.strip()
    if words in STAT_WORDS:
        return list(STAT_WORDS[words])
    match = re.fullmatch(r'(.+?) resists?', words)
    stats: list[str] = []
    if match:
        for part in re.split(r',\s*(?:and\s+)?|\s+and\s+', match.group(1)):
            key = f'{part.strip()} resist'
            if key not in STAT_WORDS:
                return None
            stats.extend(STAT_WORDS[key])
        return stats
    for part in re.split(r',\s*(?:and\s+)?|\s+and\s+', words):
        if part.strip() not in STAT_WORDS:
            return None
        stats.extend(STAT_WORDS[part.strip()])
    return stats


def effect(stat: str, value: int, *, on: str = 'battle', scope: str = 'self', who=None, vs=None, wearer=None,
           basis: str = 'base', each: bool = False) -> dict:
    row = {'stat': stat, 'value': value, 'on': on, 'scope': scope, 'basis': basis}
    if who:
        row['who'] = who
    if vs:
        row['vs'] = vs
    if wearer:
        row['wearer'] = wearer
    if each:
        row['each'] = True
    return row


def parse_flat(body: str, on: str, resolver: TermResolver) -> list[dict] | None:
    body = body.replace('(self)', '').strip()
    body = re.sub(r'\bper hit$', '', body).strip()
    each = body.endswith('(stacking)')
    body = body.replace('(stacking)', '').strip()
    double = re.fullmatch(r'(.+), double vs (.+)', body)
    if double:
        first = parse_flat(double.group(1), on, resolver)
        terms = resolver.many(double.group(2))
        if not first or not terms:
            return None
        return first + [dict(row, vs=terms) for row in first]
    match = re.fullmatch(r'(?:(?:gains?|allies gain|all allies gain|heals?|allies heal|heals allies) )?'
                         r'\+?(-?\d+)(%?)(?: of)? ?(.*)', body)
    if not match:
        return None
    value, percent, rest = int(match.group(1)), match.group(2), match.group(3).strip()
    if body.startswith('-') is False and body.lstrip('+').startswith('-'):
        value = -abs(value)
    allies_lead = re.match(r'(?:all )?allies|heals allies', body) is not None
    if not percent:
        raw = re.fullmatch(r'damage resist', rest)
        return [effect('raw_damage_resist', value, on=on)] if raw else None
    healing = re.match(r'(?:heals?|allies heal|heals allies)\b', body) is not None
    if healing and not rest:
        return [effect('health', value, on=on, scope='allies' if allies_lead else 'self')]
    for pattern, scope, kind in (
        (r'(.+?) vs (.+?) for allies', 'allies', 'vs'),
        (r'(.+?) vs (.+)', 'self', 'vs'),
        (r'(.+?) (?:to|for) enemies', 'enemies', 'all'),
        (r'(.+?) for allies', 'allies', 'all'),
        (r'(.+?) (?:to|for) allies', 'allies', 'all'),
        (r'(.+?) (?:to|for) (.+)', 'allies', 'who'),
    ):
        found = re.fullmatch(pattern, rest)
        if not found:
            continue
        stats = split_stats(found.group(1))
        if stats is None:
            continue
        terms = resolver.many(found.group(2)) if kind in ('vs', 'who') else None
        if kind != 'all' and not terms:
            continue
        if kind == 'vs':
            return [effect(s, value, on=on, scope=scope, vs=terms, each=each) for s in stats]
        if kind == 'who' and value < 0:
            scope = 'enemies'
        return [effect(s, value, on=on, scope=scope, who=terms, each=each) for s in stats]
    stats = split_stats(rest)
    if stats is None:
        return None
    scope = 'allies' if allies_lead else 'self'
    return [effect(s, value, on=on, scope=scope) for s in stats]


def parse_special(body: str, on: str, resolver: TermResolver) -> list[dict] | None:
    if body in TARGET_SENTENCES:
        stat, _ = TARGET_SENTENCES[body]
        return [effect(stat, 1)]
    if body.startswith('perceptive'):
        return [effect('perceptive', 1)]
    if body.startswith('stealth'):
        return [effect('stealth', 1)]
    match = re.fullmatch(r'more likely to attack (.+)', body)
    if match and resolver.many(match.group(1)):
        return [effect('target_match', 1, who=resolver.many(match.group(1)))]
    match = re.fullmatch(r'immune to (stun|slow|power drain)', body) or re.fullmatch(
        r'(curse power|stun|slow|power drain) immunity', body)
    if match:
        return [effect(RESIST_SUFFIX_WORDS[match.group(1)], 100)]
    match = re.fullmatch(r'(?:absorbs )?\+?(\d+)% (?:aoe damage for allies|defender)', body)
    if match:
        return [effect('defender', int(match.group(1)))]
    match = re.fullmatch(r'allies resist (?:the first )?(\d+) damage', body)
    if match:
        return [effect('raw_damage_resist', int(match.group(1)), scope='allies')]
    match = re.fullmatch(r'resist (\d+) damage dealt by (.+)', body)
    if match and resolver.many(match.group(2)):
        return [effect('raw_damage_resist', int(match.group(1)), vs=resolver.many(match.group(2)))]
    match = re.fullmatch(r'allies suffer (\d+)% health loss', body)
    if match:
        return [effect('damage', int(match.group(1)), on=on, scope='allies')]
    match = re.fullmatch(r'(?:all )?(enemies|allies) take (\d+)% of damage dealt', body)
    if match:
        return [effect('damage', int(match.group(2)), on=on, scope=match.group(1), basis='damage')]
    match = re.fullmatch(r'(?:deals? )\+?(\d+)% of damage dealt to (all enemies|enemy dragons)', body)
    if match:
        who = None if match.group(2) == 'all enemies' else resolver.many('dragons')
        return [effect('damage', int(match.group(1)), on=on, scope='enemies', basis='damage', who=who)]
    match = re.fullmatch(r'reverts (\d+)% of damage received to enemies', body)
    if match:
        trigger = 'self_hit' if on == 'battle' else on
        return [effect('damage', int(match.group(1)), on=trigger, scope='enemies', basis='damage')]
    match = re.fullmatch(r'drains (\d+)% power from enemies', body)
    if match:
        return [effect('drain', int(match.group(1)), on=on, scope='enemies')]
    match = re.fullmatch(r'converts? (\d+)% of damage (taken by enemies|taken|received|received by allies|taken by allies)'
                         r'(?: in)?to (ally )?(power|health)(?: for allies)?', body)
    if match:
        source = {'taken by enemies': 'enemy_hit', 'received by allies': 'ally_hit', 'taken by allies': 'ally_hit'}
        trigger = source.get(match.group(2), 'self_hit')
        scope = 'allies' if match.group(3) or body.endswith('for allies') else 'self'
        return [effect(match.group(4), int(match.group(1)), on=trigger, scope=scope, basis='damage')]
    return None


def curated_sentences(resolver: TermResolver) -> dict[str, list[dict]]:
    terms = resolver.many
    all_resists = ['stun_resist', 'slow_resist', 'drain_resist', 'curse_power_resist', 'curse_health_resist']
    curse_resists = ['curse_power_resist', 'curse_health_resist']
    dwarves, avvar_spirits = terms('dwarves'), terms('avvar and spirits')
    wardens, chantry, venatori = terms('wardens'), terms('chantry'), terms('venatori')
    return {
        '+7% bonus damage for, and against, black faction and demon allies (stacking)': [
            effect('bonus_damage', 7, scope='allies', who=terms('black faction and demon'), each=True),
            effect('bonus_damage', 7, scope='allies', vs=terms('black faction and demon'), each=True)],
        'slow immunity for the divines': [effect('slow_resist', 100, scope='allies', who=terms('divine'))],
        'templar equip: extra +5%': [effect('bonus_damage', 5, vs=terms('blue faction'), wearer=terms('templars'))],
        '+20% health and +10% damage resist for dwarves': [
            effect('health', 20, scope='allies', who=dwarves), effect('damage_resist', 10, scope='allies', who=dwarves)],
        '+6% curse resist for allies, double for avvar and spirits': [
            *[effect(s, 6, scope='allies') for s in curse_resists],
            *[effect(s, 6, scope='allies', who=avvar_spirits) for s in curse_resists]],
        'dog equip: +15% health and +10% 2x damage chance': [
            effect('health', 15, wearer=terms('dog')), effect('crit', 10, wearer=terms('dog'))],
        '-10% all resists (self)': [effect(s, -10) for s in (*all_resists, 'damage_resist')],
        '+7% damage vs demons': [effect('bonus_damage', 7, vs=terms('demons'))],
        'converts 2% of damage taken to health for allies': [
            effect('health', 2, on='self_hit', scope='allies', basis='damage')],
        '+10% power and power drain resist for allies': [
            effect('power', 10, scope='allies'), effect('drain_resist', 10, scope='allies')],
        'when enemies are hit, all enemies take 6% of damage dealt, but allies take 2%': [
            effect('damage', 6, on='enemy_hit', scope='enemies', basis='damage'),
            effect('damage', 2, on='enemy_hit', scope='allies', basis='damage')],
        'when enemies are hit, all enemies take 3% of damage dealt, but allies take 1%': [
            effect('damage', 3, on='enemy_hit', scope='enemies', basis='damage'),
            effect('damage', 1, on='enemy_hit', scope='allies', basis='damage')],
        'allies resist the first 2000 damage per hit': [effect('raw_damage_resist', 2000, scope='allies')],
        '+10% all effect resists for free marches': [
            effect(s, 10, scope='allies', who=[['group', 'FreeMarches']]) for s in all_resists],
        'witch of the wilds equip: +2,500 damage resist per hit': [
            effect('raw_damage_resist', 2500, wearer=terms('witch of the wilds'))],
        'convert 10% of damage received into health and power': [
            effect('health', 10, on='self_hit', basis='damage'), effect('power', 10, on='self_hit', basis='damage')],
        'elven allies get +10% bonus damage and damage resist vs wardens': [
            effect('bonus_damage', 10, scope='allies', who=terms('elves'), vs=wardens),
            effect('damage_resist', 10, scope='allies', who=terms('elves'), vs=wardens)],
        'allies gain +15% power and health on any death': [
            effect(stat, 15, on=on, scope='allies')
            for on in ('ally_death', 'enemy_death') for stat in ('power', 'health')],
        'avvar equip: converts 4% of damage dealt by self into health for avvar': [
            effect('health', 4, on='self_attack', scope='allies', who=terms('avvar'), wearer=terms('avvar'),
                   basis='damage')],
        '+20% power and +25% curse resist to wardens': [
            effect('power', 20, scope='allies', who=wardens),
            *[effect(s, 25, scope='allies', who=wardens) for s in curse_resists]],
        'creatures resist the first 2500 points of damage': [
            effect('raw_damage_resist', 2500, scope='allies', who=terms('creatures'))],
        'dragon equip: more likely to attack first within initiative': [effect('haste', 1, wearer=terms('dragons'))],
        'more likely to attack first within initiative': [effect('haste', 1)],
        '+5% curse health and power resist': [effect(s, 5) for s in curse_resists],
        'cat equip: gain 10% power when an ally dies': [effect('power', 10, on='ally_death', wearer=terms('cat'))],
        '+25% all resists for blue faction': [
            effect(s, 25, scope='allies', who=terms('blue faction')) for s in (*all_resists, 'damage_resist')],
        'vivienne equip: +25% power and +20% curse resist': [
            effect('power', 25, wearer=terms('vivienne')),
            *[effect(s, 20, wearer=terms('vivienne')) for s in curse_resists]],
        'convert 3% of damage taken to power': [effect('power', 3, on='self_hit', basis='damage')],
        '+5% bonus damage vs, and more likely to attack, power gainers (who love raisin cookies)': [
            effect('bonus_damage', 5, vs=terms('power gainers')), effect('target_match', 1, who=terms('power gainers'))],
        'cassandra equip: +10% heal when hit': [effect('health', 10, on='self_hit', wearer=terms('cassandra'))],
        '+15% health, power drain and damage resist for chantry': [
            effect(s, 15, scope='allies', who=chantry) for s in ('health', 'drain_resist', 'damage_resist')],
        'allies gain +8% health and +2% bonus damage when allies attack': [
            effect('health', 8, on='ally_attack', scope='allies'),
            effect('bonus_damage', 2, on='ally_attack', scope='allies')],
        '+3% health and stun resist for venatori': [
            effect('health', 3, scope='allies', who=venatori), effect('stun_resist', 3, scope='allies', who=venatori)],
        '-10% power, stun and slow resists to enemies': [
            effect(s, -10, scope='enemies') for s in ('power', 'stun_resist', 'slow_resist')],
        '+10% power, health, and all effect resists': [
            effect(s, 10) for s in ('power', 'health', *all_resists)],
        'when allies attack, allies gain 4% power': [effect('power', 4, on='ally_attack', scope='allies')],
        'zither equip: +20% health, power and damage resist': [
            effect(s, 20, wearer=terms('zither')) for s in ('health', 'power', 'damage_resist')],
        'convert 2% of damage taken by allies to health': [effect('health', 2, on='ally_hit', basis='damage')],
        'resists the first 300 damage per hit, since there\'s a lot of cheese to get through': [
            effect('raw_damage_resist', 300)],
        '-50% slow resist': [effect('slow_resist', -50, scope='enemies')],
        '-25% power drain resist': [effect('drain_resist', -25, scope='enemies')],
        'fen\'harel equip: when hit, reverts 10% of damage received to enemy black faction': [
            effect('damage', 10, on='self_hit', scope='enemies', basis='damage', who=terms('black faction'),
                   wearer=terms("fen'harel"))],
    }


def split_clauses(sentence: str) -> list[str]:
    return re.split(r' and (?=(?:gain|heal|allies)\b)|, but (?=allies)', sentence)


def parse_clause(body: str, on: str, wearer, resolver: TermResolver) -> tuple[list[dict] | None, str]:
    for pattern, trigger in TRIGGER_PHRASES:
        if on == 'battle' and re.search(pattern, body):
            body = re.sub(pattern, '', body).strip(' ,')
            on = trigger
            break
    rows = parse_special(body, on, resolver)
    if rows is None:
        rows = parse_flat(body, on, resolver)
    if rows is not None and wearer:
        rows = [dict(row, wearer=wearer) for row in rows]
    return rows, on


def parse_sentence(sentence: str, resolver: TermResolver, curated: dict[str, list[dict]]) -> list[dict] | None:
    if sentence.lower() in curated:
        return curated[sentence.lower()]
    body = sentence.lower().replace('extra ', '').strip()
    wearer = None
    match = re.match(r'^(.+?) equip:\s*(.+)$', body)
    if match:
        wearer = resolver.many(match.group(1))
        body = match.group(2)
        if wearer is None:
            return None
    rows: list[dict] = []
    on = 'battle'
    for clause in split_clauses(body):
        parsed, on = parse_clause(clause.strip(), on, wearer, resolver)
        if parsed is None:
            return None
        rows.extend(parsed)
    return rows


def absorbs_damage(effects: list[dict]) -> bool:
    return any(row['stat'] == 'defender' for row in effects)


def build_item(
    gear: dict, reference: dict, table: dict, resolver: TermResolver, curated: dict[str, list[dict]]
) -> tuple[dict, list[str]]:
    text = effective_text(gear, table)
    effects: list[dict] = []
    unparsed: list[str] = []
    for sentence in sentences_of(text):
        if sentence.lower() in UNSUPPORTED_SENTENCES:
            continue
        rows = parse_sentence(sentence, resolver, curated)
        if rows is None:
            unparsed.append(sentence)
        else:
            effects.extend(rows)
    categories = []
    for category in gear['Categories']:
        kind = CATEGORY_TYPES[category['Type']]
        value = category['Value']
        if kind == 'faction':
            if value not in FACTION_NAMES:
                continue
            value = FACTION_NAMES[value]
        if kind == 'initiative':
            value = INITIATIVE_SPEEDS[value]
        categories.append({'kind': kind, 'value': value})
    aura_ru = rich(reference.get('rune_override_ru') or ' '.join(reference.get('rune_effects_ru') or []))
    return {
        'id': gear['Id'],
        'title_ru': reference.get('title_ru') or reference.get('title_en') or gear['Id'],
        'description_ru': clean(reference.get('desc_ru') or reference.get('desc_en') or ''),
        'aura_ru': aura_ru or text,
        'rarity': gear['Rarity'],
        'icon': f'{EXTRA_ICON_REF}{gear["Icon"]}.png',
        'categories': categories,
        'categories_all': bool(gear['CategoriesSemanticAnd']),
        'absorbs_damage': absorbs_damage(effects),
        'effects': effects,
    }, unparsed


def copy_icons(items: list[dict], *, dry_run: bool) -> int:
    missing = 0
    for item in items:
        source = REFERENCE_ICONS / Path(item['icon']).name
        if not source.exists():
            missing += 1
            continue
        if not dry_run:
            EXTRA_ICON_DIR.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, EXTRA_ICON_DIR / source.name)
    if not dry_run:
        for name in UI_TEXTURES:
            shutil.copyfile(UI_TEXTURE_DIR / f'{name}.png', EXTRA_ICON_DIR / f'{name}.png')
    return missing


def build() -> tuple[dict, list[str], int]:
    gear = json.loads(GEAR_DEFS.read_text(encoding='utf-8'))
    table = json.loads(LOCALIZATION.read_text(encoding='utf-8'))
    characters = json.loads(CHARACTERS.read_text(encoding='utf-8'))['characters']
    reference = json.loads(REFERENCE_GEAR.read_text(encoding='utf-8'))
    resolver = TermResolver(characters)
    curated = curated_sentences(resolver)
    items = []
    unparsed = []
    sentences = 0
    for gear_id in sorted(gear):
        item, misses = build_item(gear[gear_id], reference.get(gear_id, {}), table, resolver, curated)
        sentences += len(sentences_of(effective_text(gear[gear_id], table)))
        items.append(item)
        unparsed.extend(f'{gear_id}: {miss}' for miss in misses)
    russian = json.loads(RUSSIAN_LOCALIZATION.read_text(encoding='utf-8'))
    packs = [dict(pack, name_ru=russian.get(pack['loc'], TOME_LOCAL_NAME)) for pack in GEAR_PACKS]
    return {
        'version': 1,
        'balance_note': 'Sell values, the prestige slot cost and pack contents were served remotely; they are local balance.',
        'rarity_gold_values': RARITY_GOLD_VALUES,
        'prestige_slot_gem_cost': PRESTIGE_SLOT_GEM_COST,
        'packs': packs,
        'strings': [{'key': key, 'text': russian[key]} for key in TEXT_KEYS]
        + [{'key': key, 'text': text} for key, text in LOCAL_FILTER_NAMES.items()],
        'unparsed': unparsed,
        'items': items,
    }, unparsed, sentences


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    catalog, unparsed, sentences = build()
    coverage = 1.0 - len(unparsed) / sentences
    missing = copy_icons(catalog['items'], dry_run=args.dry_run)
    for line in unparsed:
        print(line)
    print(f'{len(catalog["items"])} items, {sentences} sentences, {coverage:.1%} parsed, {missing} icons missing')
    if not args.dry_run:
        OUTPUT.write_text(json.dumps(catalog, ensure_ascii=False, indent=1) + '\n', encoding='utf-8')
    return 0 if coverage >= COVERAGE_FLOOR else 1


if __name__ == '__main__':
    sys.exit(main())
