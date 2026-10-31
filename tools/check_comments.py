#!/usr/bin/env python3
import argparse
import ast
import io
from pathlib import Path
import subprocess
import sys
import tokenize

ROOT = Path(__file__).resolve().parents[1]
CLI_DESCRIPTION = 'Reject comments and docstrings in the project-owned C and Python code'
C_DIRECTORIES = ('source', 'tests')
PYTHON_DIRECTORIES = ('tools', 'tests', 'client-additional-data')
VENDORED = ('source/vendors',)
PYTHON_PRESERVED = ('#!', '# noqa', '# type:', '# pragma')
C_PRESERVED = ('// clang-format', '// NOLINT')
DOCSTRING_HOLDERS = (ast.Module, ast.ClassDef, ast.FunctionDef, ast.AsyncFunctionDef)


def project_files():
    listing = subprocess.run(
        ['git', 'ls-files', '--cached', '--others', '--exclude-standard', '-z'],
        cwd=ROOT, check=True, capture_output=True, text=True,
    ).stdout
    return [ROOT / name for name in listing.split('\0') if name and (ROOT / name).is_file()]


def owned_files(directories, suffixes):
    return sorted(
        path for path in project_files()
        if path.suffix in suffixes
        and path.relative_to(ROOT).parts[0] in directories
        and not path.relative_to(ROOT).as_posix().startswith(VENDORED)
    )


def python_violations(path):
    source = path.read_text(encoding='utf-8')
    for token in tokenize.generate_tokens(io.StringIO(source).readline):
        if token.type == tokenize.COMMENT and not token.string.startswith(PYTHON_PRESERVED):
            yield token.start[0], 'comment'
    for node in ast.walk(ast.parse(source)):
        if isinstance(node, DOCSTRING_HOLDERS) and ast.get_docstring(node, clean=False) is not None:
            yield node.body[0].lineno, 'docstring'


def c_comment_spans(source):
    index, line, length = 0, 1, len(source)
    while index < length:
        char = source[index]
        pair = source[index:index + 2]
        if char in '"\'':
            index += 1
            while index < length and source[index] != char:
                index += 2 if source[index] == '\\' else 1
            index += 1
        elif pair == '//':
            end = source.find('\n', index)
            end = length if end < 0 else end
            yield index, end, line
            index = end
        elif pair == '/*':
            end = source.find('*/', index + 2)
            end = length if end < 0 else end + 2
            yield index, end, line
            line += source.count('\n', index, end)
            index = end
        else:
            line += char == '\n'
            index += 1


def c_violations(path):
    source = path.read_text(encoding='utf-8')
    for start, end, line in c_comment_spans(source):
        if not source[start:end].startswith(C_PRESERVED):
            yield line, 'comment'


def checked_files():
    for path in owned_files(PYTHON_DIRECTORIES, ('.py',)):
        yield path, python_violations
    for path in owned_files(C_DIRECTORIES, ('.c', '.h')):
        yield path, c_violations


def main(argv=None):
    argparse.ArgumentParser(description=CLI_DESCRIPTION).parse_args(argv)
    found = [(path, line, kind) for path, check in checked_files() for line, kind in check(path)]
    for path, line, kind in found:
        print(f'{path.relative_to(ROOT)}:{line}: {kind}')
    return int(bool(found))


if __name__ == '__main__':
    sys.exit(main())
