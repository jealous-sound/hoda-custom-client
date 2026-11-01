#!/usr/bin/env python3

from __future__ import annotations

import json
import re
from pathlib import Path

import build_gear as generator

CLI_DESCRIPTION = 'Check that data/gear.json is current and only references known groups, characters and icons'
ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'data/gear.json'
EXTRA_DIR = ROOT / 'client-additional-data/extra'
TERM_KINDS = {'group', 'faction', 'character_id', 'specialty', 'rarity', 'speed', 'tag'}
ALLOWED_UNPARSED = re.compile(r'^spear_thrower: \+5% bonus damage vs non-Spirits$')


def fail(message: str) -> None:
    raise SystemExit(f'gear validation failed: {message}')


def check_terms(item_id: str, terms: list[list[str]], groups: set[str], ids: set[str]) -> None:
    for kind, value in terms:
        if kind not in TERM_KINDS:
            fail(f'{item_id}: unknown term kind {kind}')
        if kind == 'group' and value not in groups:
            fail(f'{item_id}: unknown group {value}')
        if kind == 'character_id' and value not in ids:
            fail(f'{item_id}: unknown character {value}')


def main() -> None:
    root = json.loads(MANIFEST.read_text(encoding='utf-8'))
    regenerated, _, _ = generator.build()
    if root != regenerated:
        fail('manifest is stale; run python3 tools/build_gear.py')
    characters = json.loads(generator.CHARACTERS.read_text(encoding='utf-8'))['characters']
    groups = {group for character in characters for group in character['groups']}
    ids = {character['id'] for character in characters}
    for line in root['unparsed']:
        if not ALLOWED_UNPARSED.match(line):
            fail(f'unparsed aura sentence: {line}')
    for item in root['items']:
        if not (EXTRA_DIR / item['icon']).exists() and EXTRA_DIR.exists() and (EXTRA_DIR / 'gui/gear').exists():
            fail(f'{item["id"]}: missing icon {item["icon"]}')
        for category in item['categories']:
            if category['kind'] == 'group' and category['value'] not in groups:
                fail(f'{item["id"]}: unknown category group {category["value"]}')
        for effect in item['effects']:
            for field in ('who', 'vs', 'wearer'):
                check_terms(item['id'], effect.get(field, []), groups, ids)
    print(f'gear ok: {len(root["items"])} items')


if __name__ == '__main__':
    main()
