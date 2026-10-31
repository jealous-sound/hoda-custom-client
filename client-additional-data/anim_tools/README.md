# Humanoid animation pipeline

Bakes the original Unity humanoid clips into `char_pack/<unit>/<unit>_tN_rl.glb` files that raylib loads directly.
Each GLB carries `idle` first, then `attack`, `attack2`, `hit`, `death` and `cheer` where the unit has them. The
game prefers `*_rl.glb` over the raw pack GLBs.

## Scripts

- `extract_hoda.py`: dumps Avatars, bone lists and AnimationClips from the OBB cache into `extracted/`.
- `hoda_anim.py`, `muscle_solver.py`: clip evaluation and muscle-to-rotation solving.
- `glb_writer.py`: rewrites a pack GLB with the baked skeleton and animation channels.
- `bake_idle.py`, `bake_all.py`: per-unit and batch bakers.
- `rebuild_stub_src.py`: rebuilds one exact-tier source GLB from its CAB.
- `import_fbx.py`: imports a post-4.4 reference FBX (requires Assimp) and writes its take manifest.

## Commands

```sh
python3 tools/current_tier_models.py
uv run --with UnityPy --with Pillow tools/current_tier_models.py --bake --only em_mw_solas_t1
uv run --with UnityPy --with Pillow tools/current_tier_models.py --bake --all --workers 4
python3 client-additional-data/anim_tools/import_fbx.py --inventory INVENTORY.json --report REPORT.json
uv run --offline --with UnityPy --with Pillow python client-additional-data/anim_tools/bake_all.py \
  --inventory INVENTORY.json --workers 4
```

The first command is a read-only audit. The full batch needs the explicit `--all` flag. The importer skips complete
source and manifest pairs, rejects partial pairs and replaces only with `--force`.

## Adding a unit

1. Dump the unit's Avatar and idle AnimationClip with `extract_hoda.py`.
2. Add a row to `UNITS` in `bake_idle.py` and run it.
