#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

import original_cache

CLI_DESCRIPTION = 'Check that every original-cache reference in the C sources and fx_presets.json names a cache object'
ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIR = ROOT / 'source'
DEFAULT_PRESETS = ROOT / 'data' / 'fx_presets.json'
LITERAL_REF = re.compile(r'CACHE_(?:BUNDLE_)?RESOURCE\(\s*"([^"]*)"\s*(?:,\s*"([^"]*)"\s*)?\)')
FORMAT_MARK = '%'
BARE_FX_TEXTURE = re.compile(r'\.tex\s*=\s*"|"[\w\-]+__[\w\- ]+\.png"')


def literal_refs(path: Path) -> list[tuple[int, str]]:
    found = []
    for number, line in enumerate(path.read_text().splitlines(), 1):
        for match in LITERAL_REF.finditer(line):
            first, second = match.groups()
            name = f'{first}/{second}' if second is not None else first
            if FORMAT_MARK not in name:
                found.append((number, original_cache.cache_ref(name)))
    return found


def bare_fx_textures(path: Path) -> list[int]:
    return [number for number, line in enumerate(path.read_text().splitlines(), 1) if BARE_FX_TEXTURE.search(line)]


def preset_textures(path: Path) -> list[tuple[str, str]]:
    root = json.loads(path.read_text())
    return [
        (preset['name'], emitter['tex'])
        for preset in root.get('presets', [])
        for emitter in preset.get('emitters', [])
        if emitter.get('tex')
    ]


def resolves_any(ref: str) -> bool:
    return any(original_cache.resolves(ref, class_id) for class_id in original_cache.NAMED_CLASSES)


def problems(source_dir: Path, presets: Path) -> list[str]:
    found = []
    for path in sorted(source_dir.glob('*.c')):
        label = path.name
        for number in bare_fx_textures(path):
            found.append(f'{label}:{number}: FX texture names an extracted file, not CACHE_BUNDLE_RESOURCE(bundle, name)')
        for number, ref in literal_refs(path):
            if not resolves_any(ref):
                found.append(f'{label}:{number}: {ref} is not in the original cache')
    for preset, ref in preset_textures(presets):
        if not original_cache.resolves(ref, original_cache.TEXTURE_2D):
            found.append(f'{presets.name}: {preset}: texture {ref} is not in the original cache')
    return found


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument('--source-dir', type=Path, default=SOURCE_DIR)
    parser.add_argument('--presets', type=Path, default=DEFAULT_PRESETS)
    args = parser.parse_args()
    found = problems(args.source_dir, args.presets)
    for problem in found:
        print(problem, file=sys.stderr)
    if found:
        return 1
    print('cache refs: ok')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
