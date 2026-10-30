---
name: hoda-reference
description: Heroes of Dragon Age (the shut-down 2013 mobile game) as a design reference for this project — a 676-character dataset (factions, rarities, per-tier stats, abilities, card art), the gear (266) and rune (122) item systems, EN+RU localization, and local avatar + model assets. Use when designing characters, abilities, stats, items/gear/runes, or art for the heroes-of-dragons game.
---

# HoDA Reference

Heroes of Dragon Age (HoDA) was a 2013 free-to-play mobile collectible-battler set in BioWare's Dragon Age universe: players collected square "card" heroes — warriors, mages, dragons, demons, and giant creatures — and pitted 5-unit squads against each other in auto-resolving grid battles. EA shut the servers down years ago, so the game is no longer playable. This skill is a design reference distilled from the **Dragon Age Wiki** (Fandom): it captures HoDA's roster, mechanics vocabulary, stat curves, and card art so we can borrow *ideas and structure* while building this project's own original game. It is reference material, not a spec to copy.

## What's here

Open the file that matches what you're doing:

- **systems.md** — the game-systems glossary: factions, rarities/star tiers, speed/attack patterns, the stat model, and the abilities vocabulary (stun, drain, heal, slow, curse, Defender, Levy, mark, immunity, resist, etc.). Open this when designing mechanics, balance, or ability text for our own units.
- **roster.md** — the full human-readable table of all 739 wiki characters with rarity, faction, stats, ability prose, and image links. Open this to browse, skim, or hand-pick exemplars.
- **data/characters.json** — **the primary machine-readable character dataset: 676 entries** keyed by id. Each entry has a `sources` array (`wiki`, `hoda-base`, or both — which determines the populated fields) and these fields:
  - identity — `name`, `name_ru`, `rarity`, `stars`, `faction`, `speed`, `attack`
  - combat & progression — `ability_en`/`ability_ru`/`ability_key`, `equipped_rune_en`, `groups`, `specialties`, `tags`, `stats` (per-tier `{I,II,III,IV}` maps: `base_*`/`max_*` power+health, per-level deltas, fortify bonuses, `max_level`), `num_gear_slots`, `has_prestige_slot`, `obtainable`
  - prose & art — `abilities_text` (ability prose), `acquisition`, `card_art_url`, `wiki_stats`, `thumb_image`, `unit_prefab`, and local asset paths: **`avatar_files`** (128² portraits), **`model_files`** (3D models — **`.fbx`** and/or **`.glb`**, present for every unit), **`model_textures`** (the model texture PNGs), **`model_bundles`** (the raw encrypted source bundles — always present)

Item systems and localization:

