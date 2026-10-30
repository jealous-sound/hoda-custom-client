---
name: asset-gen
description: Generate 2D and isometric game art — sprites, tiles, icons, UI buttons — with OpenAI's gpt-image-2 Images API in a stylized PS2-era game look, then chroma-key the background to transparency and crop to a tight bounding box. Use when creating, generating, or requesting game art/assets for the heroes-of-dragons project.
---

# Game asset generation (OpenAI · gpt-image-2)

Generate game art from text prompts (and reference images) using OpenAI's
Images API, then bake transparent PNGs ready for raylib. Stdlib-only scripts,
no `pip install` needed:

- **`generate.py`** — text/image → PNG via `gpt-image-2`.
- **`keyout.py`** — knock a solid background out to transparency for crisp sprite
  edges. gpt-image-2 has **no** transparent background, so generate on a flat color
  and key it out — see "Transparency workflow". `despill_key.py` is a softer
  variant that feathers the edge and removes green spill.
- **`crop.py`** — trim the transparent padding around the art down to a tight
  bounding box (run it after keying out) — see "crop.py options".

All facts below were verified against the live OpenAI Images API on 2026-06-26.

## API key (kept out of git)

The key is **not** hardcoded in any tracked file. `generate.py` resolves it from:

1. `$OPENAI_API_KEY` (environment), else
2. `secrets/openai.key` (in this folder, `chmod 600`).

`secrets/`, `*.key`, and `out/` are gitignored at both this folder and the repo
root, so the key and generated art are never committed. To rotate the key, edit
`secrets/openai.key` or export `OPENAI_API_KEY`.

## House art style (stylized, PS2-era — NOT photorealistic)

Every character/creature is a **stylized early-2000s console game model**, not a
hyper-realistic render. Append this **style suffix** to every character prompt:

> *"rendered as a stylized PS2-era low-poly 3D game character — clean simplified
> textures, slightly chunky readable proportions, soft baked lighting, crisp
> game-asset look, NOT photorealistic, not hyper-detailed, like an early-2000s
> console RPG character model"*

Think Dark Cloud / Jak / Final Fantasy X / Kingdom Hearts model fidelity: solid
readable shapes, simpler textures, gentle baked shading — appealing and game-like,
not a film-quality render. Keep the faction palettes/motifs (`hoda-reference`
`art-reference.md`); only the *fidelity* drops to PS2-era.

## Quick start

```sh
cd .Codex/skills/asset-gen

# 1) generate a single asset on a flat background
python3 generate.py "a stone-and-gold fantasy UI button with a tent/camp icon, single \
object centered, rendered as a stylized PS2-era low-poly 3D game asset, clean simplified \
textures, soft baked lighting, NOT photorealistic, solid bright green background, no text, \
no shadows" --out out/camp_button.png   # quality defaults to medium

# 2) bake the background to transparent RGBA (auto-detects the bg color)
python3 keyout.py out/camp_button.png out/camp_button_rgba.png

# 3) crop the transparent padding down to a tight bounding box
python3 crop.py out/camp_button_rgba.png

# 4) use out/camp_button_rgba.png in raylib (see "Using in raylib" below)
```

`--dry-run` prints the request and calls nothing (free). The script prints token usage
and an **estimated** cost after each call (OpenAI returns token counts, not a dollar figure).

## What gpt-image-2 actually supports (verified)

| Capability | Reality |
|---|---|
| Endpoint (generate) | `POST https://api.openai.com/v1/images/generations` |
| Endpoint (with `--ref`) | `POST https://api.openai.com/v1/images/edits` (multipart; the only way to pass reference images on OpenAI) |
| Response | base64 PNG at `b64_json`; **no dollar cost is returned**, only token `usage` |
| `quality` | `auto` / `low` / `medium` — **`medium` is the default**. (`high` is not used: it renders >60s and the connection times out — see below.) |
| `background` | `auto` / `opaque` **only — gpt-image-2 rejects `transparent` (HTTP 400)**. Generate on a flat color and key it out. |
| `size` | `1024x1024`, `1536x1024`, `1024x1536`, `auto` |
| `output_format` | `png` (default) / `jpeg` / `webp` |
| `n` | 1–10 images per call |
| references | up to **16** via `--ref` → the edit endpoint (style/character consistency). `input_fidelity` is **not** supported by gpt-image-2. |
| `seed` | not supported by this model (consistency comes from references, not seeds) |
| Exact hex colors | **NOT reliable** — ask for "magenta #FF00FF" and you may get `(241,5,166)`. This is why `keyout.py` auto-detects the background instead of trusting the hex. |

