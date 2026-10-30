# Heroes of Dragon Age — Game Systems Reference

A design reference distilled from the Dragon Age wiki (`Heroes_of_Dragon_Age`,
`Factions_(Heroes_of_Dragon_Age)`, `Characters_(Heroes_of_Dragon_Age)`) and from a
parsed roster of **739 wiki character rows** (the dataset lives in `data/characters.json`). All numbers below are pulled
from that source data or the wiki; nothing here is invented. This documents the *systems*
of a shut-down 2013 mobile title as inspiration for a new game — it is a reference, not a
spec to copy.

---

## 1. Overview

*Heroes of Dragon Age* (HoDA) was a freemium **collectible squad battler** set in BioWare's
Dragon Age universe, developed by Capital Games and published by Electronic Arts. It
launched **December 5, 2013** on iOS/Android and was **shut down January 24, 2023** (server
deactivation; reference only — this game no longer runs).

**Core loop:** collect "heroes" (3D figures of Dragon Age characters) from packs, quests, and
events → build a **five-person squad** with a chosen battlefield formation → battle PvE maps
and PvP/guild opponents. Battles resolve **automatically**: the outcome depends on character
selection, abilities, formation, faction matchups, and equipped runes — the player's strategy
is in the *team-building and positioning*, not in moment-to-moment control. The game is "light
on lore," built around *what-if* scenarios re-staging moments from the games, novels, and codex.

Surrounding the loop are deep progression systems — **consuming** characters to power up others,
**fortification**, **gear**, **runes**, and **Mythic ascension** — detailed below.

---

## 2. Factions

Every character belongs to a **faction**, shown as the color of the circular plate in its base.
There are five colors; the in-universe meaning of each (from the Factions page):

| Faction | In-universe meaning |
|---|---|
| **Black** | The dark forces of Thedas — darkspawn, undead, demons. |
| **Blue** | Magical beings — elves, mages, spirits. |
| **Red** | Those outside the major order — wild animals, rogues, apostates, Qunari. |
| **White** | The established order — nobility, ordinary soldiers, templars, Grey Wardens. |
| **Large** (a.k.a. Giant / Large Creatures) | Beings larger than ordinary men — golems, ogres, dragons. |

> **Large does NOT count toward faction bonuses.** It is "technically considered a faction,"
> but Large characters never contribute to the squad faction bonus (see §6).

**Dual factions:** some heroes have a two-color plate denoting a mixed nature (e.g. Grey Warden
Alistair = black/white; Dalish blood mage Merrill = blue/black). A dual-faction hero counts
toward **either** of its colors, but a squad made entirely of matching dual-faction heroes does
**not** earn a doubled bonus.

**Cross-faction damage (rock-paper-scissors):** attacking the opposing color grants **+25% damage**:
White > Red, Red > Blue, Blue > Black, Black > White.

**Parsed faction_counts** (739 chars). Single-color heroes:

| Faction | Single-color count |
|---|---|
| White | 122 |
| Red | 107 |
| Large | 103 |
| Black | 75 |
| Blue | 55 |

277 of the 739 are **dual/multi-faction** (e.g. Red/White 75+1, Red/Blue 44, Black/Red 43,
Black/White 42, Black/Blue 33, White/Blue 29+9, plus one four-color hero). Counting every
character that *includes* a color (singles + duals): **White 279, Red 271, Black 194, Blue 171,
Large 103.**

---

## 3. Rarities

Characters come in escalating rarities — the rarity reflects how hard the hero is to acquire and
how strong it is. From low to high: **Common, Uncommon, Rare, Epic, Legendary, Mythic.**

**Legendary star tiers.** Legendary is split into three sub-grades marked by **1, 2, or 3 stars**.
More stars = more powerful and a higher level cap:
- **1-star** (most Legendaries) — max level **85**
- **2-star** — max level **90**
- **3-star** — max level **95**

**Mythic (via Tome of Ascension).** Mythic is not pulled from normal packs (except certain Wisps).
A **Legendary** hero is upgraded to Mythic by:
1. filling all **3 Gear slots**, which unlocks a **4th** slot, then
2. equipping a **Tome of Ascension** into that 4th slot.

