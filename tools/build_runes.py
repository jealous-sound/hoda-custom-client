#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
from pathlib import Path

import economy_localization as loc
import original_cache

CLI_DESCRIPTION = 'Build data/runes.json from the 5.4.4 rune reference and copy icons the 4.4.0 cache lacks into extra/'
ROOT = Path(__file__).resolve().parents[1]
REFERENCE = ROOT / '.claude/skills/hoda-reference'
RUNES_JSON = REFERENCE / 'data/runes.json'
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
PERCENT_VALUE_FORMAT = 'buff-type-power-value-format'
LEVEL_VALUE_FORMAT = 'buff-type-highest-power-unit-value-format'
VALUE_FORMATS_MISSING_FROM_THE_ORIGINAL = {
    'gold': PERCENT_VALUE_FORMAT,
    'xp': PERCENT_VALUE_FORMAT,
    'damage-resist-percent': PERCENT_VALUE_FORMAT,
    'damage-retaliation-percent': PERCENT_VALUE_FORMAT,
    'stealth': LEVEL_VALUE_FORMAT,
    'perceptive': LEVEL_VALUE_FORMAT,
    'damage': 'HODA_RUNE_VALUE_DAMAGE',
    'power-cursed': 'HODA_RUNE_VALUE_IMMUNE',
}
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


def original_or(key: str, fallback: str) -> str:
    return key if loc.has_original(key) else fallback


def name_key(rune_id: str, family: str) -> str:
    if loc.has_original(rune_id):
        return rune_id
    return original_or(f'{family}-1', loc.project_key(f'{family}-name'))


def value_key(buff_type: str) -> str:
    key = f'buff-type-{buff_type}-value-format'
    return key if loc.has_original(key) else VALUE_FORMATS_MISSING_FROM_THE_ORIGINAL[buff_type]


def build_entry(rune: dict, known: set[str]) -> dict:
    rune_id = rune['id']
    family, _, _ = family_of(rune_id)
    buff_type = rune['buff_type']
    return {
        'id': rune_id,
        'family': family,
        'rarity': rune['rarity'],
        'buff_type': buff_type,
        'buff_value': abs(rune['buff_value']),
        'duration_minutes': rune['duration'],
        'name_key': name_key(rune_id, family),
        'desc_key': original_or(f'desc-{family}', loc.project_key(f'{family}-description')),
        'buff_key': original_or(f'buff-type-{buff_type}', loc.project_key(f'rune-buff-{buff_type}')),
        'value_key': value_key(buff_type),
        'icon': icon_ref(rune['icon']),
        'buff_icon': icon_ref(rune['buff_type_icon']),
        'craft_to': craft_target(rune_id, known),
    }


def sort_key(entry: dict) -> tuple:
    return entry['rarity'], entry['family'], entry['duration_minutes'], entry['id']


def build() -> dict:
    runes = json.loads(RUNES_JSON.read_text())
    known = set(runes)
    return {'version': 1, 'runes': sorted((build_entry(rune, known) for rune in runes.values()), key=sort_key)}


def unknown_text_keys(catalog: dict) -> list[str]:
    fields = ('name_key', 'desc_key', 'buff_key', 'value_key')
    return loc.unknown_keys(entry[field] for entry in catalog['runes'] for field in fields)


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
    unknown = unknown_text_keys(catalog)
    if unknown:
        print(f'localization keys missing from the tables: {", ".join(unknown)}', file=sys.stderr)
        return 1
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
