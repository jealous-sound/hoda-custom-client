from __future__ import annotations

import functools
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ORIGINAL_TABLE = (
    ROOT
    / 'client-additional-data/Heroes-of-Dragon-Age-v4-4-0-cache-androidp1/com.ea.game.dragonage_row/cache/Localization'
    / 'Loc_ENG_US.txt'
)
PROJECT_TABLE = ROOT / 'data/localization/economy/Loc_ENG_US.txt'
PROJECT_KEY_PREFIX = 'HODA_'


def table_keys(path: Path) -> frozenset[str]:
    lines = path.read_text(encoding='utf-8-sig').splitlines()
    return frozenset(line.partition('|')[0].lower() for line in lines if '|' in line)


@functools.cache
def original_keys() -> frozenset[str]:
    return table_keys(ORIGINAL_TABLE)


@functools.cache
def project_keys() -> frozenset[str]:
    return table_keys(PROJECT_TABLE)


def has_original(key: str) -> bool:
    return key.lower() in original_keys()


def project_key(name: str) -> str:
    return PROJECT_KEY_PREFIX + name.upper().replace('-', '_')


def unknown_keys(keys) -> list[str]:
    known = original_keys() | project_keys()
    return sorted({key for key in keys if key.lower() not in known})