### Cost (estimated from tokens)

OpenAI returns **token counts, not a dollar cost**, so `generate.py` prints an
*estimated* cost from the image-output tokens (~$40 per 1M image tokens). Treat it as a
guide; the OpenAI dashboard is authoritative.

| quality | ≈ est cost (1024²) | ≈ est cost (1536×1024) | render time |
|---|---|---|---|
| low | ~$0.01 | ~$0.012 | a few s |
| **medium (default)** | ~$0.04 | ~$0.06 | ~20–45s |

Every render finishes well under a minute, so calls return promptly. Draft a prompt at
**`--quality low`** (seconds, ~$0.01), then drop the flag to render at the default `medium`.

> **Why not `high`?** gpt-image-2 at `high` renders for 60–210s, but the connection has a
> hard ~60s timeout with no keep-alive, so high-quality calls die with `RemoteDisconnected`
> before returning. Medium is the ceiling that reliably completes. Don't reach for `high`.

## Transparency workflow

gpt-image-2 always renders a background, so generate on a flat color and key it out — this
gives crisp edges and works for multi-figure sheets. Two steps:

1. **Generate on a flat, uniform background** in a color that does NOT appear in
   the sprite. Good keys: bright green (`solid bright green background`) or
   magenta. State "flat", "solid", "no shadows", "no gradient" in the prompt.
2. **Key it out.** Two equivalent options:
   - **`keyout.py`** (bake a transparent PNG on disk): `python3 keyout.py in.png out.png`.
     It auto-detects the actual background from the image border (because the model
     won't hit your exact hex) and removes pixels within `--threshold 70` (raise to
     ~90 if anti-aliased edges keep a colored halo; lower if it eats the sprite).
   - **In raylib at load time** (no pre-baking) — see below.
3. **Crop the padding (optional).** The keyed art usually sits in a lot of empty
   transparent space; `python3 crop.py in.png` trims it to a tight bounding box
   (keeps an 8px margin by default) — see "crop.py options".

## Using in raylib

This project has a sibling **`raylib`** skill. A generated asset is just a
texture; draw it with `DrawTexture` / `DrawTextureV` (see that skill's
`examples/04_texture_render.c` and `07_tilemap.c`). To key out the background while
loading (instead of running `keyout.py`):

```c
#include "raylib.h"

// Load an asset generated on a solid background and make that color transparent.
Image img = LoadImage("out/camp_button.png");
// Sample the top-left pixel as the background key, then replace it with BLANK:
Color *px = LoadImageColors(img);
Color bg = px[0];                 // border pixel = the flat background
UnloadImageColors(px);
ImageColorReplace(&img, bg, BLANK);   // exact-match replace -> transparent
Texture2D tex = LoadTextureFromImage(img);
UnloadImage(img);

DrawTexture(tex, 100, 100, WHITE);
```

`ImageColorReplace` is exact-match; for soft/anti-aliased edges prefer baking with
`keyout.py` (threshold-based) and loading the resulting RGBA PNG directly.

## Prompt engineering for sprites & isometric art

gpt-image-2 is strong at clean sprite art but, like all image models, it is
imperfect at exact grids and consistency across generations. Techniques that help:

**Single sprites, icons & UI**
- Describe one centered subject: *"single object centered, consistent scale"*.
- Always include the **house style suffix** (PS2-era stylized — see "House art style").
- Add: *"flat solid <color> background, no text, no labels, no drop shadow, no
  gradient, centered"*.
