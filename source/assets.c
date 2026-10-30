typedef enum {

    GUI_HUD_GRADIENT,
    GUI_HUD_XP,
    GUI_HUD_XP_FRAME,
    GUI_HUD_XP_BG,
    GUI_HUD_XP_METER,
    GUI_HUD_RESOURCE_FRAME,
    GUI_HUD_ENERGY,
    GUI_HUD_STAMINA,
    GUI_HUD_GOLD,
    GUI_HUD_GOLD_SELECTED,
    GUI_HUD_GEMS,
    GUI_HUD_GEMS_SELECTED,
    GUI_HUD_OPTIONS,

    GUI_BTN_CAMP,
    GUI_BTN_CAMP_ON,
    GUI_BTN_CAMP_INACTIVE,
    GUI_BTN_STORE,
    GUI_BTN_STORE_ON,
    GUI_BTN_HISTORY,
    GUI_BTN_HISTORY_ON,
    GUI_BTN_HEROES,
    GUI_BTN_HEROES_ON,
    GUI_BTN_HOMEQUEST,
    GUI_BTN_HOMEQUEST_ON,
    GUI_BTN_PVP,
    GUI_BTN_PVP_ON,
    GUI_BTN_GREY,
    GUI_BTN_GREY_ON,
    GUI_BTN_GOLD,
    GUI_BTN_GOLD_ON,
    GUI_BTN_TRAIN,
    GUI_BTN_TRAIN_ON,
    GUI_BTN_FORTIFY,
    GUI_BTN_FORTIFY_ON,
    GUI_BTN_RUNES,
    GUI_BTN_RUNES_ON,
    GUI_BTN_GALLERY,
    GUI_BTN_GALLERY_ON,
    GUI_BTN_SQUAD,
    GUI_BTN_SQUAD_ON,
    GUI_BTN_MAINQUEST,
    GUI_BTN_MAINQUEST_ON,
    GUI_BTN_PACK_TRAIN,
    GUI_BTN_PACK_TRAIN_ON,
    GUI_BTN_PACK_RUNES,
    GUI_BTN_PACK_RUNES_ON,
    GUI_BTN_PACK_SQUAD,
    GUI_BTN_PACK_SQUAD_ON,
    GUI_BTN_CLOSE,
    GUI_BTN_MAPS,

    GUI_STORE_HEADER_RECRUIT,
    GUI_STORE_HEADER_SOLDIER,
    GUI_STORE_HEADER_CHAMPION,
    GUI_STORE_HEADER_GRAND,
    GUI_STORE_ICON_RECRUIT,
    GUI_STORE_ICON_SOLDIER,
    GUI_STORE_ICON_CHAMPION,
    GUI_STORE_ICON_GRAND,
    GUI_STORE_CARD,
    GUI_STORE_BUY,
    GUI_STORE_BUY_ON,
    GUI_STORE_POPUP,
    GUI_STORE_TITLE,
    GUI_STORE_OVERLAY,
    GUI_STORE_BACK,
    GUI_STORE_CATEGORY_FRAME,
    GUI_STORE_CATEGORY_UNITS,
    GUI_STORE_CATEGORY_GEMS,
    GUI_STORE_CATEGORY_RUNES,
    GUI_STORE_SCROLL_ARROW,
    GUI_STORE_CURRENCY_GEMS,
    GUI_STORE_GEM_CARD,
    GUI_STORE_GEM_COIN,
    GUI_STORE_GEM_QUEST,
    GUI_STORE_GEM_INFO_FRAME,
    GUI_STORE_BANNER,
    GUI_STORE_GEM_ICON_0,
    GUI_STORE_GEM_ICON_1,
    GUI_STORE_GEM_ICON_2,
    GUI_STORE_GEM_ICON_3,
    GUI_STORE_GEM_ICON_4,
    GUI_STORE_GEM_ICON_5,
    GUI_STORE_GEM_ICON_6,
    GUI_STORE_INFO_ICON,
    GUI_STORE_INFO_ICON_ON,

    GUI_HEROES_PANEL,
    GUI_HEROES_ROW_HEALTH,
    GUI_HEROES_ROW_CRIT,

    GUI_CARD_SELECT,
    GUI_CARD_LEVEL,

    GUI_TAB_SQUAD,
    GUI_TAB_SQUAD_ON,
    GUI_TAB_DEFENSE,
    GUI_TAB_DEFENSE_ON,
    GUI_TAB_TROPHY,
    GUI_TAB_TROPHY_ON,
    GUI_CARD_BAR_BG,
    GUI_CARD_BAR_FILL,

    GUI_UNIT_DETAILS,
    GUI_PLAQUE_COMMON,
    GUI_PLAQUE_UNCOMMON,
    GUI_PLAQUE_RARE,
    GUI_PLAQUE_EPIC,
    GUI_PLAQUE_LEGENDARY,
    GUI_DETAILS_POWER,
    GUI_DETAILS_HEALTH,
    GUI_DETAILS_CRIT,
    GUI_DETAILS_INIT_QUICK,
    GUI_DETAILS_INIT_NORMAL,
    GUI_DETAILS_INIT_SLOW,
    GUI_DETAILS_PIPS,
    GUI_DETAILS_BAR,

    GUI_COIN_LARGE,
    GUI_COIN_WHITE,
    GUI_COIN_RED,
    GUI_COIN_BLUE,
    GUI_COIN_BLACK,
    GUI_COIN_WHITE_RED,
    GUI_COIN_WHITE_BLUE,
    GUI_COIN_RED_BLUE,
    GUI_COIN_WHITE_BLACK,
    GUI_COIN_RED_BLACK,
    GUI_COIN_BLUE_BLACK,

    GUI_QUESTS_BG,
    GUI_DIALOG_GEM,
    GUI_REWARD_BRUSH,

    GUI_MAP_BG_0,
    GUI_MAP_BG_1,
    GUI_MAP_BG_2,
    GUI_MAP_BG_3,
    GUI_MAP_BG_4,
    GUI_MAP_BG_5,
    GUI_MAP_BG_6,
    GUI_MAP_BG_7,
    GUI_MAP_BG_8,
    GUI_MAP_BG_9,
    GUI_MAP_BG_10,
    GUI_MAP_BG_11,
    GUI_MAP_BG_12,
    GUI_MAP_BG_13,
    GUI_MAP_BG_14,
    GUI_MAP_BG_15,
    GUI_MAP_SEPARATOR,
    GUI_MAP_ENERGY,
    GUI_MAP_LORE,
    GUI_MAP_PROGRESS,
    GUI_MAP_HELP,
    GUI_MAP_GEMS,
    GUI_MAP_REWARD_TAG,
    GUI_NODE_LOCKED,
    GUI_NODE_LOCKED_ON,
    GUI_NODE_OPEN,
    GUI_NODE_OPEN_ON,
    GUI_NODE_DONE,
    GUI_NODE_DONE_ON,
    GUI_NODE_MASTERED,
    GUI_NODE_MASTERED_ON,
    GUI_NODE_BOSS,
    GUI_NODE_BOSS_ON,
    GUI_NODE_BOSS_LOCKED,
    GUI_NODE_BOSS_LOCKED_ON,
    GUI_NODE_GRIND,
    GUI_NODE_GRIND_ON,
    GUI_NODE_UNLOCK_GLOW,

    GUI_NEXUS_POTION,
    GUI_NEXUS_DIVIDER,
    GUI_NEXUS_TIMER_PLATE,
    GUI_NEXUS_EVENT_FRAME,
    GUI_NEXUS_EVENT_TITLEBAR,
    GUI_NEXUS_KEY_1,
    GUI_NEXUS_KEY_2,
    GUI_NEXUS_KEY_3,
    GUI_NEXUS_KEY_4,

    GUI_BTN_GOLD_COST,
    GUI_BTN_GOLD_COST_ON,
    GUI_MM_PANEL,
    GUI_MM_TROUGH,
    GUI_MM_RUNE_SLOT,
    GUI_MM_TAG,
    GUI_MM_XP,
    GUI_MM_SQUAD_FRAME,
    GUI_MM_POTION,
    GUI_MM_PLATE,
    GUI_MM_PLATE_BORDER,
    GUI_MM_HEALTH,

    GUI_MSG_DAMAGE,
    GUI_MSG_CRIT,
    GUI_MSG_CRIT_REWARD,
    GUI_MSG_HEAL,
    GUI_MSG_PWR_UP,
    GUI_MSG_PWR_DOWN,
    GUI_MSG_INIT_UP,
    GUI_MSG_INIT_DOWN,
    GUI_MSG_STUN,
    GUI_MSG_CURSE_PWR,
    GUI_MSG_CURSE_HP,

    GUI_COMBAT_ROUND,
    GUI_COMBAT_BANNER,
    GUI_COMBAT_PREBATTLE_BG,
    GUI_COMBAT_GOLD_BG,
    GUI_COMBAT_FLOURISH,
    GUI_TEAMBAR_GEOM,
    GUI_TEAMBAR_FILL,
    GUI_TEAMBAR_LCAP,
    GUI_TEAMBAR_RCAP,
    GUI_COMBAT_FF,
    GUI_COMBAT_FF_ON,
    GUI_SUMMARY_VS,
    GUI_SUMMARY_PLAQUE,
    GUI_SUMMARY_DAMAGE,
    GUI_SUMMARY_FACTION,
    GUI_SUMMARY_MVP,
    GUI_SUMMARY_SURVIVORS,

    GUI_RESULTS_OVERLAY,
    GUI_RESULTS_BG,
    GUI_RESULTS_ROSTER_BG,
    GUI_RESULTS_STARBURST,
    GUI_RESULTS_MASTERY_BASE,
    GUI_RESULTS_MASTERY_PROGRESS,
    GUI_RESULTS_MASTERY_HIGHLIGHT,
    GUI_RESULTS_STORE,
    GUI_RESULTS_STORE_ON,
    GUI_RESULTS_HEROES,
    GUI_RESULTS_HEROES_ON,
    GUI_RESULTS_MAP,
    GUI_RESULTS_MAP_ON,

    GUI_SPLASH_INKSMEAR,
    GUI_SPLASH_INKSPLAT,
    GUI_SPLASH_GRUNGE,
    GUI_SPLASH_GLOW,
    GUI_SPLASH_SMOKE,

    GUI_COUNT
} GuiId;

