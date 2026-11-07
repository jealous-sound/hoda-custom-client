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
`based_on` names the shipped hero, `character` overrides catalog fields (name, `obtainable`; the displayed name comes
from `HODA_UNIT_<ID><tier>` in `data/localization/heroes/`),
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

## Fortify

The Fortify tab of the heroes screens is 5.4.4's `FortifyScreenState`, laid out from the 4.4.0 `FortifyScreenV`,
`FortifySlotSmart`, `FortifyConfirmPopupV` and `FortifyCelebrationPopupV` prefabs. Drag a hero onto the pedestal, then
drag other heroes into its ten slots; each slot is a health or power slot set by the hero's archetype, and slots open
with the hero's consume level (two at first, ten at the end of the ladder). Confirm shows the rarity, faction and tier
breakdown, and the celebration tallies the new stats. Squad members cannot be sacrificed, a hero with slotted heroes or
gear asks first, and dropping a copy of the hero already in a slot combines them a tier up. Remove Fortifications
(added in 5.4.4) gives slotted heroes back at level 1 for gems by rarity.

A slot adds `round(max * cap * bonus[rarity][I]) + round(max * cap * (bonus[rarity][tier] - bonus[rarity][I]))` to the
hero's power or health, plus 12.5% of that when the two heroes share a faction (or are both large), where `max` is the
fortified hero's stat at its current tier's maximum level. The math runs in float32 with half-to-even rounding, as
5.4.4's `FortifyBonusCalculatorDO` does, and the bonus counts in battle. `tools/build_fortify_tables.py` writes
`data/fortify_tables.json` from the decoded 4.4.0 archetypes and bonus rows and the 5.4.4 `Config.cs` removal costs.
4.4.0 has no Mythic row; its Tier IV value, 1.24, is the one that reproduces every 5.x `MaxFortify*Bonus` in
`HoDA_character_info/UnitData.json`, and Tiers I-III follow the Legendary row's shape. Slots are saved on the hero as
`fortify`, the server owns them, and `fortify` and `remove_fortify` are queued server actions. `HOD_SHOT_SCREEN=fortify`,
`fortify_parent`, `fortify_pending`, `fortify_confirm`, `fortify_celebration`, `fortify_remove_mode` and `fortify_remove`
render the screens.

## Commands

```sh
make run
make play
make test
make golden
make format-check
python3 tools/check_comments.py
```

After the game data is in place, the game shows the splash with two buttons: local play keeps the save in
`player_save.dat`, online play connects to `SERVER_URL` (baked in at build time, default
`https://api.hodareborn.com`), which then owns the save and the clock. Building with `SERVER_URL=` leaves the server
out and starts local play without asking. `HOD_BACKEND_URL=<http or https URL>` skips the choice and plays online
against that server; `make play` uses it to run the game against the Go backend in `../hoda-custom-backend`
(override with `BACKEND_DIR=`). Requests to the server go through libcurl on macOS and Linux and WinHTTP on Windows,
like the data download. `hoda-original-backend` is the separate Python server for the original Android client.

Against the backend, the server owns the economy: gold, gems, vouchers, energy, level, potions, keys, tomes and which
heroes, runes and gear exist. Store purchases and refills wait for the server's answer. Nexus awards, mission and
event claims, gear sales, prestige slots, ascension, rune crafting, fortify and tutorial rewards are applied locally at once and
queued in `source/server_actions.c`; the queue sends them in order and, once it is empty and the player is out of
battle, loads the profile the server sends back, so the server's numbers win. A Nexus recap waits for the server so it
shows the heroes the server rolled. `data/hero_pool.json` (`tools/build_hero_pool.py`) is the hero list the server
rolls from; after changing it or `store_packs.json`, `runes.json`, `gear.json`, `tutorial_rewards.json`,
`campaign_maps.json`, `nexus_maps.json`, `pvp_rules.json`, `missions.json`, `fortify_tables.json` or `new_player.json`, run
`scripts/sync-client-data.sh` in the backend. Store schedules use UTC dates.

Battles are replayed on the server. FIGHT sends `battle_start` with the squad, its active runes, the enemy and the game
changer exactly as `source/battle_replay.c` serializes them; the server checks them against its own copy, spends the
energy or stamina and answers with a seed, and the battle plays from that seed (the corner spinner shows while it
waits). The result goes back as `battle` with the battle id, and the server pays what its own replay of
`battle_sim.c` says, not the client's claim. Online PvP opponents come from the server too: opening a ladder or event
fight, picking a bot on a leaderboard, Next and Continue send `pvp_match`, the battle screen opens on the opponent
the server answers, and FIGHT names that match. The server rolls the match seed, reads its own trophies, banners and
clock, and counts the day's Next uses. Local play keeps picking its own seeds and opponents. The server's replay is a Go port
of the simulation, so `make golden` regenerates `tests/golden/`: recorded battles that cover every hero twice, the
campaign and Nexus rosters, game changers, PvP opponents and the built-in ability recipes. `make test` fails while the
corpus is stale; after `make golden`, run `scripts/sync-client-data.sh` in the backend, whose tests replay the corpus
and require the same outcome, crit count and action timeline. Builds use `-ffp-contract=off` so float results do not
depend on the compiler fusing multiply-adds.

