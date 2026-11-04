# Handoff: PvP system and PvP events

Written 2026-11-02 from a read of the code, decompiled clients, caches, backend and web archives. Phases 1 to 4 are
implemented: `source/pvp.c`, `source/goals_tabs.c`, `source/pvp_hub.c`, `source/pvp_event_view.c`, tuned through
`tools/build_pvp_rules.py`. Not done: `PVP_EVENT_SCREEN` items in the news feed (the home ticker does list the event)
and phase 5. Follow `AGENTS.md`: no source comments, name every cache object, run the relevant checks.

## Baseline

- The game changer work (`data/nexus_game_changers.json`, `BattleGameChanger`, `NexusEventGameChanger`) may still be
  uncommitted. Check `git status` first; this plan builds on it.
- `client-additional-data/` is git-ignored except a few tracked files. Research data there is local only.

## Answer: does PvP belong in Цели?

Yes. `Цели` is the original `LeaderboardPopupView` (home button tooltip `HOVER_GOALS_1`), a tabbed popup. Our
`source/mission_view.c` is only its Missions tab.

| Tab enum | Loc key | Content | Client today |
| --- | --- | --- | --- |
| `MISSIONS` | `LEADERBOARD_MISSIONS_BUTTON` | Achievements | Done |
| `TROPHIES` | `LEADERBOARD_BUTTON` | Trophy leaderboard | Missing |
| `MY_BATTLES` | `LEADERBOARD_MYBATTLES_BUTTON` | History, replay, revenge | Missing |
| `MY_EVENTS` | `LEADERBOARD_MYEVENTS_BUTTON` | Past events, reward claims | Missing |

Open logic (`LeaderboardPopupView.cs:119-141`): `GO_TO_LEADERBOARD` opens Trophies, unclaimed events open My Events,
otherwise Missions. The home button badge shows unclaimed mission plus event count, else the trophy delta
(`MainScreenView.UpdateTrophyBadge`).

32 achievements are missing from `data/missions.json`, and 26 of them are PvP or event ones:

- Trophy Master, Blood Thirsty II to V, Hold the Line I to V (`A5`, `A34`-`A37`, `A60`-`A62`, `A83`, `A84`).
- Arena Veteran I to V, Rush of Battle (`A11`-`A14`, `A80`, `A59`).
- Event Veteran I to V, Super Buffed I to V (`A121`-`A130`). Super Buffed tracks a whole squad gaining an event bonus.

The other 6 are post-map-16 (`A147`-`A150`, `A153`, `A154`); leave them out unless those maps exist.

## What exists in the client

- Home `PvP` button: drawn, click only logs (`source/game.c`, `HOME_PVP` falls to `TraceLog`).
- Stamina: full meter, HUD, refill dialog (`source/energy_dialog.c`), consumed by nothing.
- `consume_view.c` has a PvP result button; the stamina dialog has a navigate button. Both need wiring.
- Battle: deterministic `BattleSimRun` on `(Player, BattleRoster, seed)`. PvP can reuse it directly: an opponent is a
  `BattleRoster`. Rune bonuses apply to side 0 only (`SimUnitsSetup`); opponent runes need adding.
- Event bonuses: set `enemy_roster.game_changer`. Intro banner and aura already work (`BpQueueGameChangerEvent`).
  Effects support group, faction and unit id, on both sides.
- Reference pattern for a server-less rotating catalog: `NexusEventScheduled` and `tools/build_store_catalog.py`.

## Original system

Sources are `client-additional-data/decompiled_src` (5.4.4) and `decompiled_src_4.4.0`. All numeric rules were
server-side, so they must be designed, not recovered.

**Modes** (`BattleMode`): `PVP`, `PVP_EVENT`, `REPLAY`. `GameEventDO.GameType`: `GLOBAL_LEADERBOARD` (banners),
`KING_OF_THE_HILL`, `PROVING_GROUNDS`.

**Entry point differs by version.**

- 4.4.0: PvP button checks stamina, then enters `BattleMatchmakingScreenState` directly.
- 5.4.4: PvP button opens `PVPHubPopupView`, two columns: Today's Event and Proving Grounds, each with Defense
  readout, plus Weekly Rewards. `PVPHubPopupV` is absent from both caches, so its layout must be designed from
  `PVPHubPopupElements.cs`. All its text exists (`PVP_HUB_*`). The Android-only 4.4.0 flow lacks Proving Grounds.