typedef enum { FNT_MENU, FNT_COOL, FNT_COUNT } FontId;
typedef enum {
    SND_UI_CLICK,
    SND_UI_BIG_BUTTON,
    SND_UI_HOME,
    SND_UI_POPUP_CLOSE,
    SND_UI_TAB,
    SND_STAMINA_REFRESH,
    SND_PVE_NODE_SELECTED,
    SND_COMBAT_SPEED,
    SND_BATTLE_INFO_OPEN,
    SND_BATTLE_INFO_CLOSE,
    SND_LEVEL_UP,
    SND_PVE_NODE_TRANSITION,
    SND_MAP_TRANSITION,
    SND_TALLY_START,
    SND_TALLY_HIT,
    SND_TALLY_END,
    SND_TALLY_LOSS_START,
    SND_TALLY_LOSS_END,
    SND_STORE_OPEN,
    SND_PACK_SHUFFLE,
    SND_PACK_REVEAL,
    SND_STORE_INFO,
    SND_STORE_ERROR,
    SND_PACK_COMMON,
    SND_PACK_UNCOMMON,
    SND_PACK_RARE,
    SND_PACK_EPIC,
    SND_PACK_LEGENDARY,
    SND_COUNT
} SoundId;

typedef enum {
    MUS_HOME1,
    MUS_HOME2,
    MUS_HOME3,
    MUS_HOME_COUNT,
    MUS_BATTLE_INTRO = MUS_HOME_COUNT,
    MUS_BATTLE_LOOP,
    MUS_BATTLE_AMBIENCE,
    MUS_COMBAT_INTRO,
    MUS_COMBAT_1,
    MUS_COMBAT_2,
    MUS_COMBAT_3,
    MUS_VICTORY,
    MUS_DEFEAT,
    MUS_COUNT
} MusicId;