`make` builds `Heroes of Dragon Age.app`, a macOS app bundle with its icon from `packaging/app_icon.png` and its
`Info.plist` from `packaging/macos/`. The game runs from the folder that holds the bundle, so it finds `data/` and
`client-additional-data/` next to it however it is launched.

`make PLATFORM=linux` builds `build/linux/Heroes of Dragon Age`; `make PLATFORM=windows` cross-compiles
`build/windows/Heroes of Dragon Age.exe` with MinGW-w64 and embeds `packaging/windows/app_icon.ico`. Point
`RAYLIB_DIR=` at a raylib 5.5 release and set `RAYLIB_LINK=$RAYLIB_DIR/lib/libraylib.a` to link it statically.
OS-specific calls (HTTP requests, positional reads, atomic file replace, CPU count, UTC time) live in
`source/platform.c`, a separate translation unit so `<windows.h>` never meets raylib's names.

`.github/workflows/build.yml` builds all three on every push to `master` (or by hand from the Actions tab) and
attaches a universal macOS app, a Linux x86-64 binary and a Windows x86-64 executable to the run as artifacts. They
carry no game data and download it on first launch. Pushing a tag such as `v0.2.0` runs the same builds and publishes the three packages as a GitHub
Release named after the tag.

## Language and settings

The game starts in the system language (macOS preferred languages, the Windows display language, or `LANGUAGE`,
`LC_ALL`, `LC_MESSAGES` and `LANG` on Linux) when the original 4.4.0 cache has it: English, French, German, Italian,
Spanish, Brazilian Portuguese, Dutch, Russian, Japanese, Korean and Simplified or Traditional Chinese. Anything else
falls back to English, then Russian. `HOD_LANGUAGE=<code>` (`en`, `fr`, `de`, `ja`, `zh-hant`, ...) overrides
both the system and the saved choice. The gear button in the top bar opens 4.4.0's `GameOptionsPopupV`: Audio
Settings (music and sound effects on/off), Language (5.4.4's `SelectLanguagePopupV`; picking a language reloads the
game in it) and Toggle Fullscreen. The choices live in `settings.json` next to the save.

Text comes from the original `Loc_*.txt` tables. Strings the original game lacks live in
`data/localization/<area>/Loc_*.txt`, one file per language with `HODA_`-prefixed keys; they load after the original
table and win over it. `python3 tools/validate_localization.py` (part of `make test`) checks that every area has all
twelve languages with the same keys and placeholders, that every key the C sources look up exists, and that no C
literal outside the debug harness holds Russian text. `HOD_SHOT_SCREEN=options`, `options_audio` and
`options_language` render the settings popup.

The game's font, DAmFont, covers Latin, Cyrillic, Japanese and Korean but not every Chinese character; the rest come
from a system CJK font (Hiragino Sans GB or STHeiti on macOS, Microsoft YaHei or JhengHei on Windows, Noto Sans CJK
or WenQuanYi on Linux), merged into the same glyph atlas, as the original client fell back to the device font. Lines
break between CJK characters.

## Downloaded game data

When no `client-additional-data/` sits next to the game (a CI build, or any copy without `make dist` data), the game
keeps its data in a per-user folder: `~/Library/Application Support/Heroes of Dragon Age` on macOS,
`$XDG_DATA_HOME` or `~/.local/share/Heroes of Dragon Age` on Linux, and `%APPDATA%\Heroes of Dragon Age` on
Windows. Saves and `account.dat` live there too. On every launch it reads `manifest.json` from `CONTENT_URL` (baked
in at build time; `HOD_CONTENT_URL` overrides it), downloads only the packs it doesn't have yet, checks their SHA-256,
unpacks them and deletes files that later versions dropped. Interrupted downloads resume. Without a connection it
starts with the data it already has, as long as that data has the manifest format this build reads
(`CONTENT_MANIFEST_FORMAT`, `MANIFEST_FORMAT` in `tools/build_content_packs.py`; bump both whenever the data tables
change shape). A newer format on the server asks the player to download the latest version; an older one is a
server error. `HOD_CONTENT_DIR=<folder>` uses another folder and forces downloading even in a
checkout. The window stays black until the APK and core packs are installed; from then on the download screen is 5.4.4's `LoadingScreenV` (splash, spinner, `loader_message` plate with the original messages in the player's language, red bar, megabyte counter and percentage) and errors use the game's connection and download-error popups. A failure before that art arrives retries every 5 seconds; Escape quits. Downloads use libcurl on macOS and Linux (loaded at runtime, so any distro's `libcurl.so.4` works) and
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

## License

The code in this repository is licensed under the GNU General Public License v3.0 (GPL-3.0-only); see `LICENSE`.

Heroes of Dragon Age is a trademark of Electronic Arts. The original game data, art, audio, fonts and APK that the
game reads or downloads are © Electronic Arts Inc. and are not covered by this license. This is a non-commercial fan
project and is not affiliated with or endorsed by Electronic Arts or BioWare.
