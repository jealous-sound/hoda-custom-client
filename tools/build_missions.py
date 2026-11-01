#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import original_cache

CLI_DESCRIPTION = 'Build data/missions.json, the server-side mission table reconstructed from the 4.4.0 strings and icons'
ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'data/missions.json'
CAMPAIGN_MAPS = ROOT / 'data/campaign_maps.json'
LOCALIZATION = (
    ROOT / 'client-additional-data/Heroes-of-Dragon-Age-v4-4-0-cache-androidp1'
    '/com.ea.game.dragonage_row/cache/Localization/Loc_ENG_US.txt'
)
TEXTURE_CLASS = 28
ICON_PATTERN = 'tex.ico_achievement_{}'
MAX_SERIES_TIERS = 5

LADDER_REWARDS = ({'gold': 500}, {'gold': 1000}, {'gold': 2000}, {'gems': 2}, {'gems': 3})
EXPLORER_REWARDS = ({'gems': 2}, {'gems': 3}, {'gems': 5}, {'gems': 5}, {'gems': 10})
LATE_EXPLORER_REWARD = {'gems': 10}
FIRST_MAP_MASTERY_REWARD = {'gems': 5}
MAP_MASTERY_REWARD = {'gems': 10}
SINGLE_REWARD = {'gems': 2}
MASTER_REWARDS = ({'gems': 2}, {'gems': 3}, {'gems': 5}, {'gems': 10}, {'gems': 20})
TIERS = (0, 1, 2, 3, 3)
RARITIES = {'common': 1, 'uncommon': 2, 'rare': 3, 'epic': 4, 'legendary': 5}
RARITY_MAX_LEVELS = {'common': 30, 'uncommon': 35, 'rare': 45, 'epic': 65, 'legendary': 85}
ORIGINS_SQUAD_SIZE = 4


def mission(mission_id: str, stat: str, target: int, reward: dict[str, int], **params: int) -> dict:
    return {
        'id': mission_id,
        'stat': stat,
        'target': target,
        'gold': reward.get('gold', 0),
        'gems': reward.get('gems', 0),
        **params,
    }


def ladder(stat: str, rows: tuple[tuple[str, int], ...], rewards=LADDER_REWARDS, **params: int) -> list[dict]:
    return [mission(mission_id, stat, target, reward, **params) for (mission_id, target), reward in zip(rows, rewards)]


def numbered(first: int, count: int) -> list[str]:
    return [f'a{first + offset}' for offset in range(count)]


def trainer_series(first: int, rarity: str, targets: tuple[int, ...]) -> list[dict]:
    rows = tuple(zip(numbered(first, len(targets)), targets))
    return [
        mission(mission_id, 'collected', target, reward, rarity=RARITIES[rarity], hero_tier=tier)
        for (mission_id, target), reward, tier in zip(rows, LADDER_REWARDS, TIERS)
    ]


def mastery_singles(map_count: int) -> list[list[dict]]:
    ids = numbered(24, 10) + numbered(141, 10)
    return [
        [
            mission(
                mission_id,
                'map_mastered',
                1,
                FIRST_MAP_MASTERY_REWARD if map_index == 0 else MAP_MASTERY_REWARD,
                map=map_index,
            )
        ]
        for map_index, mission_id in enumerate(ids[:map_count])
    ]