static const char *const gui_paths[GUI_COUNT] = {

    [GUI_HUD_GRADIENT] = HOD_CACHE "meshes/_shared_textures/bg_HUDbake.png",
    [GUI_HUD_XP] = HOD_CACHE "gui/misc/Ico_XpHUD.png",
    [GUI_HUD_XP_FRAME] = HOD_CACHE "gui/bars_progress/playerlevel_bar_empty.png",
    [GUI_HUD_XP_BG] = HOD_CACHE "gui/bars_progress/playerlevel_bar_bg.png",
    [GUI_HUD_XP_METER] = HOD_CACHE "gui/misc/playerlevel_bar.png",
    [GUI_HUD_RESOURCE_FRAME] = HOD_CACHE "gui/misc/tex.EnergyStaminaContainer.png",
    [GUI_HUD_ENERGY] = HOD_CACHE "apk_gui/hud_bars_currency/Ico_EnergyHUD.png",
    [GUI_HUD_STAMINA] = HOD_CACHE "apk_gui/hud_bars_currency/Ico_StaminaHUD.png",
    [GUI_HUD_GOLD] = HOD_CACHE "apk_gui/hud_bars_currency/tex.softcurrency.png",
    [GUI_HUD_GOLD_SELECTED] = HOD_CACHE "gui/currency/tex.softcurrency_active.png",
    [GUI_HUD_GEMS] = HOD_CACHE "gui/currency/ico_gemHUD.png",
    [GUI_HUD_GEMS_SELECTED] = HOD_CACHE "gui/currency/ico_gemHUD_selected.png",
    [GUI_HUD_OPTIONS] = HOD_CACHE "gui/misc/ico_settings.png",

    [GUI_BTN_CAMP] = HOD_CACHE "gui/buttons_tabs/Btn_homesquare.png",
    [GUI_BTN_CAMP_ON] = HOD_CACHE "gui/buttons_tabs/Btn_homesquare_active.png",
    [GUI_BTN_CAMP_INACTIVE] = HOD_CACHE "gui/buttons_tabs/Btn_homesquare_inactive.png",
    [GUI_BTN_STORE] = "assets/store_button.png",
    [GUI_BTN_STORE_ON] = "assets/store_button_active.png",
    [GUI_BTN_HISTORY] = "assets/history_button.png",
    [GUI_BTN_HISTORY_ON] = "assets/history_button_active.png",
    [GUI_BTN_HEROES] = "assets/heroes_button.png",
    [GUI_BTN_HEROES_ON] = "assets/heroes_button_active.png",
    [GUI_BTN_HOMEQUEST] = "assets/pve_button.png",
    [GUI_BTN_HOMEQUEST_ON] = "assets/pve_button_active.png",
    [GUI_BTN_PVP] = "assets/pvp_button.png",
    [GUI_BTN_PVP_ON] = "assets/pvp_button_active.png",
    [GUI_BTN_GREY] = HOD_CACHE "apk_gui/buttons/Btn_Default_grey.png",
    [GUI_BTN_GREY_ON] = HOD_CACHE "apk_gui/buttons/Btn_Default_grey_active.png",
    [GUI_BTN_GOLD] = HOD_CACHE "apk_gui/buttons/Btn_CTA_gold.png",
    [GUI_BTN_GOLD_ON] = HOD_CACHE "apk_gui/buttons/Btn_CTA_gold_active.png",
    [GUI_BTN_TRAIN] = HOD_CACHE "gui/buttons_tabs/Btn_Heroes_Train.png",
    [GUI_BTN_TRAIN_ON] = HOD_CACHE "gui/buttons_tabs/Btn_Heroes_TrainActive.png",
    [GUI_BTN_FORTIFY] = HOD_CACHE "gui/buttons_tabs/Btn_Heroes_FortifyDefault.png",
    [GUI_BTN_FORTIFY_ON] = HOD_CACHE "gui/buttons_tabs/Btn_heroes_FortifyActive.png",
    [GUI_BTN_RUNES] = HOD_CACHE "gui/buttons_tabs/Btn_Heroes_RunesDefault.png",
    [GUI_BTN_RUNES_ON] = HOD_CACHE "gui/buttons_tabs/Btn_Heroes_RunesActive.png",
    [GUI_BTN_GALLERY] = HOD_CACHE "gui/buttons_tabs/Btn_Heroes_GalleryDefault.png",
    [GUI_BTN_GALLERY_ON] = HOD_CACHE "gui/buttons_tabs/Btn_Heroes_GalleryActive.png",
    [GUI_BTN_SQUAD] = HOD_CACHE "gui/buttons_tabs/Btn_Heroes_Squad.png",
    [GUI_BTN_SQUAD_ON] = HOD_CACHE "gui/buttons_tabs/Btn_Heroes_SquadActive.png",
    [GUI_BTN_MAINQUEST] = HOD_CACHE "gui/buttons_tabs/ButtonMainQuest.png",
    [GUI_BTN_MAINQUEST_ON] = HOD_CACHE "gui/buttons_tabs/ButtonMainQuestActive.png",
    [GUI_BTN_PACK_TRAIN] = HOD_CACHE "gui/pack_opening/Btn_TrainSmall.png",
    [GUI_BTN_PACK_TRAIN_ON] = HOD_CACHE "gui/pack_opening/Btn_TrainSmall_active.png",
    [GUI_BTN_PACK_RUNES] = HOD_CACHE "gui/pack_opening/Btn_RuneSmall.png",
    [GUI_BTN_PACK_RUNES_ON] = HOD_CACHE "gui/pack_opening/Btn_RuneSmall_active.png",
    [GUI_BTN_PACK_SQUAD] = HOD_CACHE "gui/pack_opening/Btn_SquadSmall.png",
    [GUI_BTN_PACK_SQUAD_ON] = HOD_CACHE "gui/pack_opening/Btn_SquadSmall_active.png",
    [GUI_BTN_CLOSE] = HOD_CACHE "apk_gui/buttons/Store_CloseButton.png",
    [GUI_BTN_MAPS] = HOD_CACHE "gui/buttons_tabs/Button_HUD_Full_Maps_active.png",

    [GUI_STORE_HEADER_RECRUIT] = HOD_CACHE "gui/misc/tex.Store_HeaderBGBronzePack.png",
    [GUI_STORE_HEADER_SOLDIER] = HOD_CACHE "gui/misc/tex.Store_HeaderBGSilverPack.png",
    [GUI_STORE_HEADER_CHAMPION] = HOD_CACHE "gui/misc/tex.Store_HeaderChampionPack.png",
    [GUI_STORE_HEADER_GRAND] = HOD_CACHE "gui/misc/tex.Store_HeaderGrandChampionPack.png",
    [GUI_STORE_ICON_RECRUIT] = HOD_CACHE "gui/icons/tex.Store_IconBronzePack.png",
    [GUI_STORE_ICON_SOLDIER] = HOD_CACHE "gui/icons/tex.Store_IconSilverPack.png",
    [GUI_STORE_ICON_CHAMPION] = HOD_CACHE "gui/icons/tex.Store_IconGoldPack.png",
    [GUI_STORE_ICON_GRAND] = HOD_CACHE "gui/icons/tex.Store_IconLegendaryPack.png",
    [GUI_STORE_CARD] = HOD_CACHE "gui/misc/ItemCardBase.png",
    [GUI_STORE_BUY] = HOD_CACHE "gui/buttons_tabs/ButtonBuy.png",
    [GUI_STORE_BUY_ON] = HOD_CACHE "gui/buttons_tabs/ButtonBuyActive.png",
    [GUI_STORE_POPUP] = HOD_CACHE "gui/frames_backgrounds/Bg_Popup_Stone.png",
    [GUI_STORE_TITLE] = HOD_CACHE "apk_gui/backgrounds/BG_Title.png",
    [GUI_STORE_OVERLAY] = HOD_CACHE "apk_gui/backgrounds/Bg_Overlay_Short.png",
    [GUI_STORE_BACK] = HOD_CACHE "apk_gui/buttons/ButtonBackDefault.png",
    [GUI_STORE_CATEGORY_FRAME] = HOD_CACHE "gui/frames_backgrounds/FrameStoreCategory.png",
    [GUI_STORE_CATEGORY_UNITS] = HOD_CACHE "gui/misc/tex.StoreCategoryUNITS.png",
    [GUI_STORE_CATEGORY_GEMS] = HOD_CACHE "gui/currency/tex.StoreCategoryGEMS.png",
    [GUI_STORE_CATEGORY_RUNES] = HOD_CACHE "gui/runes_buffs/tex.StoreCategoryRUNES.png",
    [GUI_STORE_SCROLL_ARROW] = HOD_CACHE "gui/misc/shared_arrow.png",
    [GUI_STORE_CURRENCY_GEMS] = HOD_CACHE "gui/currency/tex.gemscurrency.png",
    [GUI_STORE_GEM_CARD] = HOD_CACHE "gui/frames_backgrounds/FrameItemGemCard.png",
    [GUI_STORE_GEM_COIN] = HOD_CACHE "gui/currency/tex.Coin_Gems.png",
    [GUI_STORE_GEM_QUEST] = HOD_CACHE "gui/icons/tex.IconGemsQuest.png",
    [GUI_STORE_GEM_INFO_FRAME] = HOD_CACHE "gui/frames_backgrounds/FrameInformation.png",
    [GUI_STORE_BANNER] = HOD_CACHE "gui/screens_popups/BannerShopMessaging.png",
    [GUI_STORE_GEM_ICON_0] = HOD_CACHE "gui/icons/tex.IconGems0.png",
    [GUI_STORE_GEM_ICON_1] = HOD_CACHE "gui/icons/tex.IconGems1.png",
    [GUI_STORE_GEM_ICON_2] = HOD_CACHE "gui/icons/tex.IconGems2.png",
    [GUI_STORE_GEM_ICON_3] = HOD_CACHE "gui/icons/tex.IconGems3.png",
    [GUI_STORE_GEM_ICON_4] = HOD_CACHE "gui/icons/tex.IconGems4.png",
    [GUI_STORE_GEM_ICON_5] = HOD_CACHE "gui/icons/tex.IconGems5.png",
    [GUI_STORE_GEM_ICON_6] = HOD_CACHE "gui/icons/tex.IconGems6.png",
    [GUI_STORE_INFO_ICON] = HOD_CACHE "gui/icons/ico_help_info.png",
    [GUI_STORE_INFO_ICON_ON] = HOD_CACHE "gui/icons/ico_help_info_select.png",

    [GUI_HEROES_PANEL] = HOD_CACHE "gui/frames_backgrounds/bg_unittray_squads.png",
    [GUI_HEROES_ROW_HEALTH] = HOD_CACHE "vfx/textures/rowbonus_front_health__tex.rowbonus_front_health.png",
    [GUI_HEROES_ROW_CRIT] = HOD_CACHE "vfx/textures/rowbonus_back_critchance__tex.rowbonus_back_critchance.png",

    [GUI_CARD_SELECT] = HOD_CACHE "gui/misc/Unit_State_Select_Orange.png",
    [GUI_CARD_LEVEL] = HOD_CACHE "gui/misc/Ico_XP_wStar.png",

    [GUI_TAB_SQUAD] = HOD_CACHE "gui/misc/toggle_squad.png",
    [GUI_TAB_SQUAD_ON] = HOD_CACHE "gui/misc/toggle_squad_select.png",
    [GUI_TAB_DEFENSE] = HOD_CACHE "gui/misc/multisquad_defensive.png",
    [GUI_TAB_DEFENSE_ON] = HOD_CACHE "gui/misc/multisquad_defensive_select.png",
    [GUI_TAB_TROPHY] = HOD_CACHE "gui/misc/tex.trophy.png",
    [GUI_TAB_TROPHY_ON] = HOD_CACHE "gui/misc/tex.trophy_active.png",
    [GUI_CARD_BAR_BG] = HOD_CACHE "gui/frames_backgrounds/Bg_Exp_Background.png",
    [GUI_CARD_BAR_FILL] = HOD_CACHE "gui/bars_progress/MeterFillBlue.png",

    [GUI_UNIT_DETAILS] = HOD_CACHE "gui/frames_backgrounds/Bg_Unit_Details.png",
    [GUI_PLAQUE_COMMON] = HOD_CACHE "meshes/shared_resourcecontainer/Bg_FrameCommon.png",
    [GUI_PLAQUE_UNCOMMON] = HOD_CACHE "meshes/shared_resourcecontainer/Bg_FrameUncommon.png",
    [GUI_PLAQUE_RARE] = HOD_CACHE "meshes/shared_resourcecontainer/Bg_FrameRare.png",
    [GUI_PLAQUE_EPIC] = HOD_CACHE "meshes/shared_resourcecontainer/Bg_FrameEpic.png",
    [GUI_PLAQUE_LEGENDARY] = HOD_CACHE "meshes/shared_resourcecontainer/Bg_FrameLegendary.png",
    [GUI_DETAILS_POWER] = HOD_CACHE "gui/misc/tex.Ico_Power.png",
    [GUI_DETAILS_HEALTH] = HOD_CACHE "gui/misc/Ico_Health.png",
    [GUI_DETAILS_CRIT] = HOD_CACHE "gui/misc/tex.Ico_CritChance.png",
    [GUI_DETAILS_INIT_QUICK] = HOD_CACHE "gui/misc/tex.battle_initiative_quick.png",
    [GUI_DETAILS_INIT_NORMAL] = HOD_CACHE "gui/misc/tex.battle_initiative_normal.png",
    [GUI_DETAILS_INIT_SLOW] = HOD_CACHE "gui/misc/tex.battle_initiative_slow.png",
    [GUI_DETAILS_PIPS] = HOD_CACHE "gui/misc/tex.Ico_Promote2.png",
    [GUI_DETAILS_BAR] = HOD_CACHE "gui/frames_backgrounds/Bg_Exp_Bar.png",

    [GUI_COIN_LARGE] = HOD_CACHE "gui/icons/tex.faction_lrgcreature_icon.png",
    [GUI_COIN_WHITE] = HOD_CACHE "gui/icons/tex.faction_white_icon.png",
    [GUI_COIN_RED] = HOD_CACHE "gui/icons/tex.faction_red_icon.png",
    [GUI_COIN_BLUE] = HOD_CACHE "gui/icons/tex.faction_blue_icon.png",
    [GUI_COIN_BLACK] = HOD_CACHE "gui/icons/tex.faction_black_icon.png",
    [GUI_COIN_WHITE_RED] = HOD_CACHE "gui/icons/tex.faction_redwhite_icon.png",
    [GUI_COIN_WHITE_BLUE] = HOD_CACHE "gui/icons/tex.faction_bluewhite_icon.png",
    [GUI_COIN_RED_BLUE] = HOD_CACHE "gui/icons/tex.faction_redblue_icon.png",
    [GUI_COIN_WHITE_BLACK] = HOD_CACHE "gui/icons/tex.faction_blackwhite_icon.png",
    [GUI_COIN_RED_BLACK] = HOD_CACHE "gui/icons/tex.faction_blackred_icon.png",
    [GUI_COIN_BLUE_BLACK] = HOD_CACHE "gui/icons/tex.faction_blackblue_icon.png",

    [GUI_QUESTS_BG] = HOD_CACHE "meshes/_shared_textures/Bg_Overlay.png",
    [GUI_DIALOG_GEM] = HOD_CACHE "gui/icons/Store_IconGem.png",
    [GUI_REWARD_BRUSH] = HOD_CACHE "apk_gui/backgrounds/Bg_Reward_Brush.png",

    [GUI_MAP_BG_0] = HOD_CACHE "gui/map/pve_midground_01.png",
    [GUI_MAP_BG_1] = HOD_CACHE "gui/map/pve_midground_02.png",
    [GUI_MAP_BG_2] = HOD_CACHE "gui/map/pve_midground_03.png",
    [GUI_MAP_BG_3] = HOD_CACHE "gui/map/pve_midground_04.png",
    [GUI_MAP_BG_4] = HOD_CACHE "gui/map/pve_midground_05.png",
    [GUI_MAP_BG_5] = HOD_CACHE "gui/map/pve_midground_06.png",
    [GUI_MAP_BG_6] = HOD_CACHE "gui/map/pve_midground_07.png",
    [GUI_MAP_BG_7] = HOD_CACHE "gui/map/pve_midground_08.png",
    [GUI_MAP_BG_8] = HOD_CACHE "gui/map/pve_midground_09.png",
    [GUI_MAP_BG_9] = HOD_CACHE "gui/map/pve_midground_10.png",
    [GUI_MAP_BG_10] = HOD_CACHE "gui/map/pve_midground_11.png",
    [GUI_MAP_BG_11] = HOD_CACHE "gui/map/pve_midground_12.png",
    [GUI_MAP_BG_12] = HOD_CACHE "gui/map/pve_midground_13.png",
    [GUI_MAP_BG_13] = HOD_CACHE "gui/map/pve_midground_14.png",
    [GUI_MAP_BG_14] = HOD_CACHE "gui/frames_backgrounds/pve_background_15b.png",
    [GUI_MAP_BG_15] = HOD_CACHE "gui/frames_backgrounds/pve_background_16.png",
    [GUI_MAP_SEPARATOR] = HOD_CACHE "gui/map/PVE_endcap.png",
    [GUI_MAP_ENERGY] = HOD_CACHE "gui/map/pve_node_energycost.png",
    [GUI_MAP_LORE] = HOD_CACHE "gui/map/pve_node_lore.png",
    [GUI_MAP_PROGRESS] = HOD_CACHE "gui/map/pve_node_progress_bar.png",
    [GUI_MAP_HELP] = HOD_CACHE "meshes/shared_resourcecontainer/ico_help_info.png",
    [GUI_MAP_GEMS] = HOD_CACHE "apk_gui/hud_bars_currency/tex.gemscurrency.png",
    [GUI_MAP_REWARD_TAG] = HOD_CACHE "gui/frames_backgrounds/BG_RewardsTag.png",
    [GUI_NODE_LOCKED] = HOD_CACHE "meshes/SnexusDP_map1_pre/ico_nodelocked.png",
    [GUI_NODE_LOCKED_ON] = HOD_CACHE "meshes/SnexusDP_map1_pre/ico_nodelocked_selected.png",
    [GUI_NODE_OPEN] = HOD_CACHE "meshes/SnexusDP_map1_pre/ico_nodeincomplete.png",
    [GUI_NODE_OPEN_ON] = HOD_CACHE "meshes/SnexusDP_map1_pre/ico_nodeincomplete_selected.png",
    [GUI_NODE_DONE] = HOD_CACHE "meshes/SnexusDP_map1_pre/ico_nodecomplete.png",
    [GUI_NODE_DONE_ON] = HOD_CACHE "meshes/SnexusDP_map1_pre/ico_nodecomplete_selected.png",
    [GUI_NODE_MASTERED] = HOD_CACHE "meshes/SnexusDP_map1_pre/ico_nodemastered.png",
    [GUI_NODE_MASTERED_ON] = HOD_CACHE "meshes/SnexusDP_map1_pre/ico_nodemastered_selected.png",
    [GUI_NODE_BOSS] = HOD_CACHE "meshes/SnexusDP_map1_pre/ico_nodeboss.png",
    [GUI_NODE_BOSS_ON] = HOD_CACHE "meshes/SnexusDP_map1_pre/ico_nodeboss_selected.png",
    [GUI_NODE_BOSS_LOCKED] = HOD_CACHE "meshes/SnexusDP_map1_pre/ico_nodeboss_locked.png",
    [GUI_NODE_BOSS_LOCKED_ON] = HOD_CACHE "meshes/SnexusDP_map1_pre/ico_nodeboss_locked_selected.png",
    [GUI_NODE_GRIND] = HOD_CACHE "meshes/SnexusDP_map1_pre/ico_nodegrind.png",
    [GUI_NODE_GRIND_ON] = HOD_CACHE "meshes/SnexusDP_map1_pre/ico_nodegrind_selected.png",
    [GUI_NODE_UNLOCK_GLOW] = HOD_CACHE "vfx/textures/fx_pve_nodetransition__GlowDot.png",

    [GUI_NEXUS_POTION] = HOD_CACHE "gui/misc/tex.Ico_Revive.png",
    [GUI_NEXUS_DIVIDER] = HOD_CACHE "gui/icons/IconNexusDividerTone.png",
    [GUI_NEXUS_TIMER_PLATE] = HOD_CACHE "gui/frames_backgrounds/tex.FrameInfoTab.png",
    [GUI_NEXUS_EVENT_FRAME] = HOD_CACHE "gui/frames_backgrounds/NewsfeedAdSpaceFrame.png",
    [GUI_NEXUS_EVENT_TITLEBAR] = HOD_CACHE "gui/frames_backgrounds/FrameQuestTickerTitleTone.png",
    [GUI_NEXUS_KEY_1] = HOD_CACHE "gui/icons/tex.IconKey01.png",
    [GUI_NEXUS_KEY_2] = HOD_CACHE "gui/icons/tex.IconKey02.png",
    [GUI_NEXUS_KEY_3] = HOD_CACHE "gui/icons/tex.IconKey03.png",
    [GUI_NEXUS_KEY_4] = HOD_CACHE "gui/icons/tex.IconKey03.png",

    [GUI_BTN_GOLD_COST] = HOD_CACHE "apk_gui/buttons/Btn_CTA_gold_cost.png",
    [GUI_BTN_GOLD_COST_ON] = HOD_CACHE "apk_gui/buttons/Btn_CTA_gold_cost_active.png",
    [GUI_MM_PANEL] = HOD_CACHE "gui/frames_backgrounds/FrameStaticHeaderLg.png",
    [GUI_MM_TROUGH] = HOD_CACHE "gui/frames_backgrounds/Bg_TipsSliced.png",
    [GUI_MM_RUNE_SLOT] = HOD_CACHE "apk_gui/battle/tex.add_rune_slot.png",
    [GUI_MM_TAG] = HOD_CACHE "gui/frames_backgrounds/BG_RewardsTagShort.png",
    [GUI_MM_XP] = HOD_CACHE "gui/misc/tex.xp_nonumber.png",
    [GUI_MM_SQUAD_FRAME] = HOD_CACHE "gui/misc/squad_slot_container.png",
    [GUI_MM_POTION] = HOD_CACHE "apk_gui/battle/tex.Ico_PotionOrange.png",
    [GUI_MM_PLATE] = HOD_CACHE "gui/misc/MessagingBG.png",
    [GUI_MM_PLATE_BORDER] = HOD_CACHE "gui/misc/NameTagFactionBorderTone.png",
    [GUI_MM_HEALTH] = HOD_CACHE "gui/misc/tex.Ico_Health.png",

    [GUI_MSG_DAMAGE] = HOD_CACHE "apk_gui/battle/tex.damage_numerical.png",
    [GUI_MSG_CRIT] = HOD_CACHE "apk_gui/battle/tex.battle_damage_crit.png",
    [GUI_MSG_CRIT_REWARD] = HOD_CACHE "gui/misc/tex.battle_crit_reward.png",
    [GUI_MSG_HEAL] = HOD_CACHE "apk_gui/battle/tex.buff_health_numerical.png",
    [GUI_MSG_PWR_UP] = HOD_CACHE "apk_gui/battle/tex.buff_power.png",
    [GUI_MSG_PWR_DOWN] = HOD_CACHE "apk_gui/battle/tex.debuff_power.png",
    [GUI_MSG_INIT_UP] = HOD_CACHE "apk_gui/battle/tex.buff_initiative.png",
    [GUI_MSG_INIT_DOWN] = HOD_CACHE "apk_gui/battle/tex.debuff_initiative.png",
    [GUI_MSG_STUN] = HOD_CACHE "apk_gui/battle/tex.debuff_stunned.png",
    [GUI_MSG_CURSE_PWR] = HOD_CACHE "apk_gui/battle/tex.debuff_curse_power.png",
    [GUI_MSG_CURSE_HP] = HOD_CACHE "apk_gui/battle/tex.debuff_curse_health.png",

    [GUI_COMBAT_ROUND] = HOD_CACHE "gui/misc/Combat_RoundCounter.png",
    [GUI_COMBAT_BANNER] = HOD_CACHE "gui/frames_backgrounds/Bg_InitiativeMessaging.png",
    [GUI_COMBAT_PREBATTLE_BG] = HOD_CACHE "apk_gui/backgrounds/Bg_InstructionBackground.png",
    [GUI_COMBAT_GOLD_BG] = HOD_CACHE "gui/frames_backgrounds/Bg_CombatTimer.png",
    [GUI_COMBAT_FLOURISH] = HOD_CACHE "gui/frames_backgrounds/BG_PvPUpperLeftFlouish.png",
    [GUI_TEAMBAR_GEOM] = HOD_CACHE "gui/frames_backgrounds/Bg_TeamHealthBarGeom.png",
    [GUI_TEAMBAR_FILL] = HOD_CACHE "gui/bars_progress/Bar_FillTeamHealth.png",
    [GUI_TEAMBAR_LCAP] = HOD_CACHE "gui/frames_backgrounds/Bg_TeamHealthLeftCap.png",
    [GUI_TEAMBAR_RCAP] = HOD_CACHE "gui/frames_backgrounds/Bg_TeamHealthRightCap.png",
    [GUI_COMBAT_FF] = HOD_CACHE "gui/misc/ico_fastforward.png",
    [GUI_COMBAT_FF_ON] = HOD_CACHE "gui/misc/ico_fastforward_active.png",
    [GUI_SUMMARY_VS] = HOD_CACHE "gui/misc/vs_container.png",
    [GUI_SUMMARY_PLAQUE] = HOD_CACHE "gui/misc/ftue_container.png",
    [GUI_SUMMARY_DAMAGE] = HOD_CACHE "gui/misc/tex.Ico_Damage.png",
    [GUI_SUMMARY_FACTION] = HOD_CACHE "gui/misc/ico_factionhits.png",
    [GUI_SUMMARY_MVP] = HOD_CACHE "gui/misc/tex.ico_mvp.png",
    [GUI_SUMMARY_SURVIVORS] = HOD_CACHE "gui/misc/ico_survivors.png",

    [GUI_RESULTS_OVERLAY] = HOD_CACHE "apk_gui/battle/Bg_Overlay.png",
    [GUI_RESULTS_BG] = HOD_CACHE "gui/frames_backgrounds/Bg_Battle_Results.png",
    [GUI_RESULTS_ROSTER_BG] = HOD_CACHE "apk_gui/backgrounds/Bg_PureBlack.png",
    [GUI_RESULTS_STARBURST] = HOD_CACHE "gui/misc/Victory_starburst.png",
    [GUI_RESULTS_MASTERY_BASE] = HOD_CACHE "gui/frames_backgrounds/Bg_MasteryBase.png",
    [GUI_RESULTS_MASTERY_PROGRESS] = HOD_CACHE "gui/frames_backgrounds/Bg_MasteryInProgressHighlight.png",
    [GUI_RESULTS_MASTERY_HIGHLIGHT] = HOD_CACHE "gui/frames_backgrounds/Bg_MasteryHighlight.png",
    [GUI_RESULTS_STORE] = HOD_CACHE "gui/buttons_tabs/Btn_Store_HalfSize.png",
    [GUI_RESULTS_STORE_ON] = HOD_CACHE "gui/buttons_tabs/Btn_Store_HalfSize_Active.png",
    [GUI_RESULTS_HEROES] = HOD_CACHE "gui/buttons_tabs/Btn_ResultsHeroesDefault2.png",
    [GUI_RESULTS_HEROES_ON] = HOD_CACHE "gui/buttons_tabs/Btn_ResultsHeroesDefault2_active.png",
    [GUI_RESULTS_MAP] = HOD_CACHE "gui/buttons_tabs/Btn_PVEDefault2.png",
    [GUI_RESULTS_MAP_ON] = HOD_CACHE "gui/buttons_tabs/Btn_PVEDefault2_active.png",
    [GUI_SPLASH_INKSMEAR] = HOD_CACHE "vfx/textures/fx_victoryscreen_pre__victory_defeat_inksmear.png",
    [GUI_SPLASH_INKSPLAT] = HOD_CACHE "vfx/textures/fx_victoryscreen_pre__victory_defeat_inksplat.png",
    [GUI_SPLASH_GRUNGE] = HOD_CACHE "vfx/textures/fx_victoryscreen_pre__victory_defeat_texture.png",
    [GUI_SPLASH_GLOW] = HOD_CACHE "vfx/textures/fx_victoryscreen_pre__valor_glow.png",
    [GUI_SPLASH_SMOKE] = HOD_CACHE "meshes/shared_resourcecontainer/wispy10.png",
};

