#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import math
import re
import struct
import sys
from pathlib import Path
from typing import Any

import original_cache

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT / "data/ftue_script.json"
PREFAB_BUNDLE = "RevampedFtueData"
CLI_DESCRIPTION = "Extract the 5.4.4 scripted tutorial (prefab.RevampedFtueData) into data/ftue_script.json."

SCRIPT_ROOT = "Scripoted FTUE Steps"
FUNNEL_FOLDERS = {"1-Main Funnel": "main", "2-Consume 2.0": "consume", "3-Fortify": "fortify"}
SECTIONS_SKIPPED_BY_5_4_4 = ("5-PvE Quests 3.5", "6-Consume", "7-Combine")

FTUE_SIGNALS = (
    "None", "MainScreenState", "BattleMapScreenState", "BattleMatchmakingScreenState", "PVENodeCompletionScreenState",
    "BattleResultsPopupView", "StoreTabsPopupView", "PackOpeningScreenState", "EditFormationScreenState",
    "RunesScreenState", "UnitDetailsPopupView_DEPRICATED", "RuneDetailsPopupView_DEPRECATED", "TrainingScreenState",
    "PromoteScreenState", "PVEBattleScreenState_DEPRICATED", "LoginState", "BattleScreenState", "MainInitState",
    "EncyclopediaPopupView", "LeaderboardPopupView", "FortifyScreenState", "FortifyConfirmView",
    "FortifyCelebrationView", "ConsumeScreenState", "PVEHubPopupView", "NexusScreenView", "NexusQuitMapPopupView",
)
SCRIPT_EVENTS = (
    "None", "UnitPurchaseFinished", "UnitAddedToSquad", "GiantUnitAddedToSquad", "RuneActivationStarted",
    "RuneActivationCompleted", "TargetUnitAdded", "SeedUnitAdded", "TrainingComplete", "VictoryOrDefeatEvent",
    "RuneDroppedInWorld", "UnitPurchaseSucceeded", "RuneDetailsShowing", "RuneDetailsClosed", "EncyclopediaClosed",
    "BattleMapNodePanFinished", "UnitDetailsShowing", "UnitDetailsClosed", "CloseConsumeToMainScreen",
    "FortifyParentAdded", "FortifyChildAdded", "UnitPurchaseNextButtonAppeared",
)
TEXT_BAR_LOCATIONS = ("Top", "Bottom", "Right", "Left")
HOST_SIZES = ("Small", "Medium", "Large")
HOST_TYPES = ("Default", "Contemplative", "Satisfied", "Happy")
EXTRA_ICONS = (
    "None", "CenterHealthPowerSpeed", "RightHeath", "RightTimes2", "FactionColors", "EnergyIcon", "StaminaIcon",
    "LeftSpeeds", "StunIcon", "CenterBattle", "CenterGiant", "CenterInfo",
)
HIGHLIGHTED_ELEMENTS = (
    "None", "MainScreen_QuestButton", "PveMapScreen_EnterButton", "BattleMatchmakingScreen_FightButton",
    "BattleResultsPopup_NextQuestButton", "BattleResultsPopup_StoreButton", "StoreTabsPopup_BronzePack",
    "StoreTabsPopup_SilverPack", "StoreTabsPopup_GoldPack", "PackOpenScreen_SquadButton",
    "EditFormation_NewUnitButton", "UnitDetails_UnitPanelButton", "EditFormation_RuneButton",
    "RuneScreen_NewRuneButton", "RuneDetails_RunePanelButton", "RuneScreen_SquadButton", "EditFormation_QuestButton",
    "StoreTabsPopup_SilverPackButton", "StoreTabsPopup_GoldPackButton", "PveMapScreen_HomeButton",
    "MainScreen_StoreButton", "StoreTabsPopup_BronzePackButton", "PackOpenScreen_ConsumeButton",
    "TrainingScreen_ConsumeButton", "TrainingScreen_CombineButton", "TrainingScreen_SquadButton",
    "EditFormation_CombineButton", "PromoteScreen_HomeButton", "BattleMatchmakingScreen_RetreatButton",
    "PveMapScreen_EnergyBar", "MainScreen_StaminaBar", "RuneScreen_ActivateButton", "BattleScreen_VictoryScreenTap",
    "Encyclopedia_CloseButton", "ReactiveScreen_CloseButton", "PveMapScreen_MasteryBar",
    "MainScreen_LeaderboardButton", "LeaderboardPopup_CloseButton", "PackOpenScreen_RuneIcon",
    "RuneScreen_ActiveRuneLocation", "ConsumeScreen_HomeButton", "PackOpenScreen_NextButton",
    "FortifyScreen_ParentSlot", "FortifyScreen_ChildSlot", "FortifyScreen_ApplyButton",
    "FortifyConfirmScreen_ConfirmButton", "StoreTabsPopup_ConsumePack", "StoreTabsPopup_ConsumePackButton",
)
ARROW_DIRECTIONS = ("Down", "Right", "Up", "Hidden")
RARITIES = ("Undefined", "Common", "Uncommon", "Rare", "Epic", "Legendary", "Mythic", "Prestige")
CONSUME_UNIT_TYPES = ("None", "Parent", "Child")
DRAG_ITEM_TYPES = ("None", "RegularUnit", "GiantUnit", "Rune", "FortifyParentUnit", "FortifyChildUnit")
SPECIAL_ACTIONS = (
    "None", "ShowReactiveScreenAbout", "ShowReactiveScreenHelp", "AwardGiantUnit", "ShowFtueBattle",
    "ForceExitFromFtueBattle", "BattleMapFlipBossNode", "AwardHiddenStorePack",
)
HIGHLIGHT_UNITS = (
    "None", "PlayerFrontRow", "PlayerBackRow", "EnemyUncommon", "AllEnemies", "EnemyRare", "PlayerPos1", "PlayerPos2",
    "PlayerPos3", "PlayerPos4",
)
HIDDEN_ELEMENTS = ("None", "BattleMatchmakingScreen_RetreatButton", "BattleMatchmakingScreen_FightButton")
MOCK_STORE_MODES = ("None", "Consume2")
FUNNEL_STARTS = ("None", "Fortify", "Consume")
FUNNEL_ENDS = ("None", "Regular", "Fortify", "Consume")