Ascending to Mythic grants **+5 levels, an extra gear slot, increased fortification bonuses, and
an increased squad faction bonus** (6% vs Legendary's 5%).

**Parsed rarity_counts** (739 chars):

| Rarity | Count |
|---|---|
| Common | 29 |
| Uncommon | 43 |
| Rare | 82 |
| Epic | 85 |
| Legendary (1-star) | 164 |
| Legendary (2-star) | 171 |
| Legendary (3-star) | 83 |
| Mythic | 19 |
| Retired (removed/updated heroes) | 63 |

---

## 4. Stats

Each character has three core stats:

- **Power** — how much damage it deals.
- **Health** — how much damage it can take.
- **Speed** — combat **initiative**: faster characters act first. Speed is a label, not a number;
  the roster uses **Quick / Normal / Slow** (parsed distribution: Normal 357, Slow 218, Quick 164).

**Tiers and leveling.** Characters are combined (via **consuming**, §6) up through **Tier IV**. Each
tier raises the **level cap by +5**. A higher-tier hero has more Power and Health *at every level*
than a lower-tier one — so **Power and Health scale with both tier and level** (and can be raised
further by fortification and gear). A Mythic hero gains 5 levels beyond its Legendary cap.

**Parsed stat_ranges per rarity** (min→max across the roster; *base* = floor at low level/tier,
*max* = fully leveled/tiered ceiling). This shows the real numeric scale — note how each rarity
roughly multiplies the ceiling of the one below:

| Rarity | Base power | Max power | Base health | Max health |
|---|---|---|---|---|
| Common | 1–55 | up to ~1,474 | 1–127 | up to ~6,765 |
| Uncommon | 1–155 | up to ~2,716 | 1–549 | up to ~11,777 |
| Rare | 1–500 | up to ~4,898 | 1–739 | up to ~23,284 |
| Epic | 1–612 | up to ~9,285 | 1–2,913 | up to ~39,387 |
| Legendary (1-star) | 1–1,107 | up to ~18,978 | 1–7,956 | up to ~120,656 |
| Legendary (2-star) | 130–1,717 | up to ~30,273 | 282–13,705 | up to ~220,045 |
| Legendary (3-star) | 150–2,635 | up to ~32,296 | 429–17,152 | up to ~276,059 |

Representative *median* maxed (Tier IV) values give a feel for the typical (not peak) hero: Common
~1.1k power / ~2.9k health; Rare ~2.4k / ~11k; Epic ~3.4k / ~18k; Legendary 1-star ~6.7k / ~29k;
3-star ~11.4k / ~62k. Headline scale: **a Common tops out around 1.4k power; a Legendary 3-star
reaches ~32k power and ~276k health.** (Mythic stats are derived by ascension on top of a
Legendary's numbers and are not stored as separate base/max values in the roster.)

---

## 5. Abilities glossary

Most heroes carry one or more special abilities baked into a free-text ability line. Two
dimensions matter: the **attack pattern** (who gets hit) and **keyword effects**.

### Attack patterns
Which enemies a hero's attack strikes, given its formation position:

- **One** — single target.
- **Row** — a horizontal row of enemies.
- **Column** — a vertical column of enemies.
- **All** — every enemy.

Many heroes have **compound patterns** (e.g. "Row, then All", "One, then All") that chain two
hits in one turn. **Parsed attack_types distribution** (739 chars; primary/listed pattern):

| Pattern | Count |
|---|---|
| One | 261 |
| All | 157 |
| Row | 142 |
| Column | 113 |
| Compound (All+One, All+Row, Column+Row, Row+One, etc.) | ~48 |
| N / none / random (non-attacking or special) | ~18 |

### Keyword effects
One-line definitions. Frequency = **total mentions across all 739 ability descriptions** (the same
metric SKILL.md and `data/characters.json` use), so these line up with the headline counts there:

- **Stun** (509) — target loses one turn.
- **Slow** (412) — lowers the target's initiative (acts later).
- **Curse** (389) — prevents the target from gaining Power and/or Health (curse Power / curse Health).
- **Drain / power drain** (454) — lowers the target's Power, often as a % of the attacker's Power.
- **Heal** (1,079, the single most common effect) — restores health to self or allies, often on a
  trigger (on hit, on enemy death, on ally death).
- **Resist** (968) — resistance to a debuff: **stun resist, slow resist, curse resist, power-drain
  resist, damage resist** (often granted to a group, e.g. "+50% stun resist for Qunari"); of these,
  "damage resist" alone accounts for ~274 mentions.
- **Defender / Taunt** (Defender: 106) — **defender**: absorbs a % of damage for allies when also
  attacked; **taunt**: the hero is more likely to be targeted. HoDA never uses the word "taunt" — the
  effect is written as "more likely to be attacked/hit" (~108 ability lines). Used to soak hits and
  protect the back line.
- **Flank** (17) — usually attacks the enemy back row first.
- **Levy** (70) — uses a % of the target's *own* Power against it as bonus damage.
- **Gain power** (15) — gains a % of the hero's Power per attack (ramps up over a fight).
- **Immunity** (55; the phrase "immune to …" appears 73×) — full immunity to a status
  (e.g. "immune to power drain", "curse Health immunity").
- **Bonus damage vs a group** (193) — extra damage against a specific group/faction
  (e.g. "+25% damage vs Warriors"); the broader family of group damage/resist modifiers is larger.
- **Second attack** (8) — a chance to attack again in the same turn (e.g. "10% chance to attack again").
- **Mark** (38) — applies a mark to an enemy so allies target it and deal bonus damage.
- **Aura** — passive buff to the hero's whole party (raises allies' Power or Health).

**Sample ability lines** (showing how the pieces combine):
- *Templar Recruit Alistair* (Rare/White/Row/Normal): "Attacks a row with 25% chance to stun each,
  +50% power-drain resist to allies, +25% power and health to Warriors, much more likely to be
  attacked" — a taunting row-stunner.
- *Corypheus* (Epic/Black/All/Slow): "Attacks and drains 50% Power from all enemies with a 10%
  chance to attack again; +35% Health/Power and +50% slow/stun resist for Blighted."
- *Archdemon Urthemiel* (Legendary 3-star/Large/All,One/Slow): "First attacks a single enemy,
  draining large Power, then attacks all a second time; +50% damage resist vs Grey Wardens;
  immune to Power Drain" — a compound-attack, group-hate Large creature.
- *Dalish Banner* (Mythic/Red+Blue/Quick, non-attacking): "Targets all enemies and curses Power
  (does not attack); +60% Health/Power and stun resist for Elves; allies heal 25% when this hero
  is hit and gain 100% Power when it dies; much more likely to be attacked" — a pure support/taunt.

---

## 6. Squad / faction bonus, consuming, fortification & gear

### Squad and the faction bonus
A squad is **five heroes** in a formation. When **four characters of the same faction** are used
simultaneously, a **faction bonus** is granted, equal to the sum of each contributing character's
rarity percentage:

| Rarity | Faction-bonus contribution |
|---|---|
| Common | +1% |
| Uncommon | +2% |
| Rare | +3% |
| Epic | +4% |
| Legendary (all stars) | +5% |
| Mythic | +6% |

Example (from the wiki): 1 Rare + 1 Epic + 1 Legendary + 1 Mythic of one color = **18%** bonus.
**Large characters never contribute.** Dual-faction heroes can fill the four-of-a-color requirement
under either color, but an all-matching-dual squad does not double the bonus.

**Runes** (max two per squad) provide additional, squad-wide buffs in battle.

### Consuming (the main power-up loop)
Characters can be **consumed** — fed into another hero and permanently removed from inventory.
Consuming a hero into a target:
- transfers **XP** and raises **critical / "2x damage" %**,
- progresses the target toward unlocking **Fortification slots** and **Tier-ups** (Tier 2 → 3 → 4),
- gives **bonus XP when the consumed hero shares the target's faction color**, and
- **consuming a duplicate auto-tiers** the target up.

A **Consume Progress** bar shows the next unlock; higher-rarity targets need more consume points
per step. The default unlock order interleaves "2x Damage" steps with Fortification slots and Tier
milestones (Tier 2/3/4 appear partway along the track). **Wisps** are special characters meant only
to be consumed (not battled), granting large bonuses by rarity, with extra boost for matching faction.

### Fortification
Every hero has **10 fortification slots — 5 for Health, 5 for Power** — of which only **2 start
unlocked**; the rest are opened via Consume. Placing an unused character into a slot grants a
**permanent** Health or Power bonus scaled by that character's rarity, tier, and faction
(same-faction = higher bonus). Slotted heroes can themselves be tiered up by combining duplicates.
Fortifications can be removed for a small cost.

### Gear and Mythic gate
Heroes have **Gear slots**. Filling all **3 gear slots unlocks a 4th**, which is the slot a **Tome
of Ascension** goes into to convert a Legendary into a **Mythic** (see §3). Mythic ascension also
raises fortification and faction-bonus values, adds 5 levels, and adds a gear slot — the top of the
progression ceiling.

---

## 7. Gear & rune catalog (from hoda-base.xyz)

§6 describes gear/runes abstractly; the actual catalogs were recovered from the game's own data and
live in **gear.md** / **runes.md** (browseable) and **data/gear.json** / **data/runes.json** (queryable).

**Gear (266 items)** — equippable items granting **aura effects**. Each gear has a rarity
(Common→Mythic), one or more effect lines in HoDA's aura vocabulary
(e.g. *"+25% power to allies on ally death"*, *"+15% 2X Damage chance"*, *"Perceptive: Negates
stealth"*, *"-25% slow resist"* as a downside), and optional **category gates** restricting where it
applies — by **Faction**, **Group** (Warriors, Dwarves, Templars…), **Specialty** (attack pattern /
role tag), or **Speed**. Mythic gear is the strongest tier and often character-specific
(e.g. *Bianca* → Varric, *Bull's Edge* → Iron Bull). Effects routinely combine an upside with a
self-debuff, a conditional ("on enemy death…"), and a named-hero bonus.

**Runes (122)** — two kinds: **named battle runes** (82) are timed squad buffs with a buff type
(Damage Resist, Stun/Slow/Power-Drain Resist, Retaliation, Perceptive, Stealth, Dispel…), a value,
and a duration in seconds; **boost runes** (40) are meta consumables (XP, Gold, Initiative,
"fastest/slowest" battle modifiers, brave/cautious targeting) carrying only a buff type + value.
Max **two runes per squad** (§6).

### Data ↔ code mappings (authoritative, from the game data)
`data/characters.json` / `gear.json` / `runes.json` store numeric codes alongside human labels. The decode
(verified against the wiki roster's counts):

| Field | Code → label |
| --- | --- |
| **Rarity** | 1 Common · 2 Uncommon · 3 Rare · 4 Epic · 5 Legendary · 6 Mythic |
| **Stars** (Legendary tier) | 0 (non-Legendary) · 1/2/3-star |
| **Speed** (`Initiative`) | 0 Slow · 1 Normal · 2 Quick · 3 UltraFast |
| **Attack** (`AbilityUnitTarget`) | 1 One · 2 All · 3 Row · 5 Column |
| **Gear category `Type`** | 0 Faction · 1 Group · 4 Specialty · 5 Speed |
| **Faction** | derived from `FactionPrefab` (`faction_white_pre`→White, `faction_white_red`→White/Red, `faction_large_pre`→Large, `faction_all`→all four) |

In `characters.json` the per-tier numbers live under `stats` as `{I,II,III,IV}` maps: `base_*` = Tier
floor, `max_*` = `*AtMaxLevel`, plus per-level deltas, `max_fortify_*` bonuses, and `max_level` caps —
a more precise stat model than the single base/max pair in the wiki `wiki_stats` (§4).