#define FONT_BASE_SIZE 96
static const char *const font_paths[FNT_COUNT] = {
    [FNT_MENU] = HOD_CACHE "gui/fonts/DAmFont.ttf",
    [FNT_COOL] = HOD_CACHE "gui/fonts/inquisition.ttf",
};

typedef struct {
    const char *path;
    bool looping;
} MusicDef;

static const MusicDef music_defs[MUS_COUNT] = {
    [MUS_HOME1] = {HOD_CACHE "vfx/audio/mus_home1.mp3", false},
    [MUS_HOME2] = {HOD_CACHE "vfx/audio/mus_home2.mp3", false},
    [MUS_HOME3] = {HOD_CACHE "vfx/audio/mus_home3.mp3", false},
    [MUS_BATTLE_INTRO] = {HOD_CACHE "vfx/audio/sundermount_trans_in_01.mp3", false},
    [MUS_BATTLE_LOOP] = {HOD_CACHE "vfx/audio/WOUNDED COAST COMBAT LOW.mp3", true},
    [MUS_BATTLE_AMBIENCE] = {HOD_CACHE "meshes/bg_plains_01_pre/amb_rain.mp3", true},
    [MUS_COMBAT_INTRO] = {HOD_CACHE "vfx/audio/dae_cave_trans_OUT_02.mp3", false},
    [MUS_COMBAT_1] = {HOD_CACHE "vfx/audio/dae_caverns_combat_C_05.mp3", true},
    [MUS_COMBAT_2] = {HOD_CACHE "vfx/audio/kirkwall_combat_int_C_04.mp3", true},
    [MUS_COMBAT_3] = {HOD_CACHE "vfx/audio/WOUNDED COAST COMBAT HIGH.mp3", true},
    [MUS_VICTORY] = {HOD_CACHE "vfx/audio/mus_victory.mp3", false},
    [MUS_DEFEAT] = {HOD_CACHE "vfx/audio/mus_defeat.mp3", false},
};