STEP_FIELDS = (
    ("step", "f32", None),
    ("text", "string", None),
    ("signal", "i32", FTUE_SIGNALS),
    ("event", "i32", SCRIPT_EVENTS),
    ("text_bar", "i32", TEXT_BAR_LOCATIONS),
    ("guide_on_left", "bool", None),
    ("guide_size", "i32", HOST_SIZES),
    ("guide_type", "i32", HOST_TYPES),
    ("extra_icon", "i32", EXTRA_ICONS),
    ("force_scrim", "bool", None),
    ("highlight", "i32", HIGHLIGHTED_ELEMENTS),
    ("arrow", "i32", ARROW_DIRECTIONS),
    ("drag_arrow", "bool", None),
    ("drag_unit_rarity", "i32", RARITIES),
    ("drag_child_rarity", "i32", RARITIES),
    ("consume_unit_type", "i32", CONSUME_UNIT_TYPES),
    ("drag_item", "i32", DRAG_ITEM_TYPES),
    ("highlight_required", "bool", None),
    ("special_action", "i32", SPECIAL_ACTIONS),
    ("award_giant", "bool", None),
    ("hold_battle", "bool", None),
    ("input_before_screen", "bool", None),
    ("highlight_units", "i32", HIGHLIGHT_UNITS),
    ("hidden_element", "i32", HIDDEN_ELEMENTS),
    ("map_id", "string", None),
    ("map_node_step", "i32", None),
    ("restore_point_for_server", "bool", None),
    ("restore_point", "bool", None),
    ("mock_store", "i32", MOCK_STORE_MODES),
    ("funnel_start", "i32", FUNNEL_STARTS),
    ("funnel_end", "i32", FUNNEL_ENDS),
    ("continue_button", "bool", None),
    ("award_ally_link", "bool", None),
)
DEFAULTS = {"None", "Undefined", "", 0, False}
ALWAYS_WRITTEN = {"step", "signal", "guide_on_left", "guide_size", "guide_type", "continue_button", "text_bar"}


class Reader:

    def __init__(self, data: bytes):
        self.data = data
        self.offset = 0

    def _need(self, size: int) -> None:
        if self.offset + size > len(self.data):
            raise ValueError("truncated MonoBehaviour")

    def i32(self) -> int:
        self._need(4)
        value = struct.unpack_from("<i", self.data, self.offset)[0]
        self.offset += 4
        return value

    def f32(self) -> float:
        self._need(4)
        value = struct.unpack_from("<f", self.data, self.offset)[0]
        self.offset += 4
        if not math.isfinite(value):
            raise ValueError("non-finite float")
        return float(value)

    def bool(self) -> bool:
        self._need(1)
        value = self.data[self.offset]
        self.offset = (self.offset + 4) & ~3
        if value > 1:
            raise ValueError("bad bool")
        return value == 1

    def string(self) -> str:
        size = self.i32()
        if size < 0 or size > 4096:
            raise ValueError(f"bad string length {size}")
        self._need(size)
        value = self.data[self.offset : self.offset + size].decode("utf-8")
        self.offset = (self.offset + size + 3) & ~3
        return value

    def header(self) -> int:
        self.i32()
        game_object = self.i32()
        self.bool()
        self.i32()
        self.i32()
        self.string()
        return game_object


