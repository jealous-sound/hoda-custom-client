#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
from pathlib import Path

import original_cache

CLI_DESCRIPTION = 'Build data/runes.json from the 5.4.4 rune reference and copy icons the 4.4.0 cache lacks into extra/'
ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT / '.claude/skills/hoda-reference'
RUNES_JSON = REFERENCE / 'data/runes.json'
LOCALIZATION_JSON = REFERENCE / 'data/localization/Loc_RUS_RU.json'
REFERENCE_ICONS = REFERENCE / 'references/runes'
OUTPUT = ROOT / 'data/runes.json'
EXTRA_ICON_DIR = ROOT / 'client-additional-data/extra/gui/runes'
EXTRA_ICON_REF = 'gui/runes/'
TEXTURE_CLASS = 28
PRESTIGE_RARITY = 7
BASE_TIERS = 6
SPECIAL_SUFFIX = 'Special'
OVERCHARGE_STEPS = (2, 4, 8)
FAMILY_PATTERN = re.compile(r'^(rune-[a-z-]+?)-(\d|Special)(?:-(\d))?$')
UNFORMATTED_VALUE_TYPES = {'stealth': 'Ур. {0}', 'perceptive': 'Ур. {0}'}
ICON_ALIASES = {'tex.softcurrency': 'tex.Ico_BuffGold', 'tex.xp_nonumber': 'tex.Ico_BuffXP'}


def family_of(rune_id: str) -> tuple[str, str, int | None]:
    match = FAMILY_PATTERN.match(rune_id)
    if not match:
        raise ValueError(f'{rune_id}: unexpected rune id')
    family, tier, step = match.groups()
    return family, tier, int(step) if step else None


def craft_target(rune_id: str, known: set[str]) -> str:
    family, tier, step = family_of(rune_id)
    if tier == SPECIAL_SUFFIX or int(tier) >= PRESTIGE_RARITY:
        return ''
    if step is None and int(tier) < BASE_TIERS:
        target = f'{family}-{int(tier) + 1}'
    elif step is None:
        target = f'{family}-{BASE_TIERS}-{OVERCHARGE_STEPS[0]}'
    elif step == OVERCHARGE_STEPS[-1]:
        return ''
    else:
        target = f'{family}-{BASE_TIERS}-{OVERCHARGE_STEPS[OVERCHARGE_STEPS.index(step) + 1]}'
    return target if target in known else ''


def icon_ref(name: str) -> str:
    name = ICON_ALIASES.get(name, name)
    if original_cache.resolves(f'{original_cache.CACHE_REF_PREFIX}{name}', TEXTURE_CLASS):
        return f'{original_cache.CACHE_REF_PREFIX}{name}'
    return f'{EXTRA_ICON_REF}{name}.png'


def localized(table: dict, key: str) -> str:
    return (table.get(key.upper()) or '').strip()


def build_entry(rune: dict, known: set[str], table: dict) -> dict:
    rune_id = rune['id']
    family, _, _ = family_of(rune_id)
    buff_type = rune['buff_type']
    name = rune.get('name_ru') or localized(table, f'{family}-1') or localized(table, f'{family}-6')
    description = rune.get('desc_ru') or localized(table, f'desc-{family}')
    value = (rune.get('buff_value_formatted_raw') or '').strip()
    if '{' in value or not value:
        value = UNFORMATTED_VALUE_TYPES.get(buff_type, '{0}').format(rune['buff_value'])
    return {
        'id': rune_id,
        'family': family,
        'rarity': rune['rarity'],
        'buff_type': buff_type,
        'buff_value': abs(rune['buff_value']),
        'duration_minutes': rune['duration'],
        'name_ru': name,
        'desc_ru': description,
        'buff_ru': localized(table, f'buff-type-{buff_type}'),
        'value_ru': value,
        'icon': icon_ref(rune['icon']),
        'buff_icon': icon_ref(rune['buff_type_icon']),
        'craft_to': craft_target(rune_id, known),
    }


def sort_key(entry: dict) -> tuple:
    return entry['rarity'], entry['family'], entry['duration_minutes'], entry['id']


def build() -> dict:
    runes = json.loads(RUNES_JSON.read_text())
    table = json.loads(LOCALIZATION_JSON.read_text())
    known = set(runes)
    return {'version': 1, 'runes': sorted((build_entry(rune, known, table) for rune in runes.values()), key=sort_key)}


def extra_icons(catalog: dict) -> list[str]:
    refs = {entry[field] for entry in catalog['runes'] for field in ('icon', 'buff_icon')}
    return sorted(ref.removeprefix(EXTRA_ICON_REF) for ref in refs if ref.startswith(EXTRA_ICON_REF))


def sync_icons(catalog: dict) -> None:
    EXTRA_ICON_DIR.mkdir(parents=True, exist_ok=True)
    for name in extra_icons(catalog):
        shutil.copyfile(REFERENCE_ICONS / name, EXTRA_ICON_DIR / name)


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument('--check', action='store_true', help='fail if data/runes.json is out of date')
    args = parser.parse_args()

    catalog = build()
    text = json.dumps(catalog, ensure_ascii=False, indent=1) + '\n'
    if args.check:
        if OUTPUT.read_text() != text:
            print('data/runes.json is out of date; run tools/build_runes.py', file=sys.stderr)
            return 1
        return 0
    OUTPUT.write_text(text)
    sync_icons(catalog)
    print(f'{len(catalog["runes"])} runes, {len(extra_icons(catalog))} icons in extra/')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
