# Humanoid animation pipeline

Bakes the original Unity humanoid clips into `char_pack/<unit>/<unit>_tN_rl.glb` files that raylib loads directly.
Each GLB carries `idle` first, then `attack`, `attack2`, `hit`, `death` and `cheer` where the unit has them. The
game prefers `*_rl.glb` over the raw pack GLBs.

## Scripts

- `extract_hoda.py`: dumps Avatars, bone lists and AnimationClips from the OBB cache into `extracted/`.
- `hoda_anim.py`, `muscle_solver.py`: clip evaluation and muscle-to-rotation solving.
- `body_frame.py`: turns the solved body by the avatar's T-pose body frame (`m_RootX`), which Unity's clip root
  rotation is relative to, pivoting on the planted feet. Without it every humanoid leans forward by that angle
  (about 4 degrees for human rigs, 8 for dwarves).
- `glb_writer.py`: rewrites a pack GLB with the baked skeleton and animation channels.
- `bake_idle.py`, `bake_all.py`: per-unit and batch bakers.
- `prefab_scaffold.py`: reads a unit's display scale off its prefab. A cm-folded rig scales its skeleton by 0.01
  under the `_fbx` wrapper; only a small-scaled node that carries skinned bones counts as that fold, so a VFX
  attachment under a hand bone (Ketojan, Crassius Servis, the Spirits) leaves a meter rig at its wrapper scale.
- `rebuild_stub_src.py`: rebuilds one exact-tier source GLB from its CAB.
- `unit_texture.py`: bakes `_MainTex` opaque when the original material uses the built-in `Unlit/Texture` shader,
  which ignores alpha; the game's unit shader discards texels below 0.5 alpha.
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