**Battle flow:** matchmaking (`BattleMatchmakingScreenView`, `PVPMatchmakingScreenElements`): opponent squad, squad
switcher, `BUTTON_MATCHMAKING_NEXT` limited by `RemainingNexts`, Fight costs 1 stamina
(`BattleMatchmakingScreenView.cs:1086-1091`, shows `BuyStaminaPopupView` at 0). Results branch in
`BattleScreenView.cs:673-691` and `BattleResultsPopupView.cs:351`.

**Opponent data** (`MatchedPlayer`): id, name, xp, level, squad xp, avatar, units, runes, `TrophiesGain`,
`TrophiesLoss`, `Trophies`. Rewards also carry crit, opposing faction and surviving unit gold.

**Events** (`GameEventDO`): title, description, texture, start and end, `EventAwards` (banner thresholds with units,
runes, currency, trophies), `RankAwards`, `Banners`, `MaxBanners`, `PlayerRank`, `TotalPlayers`, `AttackRanges`
(rank windows of who may be attacked), `InstructionsKey`, splash image. Banner milestones pay out through
`EVENT_BANNER_ACHIEVEMENT_AWARD`, claims through `ClaimGameEventReward`. Screens: `PVPEventInfoPopupView` (tabs:
info, rewards, banners and ranks, battles), `PVPEventRewardsPopupView`, `LeaderboardKingOfTheHillScreenView`.

**Game changers in PvP** have more filters than ours (`GameEventDO.GameChangerTypes`): `FACTION`, `GIANT`,
`INITIATIVE`, `NO_FILTER`, `RUNE`, `UNIT_ID`, `UNIT_TARGET`, `GROUP`, `SPECIALTY`. Rarity based effects, resist
grants and multi-leaderboard rules also appear in late schedules.

**HUD:** `PVPProgressDisplayMode {Trophies, Banners, LeaderboardRank}` in `GlobalTopUiView`; notes in
`client-additional-data/REVERSE_ENGINEERING_NOTES.md` section on the HUD.

**Home ticker:** the original ticker was the PvP event (`INTERSTITIAL_TICKER_TODAYS_EVENT`, opens
`PVPEventInfoPopupView`). Our PvE ticker in `source/news.c` should list the PvP event first once it exists.
News items with `PVP_EVENT_SCREEN` link should join the feed too.

## Assets and text

The 5.4.4 cache is identical to 4.4.0 (same 4,716 bundles, same localization, same game data).

- Present: `LeaderboardPopupV`, `LeaderboardItemSmart`, `HistoryLineSmart`, `MissionLineSmart`,
  `BattleMatchmakingScreenV`, `PVPEventInfoPopupV`, `PVPEventRewardsPopupV`, `PVPEventRankHistorySmart`,
  `EventBannerRewardSmart`, `KingOfTheHillPVPEventSmart`, `GloablLeaderboardBasedPVPEventSmart`.
- Absent: `PVPHubPopupV`, `ProvingGroundsRankRewardsSmart`, `LeaderboardKingOfTheHillScreenV`. Design these.
- Layout dump: `scratchpad/inspect_ui_bundle.py <bundle> --resolved-layout`, run with
  `client-additional-data/vfx_tools/venv/bin/python`. Rects are in 1280x720.
- Loc: 391 `PVP_*` keys, 55 `PVP_EVENT_*`, 9 `PVP_HUB_*`, 17 KoTH, 7 Proving Grounds, in Russian too.
- Many events have original `PVP_<NAME>_*` title and description keys, including those of the 2015 era. Prefer them.

## Event catalog

`data/pvp_rewards_research.json` is the unmodified research snapshot of 380 events (SHA-256 checked by
`tools/build_pvp_rules.py`). The builder gives each catalog event the Individual rank tiers and banner milestones of
one selected record: the milestones scale to `ladder_banners`, and the 12 source tiers map in order onto `RANK_BRACKETS`.
Unmapped prize names fail the build. Keys for raids and quests, which the client lacks, are in `UNSUPPORTED_PRIZES` and
show up per event in `omitted_prizes`. The early milestones are client tuning, not recovered data. Event buffs are not
in the snapshot.