static const char *const sound_paths[SND_COUNT] = {
    [SND_UI_CLICK] = HOD_CACHE "vfx/audio/gui_gen.wav",
    [SND_UI_BIG_BUTTON] = HOD_CACHE "vfx/audio/gui_gen_large.mp3",
    [SND_UI_HOME] = HOD_CACHE "vfx/audio/gui_home.mp3",
    [SND_UI_POPUP_CLOSE] = HOD_CACHE "vfx/audio/gui_close.mp3",
    [SND_UI_TAB] = HOD_CACHE "vfx/audio/gui_tab.mp3",
    [SND_STAMINA_REFRESH] = HOD_CACHE "vfx/audio/gui_stamina_refresh.mp3",
    [SND_PVE_NODE_SELECTED] = HOD_CACHE "vfx/audio/gui_pve_node.mp3",
    [SND_COMBAT_SPEED] = HOD_CACHE "vfx/audio/gui_bonus_speed.mp3",
    [SND_BATTLE_INFO_OPEN] = HOD_CACHE "vfx/audio/gui_battle_info_open.mp3",
    [SND_BATTLE_INFO_CLOSE] = HOD_CACHE "vfx/audio/gui_battle_info_close.mp3",
    [SND_LEVEL_UP] = HOD_CACHE "vfx/audio/mus_complete.mp3",
    [SND_PVE_NODE_TRANSITION] = HOD_CACHE "vfx/audio/gui_pve_node_trans.mp3",
    [SND_MAP_TRANSITION] = HOD_CACHE "vfx/audio/gui_map_transition.mp3",
    [SND_TALLY_START] = HOD_CACHE "vfx/audio/gui_tally_win_start.mp3",
    [SND_TALLY_HIT] = HOD_CACHE "vfx/audio/gui_tally_hit.mp3",
    [SND_TALLY_END] = HOD_CACHE "vfx/audio/gui_tally_win_end.mp3",
    [SND_TALLY_LOSS_START] = HOD_CACHE "vfx/audio/gui_tally_loss_start.mp3",
    [SND_TALLY_LOSS_END] = HOD_CACHE "vfx/audio/gui_tally_loss_end.mp3",
    [SND_STORE_OPEN] = HOD_CACHE "vfx/audio/gui_store_open.mp3",
    [SND_PACK_SHUFFLE] = HOD_CACHE "vfx/audio/gui_pack_open_shuffle.mp3",
    [SND_PACK_REVEAL] = HOD_CACHE "vfx/audio/gui_pack_open_seq.mp3",
    [SND_STORE_INFO] = HOD_CACHE "vfx/audio/gui_info.mp3",
    [SND_STORE_ERROR] = HOD_CACHE "vfx/audio/gui_error.mp3",
    [SND_PACK_COMMON] = HOD_CACHE "vfx/audio/gui_levelup_1.mp3",
    [SND_PACK_UNCOMMON] = HOD_CACHE "vfx/audio/gui_levelup_2.mp3",
    [SND_PACK_RARE] = HOD_CACHE "vfx/audio/gui_purchase_rare.mp3",
    [SND_PACK_EPIC] = HOD_CACHE "vfx/audio/gui_purchase_epic.mp3",
    [SND_PACK_LEGENDARY] = HOD_CACHE "vfx/audio/gui_purchase_legendary.mp3",
};

