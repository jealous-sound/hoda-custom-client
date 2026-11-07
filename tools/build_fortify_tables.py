#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
import statistics
import struct
import sys
from pathlib import Path

CLI_DESCRIPTION = (
    'Build data/fortify_tables.json from the decoded 4.4.0 fortify tables, the 5.4.4 client config '
    'and the 5.x unit maxima'
)
ROOT = Path(__file__).resolve().parents[1]
TABLES = ROOT / 'client-additional-data/data_tables'
CONFIG_544 = ROOT / 'client-additional-data/decompiled_src/AssemblyCSharp/Config.cs'
UNIT_DATA = ROOT / 'client-additional-data/HoDA_character_info/UnitData.json'
OUTPUT = ROOT / 'data/fortify_tables.json'
VERSION = 1
STAT_NAMES = {1: 'health', 2: 'power'}
SLOTS_PER_STAT_IN_5X_MAXIMA = 5
SLOT_CAP_IN_5X_MAXIMA = 0.4
MAXIMA_FIELDS = (('PowerAtMaxLevel', 'MaxFortifyPowerBonus'), ('HealthAtMaxLevel', 'MaxFortifyHealthBonus'))
BONUS_DECIMALS = 2
REMOVE_COST_PATTERN = re.compile(r'_removeFortifyGemCostByRarity\s*=\s*"\[([0-9,\s]+)\]"')
RARITY_COUNT = 6


def load_json(path: Path):
    return json.loads(path.read_text(encoding='utf-8'))


def f32(value: float) -> float:
    return struct.unpack('<f', struct.pack('<f', value))[0]


def f32_product(*factors: float) -> float:
    result = f32(factors[0])
    for factor in factors[1:]:
        result = f32(result * f32(factor))
    return result


def unit_maxima(unit_data: dict) -> list[tuple[int, int]]:
    pairs = []
    for unit in unit_data.values():
        for stat_field, maximum_field in MAXIMA_FIELDS:
            stats, maxima = unit.get(stat_field), unit.get(maximum_field)
            if not isinstance(stats, dict) or not isinstance(maxima, dict):
                continue
            pairs.extend((stats[tier], maxima[tier]) for tier in stats if tier in maxima and stats[tier] > 0)
    return pairs


def slot_maximum(stat: int, top_bonus: float, faction_bonus: float) -> int:
    multiplier = f32(f32(1.0) + f32(faction_bonus))
    return SLOTS_PER_STAT_IN_5X_MAXIMA * round(f32_product(stat, SLOT_CAP_IN_5X_MAXIMA, top_bonus, multiplier))


def mythic_top_tier_bonus(unit_data: dict, faction_bonus: float) -> float:
    pairs = unit_maxima(unit_data)
    if not pairs:
        raise SystemExit(f'{UNIT_DATA.relative_to(ROOT)} has no fortify maxima')
    per_slot = SLOTS_PER_STAT_IN_5X_MAXIMA * SLOT_CAP_IN_5X_MAXIMA * (1.0 + faction_bonus)
    bonus = round(statistics.median(maximum / (stat * per_slot) for stat, maximum in pairs), BONUS_DECIMALS)
    misses = [(stat, maximum) for stat, maximum in pairs if slot_maximum(stat, bonus, faction_bonus) != maximum]
    if misses:
        raise SystemExit(f'mythic tier IV bonus {bonus} misses {len(misses)} of {len(pairs)} 5.x maxima: {misses[:5]}')
    return bonus


def mythic_row_scaled_from_legendary(legendary: list[float], top_bonus: float) -> list[float]:
    return [round(bonus * top_bonus / legendary[-1], BONUS_DECIMALS) for bonus in legendary]


def remove_gem_costs() -> list[int]:
    match = REMOVE_COST_PATTERN.search(CONFIG_544.read_text(encoding='utf-8'))
    if not match:
        raise SystemExit(f'{CONFIG_544.relative_to(ROOT)} has no remove-fortify gem costs')
    costs = [int(cost) for cost in match.group(1).split(',')]
    if len(costs) != RARITY_COUNT:
        raise SystemExit(f'expected {RARITY_COUNT} remove-fortify gem costs, found {costs}')
    return costs


def build() -> dict:
    fortify_data = load_json(TABLES / 'fortify_data.json')[0]
    faction_bonus = fortify_data['faction_bonus']
    rows = [row['tier_level_bonus'] for row in fortify_data['rarity_level_bonus']]
    top_bonus = mythic_top_tier_bonus(load_json(UNIT_DATA), faction_bonus)
    rows.append(mythic_row_scaled_from_legendary(rows[-1], top_bonus))
    archetypes = {
        row['id']: [{'stat': STAT_NAMES[slot['target_attribute']], 'cap': slot['value']} for slot in row['slot']]
        for row in load_json(TABLES / 'archetypes.json')
    }
    return {
        'version': VERSION,
        'faction_bonus': faction_bonus,
        'rarity_tier_bonus': rows,
        'remove_gem_cost': remove_gem_costs(),
        'archetypes': archetypes,
    }


def serialize(tables: dict) -> str:
    return json.dumps(tables, ensure_ascii=False, indent=1, sort_keys=True) + '\n'


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument('--check', action='store_true', help='fail when the committed table is stale')
    args = parser.parse_args()
    text = serialize(build())
    if args.check:
        if OUTPUT.read_text(encoding='utf-8') != text:
            print(f'{OUTPUT.relative_to(ROOT)} is stale; rerun tools/build_fortify_tables.py', file=sys.stderr)
            return 1
        return 0
    OUTPUT.write_text(text, encoding='utf-8')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
