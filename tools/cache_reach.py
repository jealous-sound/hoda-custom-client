#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from collections import defaultdict
from collections.abc import Callable
from dataclasses import dataclass, field
from pathlib import Path

import original_cache

CLI_DESCRIPTION = (
    "Work out which original-cache bundles the game can read. resource.c looks textures, audio clips and fonts up by "
    "name, optionally as bundle/name, and takes the alphabetically first bundle holding a name. A bundle ships when "
    "it is that first holder of a name a C string or shipped data string can spell, when a bundle/name string names "
    "one of its objects, or when a C string names the bundle and any string names one of its objects."
)

ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIR = ROOT / "source"
VENDOR_DIR = "vendors"
SOURCE_SUFFIXES = (".c", ".h")
APK_REF_PREFIX = "apk:"
QUALIFIER = "/"
ANY_TEXT = ".*"
C_TOKEN = re.compile(r"'(?:[^'\\\n]|\\.)*'|\"((?:[^\"\\\n]|\\.)*)\"")
C_STRING_DEFINE = re.compile(r"^\s*#\s*define\s+\w+\s+\"((?:[^\"\\\n]|\\.)*)\"\s*$", re.MULTILINE)
C_ESCAPE = re.compile(r"\\(.)")
PRINTF_CONVERSION = re.compile(r"%[-+ #0]*(?:\d+|\*)?(?:\.(?:\d+|\*))?(?:hh|h|ll|l|L|z|j|t)?([diouxXeEfgGcsp%])")
DIGIT_CONVERSIONS = frozenset("diouxX")
PATTERN_MIN_LITERAL = 3
STEM_MIN_LENGTH = 6
STEM_ENDINGS = ("_", ".")


@dataclass
class Bundle:
    path: str
    name: str
    objects: list[tuple[int, str]]


@dataclass
class Spelled:
    exact: set[str] = field(default_factory=set)
    patterns: list[str] = field(default_factory=list)

    def add(self, text: str) -> None:
        if not PRINTF_CONVERSION.search(text):
            self.exact.add(text)
            return
        pattern = format_regex(text)
        if pattern is not None:
            self.patterns.append(pattern)

    def matcher(self) -> Callable[[str], bool]:
        if not self.patterns:
            return lambda text: text in self.exact
        compiled = re.compile("|".join(f"(?:{pattern})" for pattern in sorted(set(self.patterns))))
        return lambda text: text in self.exact or compiled.fullmatch(text) is not None


@dataclass
class Reach:
    names: Spelled = field(default_factory=Spelled)
    qualified: set[tuple[str, str]] = field(default_factory=set)
    qualified_patterns: list[tuple[str, str]] = field(default_factory=list)
    source_bundles: Spelled = field(default_factory=Spelled)


def c_strings(text: str) -> list[str]:
    return [C_ESCAPE.sub(r"\1", match.group(1)) for match in C_TOKEN.finditer(text) if match.group(1) is not None]


def c_stems(text: str) -> list[str]:
    return [
        C_ESCAPE.sub(r"\1", value)
        for value in C_STRING_DEFINE.findall(text)
        if len(value) >= STEM_MIN_LENGTH and value.endswith(STEM_ENDINGS)
    ]


def source_files(source_dir: Path) -> list[Path]:
    if not source_dir.is_dir():
        return []
    return sorted(
        path
        for path in source_dir.rglob("*")
        if path.suffix in SOURCE_SUFFIXES and VENDOR_DIR not in path.relative_to(source_dir).parts
    )


def json_strings(value: object) -> list[str]:
    if isinstance(value, str):
        return [value]
    if isinstance(value, dict):
        return [text for key, child in value.items() for text in [key, *json_strings(child)]]
    if isinstance(value, list):
        return [text for child in value for text in json_strings(child)]
    return []


def data_strings(path: Path) -> list[str]:
    if path.suffix == ".json":
        return json_strings(json.loads(path.read_text()))
    return path.read_text().splitlines()


def format_regex(text: str) -> str | None:
    pieces = []
    literal = 0
    position = 0
    for match in PRINTF_CONVERSION.finditer(text):
        chunk = text[position : match.start()]
        pieces.append(re.escape(chunk))
        literal += len(chunk.replace(QUALIFIER, ""))
        conversion = match.group(1)
        if conversion == "%":
            pieces.append("%")
            literal += 1
        else:
            pieces.append(r"\d+" if conversion in DIGIT_CONVERSIONS else ANY_TEXT)
        position = match.end()
    chunk = text[position:]
    pieces.append(re.escape(chunk))
    literal += len(chunk.replace(QUALIFIER, ""))
    return "".join(pieces) if literal >= PATTERN_MIN_LITERAL else None