def parse_step(raw: bytes) -> tuple[int, dict[str, Any]] | None:
    try:
        reader = Reader(raw)
        game_object = reader.header()
        values: dict[str, Any] = {}
        for name, kind, names in STEP_FIELDS:
            value = getattr(reader, kind)()
            if names is not None:
                if not 0 <= value < len(names):
                    return None
                value = names[value]
            values[name] = value
    except (UnicodeDecodeError, ValueError, struct.error):
        return None
    if reader.offset != len(raw):
        return None
    return game_object, values


def compact(values: dict[str, Any]) -> dict[str, Any]:
    out = {}
    for name, value in values.items():
        if name in ALWAYS_WRITTEN or value not in DEFAULTS:
            out[name] = round(value, 2) if isinstance(value, float) else value
    return out


def configure_unitypy() -> Any:
    try:
        import UnityPy  # type: ignore
        from UnityPy.helpers import UnityVersion as UV  # type: ignore
    except ImportError as exc:
        raise SystemExit("build_ftue_script.py requires UnityPy; run it with the project's UnityPy venv.") from exc

    original = UV.UnityVersion.from_str.__func__

    def patched(cls: Any, version: str) -> Any:
        match = re.search(r"(\d+)\.(\d+)\.(\d+)([a-zA-Z]+)(\d+)", version)
        return original(cls, match.group(0) if match else version)

    UV.UnityVersion.from_str = classmethod(patched)
    return UnityPy


def bundle_path() -> Path:
    for path in original_cache.bundle_paths():
        if original_cache.bundle_name(path) == PREFAB_BUNDLE:
            return path
    raise SystemExit(f"{PREFAB_BUNDLE} is not in {original_cache.BUNDLE_DIR}")


def game_object_paths(objects: list[Any]) -> dict[int, list[str]]:
    names: dict[int, str] = {}
    transform_owner: dict[int, int] = {}
    transform_parent: dict[int, int] = {}
    for obj in objects:
        if obj.type.name == "GameObject":
            names[obj.path_id] = obj.read_typetree()["m_Name"]
        elif obj.type.name == "Transform":
            tree = obj.read_typetree()
            transform_owner[obj.path_id] = tree["m_GameObject"]["m_PathID"]
            transform_parent[obj.path_id] = tree["m_Father"]["m_PathID"]
    parent_of = {
        owner: transform_owner.get(transform_parent.get(transform, 0), 0)
        for transform, owner in transform_owner.items()
    }
    paths = {}
    for game_object in names:
        parts, cursor = [], game_object
        while cursor:
            parts.append(names.get(cursor, "?"))
            cursor = parent_of.get(cursor, 0)
        paths[game_object] = list(reversed(parts))
    return paths


def build(unitypy: Any) -> dict[str, Any]:
    env = unitypy.load(str(bundle_path()))
    objects = list(env.objects)
    paths = game_object_paths(objects)
    funnels: dict[str, list[dict[str, Any]]] = {name: [] for name in FUNNEL_FOLDERS.values()}
    skipped = 0
    for obj in objects:
        if obj.type.name != "MonoBehaviour":
            continue
        parsed = parse_step(bytes(obj.get_raw_data()))
        if not parsed:
            continue
        game_object, values = parsed
        path = paths.get(game_object, [])
        if SCRIPT_ROOT not in path or len(path) < path.index(SCRIPT_ROOT) + 3:
            raise SystemExit(f"step {values['step']} sits outside {SCRIPT_ROOT}: {path}")
        root = path.index(SCRIPT_ROOT)
        funnel_folder, section = path[root + 1], path[root + 2]
        if funnel_folder not in FUNNEL_FOLDERS:
            raise SystemExit(f"unknown funnel folder {funnel_folder!r}")
        if section in SECTIONS_SKIPPED_BY_5_4_4:
            skipped += 1
            continue
        step = compact(values)
        step["section"] = section
        funnels[FUNNEL_FOLDERS[funnel_folder]].append(step)
    for steps in funnels.values():
        steps.sort(key=lambda step: step["step"])
    return {
        "version": 1,
        "source": f"cache:{PREFAB_BUNDLE} FtueScriptStep components, field order from FtueScriptStep.cs",
        "skipped_sections": list(SECTIONS_SKIPPED_BY_5_4_4),
        "skipped_steps": skipped,
        "funnels": funnels,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=CLI_DESCRIPTION)
    parser.add_argument("--output", type=Path, default=OUTPUT)
    args = parser.parse_args()
    script = build(configure_unitypy())
    args.output.write_text(json.dumps(script, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
    counts = ", ".join(f"{name} {len(steps)}" for name, steps in script["funnels"].items())
    print(f"wrote {args.output}: {counts}, {script['skipped_steps']} skipped", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
