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
| `make ci-test` | the checks that need no asset cache: golden replays, squad strengths, content, update and install tests, table validators |
| `make golden` | regenerates the battle corpus in `tests/golden/` |
| `make squad-strength` | re-measures the PvP opponent squads into `data/pvp_squad_strength.json` and `data/pvp_rules.json` |
| `make balance` | PvP balance report from `tests/balance_check.c` (`BALANCE_ARGS=` passes options; see [Balance](#balance)) |
| `make format-check` | clang-format check; `make format` rewrites |
| `make dist` | a playable copy in `../hoda-custom-dist` (`DIST_DIR=`, `DIST_BACKEND_URL=`) |
| `make content` | publishes the data packs to R2 (`CONTENT_BUCKET=`, default `hoda-assets`; `npx wrangler login` once) |
| `python3 tools/check_comments.py` | fails on any comment or docstring in owned code |

Build variables: `SERVER_URL` (default `https://api.hodareborn.com`; empty means local play only), `CONTENT_URL`
(default `https://cdn.hodareborn.com`), `CLIENT_VERSION` (default `git describe --tags`), and
`RAYLIB_LINK=$RAYLIB_DIR/lib/libraylib.a` to link raylib statically, plus `RELEASE_URL` and `RELEASE_PUBLIC_KEY`
for [self-update](#self-update). The game looks for `data/` and
`client-additional-data/` next to the app bundle or executable.

## Layout

- `source/`: one module per screen or system. `main.c` is the single translation unit and `battle_sim.c` the
  simulation. OS calls sit in `platform.c`, built separately so `<windows.h>` never meets raylib, and
  `release_signature.c` wraps the vendored TweetNaCl (`vendors/tweetnacl.c`, public domain) the same way.
- `tests/`: C tests, the golden battle corpus and Python unit tests.
- `tools/`: `build_*.py` writes a table in `data/` (`--check`, run by `make test`, verifies it); `validate_*.py`
  checks one.
- `data/`: version-pinned rule tables, maps and rosters, `localization/` and `custom_heroes/`.
- `client-additional-data/`: tracked bakers next to the ignored asset cache.

## Running

At launch the player picks Local Game, saved in `player_save.dat`, or Online Game against `SERVER_URL`, where the
server owns the save and the clock. A first online launch asks whether to play as a guest or log in; either way the
device keeps its key in `account.dat`, and a new guest runs the tutorial.

Profile, the button beside the settings gear in the top bar, shows the player card and renames the player. Online it
also manages the account: a guest picks a login and a password to play on other devices, and a player with a login can
change the password, which logs out every other device, log out or delete the account. A device that was logged out
elsewhere returns to the log-in choice at its next server request. A forgotten password needs a reset by the server's
admin. The button's icon is `extra/gui/hud/profile.png`; without that file the bar shows the original social button.

| Variable | |
| --- | --- |
| `HOD_BACKEND_URL` | skips the choice and plays online against this server |
| `HOD_LANGUAGE` | `en`, `fr`, `de`, `ja`, `zh-hant`, ...; overrides the system language and the saved choice |
| `HOD_SKIP_TUTORIAL=1` | skips the tutorial |
| `HOD_NEW_PLAYER` | a starting roster in place of `data/new_player.json`, such as `client-additional-data/player_starter.json` |
| `HOD_SAVE_PATH`, `HOD_ACCOUNT_PATH` | the local save and the device's account file |
| `HOD_CONTENT_URL`, `HOD_CONTENT_DIR` | the content server, and a data folder to download into even in a checkout |
| `HOD_RELEASE_URL` | where a release's `client-update.json` and zips are fetched from, in place of the GitHub releases |
| `HOD_NO_SELF_UPDATE=1` | never downloads a build or hands over to a downloaded one; update prompts open the website |
| `HOD_SHOT=<file.png>` `HOD_SHOT_SCREEN=<name>` | renders one screen with sample data and saves it; names in `source/debug_harness.c` |
| `HOD_SHOT_INSTALL=<state>` | with an update screen: `offer`, `downloading`, `restarting` or `failed` previews the self-update popup, `live` installs for real |

## Original assets

Name every original object with `CACHE_RESOURCE("name")`, `CACHE_BUNDLE_RESOURCE("bundle", "name")` or
`APK_RESOURCE("name")`. Any other string is read as a file path and fails silently when it is missing;
`tools/validate_cache_refs.py` rejects literal refs that miss the cache. Prefer an original object, and put anything
the 4.4.0 data lacks in `client-additional-data/extra/`.

Text comes from the original `Loc_*.txt` tables. New strings go in `data/localization/<area>/Loc_*.txt`, one file per
language for all twelve languages, with `HODA_` keys. `tools/validate_localization.py` checks keys and placeholders,
checks that every key the C code looks up exists, and rejects Russian literals in C outside the debug harness.

News that is not a Nexus event goes in `data/announcements.json`: `starts_at` and `ends_at` in Unix seconds and the
`title`, `subtitle`, `summary` and `message` keys, with the text in `data/localization/news`. An announcement leads the
What's New feed while it runs. Clients older than v0.9.0 ignore the file.

## Server sync

Online, the server owns the economy and decides battles. Purchases and refills wait for its answer. Other actions apply
at once and queue in `source/server_actions.c`, and the server's profile wins once the queue drains. For a battle, the
server issues a seed, replays its Go port of `battle_sim.c`, and pays its own result.

- The wire contract is `source/backend.c` here and `internal/api` in the backend; change both together.
- The backend embeds copies of most `data/` tables and the golden corpus. After changing them, run
  `scripts/sync-client-data.sh` in the backend; its tests fail on drift.
- A duel opponent, a guildmate or a ladder rival, is that player's current squad. With `"hero_builds": true` each of
  its units carries `consume_xp`, `ascended`, five `gear` slots and its `fortify` children beside `slot`, `id`, `tier`
  and `level`. The client fields those builds and sends them back as the `enemy` of `battle_start` with the same
  flag, and both simulations build that side like the attacker's. Bots and PvE rosters carry no builds.
- After changing the simulation or its inputs, run `make squad-strength`, then `make golden` and then the sync.
  `make test` fails while the squad strengths or the corpus are stale and names the first corpus entry and field that
  differ. To find where two builds diverge, diff their `./tests/battle_replay_golden --trace <battle name>` output.
- Results must not depend on the compiler. Builds use `-ffp-contract=off`, and the corpus generator draws each random
  value in its own statement, because GCC and clang evaluate function arguments in opposite orders.

## Balance

PvP fights on the heroes' original stats: the numbers on a hero's card are the numbers in battle, and runes, gear,
passives, formation bonuses and event buffs work as they do in PvE. A battle whose enemy roster carries the PvP rules
reads them from `data/stat_budget.json` and `data/stat_budget_heroes.json`. PvP opponents carry them while
`battle.stat_budget` in `data/pvp_rules.json` is on (set in `tools/build_pvp_rules.py`), as it ships. The backend
replays PvP under the same rules and the same switches, so change them in both through the data sync, with a client
release and a raised minimum client version on the backend.

- `scale_stats` in `data/stat_budget.json` picks the mode and ships `false`. The one PvP rule left is then the trim list
  in `data/stat_budget_heroes.json`: a listed hero fights with that percent of its stats, and its status chances (stun,
  slow, curse, levy, assassinate, mark), what its ability grants allies and what its passives give or take are cut by
  the same share. A hero that is not listed fights as in PvE. The golden corpus records this mode and leads each side of
  a battle under the rules with a trimmed hero.
- `./tests/balance_check --calibrate 10 --ceiling 90 --squads 64 --seeds 12` rewrites the trim list. It lowers only the
  heroes that win more than 90% against a peer in the same seat, until they stop, and writes only those. It starts from
  the list in the file, so run it again after adding a hero that dominates, then run `make squad-strength` and
  `make golden`.
- With `scale_stats` set to `true` the stat budget scales every hero to one cost instead, and
  `data/stat_budget_heroes.json` becomes a handicap for every hero. That mode is switched off as it ships; the unit
  tests cover it and the golden corpus does not. The rules below, down to the handicap table, apply only to it.
- `data/stat_budget.json` holds the scaling rules: one stat cost per hero (`power x health_per_power + health`), a
  higher power cost for attacks that hit a row, a column or everyone, and a budget cut for the health, power and bonus
  damage a hero's passives hand out. The cost is priced on the hero fully grown: its top tier at that tier's level cap,
  before ascension and fortify. The same scale is then applied to the stats the hero really has, so a hero missing
  levels or a tier fights with that share of its budget, and ascension levels and fortify add on top.
  `rarity_budget_pct` is each rarity's share of the budget. Auras stop at `aura_cap_pct` and the health and power that
  gear adds to a hero or takes from it stop at `gear_cap_pct`, each on its own. Every hero adds the same formation bonus
  and event buffs stop at `changer_cap_pct`. Every rune works except the flat damage shield, which is larger than a PvP
  hit.
- The same file keeps one item or one rune from deciding a fight on its own. `data/gear.json` and `data/runes.json` stay
  as the original game had them and a battle without the stat budget reads none of these keys, so PvE is untouched.
  Runes of power, health and damage resist count `rune_stat_share_pct` percent of their value, and a rune of speed moves
  initiative by `rune_initiative_share_pct` percent of a class step. Flat damage resist, sized for PvE hits, becomes
  percent resist at `flat_resist_per_pct` points per percent. What one item hands out when the battle starts (power,
  health, double damage chance, bonus damage and damage resist, summed over every hero it touches on either side) is
  scaled down to `gear_item_cap_pct` percent of a hero. What one item's triggers give during the battle (healing that
  restores missing health, power gained or drained, damage and the same stat points, each as a share of the receiving
  hero's stat) is metered against `gear_trigger_cap_pct` percent of a hero: the firing that would pass it is scaled to
  what is left, and later firings of those effects give nothing. Status resists, initiative and targeting effects are
  not limited.
- A new key in `data/stat_budget.json` raises `STAT_BUDGET_VERSION` in `source/stat_budget.c` and `statBudgetVersion` in
  the backend, and a client built for the old version refuses the file. Release the client and raise the minimum client
  version on the backend before publishing content that carries the new file.
- Under scaling, `data/stat_budget_heroes.json` holds one handicap per hero, a percent of the budget from 33 to 300.
  Below 100 it also cuts the hero's status chances and what it grants allies by the same share. `./tests/balance_check
  --calibrate 16 --neutral --squads 48 --seeds 6` rewrites the whole table.
- `./tests/balance_check` seats every hero in reference squads of its faction against a peer in the same seat and prints
  win rates by class. `--holdout` uses squads and peers the calibration never saw, `--stacks` fields four copies,
  `--squad-report` fights the reference squads against each other and `--search 3000` climbs to the strongest squad it
  can find per faction.
- `./tests/balance_check --adversarial 8` measures what the reference squads cannot: how far a squad built to win gets.
  Each round it climbs, by single swaps and seat changes, to the squads that best beat the current meta, one climb from
  the strongest pooled squad and one from a random squad for every faction and for squads across factions, in parallel.
  It adds them to the pool and solves the meta again: the mix of pooled squads that no pooled squad beats. Round 1's
  meta is the reference pool, so its row is how hard the ladder's squads can be farmed. The report ends on the meta's
  squads, its faction shares and its most fielded heroes; a level game has a wide meta with every faction in it. It
  changes no table. `--swaps N` sets the length of a climb and `--unique` forbids copies of a hero.
- Under scaling, `./tests/balance_check --adversarial 2 --swaps 1000 --calibrate 40`, run after the seat calibration,
  rewrites the handicaps against that meta. It runs 40 passes of the search and carries the squads it found from pass to
  pass. Each pass lowers a hero's handicap by its share of the meta and lets it recover toward the handicap it started
  with, so a hero pays for the squads it wins in rather than for its average squad. The table written is the mean of the
  last half of the passes.
- A new hero under scaling: `./tests/balance_check --calibrate 12 --hero UNIT_ID` settles its handicap against the
  existing table, and `./tests/balance_check --hero UNIT_ID` exits non-zero while it wins outside 45-55%. On original
  stats a new hero needs no entry unless it wins more than the ceiling.
- `./tests/balance_check --progress` measures what a player's work is worth: reference squads fight themselves with one
  side changed, and the report lists the win rate of each growth stage, rarity, fortify load, rune and gear item. Runes
  and items are ranked from the strongest with the battles behind each win rate and the mean win rate of each rarity,
  which should rise with rarity; `--all` lists every rune and item instead of the first twelve. An item only one
  reference squad can wear fights few battles, so read its row with `--seeds 16` and with that in mind. The limits above
  aim at no single rune and no single item on one hero winning more than about 65%. Every other mode judges fully grown
  heroes at one rarity share, so the handicaps stay a measure of the kit alone.
- Every squad in `squad_pool` carries a `strength`: its wins per 10000 battles against the rest of the pool, attacking
  and defending once against each, fielded as the ladder fields it. A bot's trophies pick its squads: a bot at zero
  draws from the weakest squads and the top bot from the strongest, so opponents get harder as trophies rise, whatever
  the player's level. Pooled squads fight at the tier and level of their PvE roster, and the three-hero rosters are what
  a new account meets. `make squad-strength` measures them again after a change to the simulation, the handicaps or the
  pool, and `./tests/pvp_squad_strength --report` prints the spread and the strength met at each trophy count. The
  strengths hold for the `battle.stat_budget` switch they were measured under, so measure again after flipping it. The
  pool keeps its order, with the three-hero squads after the rest, so a squad index stored in a save or a pending match
  still names the same squad.

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

The same endpoint announces a server restart. Online players get a popup and a countdown in its last half hour, and
battles cannot start from 3 minutes before it until the server is back; `scripts/schedule-maintenance.sh` in the
backend sets the time.

### Self-update

The update prompt's **Update** button installs the new release without the website. The installed app stays where
the player put it and acts as the launcher: new builds are unpacked into `client/<version>/` in the game's data
folder, and every launch hands over to the newest one there (`source/client_install.c`).

- The tag's run writes `client-update.json` (`tools/build_release_manifest.py`: each platform zip's size, SHA-256
  and executable) and signs it with the Ed25519 key in the `RELEASE_SIGNING_KEY` secret; the run fails without it.
  The matching public key is `RELEASE_PUBLIC_KEY` in the `Makefile`. To replace the key, change both, and note that
  builds carrying the old public key can only update through the website.
- The game accepts a manifest only if the signature matches, its version is the tag it asked for and that tag is
  newer than the running build; the zip must match the manifest's size and SHA-256.
- A downloaded build that fails to open a window on two launches in a row is dropped and the installed app runs
  instead. Any failure to install falls back to the website button.
- Only tagged builds update themselves; a `git describe` build with commits past its tag, a run from a checkout
  and `HOD_NO_SELF_UPDATE=1` never do.

## License

The code in this repository is licensed under the GNU General Public License v3.0 (GPL-3.0-only); see `LICENSE`.

Heroes of Dragon Age is a trademark of Electronic Arts. The original game data, art, audio, fonts and APK that the
game reads or downloads are © Electronic Arts Inc. and are not covered by this license. This is a non-commercial fan
project and is not affiliated with or endorsed by Electronic Arts or BioWare.