def build_series(map_count: int) -> list[list[dict]]:
    explorer_maps = (('a7', 3), ('a8', 5), ('a9', 8), ('a10', 10))
    late_explorer_maps = tuple(row for row in (('a151', 12), ('a152', 15), ('a153', 18), ('a154', 20)) if row[1] <= map_count)
    common_targets = (4, 8, 12, 16, 20)
    legendary_targets = (2, 4, 6, 8, 10)
    series = [
        [mission('a1', 'first_boss', 1, {'gems': 5})],
        [mission('a2', 'node_mastered', 1, SINGLE_REWARD)],
        [mission('a6', 'first_win', 1, EXPLORER_REWARDS[0])]
        + ladder('maps_unlocked', explorer_maps, EXPLORER_REWARDS[1:]),
        ladder('maps_unlocked', late_explorer_maps, (LATE_EXPLORER_REWARD,) * len(late_explorer_maps)),
        ladder('summoned', (('a15', 25), ('a16', 50), ('a17', 100), ('a18', 500), ('a81', 1000))),
        ladder(
            'gold_earned',
            (('a19', 3000), ('a20', 20000), ('a21', 100000), ('a22', 500000), ('a23', 2000000)),
        ),
        ladder('consumed', (('a3', 1), ('a38', 10), ('a39', 50), ('a40', 150), ('a41', 500))),
        ladder('combined', (('a4', 1), ('a56', 2), ('a57', 10), ('a58', 25), ('a79', 50))),
        ladder('player_level', (('a42', 5), ('a43', 10), ('a44', 20), ('a45', 35), ('a46', 50))),
        ladder('hero_level', (('a68', 5), ('a69', 15), ('a70', 30), ('a71', 50), ('a72', 70))),
        ladder('crits', (('a48', 3), ('a49', 20), ('a50', 50), ('a85', 100), ('a86', 500))),
        ladder('battle_damage', (('a73', 5000), ('a74', 10000), ('a75', 15000), ('a87', 20000), ('a88', 25000))),
        ladder('challenge_attempts', (('a76', 25), ('a77', 50), ('a78', 100), ('a89', 500), ('a90', 1000))),
        ladder('runes_used', (('a64', 2), ('a65', 10), ('a66', 50), ('a67', 250), ('a82', 1000))),
        ladder('battle_days', (('a136', 3), ('a137', 7), ('a138', 14), ('a139', 30), ('a140', 90))),
        [mission('a51', 'faction_hits', 1, SINGLE_REWARD)]
        + [
            mission(mission_id, 'squad_faction', 1, reward, faction=index)
            for index, (mission_id, reward) in enumerate(zip(numbered(52, 4), LADDER_REWARDS[1:]))
        ],
        [mission('a63', 'origins_squad', ORIGINS_SQUAD_SIZE, {'gems': 5})],
        trainer_series(91, 'common', common_targets),
        trainer_series(96, 'uncommon', common_targets),
        trainer_series(101, 'rare', common_targets),
        trainer_series(106, 'epic', common_targets),
        trainer_series(111, 'legendary', legendary_targets),
    ]
    masters = zip(numbered(116, 5), RARITIES.items(), MASTER_REWARDS, numbered(95, 21)[::5])
    series += [
        [
            mission(
                mission_id,
                'tier_iv_level',
                RARITY_MAX_LEVELS[rarity],
                reward,
                rarity=rarity_id,
                icon=icon,
            )
        ]
        for mission_id, (rarity, rarity_id), reward, icon in masters
    ]
    return series + mastery_singles(map_count)


def build_table() -> dict:
    map_count = len(json.loads(CAMPAIGN_MAPS.read_text(encoding='utf-8')))
    series = [rows for rows in build_series(map_count) if rows]
    return {
        'missions': [
            {'series': number, 'tier': tier, **row}
            for number, rows in enumerate(series)
            for tier, row in enumerate(rows)
        ]
    }


def localized_keys() -> set[str]:
    with LOCALIZATION.open(encoding='utf-8') as lines:
        return {line.split('|', 1)[0].upper() for line in lines if '|' in line}


def validate(table: dict) -> list[str]:
    keys = localized_keys()
    textures = {name for class_id, name in original_cache.catalog() if class_id == TEXTURE_CLASS}
    errors = []
    tiers: dict[int, int] = {}
    seen: set[str] = set()
    for row in table['missions']:
        mission_id = row['id']
        tiers[row['series']] = tiers.get(row['series'], 0) + 1
        if mission_id in seen:
            errors.append(f'{mission_id}: duplicate id')
        seen.add(mission_id)
        for suffix in ('TITLE', 'LOCKED_DESC', 'UNLOCKED_DESC'):
            if f'ACHIEVEMENT_{mission_id.upper()}_{suffix}' not in keys:
                errors.append(f'{mission_id}: no {suffix} string')
        icon = ICON_PATTERN.format(row.get('icon', mission_id))
        if icon not in textures:
            errors.append(f'{mission_id}: {icon} is not in the original cache')
        if row['target'] <= 0 or (row['gold'] > 0) == (row['gems'] > 0):
            errors.append(f'{mission_id}: needs a positive target and exactly one reward currency')
    errors += [f'series {number}: {count} tiers' for number, count in tiers.items() if count > MAX_SERIES_TIERS]
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument('--check', action='store_true', help='fail when the committed table is stale')
    args = parser.parse_args()
    table = build_table()
    errors = validate(table)
    for error in errors:
        print(error, file=sys.stderr)
    if errors:
        return 1
    text = json.dumps(table, ensure_ascii=False, indent=1) + '\n'
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding='utf-8') != text:
            print(f'{OUTPUT.name} is stale; run tools/build_missions.py', file=sys.stderr)
            return 1
        return 0
    OUTPUT.write_text(text, encoding='utf-8')
    print(f'wrote {len(table["missions"])} missions to {OUTPUT.relative_to(ROOT)}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