def side_regex(text: str) -> str | None:
    if not text:
        return None
    return format_regex(text) if PRINTF_CONVERSION.search(text) else re.escape(text)


def ref_text(raw: str) -> str | None:
    return None if raw.startswith(APK_REF_PREFIX) else raw.removeprefix(original_cache.CACHE_REF_PREFIX)


def add_ref(reach: Reach, text: str) -> None:
    reach.names.add(text)
    if QUALIFIER not in text:
        return
    bundle, _, name = text.rpartition(QUALIFIER)
    reach.names.add(name)
    if not PRINTF_CONVERSION.search(text):
        reach.qualified.add((bundle, name))
        return
    bundle_pattern, name_pattern = side_regex(bundle), side_regex(name)
    if bundle_pattern is not None or name_pattern is not None:
        reach.qualified_patterns.append((bundle_pattern or ANY_TEXT, name_pattern or ANY_TEXT))


def gather(source: list[str], data: list[str], stems: list[str]) -> Reach:
    reach = Reach()
    for raw in source + data:
        text = ref_text(raw)
        if text is not None:
            add_ref(reach, text)
    for raw in source:
        text = ref_text(raw)
        if text is not None:
            reach.source_bundles.add(text.rpartition(QUALIFIER)[0] or text)
    reach.names.patterns += [re.escape(stem) + ANY_TEXT for stem in stems]
    return reach


def first_holders(bundles: list[Bundle], spelled: Callable[[str], bool]) -> set[str]:
    holders: dict[tuple[int, str], list[Bundle]] = defaultdict(list)
    for bundle in bundles:
        for key in dict.fromkeys(bundle.objects):
            holders[key].append(bundle)
    kept: set[str] = set()
    for (_, name), owners in holders.items():
        if spelled(name):
            first = min(owner.name for owner in owners)
            kept.update(owner.path for owner in owners if owner.name == first)
    return kept


def named_bundle_reached(
    bundle: Bundle,
    reach: Reach,
    spelled: Callable[[str], bool],
    in_source: Callable[[str], bool],
    pairs: list[tuple[re.Pattern[str], re.Pattern[str]]],
) -> bool:
    names = {name for _, name in bundle.objects}
    if any((bundle.name, name) in reach.qualified for name in names):
        return True
    for bundle_pattern, name_pattern in pairs:
        if bundle_pattern.fullmatch(bundle.name) and any(name_pattern.fullmatch(name) for name in names):
            return True
    return in_source(bundle.name) and any(spelled(name) for name in names)


def reachable_bundles(bundles: list[Bundle], reach: Reach) -> set[str]:
    spelled = reach.names.matcher()
    in_source = reach.source_bundles.matcher()
    pairs = [(re.compile(bundle), re.compile(name)) for bundle, name in dict.fromkeys(reach.qualified_patterns)]
    kept = first_holders(bundles, spelled)
    kept.update(
        bundle.path
        for bundle in bundles
        if bundle.path not in kept and named_bundle_reached(bundle, reach, spelled, in_source, pairs)
    )
    return kept


def bundle_files(root: Path, bundle_dir: str) -> list[str]:
    base = root / bundle_dir
    if not base.is_dir():
        raise SystemExit(f"missing {base}")
    return sorted(
        str(path.relative_to(root)) for path in base.glob(f"*/{original_cache.BUNDLE_FILE_PREFIX}*") if path.is_file()
    )


def load_bundles(root: Path, files: list[str]) -> list[Bundle]:
    return [
        Bundle(relative, original_cache.bundle_name(Path(relative)), original_cache.named_objects(root / relative))
        for relative in files
    ]


def game_reach(source_dir: Path, data_files: list[Path]) -> Reach:
    source: list[str] = []
    stems: list[str] = []
    for path in source_files(source_dir):
        text = path.read_text()
        source += c_strings(text)
        stems += c_stems(text)
    data = [text for path in data_files for text in data_strings(path)]
    return gather(source, data, stems)


def main() -> int:
    import build_dist

    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--list-dropped", action="store_true", help="print every bundle that does not ship")
    args = parser.parse_args()
    every = bundle_files(ROOT, build_dist.ORIGINAL_BUNDLES)
    kept = set(build_dist.runtime_bundle_files(ROOT, build_dist.loose_content_files(ROOT)))
    dropped = [relative for relative in every if relative not in kept]
    megabytes = lambda files: sum((ROOT / relative).stat().st_size for relative in files) / 1e6
    print(
        f"{len(kept)} of {len(every)} bundles ship ({megabytes(kept):.0f} MB); "
        f"{len(dropped)} do not ({megabytes(dropped):.0f} MB)"
    )
    if args.list_dropped:
        print("\n".join(dropped))
    return 0


if __name__ == "__main__":
    sys.exit(main())