- Use a square size (`--size 1024x1024`) for most icons/props; a wide size
  (`--size 1536x1024`) for banners or wide buttons.

**Isometric art (tiles, buildings, props)**
- Specify the projection: *"isometric 2:1 dimetric view, 45-degree top-down angle"*.
- For tilesets: *"seamless isometric ground tile, diamond shape, tileable edges,
  solid background"*; generate pieces separately and compose in-engine.
- For buildings/units: *"isometric game asset, single object centered, consistent
  light from top-left, flat solid background"*.

**Consistency across multiple generations (a whole character set)**
- Generate one "model sheet" first, then pass it back as a reference so new poses
  match: `--ref out/knight_model.png` (repeatable, up to 16 refs). References drive
  style/character continuity since `seed` isn't available on this model.
- Keep a fixed style suffix you append to every prompt (the house PS2 style + palette).

**Iterate cheaply**: draft at `--quality low` (seconds), lock the prompt, then drop the
flag to render the keeper at the default `medium`.

## generate.py options

```
python3 generate.py "PROMPT" [options]
  --out PATH         output PNG (out/<slug>.png by default; index added for --n>1)
  --model SLUG       default gpt-image-2
  --size WxH         1024x1024 | 1536x1024 | 1024x1536 | auto
  --quality Q        auto | low | medium   (default medium; --quality low to draft)
  --background B     auto | opaque   (transparent is NOT supported by gpt-image-2)
  --format F         png | jpeg | webp   (default png)
  --n N              1-10 images (default 1)
  --ref PATH_OR_URL  reference image (repeatable, up to 16) — routes through the edit
                     endpoint so the output keeps the reference's character/style
  --dry-run          print the request, call nothing (free)
  --raw-json         also print the token usage summary
```

## keyout.py options

```
python3 keyout.py IN.png [OUT.png] [--key R,G,B|auto] [--threshold N]
  --key       background color to remove; default AUTO-DETECT from the border
  --threshold Chebyshev color distance counted as background (default 70)
```
Outputs an 8-bit RGBA PNG. Pure stdlib (no Pillow). Supports the 8-bit
non-interlaced RGB/RGBA PNGs the image API returns.

`despill_key.py` takes the same input but feathers the alpha edge and clamps
green spill (`--inner`/`--outer` distance band) — use it when `keyout.py` leaves a
green-tinted halo on anti-aliased edges.

## crop.py options

```
python3 crop.py IN.png [OUT.png] [--pad N] [--threshold N]
  --pad        margin in px kept on every side (default 8)
  --threshold  alpha <= this counts as transparent (default 8)
```
Finds the bounding box of the non-transparent pixels and crops to it, so the
asset has no wasted padding. OUT.png defaults to overwriting IN.png in place.
Run it after `keyout.py`/`despill_key.py`. Pure stdlib (reuses keyout.py's PNG
reader/writer).

## Troubleshooting

- **`RemoteDisconnected` at ~60s** — you used `--quality high` (or `auto` picked a slow
  render); high exceeds the ~60s connection timeout. Use `medium` (the default).
- **HTTP 400 "Transparent background is not supported for this model"** — gpt-image-2 has
  no alpha channel; use the green-screen + `keyout.py` workflow (or `--background opaque`).
- **keyout removed 0 px** — the real background differs from your `--key`; omit
  `--key` to auto-detect, or raise `--threshold`.
- **keyout ate part of the sprite** — the sprite contains colors near the
  background; regenerate on a more contrasting background color, or lower `--threshold`.
- **HTTP 401/403** — bad/expired key in `secrets/openai.key` or `$OPENAI_API_KEY`.
- **Render looks photorealistic** — you dropped the house style suffix; add the PS2-era
  stylized wording (see "House art style").
- **crop.py removed too much / nothing** — anti-aliased edges left semi-transparent
  pixels; raise `--threshold` to trim more, lower it to keep more. Increase `--pad` if
  the crop hugs the art too tightly.