`client-additional-data/pvp_research/archived_pvp_events.json` (local): 179 named entries extracted from archived
weekly Event Schedule threads, 2016 to 2021, with rules, banner prizes, rank prizes and run windows. 144 have prize
lists. Caveats: the extraction also caught a few Quest events (Elven Mages, Nature Nexus, Drop-a-Hawke) and some
Onslaught variants; filter it. Values are late game, and prizes name post-4.4 items. Thread URLs are in
`client-additional-data/event_research/archived_threads.txt`. Treat text as reference and use original loc keys.

The archive also confirmed `Elven Mages +300/+125` and `Nature Nexus +250% Black and Red`, matching loc keys.

## Player-reported rules

From archived forum threads 2221957 (FAQ) and 74982. These are community posts, not client data. The client follows the
first three.

- Max banners per win starts at 25 and rises by 1 every tenth of the ladder to 30 at the midpoint, then slowly drops.
  The drop rate is not recorded, so `win_banners_decline_slowdown` is a guess.
- My Events holds 19 events, unclaimed first and then by date. Older unclaimed rewards are lost.
- PvP event and Proving Grounds matchmaking picks a random opponent from a pool based on rank and trophy or banner count.
- Defender gear with no stated value absorbs 50%.

## Decisions to confirm with the owner before building

1. **Opponents.** No server or real players exist. Recommended: a local generated ladder of named bots whose squads
   come from `data/campaign_rosters.json`, `data/nexus_rosters.json` and the character catalog, scaled to player
   level and deterministic per seed. Alternative: real asynchronous PvP via `hoda-original-backend`, which has
   no matchmaking, leaderboard or PvP RPC today. That is far larger.
2. **Entry.** Recommended: the 5.4.4 hub, since the project targets 5.4.4 screens and has the text.
3. **Stat values.** Trophy gain and loss, nexts per day, attack ranges, reward tiers and event cadence have to be
   invented. Keep them in one generated table so they can be tuned.
4. **Defense and revenge.** Being attacked needs a simulated offline attack log. Recommended: fake incoming battles
   at login so the Battles tab and revenge are meaningful, or cut revenge from the first version.
5. **Scope.** Recommended first release: Цели tabs, ladder battles, one banner event type. Defer King of the Hill,
   Proving Grounds weekly rewards, replays and guild events.

## Plan

Each phase ends playable and tested. Add a `tools/build_pvp_*.py --check` to `make test` for any generated data.

1. **Foundation.** `source/pvp.c`, save fields (trophies, wins, losses, best trophies, history, event state) with
   `PROGRESSION_SAVE_VERSION` handling, opponent generator, trophy rules. Unit tests in `tests/battle_sim_test.c`:
   determinism, level scaling, trophy bounds.
2. **Цели tabs.** Refactor `mission_view.c` into a tabbed `LeaderboardPopupV` screen. Add Trophies, Battles, My Events
   with empty states. Add the missing achievements to `tools/build_missions.py` and `MissionsSync`.
3. **PvP battle loop.** PvP hub, matchmaking screen, stamina spend, opponent runes, results with trophy delta,
   home button wiring, `GameExitBattleTo` routing, HUD trophy mode, consume and stamina dialog navigation.
4. **Banner events.** Event table and rotation, event info popup, banner awards and milestones, rank simulation,
   reward claims, HUD banner mode, ticker and news integration. Extend game changer filters as events need them.
5. **Optional.** King of the Hill, Proving Grounds, replays, defense log, Super Buffed tracking.

## Verification

- Add harness shots to `source/debug_harness.c` (`pvp_hub`, `pvp_match`, `goals_trophies`, `goals_battles`,
  `goals_events`, `pvp_event`). `TakeScreenshot` writes to the working directory under the file name only, so give
  `HOD_SHOT` a bare name and move the file out afterwards.
- The harness cannot click. Note that a `HOD_FIGHT` seed starts fights for `battle` and `nexus_battle` shots only;
  add it for PvP. Intro banners appear around 200 frames.
- `make test`, `python3 tools/check_comments.py`, `clang-format`. Never commit `player_save.dat`, recordings or cache.
- Report click flows you could not exercise.
