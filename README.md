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

`make dist` builds a playable copy in `../hoda-custom-dist` (override with `DIST_DIR=`): the binary with raylib
bundled next to it, `data/`, and only the parts of `client-additional-data` the game reads (about 2.1 GB of the
local cache). Players start it with `play.sh`; `DIST_BACKEND_URL=http://host:port` makes that the default server.
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
`source/backend.c` so polling never waits on profile saves. Onslaught shows its original "no leaderboard yet" state;
duels and abuse reports are left out. `HOD_SHOT_SCREEN=guild_find`, `guild_create`, `guild_home`, `guild_observe`,
`guild_applications`, `guild_chat`, `guild_war` and `guild_home_buttons` render the screens with sample data;
`guild_live_home` and `guild_live_chat` open them from a running backend.
