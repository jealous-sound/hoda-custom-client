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
make test
make format-check
python3 tools/check_comments.py
```

The companion backend lives in `heroes-of-dragon-age-backend`.