- **gear.md** — human-readable table of all **266 gear items** (equippable aura items): title, rarity, restriction categories, effects, flavor text. Open this to browse equipment.
- **runes.md** — human-readable table of all **122 runes** (82 named battle runes + 40 boost runes): buff type, value, duration, effect.
- **data/gear.json** — machine-readable gear, keyed by id. Each entry: EN+RU title/description, rarity, `rune_effects_*` (the aura lines), `categories` (faction/group/specialty/speed gates), icon URLs + local file paths.
- **data/runes.json** — machine-readable runes, keyed by id: EN+RU name/description, `buff_type`, `buff_value`(+formatted), `duration`, `rarity`, icon paths.
- **data/localization/Loc_ENG_US.json**, **Loc_RUS_RU.json** — the full decrypted string tables (6,420 EN keys) that every `*_key` field resolves against. Lookups are by **UPPERCASE** key.
- **references/** (gitignored — EA/BioWare IP): `gears/` (266×2 icons), `runes/` (rune + buff-type icons), `avatars/` (2,315 per-tier 128×128 character portraits), **`fbx/`** (2,150 decrypted 3D models — `.fbx` + texture, organized by race: Creature, Elf Female, Human Male, …), **`glb/`** (128 prefab-tiers across 33 characters as rigged **`.glb`** + `.gltf`/`.bin`, textures, per-clip animation JSON, and the `.unity3d` bundle; one folder per character), `models/` (2,233 raw encrypted Unity AssetBundles — the source bundles for `fbx/` and `glb/`), **`game-assets/`** (the game's APK UI assets: loading screens + UI `textures/`, `fonts/`, `meshes/`, `text/`, `audio/`).

## The data at a glance

Distributions over the **676** characters in `data/characters.json`.

**By rarity:**

| Rarity | Count |
| --- | --- |
| Common | 29 |
| Uncommon | 43 |
| Rare | 82 |
| Epic | 85 |
| Legendary (1-star) | 164 |
| Legendary (2-star) | 171 |
| Legendary (3-star) | 83 |
| Mythic | 19 |

**By faction** (the four "color" alignments plus *Large* for big creatures/dragons, then dual/special combos):

| Faction | Count | | Faction | Count |
| --- | --- | --- | --- | --- |
| White | 110 | | Red/White | 66 |
| Red | 98 | | Red/Blue | 43 |
| Large | 97 | | Black/Red | 42 |
| Black | 65 | | Black/White | 36 |
| Blue | 46 | | Black/Blue | 31 |
| White/Blue | 28 | | Blue/White | 9 |
| White/Red | 4 | | Black/Red/White/Blue | 1 |

**Attack patterns** — every unit hits **One** (241), **All** (130), **Row** (131), or **Column** (108) targets, with ~49 hybrid/compound cases and a handful of non-attackers. **Speed** is **Normal** (329), **Slow** (193), or **Quick** (154).

**Most common ability mechanics** (frequency across ability text): heal 961, resist 823, stun 447, drain 397, slow 369, curse 326, bonus damage 166, immunity 113, Defender 92, Levy 54, second attack 30, mark 28, flank 15. Note what HoDA's vocabulary does *not* include — taunt, revive, shield, splash, bleed, execute, reflect, regen, and cleanse all appear 0 times — useful when deciding which mechanics are authentic to the source versus our own additions.

**Stat model** — each hero has two stats, **Power** (attack) and **Health**. The `base_*` values are the unit at **Tier I** (freshly acquired); the `max_*` values are the same unit fully leveled to **Tier IV**. The scale grows enormously by rarity:

- A top **Common** caps near 1,474 Power / 6,765 Health at Tier IV.
- A top **Epic** caps near 9,285 Power / 39,387 Health.
- A top **Legendary (3-star)** caps near 32,296 Power / 276,059 Health.

So the usable design range spans roughly single-digit base stats up to ~32K Power and ~276K Health. (**Mythic** entries are boost/consumable items and carry null stat ranges.)

Need just a handful of reference units rather than the whole roster? Sort `data/characters.json` by `wiki_stats.max_power` for the strongest Legendary/Mythic exemplars — good representative starting points.

## How to use it

1. **Design mechanics:** read **systems.md** to see how HoDA structured factions, rarities, speed/attack patterns, and abilities, then adapt those patterns into your own ability system. Don't reuse names verbatim — use them as a vocabulary and a balance anchor.
2. **Seed your own units:** query **data/characters.json** (filter by `rarity`/`faction`/`specialties`) to bootstrap real per-tier stat curves, target patterns, ability text, and item-slot counts for a batch of *original* units, then rename/rework them so they're your own.
3. **Reference the art style:** use a unit's `card_art_url`, a local `references/avatars/` portrait, or its `model_files` FBX as a visual anchor — reference and inspiration only; never ship the source art.
4. **Design items & equipment:** read **gear.md** / **runes.md** (or query `data/gear.json` / `data/runes.json`) to see how HoDA structured equippable buffs, conditional aura effects, and consumable boosts — then design your own item system from that vocabulary.

## IP note

All HoDA characters, names, art, and Dragon Age lore belong to **EA / BioWare**. Use this skill for reference and inspiration only — create your own original assets and names; do not ship copyrighted content.

## Source

Reference-only data (the game's servers are offline) from two places, tracked per entry via `sources`:

1. **Dragon Age Wiki** (Fandom) — ability prose, acquisition, card-art URLs, mechanics, and stat curves.
2. **hoda-base.xyz** — the game's own data files: per-tier stats, ability/specialty/tag structure, the gear and rune systems, EN+RU localization, and the `references/` icon/avatar/bundle assets.
3. **Community FBX rip** — decrypted 3D models (`references/fbx/`) and the game's APK UI assets (`references/game-assets/`). The character models were streamed from EA's now-dead servers (never in the APK), so this rip is the only surviving model cache.

**Re-fetching from hoda-base.xyz:** its JSON payloads are **RC4-encrypted** (key `"HoDA"`, hex-encoded ciphertext → `decodeURI` → JSON); text fields are localization keys resolved against the `Loc_*` tables by **uppercased** key. Asset URL patterns: gear `/<site>/assets/gears/<Icon>{,_Portrait}.png`, runes `/assets/runes/<Icon>.png`, buff icons `/assets/images/<BuffTypeIcon>.png`, avatars `/assets/avatars/<ThumbImage>.png`, models `/assets/bundles/<UnitPrefab lowercased>` (directory listing is disabled — fetch exact paths).

**Models:** every character resolves to a usable mesh via `model_files` — 2,150 `.fbx` in `references/fbx/` (grouped by race) plus 128 rigged `.glb` in `references/glb/` (33 characters across their tiers). Animation clips ship as **per-clip JSON** beside each `.glb` (Mecanim humanoid/muscle-space), not baked into the mesh: the `.glb` itself is rigged and bind-posed, but replaying the original motion needs Unity retargeting through the bundled Avatar. The raw encrypted source bundles for both sets live in `references/models/` (Unity 2019 AssetBundles under a custom WASM obfuscation distinct from the RC4 above).
