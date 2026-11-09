# Heroes of Dragon Age client

A native Heroes of Dragon Age client in C on raylib 5.5. It reads the original 4.4.0 game data, runs a
deterministic battle simulation, and plays offline or against the Go server in `../hoda-custom-backend`.

## Setup

- raylib 5.5 (`brew install raylib`; set `RAYLIB_DIR=` for another install), Python 3, and `uv` for the bakers.
- The original 4.4.0 data in `client-additional-data/` (git-ignored, never commit it):
  - `Heroes-of-Dragon-Age-v4-4-0-cache-androidp1/`: the Unity bundle cache (textures, audio, localization).
  - `Heroes-of-Dragon-Age-v4-4-0-androidp1.com.apk`: the APK (fonts, boot UI).
  - Derived files: `meshes/`, `char_pack/`, `characters.json`, `player_starter.json`,
    `anim_tools/display_scales.json`.
  - `extra/`: what the 4.4.0 data lacks, including generated art. It ships with `make content`.

Without a local copy the game downloads its data on first launch (see [Content](#content)).

## Commands

| Command | |
| --- | --- |
| `make` | builds `Heroes of Dragon Age.app`; `PLATFORM=linux` or `PLATFORM=windows` (MinGW-w64) build into `build/` |
| `make run` | builds and runs the game |
| `make play` | runs the game against a local backend from `../hoda-custom-backend` (`BACKEND_DIR=`) |
| `make test` | every check; needs the local asset cache |
| `make ci-test` | the checks that need no asset cache: golden replays, content and update tests, table validators |
| `make golden` | regenerates the battle corpus in `tests/golden/` |
| `make balance` | swaps every hero into reference PvP squads of its faction under `data/stat_budget.json` (stats, attack pattern and aura costs) and reports win rates; `BALANCE_ARGS="--hero UNIT_ID"` fails when that hero leaves the 45-55% band |
| `make format-check` | clang-format check; `make format` rewrites |
| `make dist` | a playable copy in `../hoda-custom-dist` (`DIST_DIR=`, `DIST_BACKEND_URL=`) |
| `make content` | publishes the data packs to R2 (`CONTENT_BUCKET=`, default `hoda-assets`; `npx wrangler login` once) |
| `python3 tools/check_comments.py` | fails on any comment or docstring in owned code |

Build variables: `SERVER_URL` (default `https://api.hodareborn.com`; empty means local play only), `CONTENT_URL`
(default `https://cdn.hodareborn.com`), `CLIENT_VERSION` (default `git describe --tags`), and
`RAYLIB_LINK=$RAYLIB_DIR/lib/libraylib.a` to link raylib statically. The game looks for `data/` and
`client-additional-data/` next to the app bundle or executable.

## Layout

- `source/`: one module per screen or system. `main.c` is the single translation unit and `battle_sim.c` the
  simulation. OS calls sit in `platform.c`, built separately so `<windows.h>` never meets raylib.
- `tests/`: C tests, the golden battle corpus and Python unit tests.
- `tools/`: `build_*.py` writes a table in `data/` (`--check`, run by `make test`, verifies it); `validate_*.py`
  checks one.
- `data/`: version-pinned rule tables, maps and rosters, `localization/` and `custom_heroes/`.
- `client-additional-data/`: tracked bakers next to the ignored asset cache.

## Running

At launch the player picks Local Game, saved in `player_save.dat`, or Online Game against `SERVER_URL`, where the
server owns the save and the clock. A first launch creates a guest account (`account.dat`) and runs the tutorial.

| Variable | |
| --- | --- |
| `HOD_BACKEND_URL` | skips the choice and plays online against this server |
| `HOD_LANGUAGE` | `en`, `fr`, `de`, `ja`, `zh-hant`, ...; overrides the system language and the saved choice |
| `HOD_SKIP_TUTORIAL=1` | skips the tutorial |
| `HOD_NEW_PLAYER` | a starting roster in place of `data/new_player.json`, such as `client-additional-data/player_starter.json` |
| `HOD_SAVE_PATH`, `HOD_ACCOUNT_PATH` | the local save and the guest account file |
| `HOD_CONTENT_URL`, `HOD_CONTENT_DIR` | the content server, and a data folder to download into even in a checkout |
| `HOD_SHOT=<file.png>` `HOD_SHOT_SCREEN=<name>` | renders one screen with sample data and saves it; names in `source/debug_harness.c` |

## Original assets

Name every original object with `CACHE_RESOURCE("name")`, `CACHE_BUNDLE_RESOURCE("bundle", "name")` or
`APK_RESOURCE("name")`. Any other string is read as a file path and fails silently when it is missing;
`tools/validate_cache_refs.py` rejects literal refs that miss the cache. Prefer an original object, and put anything
the 4.4.0 data lacks in `client-additional-data/extra/`.

Text comes from the original `Loc_*.txt` tables. New strings go in `data/localization/<area>/Loc_*.txt`, one file per
language for all twelve languages, with `HODA_` keys. `tools/validate_localization.py` checks keys and placeholders,
checks that every key the C code looks up exists, and rejects Russian literals in C outside the debug harness.

## Server sync

Online, the server owns the economy and decides battles. Purchases and refills wait for its answer. Other actions apply
at once and queue in `source/server_actions.c`, and the server's profile wins once the queue drains. For a battle, the
server issues a seed, replays its Go port of `battle_sim.c`, and pays its own result.

- The wire contract is `source/backend.c` here and `internal/api` in the backend; change both together.
- The backend embeds copies of most `data/` tables and the golden corpus. After changing them, run
  `scripts/sync-client-data.sh` in the backend; its tests fail on drift.
- After changing the simulation or its inputs, run `make golden` and then the sync. `make test` fails while the corpus
  is stale and names the first entry and field that differ. To find where two builds diverge, diff their
  `./tests/battle_replay_golden --trace <battle name>` output.
- Results must not depend on the compiler. Builds use `-ffp-contract=off`, and the corpus generator draws each random
  value in its own statement, because GCC and clang evaluate function arguments in opposite orders.

## Custom heroes

A custom hero is a shipped hero with new stats, name, model and portraits. Its spec is
`data/custom_heroes/<name>.json`; `veilwarden.json` is the worked example.

```sh
python3 tools/build_custom_heroes.py
uv run --offline --with numpy --with Pillow --with UnityPy python tools/bake_custom_heroes.py
```

The first command writes `data/custom_heroes.json`. The second bakes the git-ignored models, portraits and scales into
`client-additional-data/`. `build_custom_heroes.py --check` fails when a bake is missing or older than its spec.

## Content

When no `client-additional-data/` sits next to the game, the game keeps its data, saves and `account.dat` in a
per-user folder: `~/Library/Application Support/Heroes of Dragon Age`, `~/.local/share/Heroes of Dragon Age` (or
`$XDG_DATA_HOME`), or `%APPDATA%\Heroes of Dragon Age`. Each launch reads `manifest.json` from `CONTENT_URL` and
downloads only the packs that changed.

`make content` packs the files `make dist` would ship into deterministic zips named by SHA-256, uploads only the
changed packs, and uploads the manifest last. `python3 tools/publish_content.py --dry-run` builds the packs without
uploading them. Whenever a data table changes shape, bump `CONTENT_MANIFEST_FORMAT` in `source/content_update.c`
and `MANIFEST_FORMAT` in `tools/build_content_packs.py` together.

## Releases

`.github/workflows/build.yml` builds macOS, Linux and Windows packages, without game data, on every pull request and
every push to `master`, and runs `make ci-test`. Merging publishes nothing. To release, push a tag:

```sh
git tag v0.7.0
git push origin v0.7.0
```

The tag's run publishes a GitHub Release. The website and the server's `GET /v1/client` follow the latest release,
so players see an update prompt. A suffix such as `v0.7.0-rc.1` makes a pre-release that `releases/latest` skips.
Bump the minor version for features and the patch version for fixes. To block old clients from online play, raise
`HOD_MIN_CLIENT_VERSION` on the backend.

## License

The code in this repository is licensed under the GNU General Public License v3.0 (GPL-3.0-only); see `LICENSE`.

Heroes of Dragon Age is a trademark of Electronic Arts. The original game data, art, audio, fonts and APK that the
game reads or downloads are © Electronic Arts Inc. and are not covered by this license. This is a non-commercial fan
project and is not affiliated with or endorsed by Electronic Arts or BioWare.
