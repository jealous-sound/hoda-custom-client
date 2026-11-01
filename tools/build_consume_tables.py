#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

CLI_DESCRIPTION = 'Build data/consume_tables.json from the decoded game data tables and the character catalog'
ROOT = Path(__file__).resolve().parents[1]
TABLES = ROOT / 'client-additional-data/data_tables'
CATALOG = ROOT / 'client-additional-data/characters.json'
OUTPUT = ROOT / 'data/consume_tables.json'
TIERS = ('I', 'II', 'III', 'IV')
UNIQUE_TIER_TABLE = 'consume-level-uniquetier'
UNIQUE_TIER_PROGRESSION = 'uniquetier'
STANDARD_PROGRESSION = 'standard'
PROMOTION_UNLOCK, FORTIFY_UNLOCK, CRIT_UNLOCK = 1, 2, 3
UNIT_DATA = ROOT / 'client-additional-data/HoDA_character_info/UnitData.json'
TIER_FOUR_WORTH = 4.6
RARITY_TABLE_SUFFIX = {
    'Common': 'C',
    'Uncommon': 'UC',
    'Rare': 'R',
    'Epic': 'E',
    'Legendary': 'L',
    'Mythic': 'L',
}
RARITY_LEVELS = {'Common': 1, 'Uncommon': 2, 'Rare': 3, 'Epic': 4, 'Legendary': 5, 'Mythic': 6}
LOW_RARITY_LEVEL_TABLE = 'unit-level-C-UC'
GIANT_LEVEL_TABLE = 'giant-unit-level-1'
DEFAULT_LEVEL_TABLE = 'unit-level-Default'
THREE_STAR = 3


def load_json(path: Path):
    return json.loads(path.read_text(encoding='utf-8'))


def build_xp_tables(rows: list[dict]) -> dict[str, list[int]]:
    return {table['name']: [row.get('xp', 0) for row in table['row']] for table in rows}


def build_progressions(rows: list[dict]) -> dict[str, list[list[int]]]:
    tables = {}
    for table in rows:
        levels = []
        for level in table['level']:
            totals = {unlock['type']: unlock['total_unlocked'] for unlock in level['unlocks']}
            levels.append([totals.get(kind, 0) for kind in (PROMOTION_UNLOCK, FORTIFY_UNLOCK, CRIT_UNLOCK)])
        tables[table['id']] = levels
    return tables


def index_units(units: list[dict]) -> dict[tuple[str, int], dict]:
    index = {}
    for unit in units:
        index.setdefault((unit['name'], unit['rarity']), unit)
    return index


def fallback_consume_table(character: dict) -> str:
    suffix = RARITY_TABLE_SUFFIX[character['rarity']]
    if character['giant']:
        return f'consume-level-{suffix}G'
    if suffix == 'L' and character['stars'] == THREE_STAR:
        return 'consume-level-L-3Star'
    return f'consume-level-{suffix}'


def fallback_level_table(character: dict) -> str:
    if character['giant']:
        return GIANT_LEVEL_TABLE
    if RARITY_LEVELS[character['rarity']] <= RARITY_LEVELS['Uncommon']:
        return LOW_RARITY_LEVEL_TABLE
    return DEFAULT_LEVEL_TABLE


def linear_tier_worth(multiplier: float) -> list[float]:
    return [round(multiplier * TIER_FOUR_WORTH * (tier + 1) / len(TIERS), 4) for tier in range(len(TIERS))]


def character_profile(character: dict, units: dict[tuple[str, int], dict], multiplier: float) -> dict:
    rarity = RARITY_LEVELS[character['rarity']]
    rows = {tier: units.get((character['id'] + tier, rarity)) for tier in TIERS}
    known = next((row for row in rows.values() if row), None)
    single_tier = len(character['tiers']) == 1
    return {
        'level_table': known['level_table_ref'] if known else fallback_level_table(character),
        'consume_table': UNIQUE_TIER_TABLE
        if single_tier
        else known['consume_level_table_ref']
        if known
        else fallback_consume_table(character),
        'progression': UNIQUE_TIER_PROGRESSION if single_tier else STANDARD_PROGRESSION,
        'archetype': known['archetype'] if known else 'Giant' if character['giant'] else 'default',
        'worth': linear_tier_worth(multiplier),
    }


def build() -> dict:
    units = index_units(load_json(TABLES / 'units.json'))
    catalog = load_json(CATALOG)['characters']
    unit_data = load_json(UNIT_DATA)
    archetypes = {row['id']: [slot['target_attribute'] for slot in row['slot']] for row in load_json(TABLES / 'archetypes.json')}
    return {
        'version': 1,
        'xp_tables': build_xp_tables(load_json(TABLES / 'xp_tables.json')),
        'progressions': build_progressions(load_json(TABLES / 'consume_progression.json')),
        'archetypes': archetypes,
        'characters': {character['id']: character_profile(character, units, unit_data[character['id']]['ConsumeXpWorthMultiplier']) for character in catalog},
    }


def serialize(tables: dict) -> str:
    return json.dumps(tables, ensure_ascii=False, separators=(',', ':'), sort_keys=True) + '\n'


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument('--check', action='store_true', help='fail when the committed table is stale')
    args = parser.parse_args()
    text = serialize(build())
    if args.check:
        if OUTPUT.read_text(encoding='utf-8') != text:
            print(f'{OUTPUT.relative_to(ROOT)} is stale; rerun tools/build_consume_tables.py', file=sys.stderr)
            return 1
        return 0
    OUTPUT.write_text(text, encoding='utf-8')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