static Texture2D gui_tex[GUI_COUNT];
static Font gui_fonts[FNT_COUNT];
static Music gui_music[MUS_COUNT];
static Sound gui_sounds[SND_COUNT];

#define GUI_SOUND_VOICE_COUNT 20
typedef struct {
    Sound alias;
    rAudioBuffer *source_buffer;
} GuiSoundVoice;
static GuiSoundVoice gui_sound_voices[GUI_SOUND_VOICE_COUNT];

bool GuiSoundAllowsDuplicate(SoundId id) {
    switch (id) {
        case SND_UI_BIG_BUTTON:
        case SND_UI_POPUP_CLOSE:
        case SND_UI_TAB:
        case SND_STAMINA_REFRESH:
        case SND_PVE_NODE_SELECTED:
        case SND_COMBAT_SPEED:
        case SND_BATTLE_INFO_OPEN:
        case SND_BATTLE_INFO_CLOSE:
        case SND_LEVEL_UP:
        case SND_PVE_NODE_TRANSITION:
        case SND_MAP_TRANSITION:
        case SND_STORE_OPEN:
        case SND_STORE_INFO:
        case SND_STORE_ERROR:
        case SND_PACK_COMMON:
        case SND_PACK_UNCOMMON:
        case SND_PACK_RARE:
        case SND_PACK_EPIC:
        case SND_PACK_LEGENDARY:
            return true;
        default:
            return false;
    }
}

