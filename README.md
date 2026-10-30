# Heroes of Dragon Age client

Native raylib 5.5 client in C for Heroes of Dragon Age: home, heroes, campaign, Nexus, Store and quests, with a
deterministic battle simulation.

## Layout

- `source/`: game code; `main.c` is the single translation unit entry, `vendors/` holds cJSON.
- `tests/`: battle simulation test and Python unit tests.
- `tools/`: build and validation scripts for data tables.
- `data/`: version-pinned rule tables and Nexus maps and rosters.
- `heroes-of-dragons-cache/`: model and animation bakers; the rest of the directory is the git-ignored local asset cache.

## Commands

```sh
make run
make test
make format-check
python3 tools/check_comments.py
```

The companion backend lives in `heroes-of-dragon-age-backend`.
