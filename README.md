# Heroes of Dragon Age client

Native raylib 5.5 client in C for Heroes of Dragon Age: home, heroes, campaign, Nexus, Store and quests, with a
deterministic battle simulation.

## Layout

- `source/`: game code; `main.c` is the single translation unit entry, `vendors/` holds cJSON.
- `tests/`: battle simulation test and Python unit tests.
- `tools/`: build and validation scripts for data tables.
- `data/`: version-pinned rule tables and Nexus maps and rosters.
- `client-additional-data/`: model and animation bakers; the rest of the directory is the git-ignored local asset cache.

## Game data

The client reads the original 4.4.0 files as shipped, from `client-additional-data/`:

- `Heroes-of-Dragon-Age-v4-4-0-cache-androidp1/`: the Unity bundle cache. Textures, audio and localization load
  from it directly.
- `Heroes-of-Dragon-Age-v4-4-0-androidp1.com.apk`: the APK. Fonts and the boot UI textures load from it directly.

This directory used to be `heroes-of-dragons-cache/`. In an older clone, move the local files over once with
`mv heroes-of-dragons-cache client-additional-data`.

Everything else it needs is derived or post-4.4:

- `meshes/`, `char_pack/`: scene geometry and baked unit models.
- `characters.json`, `player_starter.json`, `anim_tools/display_scales.json`: derived catalogs.
- `extra/`: files the 4.4.0 data lacks (post-4.4 portraits, the curse icons, the click sound).

In code, `CACHE_RESOURCE("name")`, `CACHE_BUNDLE_RESOURCE("bundle", "name")` and `APK_RESOURCE("name")` name an
original object; any other string is a file path.

## Custom heroes

A custom hero is a new hero derived from a shipped one. Its spec lives in `data/custom_heroes/<name>.json`:
`based_on` names the shipped hero, `character` overrides catalog fields (name, Russian name, `obtainable`),
`stat_scale` scales power and health, and `look` describes the new model. `python3 tools/build_custom_heroes.py`
writes the rows to `data/custom_heroes.json`, which the game appends to the hero catalog. A custom hero keeps its
base hero's ability, passives, ability VFX, audio and XP tables, and has its own name, stats, models and portraits.

```sh
python3 tools/build_custom_heroes.py
uv run --offline --with numpy --with Pillow --with UnityPy python tools/bake_custom_heroes.py
```

The baker starts from the base hero's runtime bake (`look.tiers.<tier>.source`) and writes the local, git-ignored
assets: `char_pack/<prefab>/<prefab>_tN_rl.glb` with its textures, `extra/portraits/tex.<prefab>_tN_pt.png` in the
base hero's rarity frame, `display_scales.json` entries, and `char_pack/<prefab>/<prefab>_bake.json`, the hash of the
spec it baked. `python3 tools/build_custom_heroes.py --check` (part of `make test`) fails when a bake is missing or was
made from an older spec. `look` holds:

- `texture`: `recolor` maps a hue band to a new hue, optionally only on some bones; `gradient` tints by height;
  `strokes` draws glowing lines on the front of a bone, such as face markings.
- `halo`: a ring with optional shards on its own bone under `bone`, spinning a set number of turns per clip.
- `clips`: each runtime clip (`idle` first) comes from a `clip` of the base bake, or of a `source` bake on the same
  skeleton, with `speed`, `window` and `time_keys`, plus `layers` that rotate or move a bone in the body's frame by
  keys or waves.
- `portrait`: the camera and backdrop for the rendered portrait.

`data/custom_heroes/veilwarden.json`, Veilwarden Ithriel from Solas, is the worked example.

## Commands

```sh
make run
make play
make test
make format-check
python3 tools/check_comments.py
```

`make play` runs the game against the Go backend in `../hoda-custom-backend` (override with
`BACKEND_DIR=`), which then owns the save and the clock. With `HOD_BACKEND_URL` unset the game uses
`player_save.dat` as before. `hoda-original-backend` is the separate Python server for the original
Android client.

Against the backend, the server owns the economy: gold, gems, vouchers, energy, level, potions, keys, tomes and which
heroes, runes and gear exist. Store purchases and refills wait for the server's answer. Battles, Nexus awards,
mission and event claims, gear sales, prestige slots, ascension, rune crafting and tutorial rewards are applied
locally at once and queued in `source/server_actions.c`; the queue sends them in order and, once it is empty and the
player is out of battle, loads the profile the server sends back, so the server's numbers win. A Nexus recap waits
for the server so it shows the heroes the server rolled. `data/hero_pool.json` (`tools/build_hero_pool.py`) is the
hero list the server rolls from; after changing it or `store_packs.json`, `runes.json`, `gear.json`,
`tutorial_rewards.json`, `campaign_maps.json`, `nexus_maps.json`, `pvp_rules.json`, `missions.json` or
`new_player.json`, run `scripts/sync-client-data.sh` in the backend. Store schedules use UTC dates.

`make` builds `Heroes of Dragon Age.app`, a macOS app bundle with its icon from `packaging/app_icon.png` and its
`Info.plist` from `packaging/macos/`. The game runs from the folder that holds the bundle, so it finds `data/` and
`client-additional-data/` next to it however it is launched.

`make PLATFORM=linux` builds `build/linux/Heroes of Dragon Age`; `make PLATFORM=windows` cross-compiles
`build/windows/Heroes of Dragon Age.exe` with MinGW-w64 and embeds `packaging/windows/app_icon.ico`. Point
`RAYLIB_DIR=` at a raylib 5.5 release and set `RAYLIB_LINK=$RAYLIB_DIR/lib/libraylib.a` to link it statically.
OS-specific calls (sockets, positional reads, atomic file replace, CPU count, UTC time) live in
`source/platform.c`, a separate translation unit so `<windows.h>` never meets raylib's names.

