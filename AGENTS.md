# AGENTS.md

Heroes of Dragon Age native client: a raylib 5.5 game in C under `source/`, with Python data and asset tooling.
Setup, run and test details are in `README.md`.

## Agent rules

- Source code has no comments. Express intent through names, structure and tests; do not add explanatory
  comments or docstrings. If a comment would carry something the code does not show (a client quirk, a
  reconstruction choice, a magic number), make the code show it: a named constant, a well-named helper
  or a test that pins the behaviour. Command-line help is code: give a tool a `CLI_DESCRIPTION` constant
  instead of reading `__doc__`.
- Scope: C under `source/` and `tests/` (except `source/vendors/`) and Python under `tools/`, `tests/` and
  `heroes-of-dragons-cache/`. Data, config and Markdown are out of scope.
- Keep interpreter lines (`#!`) and machine-read directives (`# noqa`, `# type: ignore`, `# pragma`,
  `// clang-format on/off`, `// NOLINT`).
- `python3 tools/check_comments.py` checks every owned file and exits non-zero on any comment or docstring.

## Layout

- `source/`: game code, one module per screen or system; `battle_sim.c` is the deterministic simulation.
- `tests/`: `battle_sim_test.c` and Python unit tests.
- `tools/`: data builders and validators.
- `data/`: version-pinned rule tables.
- `heroes-of-dragons-cache/`: tracked bakers plus a local, git-ignored asset cache; never commit the cache.

## Working here

- Build with `make`; run the area you changed with `make test` pieces (`./tests/battle_sim_test`, the
  `python3 -m unittest` modules) once.
- Never commit `player_save.dat`, recordings or the cache directory.
