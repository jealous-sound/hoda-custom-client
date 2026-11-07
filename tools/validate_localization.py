#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

CLI_DESCRIPTION = (
    "Check the project localization: every area under data/localization carries the same keys in all twelve "
    "languages with matching {N} placeholders, every literal key the C sources look up exists, and no C string "
    "literal outside the debug harness holds Cyrillic text."
)

ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIR = ROOT / "source"
PROJECT_DIR = ROOT / "data" / "localization"
ORIGINAL_DIR = (
    ROOT
    / "client-additional-data/Heroes-of-Dragon-Age-v4-4-0-cache-androidp1/com.ea.game.dragonage_row/cache/Localization"
)
REFERENCE_FILE = "Loc_ENG_US.txt"
LANGUAGE_FILES = (
    "Loc_ENG_US.txt",
    "Loc_FRE_FR.txt",
    "Loc_GER_DE.txt",
    "Loc_ITA_IT.txt",
    "Loc_SPA_XM.txt",
    "Loc_POR_BR.txt",
    "Loc_DUT_NL.txt",
    "Loc_RUS_RU.txt",
    "Loc_JPN_JP.txt",
    "Loc_KOR_KR.txt",
    "Loc_CHS_CN.txt",
    "Loc_CHT_CN.txt",
)
PROJECT_KEY_PREFIX = "HODA_"
CYRILLIC_EXEMPT_SOURCES = frozenset({"debug_harness.c"})
PLACEHOLDER = re.compile(r"\{\d\}")
STRING_LITERAL = re.compile(r'"((?:[^"\\\n]|\\.)*)"')
CYRILLIC = re.compile("[Ѐ-ӿ]")
LOOKUP_KEY = re.compile(
    r'\bLoc(?:Get|GetOr|GetAbility)\(\s*"([A-Za-z0-9_]+)"'
    r'|\bLocFormat\w*\(\s*[^,()]+(?:\([^()]*\))?\s*,\s*[^,()]+(?:\([^()]*\))?\s*,\s*"([A-Za-z0-9_]+)"'
)


def read_table(path: Path) -> dict[str, str]:
    table: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        key, separator, value = line.partition("|")
        if separator:
            table[key.lower()] = value
    return table


def project_errors() -> list[str]:
    errors = []
    if not PROJECT_DIR.is_dir():
        return errors
    for area in sorted(path for path in PROJECT_DIR.iterdir() if path.is_dir()):
        tables = {}
        for name in LANGUAGE_FILES:
            path = area / name
            if not path.is_file():
                errors.append(f"{path.relative_to(ROOT)}: missing")
                continue
            tables[name] = read_table(path)
        reference = tables.get(REFERENCE_FILE, {})
        for key in reference:
            if not key.startswith(PROJECT_KEY_PREFIX.lower()):
                errors.append(f"{area.name}: key {key.upper()} lacks the {PROJECT_KEY_PREFIX} prefix")
        for name, table in tables.items():
            for key in sorted(reference.keys() - table.keys()):
                errors.append(f"{area.name}/{name}: missing {key.upper()}")
            for key in sorted(table.keys() - reference.keys()):
                errors.append(f"{area.name}/{name}: {key.upper()} is not in {REFERENCE_FILE}")
            for key, value in table.items():
                if not value.strip():
                    errors.append(f"{area.name}/{name}: {key.upper()} is empty")
                expected = sorted(PLACEHOLDER.findall(reference.get(key, "")))
                if key in reference and sorted(PLACEHOLDER.findall(value)) != expected:
                    errors.append(f"{area.name}/{name}: {key.upper()} placeholders differ from {REFERENCE_FILE}")
    return errors


def known_keys() -> set[str] | None:
    original = ORIGINAL_DIR / REFERENCE_FILE
    if not original.is_file():
        return None
    keys = set(read_table(original))
    if PROJECT_DIR.is_dir():
        for path in PROJECT_DIR.glob(f"*/{REFERENCE_FILE}"):
            keys |= set(read_table(path))
    return keys


def source_errors(keys: set[str] | None) -> list[str]:
    errors = []
    for path in sorted(SOURCE_DIR.glob("*.c")):
        text = path.read_text(encoding="utf-8")
        for number, line in enumerate(text.splitlines(), 1):
            if path.name not in CYRILLIC_EXEMPT_SOURCES:
                for match in STRING_LITERAL.finditer(line):
                    if CYRILLIC.search(match.group(1)):
                        errors.append(f"{path.relative_to(ROOT)}:{number}: Cyrillic literal {match.group(0)}")
            if keys is None:
                continue
            for match in LOOKUP_KEY.finditer(line):
                key = match.group(1) or match.group(2)
                if key.lower() not in keys:
                    errors.append(f"{path.relative_to(ROOT)}:{number}: unknown localization key {key}")
    return errors


def main() -> int:
    argparse.ArgumentParser(description=CLI_DESCRIPTION).parse_args()
    errors = project_errors() + source_errors(known_keys())
    for error in errors:
        print(error)
    print(f"localization: {'ok' if not errors else f'{len(errors)} problems'}")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