`.github/workflows/build.yml` builds all three on every push to `master` (or by hand from the Actions tab) and
attaches a universal macOS app, a Linux x86-64 binary and a Windows x86-64 executable to the run as artifacts. They
carry no game data and download it on first launch.

## Downloaded game data

When no `client-additional-data/` sits next to the game (a CI build, or any copy without `make dist` data), the game
keeps its data in a per-user folder: `~/Library/Application Support/Heroes of Dragon Age` on macOS,
`$XDG_DATA_HOME` or `~/.local/share/Heroes of Dragon Age` on Linux, and `%APPDATA%\Heroes of Dragon Age` on
Windows. Saves and `account.dat` live there too. On every launch it reads `manifest.json` from `CONTENT_URL` (baked
in at build time; `HOD_CONTENT_URL` overrides it), downloads only the packs it doesn't have yet, checks their SHA-256,
unpacks them and deletes files that later versions dropped. Interrupted downloads resume. Without a connection it
starts with the data it already has. `HOD_CONTENT_DIR=<folder>` uses another folder and forces downloading even in a
checkout. The APK and core packs install first; from then on the download screen is 5.4.4's `LoadingScreenV` (splash, spinner, `loader_message` plate with the original Russian messages, red bar, `МБ из` counter and percentage) and errors use the game's connection and download-error popups. Downloads use libcurl on macOS and Linux (loaded at runtime, so any distro's `libcurl.so.4` works) and
WinHTTP on Windows.

`make content` publishes the data from this checkout to the R2 bucket `CONTENT_BUCKET` (default `hoda-assets`)
through Wrangler (`npx wrangler login` once). `tools/build_content_packs.py` groups the files `make dist` would ship
into deterministic zip packs (rule tables, the APK, core files, and hashed buckets of heroes, scenes, Unity bundles
and extras) named by their SHA-256. Packs whose files did not change keep their published entry, so a release
uploads, and players download, only the packs that changed. The manifest goes up last with `Cache-Control:
no-cache`; packs never change once uploaded and are cached for a year. `--dry-run` builds into `build/content`
without uploading.

`make dist` builds a playable copy in `../hoda-custom-dist` (override with `DIST_DIR=`): the app bundle with raylib
in its `Frameworks`, `data/`, and only the parts of `client-additional-data` the game reads (about 2.1 GB of the
local cache). Players open the app or run `play.sh`; `DIST_BACKEND_URL=http://host:port` makes that the default
server for both.
Rebuilding replaces only a folder this target made. Most of the copy is EA's original 4.4.0 game data.

A first launch (no save, or no backend profile) creates a guest account and runs the 5.4.4 tutorial on the
three-hero starter squad in `data/new_player.json`; the name prompt follows it. Against the backend the guest's id
and secret live in `account.dat` (`HOD_ACCOUNT_PATH`). `HOD_SKIP_TUTORIAL=1` skips the tutorial and
`HOD_NEW_PLAYER=<file>` starts from another roster, such as the full `client-additional-data/player_starter.json`.
The script comes from the original prefab via `tools/build_ftue_script.py` (UnityPy venv); its reconstructed parts
(intro battle, pack contents, giant, gem pack, parting gift) are marked in `data/tutorial_intro.json` and
`data/tutorial_rewards.json`.

Guilds and guild chat (5.4.4's only chat) need the backend. The Guild and Chat buttons sit under the news feed on the
home screen; Guild unlocks at level 5 and Chat once you are in a guild. The screens follow the original `GuildFindV`,
`GuildCreateV`, `GuildHomeV` and `GuildChatV` layouts and art from the 4.4.0 cache: find, search, create (10000 gold),
settings, member ranks, applications, chat with its event log, and rank-change notices on the home screen. The client
polls chat every 10 then 30 seconds while it is open and every 60 seconds otherwise, through its own request slot in
`source/backend.c` so polling never waits on profile saves.

Onslaught follows `GuildWarPopupView`: every fourth PvP event cycle is a guild event from `guild_catalog` in
`data/pvp_rules.json` (Dangerous Purpose, War of Attrition, Ascension). The War tab shows the running, next (within 2
days) or previous Onslaught, the leaderboard of guilds by summed member banners with a toggle to your own guild's
members and a help text, the guild rank rewards from the original announcement tables (`tools/build_pvp_rules.py`
parses them from the English localization), the minimum of 100 banners, Enter (opens the PvP event) and Claim (opens
Goals → My Events, where the guild row shows `GUILD_EVENT_RANKING` and claims through the server). Banners count for
the guild a player was in when the event started. The Duel button on member rows fetches the member's current squad
and active runes and starts an ordinary PvP fight against it (1 stamina, trophies by the usual rules, no Next); the
member's own trophies do not change, and the cached `GuildMemberSmart` puts Duel over the last-login column, so it sits
in the rightmost action slot. Report Abuse (`ReportAbuseV`) sits in the footer of member and observed guilds and in
the application review; reports go to the server's `runtime/reports.jsonl`.

`HOD_SHOT_SCREEN=guild_find`, `guild_create`, `guild_home`, `guild_observe`, `guild_applications`, `guild_chat`,
`guild_war` (running Onslaught), `guild_war_members`, `guild_war_help`, `guild_war_previous` (claim),
`guild_war_next`, `guild_report`, `guild_report_application`, `guild_review`, `goals_guild_award` and
`guild_home_buttons` render the screens with sample data and a pinned clock. `guild_live_home`, `guild_live_chat`,
`guild_live_war`, `guild_live_duel`, `guild_live_report` and `guild_live_claim` drive them against a running
backend.