void GuiSoundPlayHandle(Sound source, bool allow_duplicate) {
    if (!IsSoundValid(source)) return;
    int voice = -1;
    for (int i = 0; i < GUI_SOUND_VOICE_COUNT; i++) {
        GuiSoundVoice *slot = &gui_sound_voices[i];
        if (slot->source_buffer != source.stream.buffer || !IsSoundValid(slot->alias)) continue;
        if (IsSoundPlaying(slot->alias)) {
            if (!allow_duplicate) return;
        } else if (voice < 0) {
            voice = i;
        }
    }
    if (voice < 0) {
        for (int i = 0; i < GUI_SOUND_VOICE_COUNT; i++) {
            if (!IsSoundValid(gui_sound_voices[i].alias) || !IsSoundPlaying(gui_sound_voices[i].alias)) {
                voice = i;
                break;
            }
        }
    }
    if (voice < 0) {
        TraceLog(LOG_WARNING, "Assets: no free duplicate UI sound voice");
        return;
    }

    GuiSoundVoice *slot = &gui_sound_voices[voice];
    if (slot->source_buffer != source.stream.buffer) {
        if (IsSoundValid(slot->alias)) UnloadSoundAlias(slot->alias);
        slot->alias = LoadSoundAlias(source);
        slot->source_buffer = IsSoundValid(slot->alias) ? source.stream.buffer : NULL;
    }
    if (IsSoundValid(slot->alias)) PlaySound(slot->alias);
}

