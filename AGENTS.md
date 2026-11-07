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
  `client-additional-data/`. Data, config and Markdown are out of scope.
- Keep interpreter lines (`#!`) and machine-read directives (`# noqa`, `# type: ignore`, `# pragma`,
  `// clang-format on/off`, `// NOLINT`).
- `python3 tools/check_comments.py` checks every owned file and exits non-zero on any comment or docstring.

## Layout

- `source/`: game code, one module per screen or system; `battle_sim.c` is the deterministic simulation.
- `tests/`: `battle_sim_test.c` and Python unit tests.
- `tools/`: data builders and validators.
- `data/`: version-pinned rule tables.
- `client-additional-data/`: tracked bakers plus a local, git-ignored asset cache; never commit the cache.
- `source/resource.c`: loads textures, audio and fonts straight from the original 4.4.0 cache and APK
  (`unity_asset.c`, `unity_texture.c`, `apk_archive.c`). Prefer an original object over an extracted file; put
  anything the 4.4.0 data lacks in `client-additional-data/extra/`.
- Name every original object with `CACHE_RESOURCE` / `CACHE_BUNDLE_RESOURCE` / `APK_RESOURCE`, including FX
  emitter textures in `battle.c`; a bare string is read as a file path and silently fails.
  `python3 tools/validate_cache_refs.py` (part of `make test`) rejects literal refs that miss the cache.

## Working here

- Build with `make`; run the area you changed with `make test` pieces (`./tests/battle_sim_test`, the
  `python3 -m unittest` modules) once.
- Never commit `player_save.dat`, recordings or the cache directory.
- When a task is done and builds with its checks passing, commit it, push the branch and open a pull request
  against `master` with `gh pr create`. Do this yourself: do not ask for permission to commit, push or open the PR.
  Later changes to the same task go to that branch and PR. Leave merging to the maintainer.
- Pushing a `v*` tag publishes a release that players download; leave tagging to the maintainer.
- Work on a branch named for the change, cut from `origin/master`; never commit to `master`, to a branch whose PR is
  already merged, or to a branch holding unrelated work.