void GuiSoundPlay(SoundId id) {
    if (id < 0 || id >= SND_COUNT) return;
    GuiSoundPlayHandle(gui_sounds[id], GuiSoundAllowsDuplicate(id));
}

void GuiSoundStop(SoundId id) {
    if (id < 0 || id >= SND_COUNT || !IsSoundValid(gui_sounds[id])) return;
    for (int i = 0; i < GUI_SOUND_VOICE_COUNT; i++) {
        GuiSoundVoice *slot = &gui_sound_voices[i];
        if (slot->source_buffer != gui_sounds[id].stream.buffer || !IsSoundValid(slot->alias)) continue;
        if (IsSoundPlaying(slot->alias)) StopSound(slot->alias);
    }
}

void GuiClickSoundPlay(void) {
    GuiSoundPlay(SND_UI_CLICK);
}

void GuiClickSoundPlayDuplicate(void) {
    GuiSoundPlayHandle(gui_sounds[SND_UI_CLICK], true);
}

static void GuiSoundVoicesUnload(void) {
    for (int i = 0; i < GUI_SOUND_VOICE_COUNT; i++) {
        if (!IsSoundValid(gui_sound_voices[i].alias)) continue;
        StopSound(gui_sound_voices[i].alias);
        UnloadSoundAlias(gui_sound_voices[i].alias);
    }
    memset(gui_sound_voices, 0, sizeof(gui_sound_voices));
}

static int FillRussianFontCodepoints(int codepoints[256]) {
    int n = 0;
    for (int c = 32; c <= 126; c++) codepoints[n++] = c;
    codepoints[n++] = 0x00A0;
    codepoints[n++] = 0x00AB;
    codepoints[n++] = 0x00BB;
    codepoints[n++] = 0x0401;
    codepoints[n++] = 0x0451;
    for (int c = 0x0410; c <= 0x044F; c++) codepoints[n++] = c;
    codepoints[n++] = 0x2014;
    codepoints[n++] = 0x2212;
    return n;
}

static Font LoadCyrillicFont(const char *path, int size) {
    int codepoints[256];
    int n = FillRussianFontCodepoints(codepoints);
    return LoadFontEx(path, size, codepoints, n);
}

void AssetsLoad(void) {
    for (int i = 0; i < GUI_COUNT; i++) {
        gui_tex[i] = LoadTexture(gui_paths[i]);
        if (gui_tex[i].id == 0) TraceLog(LOG_FATAL, "Assets: failed to load %s", gui_paths[i]);
    }
    for (int i = 0; i < FNT_COUNT; i++) {
        gui_fonts[i] = LoadCyrillicFont(font_paths[i], FONT_BASE_SIZE);
        SetTextureFilter(gui_fonts[i].texture, TEXTURE_FILTER_BILINEAR);
    }
    for (int i = 0; i < MUS_COUNT; i++) {
        gui_music[i] = LoadMusicStream(music_defs[i].path);
        gui_music[i].looping = music_defs[i].looping;
        if (gui_music[i].ctxData == NULL) TraceLog(LOG_FATAL, "Assets: failed to load %s", music_defs[i].path);
    }
    for (int i = 0; i < SND_COUNT; i++) {
        gui_sounds[i] = LoadSound(sound_paths[i]);
        if (gui_sounds[i].frameCount == 0) TraceLog(LOG_FATAL, "Assets: failed to load %s", sound_paths[i]);
    }
}

void AssetsUnload(void) {
    for (int i = 0; i < GUI_COUNT; i++) UnloadTexture(gui_tex[i]);
    for (int i = 0; i < FNT_COUNT; i++) UnloadFont(gui_fonts[i]);
    for (int i = 0; i < MUS_COUNT; i++) UnloadMusicStream(gui_music[i]);
    GuiSoundVoicesUnload();
    for (int i = 0; i < SND_COUNT; i++) UnloadSound(gui_sounds[i]);
}

Texture2D GuiTex(GuiId id) {
    return gui_tex[id];
}
Font GuiFont(FontId id) {
    return gui_fonts[id];
}
Music GuiMusic(MusicId id) {
    return gui_music[id];
}
Sound GuiSound(SoundId id) {
    return gui_sounds[id];
}

Texture2D GuiPortraitLoad(const char *icon) {
    char path[512];
    snprintf(path, sizeof(path), HOD_CACHE "gui/portraits/%s.png", icon);
    if (!FileExists(path)) TraceLog(LOG_FATAL, "Assets: missing portrait %s", path);
    Texture2D t = LoadTexture(path);
    if (t.id == 0) TraceLog(LOG_FATAL, "Assets: failed to load portrait %s", path);
    GenTextureMipmaps(&t);
    SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
    return t;
}
