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
    GUI_STORE_PLATE,
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

#define POST_4_4_PORTRAIT_DIR EXTRA_ASSET_DIR "portraits/"

static const char *const gui_paths[GUI_COUNT] = {

    [GUI_HUD_GRADIENT] = CACHE_RESOURCE("bg_HUDbake"),
    [GUI_HUD_XP] = CACHE_RESOURCE("Ico_XpHUD"),
    [GUI_HUD_XP_FRAME] = CACHE_RESOURCE("playerlevel_bar_empty"),
    [GUI_HUD_XP_BG] = CACHE_RESOURCE("playerlevel_bar_bg"),
    [GUI_HUD_XP_METER] = CACHE_RESOURCE("playerlevel_bar"),
    [GUI_HUD_RESOURCE_FRAME] = CACHE_RESOURCE("tex.EnergyStaminaContainer"),
    [GUI_HUD_ENERGY] = APK_RESOURCE("Ico_EnergyHUD"),
    [GUI_HUD_STAMINA] = APK_RESOURCE("Ico_StaminaHUD"),
    [GUI_HUD_GOLD] = APK_RESOURCE("tex.softcurrency"),
    [GUI_HUD_GOLD_SELECTED] = CACHE_RESOURCE("tex.softcurrency_active"),
    [GUI_HUD_GEMS] = CACHE_RESOURCE("ico_gemHUD"),
    [GUI_HUD_GEMS_SELECTED] = CACHE_RESOURCE("ico_gemHUD_selected"),
    [GUI_HUD_OPTIONS] = CACHE_RESOURCE("ico_settings"),

    [GUI_BTN_CAMP] = CACHE_RESOURCE("Btn_homesquare"),
    [GUI_BTN_CAMP_ON] = CACHE_RESOURCE("Btn_homesquare_active"),
    [GUI_BTN_CAMP_INACTIVE] = CACHE_RESOURCE("Btn_homesquare_inactive"),
    [GUI_BTN_STORE] = CACHE_RESOURCE("Button_HUD_Full_Store"),
    [GUI_BTN_STORE_ON] = CACHE_RESOURCE("Button_HUD_Full_Store_active"),
    [GUI_BTN_HISTORY] = CACHE_RESOURCE("Button_HUD_Full_History"),
    [GUI_BTN_HISTORY_ON] = CACHE_RESOURCE("Button_HUD_Full_History_active"),
    [GUI_BTN_HEROES] = CACHE_RESOURCE("Button_HUD_Full_Heroes"),
    [GUI_BTN_HEROES_ON] = CACHE_RESOURCE("Button_HUD_Full_Heroes_active"),
    [GUI_BTN_HOMEQUEST] = CACHE_RESOURCE("Button_HUD_Full_PVE"),
    [GUI_BTN_HOMEQUEST_ON] = CACHE_RESOURCE("Button_HUD_Full_PVE_active"),
    [GUI_BTN_PVP] = CACHE_RESOURCE("Button_HUD_Full_PVP"),
    [GUI_BTN_PVP_ON] = CACHE_RESOURCE("Button_HUD_Full_PVP_active"),
    [GUI_BTN_GREY] = APK_RESOURCE("Btn_Default_grey"),
    [GUI_BTN_GREY_ON] = APK_RESOURCE("Btn_Default_grey_active"),
    [GUI_BTN_GOLD] = APK_RESOURCE("Btn_CTA_gold"),
    [GUI_BTN_GOLD_ON] = APK_RESOURCE("Btn_CTA_gold_active"),
    [GUI_BTN_TRAIN] = CACHE_RESOURCE("Btn_Heroes_Train"),
    [GUI_BTN_TRAIN_ON] = CACHE_RESOURCE("Btn_Heroes_TrainActive"),
    [GUI_BTN_FORTIFY] = CACHE_RESOURCE("Btn_Heroes_FortifyDefault"),
    [GUI_BTN_FORTIFY_ON] = CACHE_RESOURCE("Btn_heroes_FortifyActive"),
    [GUI_BTN_RUNES] = CACHE_RESOURCE("Btn_Heroes_RunesDefault"),
    [GUI_BTN_RUNES_ON] = CACHE_RESOURCE("Btn_Heroes_RunesActive"),
    [GUI_BTN_GALLERY] = CACHE_RESOURCE("Btn_Heroes_GalleryDefault"),
    [GUI_BTN_GALLERY_ON] = CACHE_RESOURCE("Btn_Heroes_GalleryActive"),
    [GUI_BTN_SQUAD] = CACHE_RESOURCE("Btn_Heroes_Squad"),
    [GUI_BTN_SQUAD_ON] = CACHE_RESOURCE("Btn_Heroes_SquadActive"),
    [GUI_BTN_MAINQUEST] = CACHE_RESOURCE("ButtonMainQuest"),
    [GUI_BTN_MAINQUEST_ON] = CACHE_RESOURCE("ButtonMainQuestActive"),
    [GUI_BTN_PACK_TRAIN] = CACHE_RESOURCE("Btn_TrainSmall"),
    [GUI_BTN_PACK_TRAIN_ON] = CACHE_RESOURCE("Btn_TrainSmall_active"),
    [GUI_BTN_PACK_RUNES] = CACHE_RESOURCE("Btn_RuneSmall"),
    [GUI_BTN_PACK_RUNES_ON] = CACHE_RESOURCE("Btn_RuneSmall_active"),
    [GUI_BTN_PACK_SQUAD] = CACHE_RESOURCE("Btn_SquadSmall"),
    [GUI_BTN_PACK_SQUAD_ON] = CACHE_RESOURCE("Btn_SquadSmall_active"),
    [GUI_BTN_CLOSE] = APK_RESOURCE("Store_CloseButton"),
    [GUI_BTN_MAPS] = CACHE_RESOURCE("Button_HUD_Full_Maps_active"),

    [GUI_STORE_HEADER_RECRUIT] = CACHE_RESOURCE("tex.Store_HeaderBGBronzePack"),
    [GUI_STORE_HEADER_SOLDIER] = CACHE_RESOURCE("tex.Store_HeaderBGSilverPack"),
    [GUI_STORE_HEADER_CHAMPION] = CACHE_RESOURCE("tex.Store_HeaderChampionPack"),
    [GUI_STORE_HEADER_GRAND] = CACHE_RESOURCE("tex.Store_HeaderGrandChampionPack"),
    [GUI_STORE_ICON_RECRUIT] = CACHE_RESOURCE("tex.Store_IconBronzePack"),
    [GUI_STORE_ICON_SOLDIER] = CACHE_RESOURCE("tex.Store_IconSilverPack"),
    [GUI_STORE_ICON_CHAMPION] = CACHE_RESOURCE("tex.Store_IconGoldPack"),
    [GUI_STORE_ICON_GRAND] = CACHE_RESOURCE("tex.Store_IconLegendaryPack"),
    [GUI_STORE_CARD] = CACHE_RESOURCE("ItemCardBase"),
    [GUI_STORE_BUY] = CACHE_RESOURCE("ButtonBuy"),
    [GUI_STORE_BUY_ON] = CACHE_RESOURCE("ButtonBuyActive"),
    [GUI_STORE_POPUP] = CACHE_RESOURCE("Bg_Popup_Stone"),
    [GUI_STORE_PLATE] = CACHE_RESOURCE("Bg_Overlay"),
    [GUI_STORE_OVERLAY] = APK_RESOURCE("Bg_Overlay_Short"),
    [GUI_STORE_BACK] = APK_RESOURCE("ButtonBackDefault"),
    [GUI_STORE_CATEGORY_FRAME] = CACHE_RESOURCE("FrameStoreCategory"),
    [GUI_STORE_CATEGORY_UNITS] = CACHE_RESOURCE("tex.StoreCategoryUNITS"),
    [GUI_STORE_CATEGORY_GEMS] = CACHE_RESOURCE("tex.StoreCategoryGEMS"),
    [GUI_STORE_CATEGORY_RUNES] = CACHE_RESOURCE("tex.StoreCategoryRUNES"),
    [GUI_STORE_SCROLL_ARROW] = CACHE_RESOURCE("shared_arrow"),
    [GUI_STORE_CURRENCY_GEMS] = CACHE_RESOURCE("tex.gemscurrency"),
    [GUI_STORE_GEM_CARD] = CACHE_RESOURCE("FrameItemGemCard"),
    [GUI_STORE_GEM_COIN] = CACHE_RESOURCE("tex.Coin_Gems"),
    [GUI_STORE_GEM_QUEST] = CACHE_RESOURCE("tex.IconGemsQuest"),
    [GUI_STORE_GEM_INFO_FRAME] = CACHE_RESOURCE("FrameInformation"),
    [GUI_STORE_BANNER] = CACHE_RESOURCE("BannerShopMessaging"),
    [GUI_STORE_GEM_ICON_0] = CACHE_RESOURCE("tex.IconGems0"),
    [GUI_STORE_GEM_ICON_1] = CACHE_RESOURCE("tex.IconGems1"),
    [GUI_STORE_GEM_ICON_2] = CACHE_RESOURCE("tex.IconGems2"),
    [GUI_STORE_GEM_ICON_3] = CACHE_RESOURCE("tex.IconGems3"),
    [GUI_STORE_GEM_ICON_4] = CACHE_RESOURCE("tex.IconGems4"),
    [GUI_STORE_GEM_ICON_5] = CACHE_RESOURCE("tex.IconGems5"),
    [GUI_STORE_GEM_ICON_6] = CACHE_RESOURCE("tex.IconGems6"),
    [GUI_STORE_INFO_ICON] = CACHE_RESOURCE("ico_help_info"),
    [GUI_STORE_INFO_ICON_ON] = CACHE_RESOURCE("ico_help_info_select"),

    [GUI_HEROES_PANEL] = CACHE_RESOURCE("bg_unittray_squads"),
    [GUI_HEROES_ROW_HEALTH] = CACHE_RESOURCE("tex.rowbonus_front_health"),
    [GUI_HEROES_ROW_CRIT] = CACHE_RESOURCE("tex.rowbonus_back_critchance"),

    [GUI_CARD_SELECT] = CACHE_RESOURCE("Unit_State_Select_Orange"),
    [GUI_CARD_LEVEL] = CACHE_BUNDLE_RESOURCE("GloablLeaderboardBasedPVPEventSmart", "Ico_XP_wStar"),

    [GUI_TAB_SQUAD] = CACHE_RESOURCE("toggle_squad"),
    [GUI_TAB_SQUAD_ON] = CACHE_RESOURCE("toggle_squad_select"),
    [GUI_TAB_DEFENSE] = CACHE_RESOURCE("multisquad_defensive"),
    [GUI_TAB_DEFENSE_ON] = CACHE_RESOURCE("multisquad_defensive_select"),
    [GUI_TAB_TROPHY] = CACHE_RESOURCE("tex.trophy"),
    [GUI_TAB_TROPHY_ON] = CACHE_RESOURCE("tex.trophy_active"),
    [GUI_CARD_BAR_BG] = CACHE_RESOURCE("Bg_Exp_Background"),
    [GUI_CARD_BAR_FILL] = CACHE_RESOURCE("MeterFillBlue"),

    [GUI_UNIT_DETAILS] = CACHE_RESOURCE("Bg_Unit_Details"),
    [GUI_PLAQUE_COMMON] = CACHE_RESOURCE("Bg_FrameCommon"),
    [GUI_PLAQUE_UNCOMMON] = CACHE_RESOURCE("Bg_FrameUncommon"),
    [GUI_PLAQUE_RARE] = CACHE_BUNDLE_RESOURCE("shared_resourcecontainer", "Bg_FrameRare"),
    [GUI_PLAQUE_EPIC] = CACHE_RESOURCE("Bg_FrameEpic"),
    [GUI_PLAQUE_LEGENDARY] = CACHE_RESOURCE("Bg_FrameLegendary"),
    [GUI_DETAILS_POWER] = CACHE_RESOURCE("tex.Ico_Power"),
    [GUI_DETAILS_HEALTH] = CACHE_RESOURCE("Ico_Health"),
    [GUI_DETAILS_CRIT] = CACHE_RESOURCE("tex.Ico_CritChance"),
    [GUI_DETAILS_INIT_QUICK] = CACHE_RESOURCE("tex.battle_initiative_quick"),
    [GUI_DETAILS_INIT_NORMAL] = CACHE_RESOURCE("tex.battle_initiative_normal"),
    [GUI_DETAILS_INIT_SLOW] = CACHE_RESOURCE("tex.battle_initiative_slow"),
    [GUI_DETAILS_PIPS] = CACHE_RESOURCE("tex.Ico_Promote2"),
    [GUI_DETAILS_BAR] = CACHE_RESOURCE("Bg_Exp_Bar"),

    [GUI_COIN_LARGE] = CACHE_RESOURCE("tex.faction_lrgcreature_icon"),
    [GUI_COIN_WHITE] = CACHE_RESOURCE("tex.faction_white_icon"),
    [GUI_COIN_RED] = CACHE_RESOURCE("tex.faction_red_icon"),
    [GUI_COIN_BLUE] = CACHE_RESOURCE("tex.faction_blue_icon"),
    [GUI_COIN_BLACK] = CACHE_RESOURCE("tex.faction_black_icon"),
    [GUI_COIN_WHITE_RED] = CACHE_RESOURCE("tex.faction_redwhite_icon"),
    [GUI_COIN_WHITE_BLUE] = CACHE_RESOURCE("tex.faction_bluewhite_icon"),
    [GUI_COIN_RED_BLUE] = CACHE_RESOURCE("tex.faction_redblue_icon"),
    [GUI_COIN_WHITE_BLACK] = CACHE_RESOURCE("tex.faction_blackwhite_icon"),
    [GUI_COIN_RED_BLACK] = CACHE_RESOURCE("tex.faction_blackred_icon"),
    [GUI_COIN_BLUE_BLACK] = CACHE_RESOURCE("tex.faction_blackblue_icon"),

    [GUI_QUESTS_BG] = CACHE_RESOURCE("Bg_Overlay"),
    [GUI_DIALOG_GEM] = CACHE_RESOURCE("Store_IconGem"),
    [GUI_REWARD_BRUSH] = CACHE_RESOURCE("Bg_Reward_Brush"),

    [GUI_MAP_BG_0] = CACHE_RESOURCE("pve_midground_01"),
    [GUI_MAP_BG_1] = CACHE_RESOURCE("pve_midground_02"),
    [GUI_MAP_BG_2] = CACHE_RESOURCE("pve_midground_03"),
    [GUI_MAP_BG_3] = CACHE_RESOURCE("pve_midground_04"),
    [GUI_MAP_BG_4] = CACHE_RESOURCE("pve_midground_05"),
    [GUI_MAP_BG_5] = CACHE_RESOURCE("pve_midground_06"),
    [GUI_MAP_BG_6] = CACHE_RESOURCE("pve_midground_07"),
    [GUI_MAP_BG_7] = CACHE_RESOURCE("pve_midground_08"),
    [GUI_MAP_BG_8] = CACHE_RESOURCE("pve_midground_09"),
    [GUI_MAP_BG_9] = CACHE_RESOURCE("pve_midground_10"),
    [GUI_MAP_BG_10] = CACHE_RESOURCE("pve_midground_11"),
    [GUI_MAP_BG_11] = CACHE_RESOURCE("pve_midground_12"),
    [GUI_MAP_BG_12] = CACHE_RESOURCE("pve_midground_13"),
    [GUI_MAP_BG_13] = CACHE_RESOURCE("pve_midground_14"),
    [GUI_MAP_BG_14] = CACHE_RESOURCE("pve_background_15b"),
    [GUI_MAP_BG_15] = CACHE_RESOURCE("pve_background_16"),
    [GUI_MAP_SEPARATOR] = CACHE_RESOURCE("PVE_endcap"),
    [GUI_MAP_ENERGY] = CACHE_RESOURCE("pve_node_energycost"),
    [GUI_MAP_LORE] = CACHE_RESOURCE("pve_node_lore"),
    [GUI_MAP_PROGRESS] = CACHE_RESOURCE("pve_node_progress_bar"),
    [GUI_MAP_HELP] = CACHE_RESOURCE("ico_help_info"),
    [GUI_MAP_GEMS] = CACHE_RESOURCE("tex.gemscurrency"),
    [GUI_MAP_REWARD_TAG] = CACHE_RESOURCE("BG_RewardsTag"),
    [GUI_NODE_LOCKED] = CACHE_BUNDLE_RESOURCE("SnexusDP_map1_pre", "ico_nodelocked"),
    [GUI_NODE_LOCKED_ON] = CACHE_BUNDLE_RESOURCE("SnexusDP_map1_pre", "ico_nodelocked_selected"),
    [GUI_NODE_OPEN] = CACHE_BUNDLE_RESOURCE("SnexusDP_map1_pre", "ico_nodeincomplete"),
    [GUI_NODE_OPEN_ON] = CACHE_BUNDLE_RESOURCE("SnexusDP_map1_pre", "ico_nodeincomplete_selected"),
    [GUI_NODE_DONE] = CACHE_RESOURCE("ico_nodecomplete"),
    [GUI_NODE_DONE_ON] = CACHE_RESOURCE("ico_nodecomplete_selected"),
    [GUI_NODE_MASTERED] = CACHE_BUNDLE_RESOURCE("SnexusDP_map1_pre", "ico_nodemastered"),
    [GUI_NODE_MASTERED_ON] = CACHE_BUNDLE_RESOURCE("SnexusDP_map1_pre", "ico_nodemastered_selected"),
    [GUI_NODE_BOSS] = CACHE_BUNDLE_RESOURCE("SnexusDP_map1_pre", "ico_nodeboss"),
    [GUI_NODE_BOSS_ON] = CACHE_BUNDLE_RESOURCE("SnexusDP_map1_pre", "ico_nodeboss_selected"),
    [GUI_NODE_BOSS_LOCKED] = CACHE_RESOURCE("ico_nodeboss_locked"),
    [GUI_NODE_BOSS_LOCKED_ON] = CACHE_RESOURCE("ico_nodeboss_locked_selected"),
    [GUI_NODE_GRIND] = CACHE_RESOURCE("ico_nodegrind"),
    [GUI_NODE_GRIND_ON] = CACHE_RESOURCE("ico_nodegrind_selected"),
    [GUI_NODE_UNLOCK_GLOW] = CACHE_RESOURCE("GlowDot"),

    [GUI_NEXUS_POTION] = CACHE_RESOURCE("tex.Ico_Revive"),
    [GUI_NEXUS_DIVIDER] = CACHE_RESOURCE("IconNexusDividerTone"),
    [GUI_NEXUS_TIMER_PLATE] = CACHE_RESOURCE("tex.FrameInfoTab"),
    [GUI_NEXUS_EVENT_FRAME] = CACHE_RESOURCE("NewsfeedAdSpaceFrame"),
    [GUI_NEXUS_EVENT_TITLEBAR] = CACHE_RESOURCE("FrameQuestTickerTitleTone"),
    [GUI_NEXUS_KEY_1] = CACHE_RESOURCE("tex.IconKey01"),
    [GUI_NEXUS_KEY_2] = CACHE_RESOURCE("tex.IconKey02"),
    [GUI_NEXUS_KEY_3] = CACHE_RESOURCE("tex.IconKey03"),
    [GUI_NEXUS_KEY_4] = CACHE_RESOURCE("tex.IconKey03"),

    [GUI_BTN_GOLD_COST] = APK_RESOURCE("Btn_CTA_gold_cost"),
    [GUI_BTN_GOLD_COST_ON] = APK_RESOURCE("Btn_CTA_gold_cost_active"),
    [GUI_MM_PANEL] = CACHE_RESOURCE("FrameStaticHeaderLg"),
    [GUI_MM_TROUGH] = CACHE_RESOURCE("Bg_TipsSliced"),
    [GUI_MM_RUNE_SLOT] = CACHE_RESOURCE("tex.add_rune_slot"),
    [GUI_MM_TAG] = CACHE_RESOURCE("BG_RewardsTagShort"),
    [GUI_MM_XP] = CACHE_RESOURCE("tex.xp_nonumber"),
    [GUI_MM_SQUAD_FRAME] = CACHE_RESOURCE("squad_slot_container"),
    [GUI_MM_POTION] = CACHE_RESOURCE("tex.Ico_PotionOrange"),
    [GUI_MM_PLATE] = CACHE_RESOURCE("MessagingBG"),
    [GUI_MM_PLATE_BORDER] = CACHE_RESOURCE("NameTagFactionBorderTone"),
    [GUI_MM_HEALTH] = CACHE_RESOURCE("tex.Ico_Health"),

    [GUI_MSG_DAMAGE] = CACHE_RESOURCE("tex.damage_numerical"),
    [GUI_MSG_CRIT] = CACHE_RESOURCE("tex.battle_damage_crit"),
    [GUI_MSG_CRIT_REWARD] = CACHE_RESOURCE("tex.battle_crit_reward"),
    [GUI_MSG_HEAL] = CACHE_RESOURCE("tex.buff_health_numerical"),
    [GUI_MSG_PWR_UP] = CACHE_RESOURCE("tex.buff_power"),
    [GUI_MSG_PWR_DOWN] = CACHE_RESOURCE("tex.debuff_power"),
    [GUI_MSG_INIT_UP] = CACHE_RESOURCE("tex.buff_initiative"),
    [GUI_MSG_INIT_DOWN] = CACHE_RESOURCE("tex.debuff_initiative"),
    [GUI_MSG_STUN] = CACHE_RESOURCE("tex.debuff_stunned"),
    [GUI_MSG_CURSE_PWR] = EXTRA_ASSET_DIR "gui/tex.debuff_curse_power.png",
    [GUI_MSG_CURSE_HP] = EXTRA_ASSET_DIR "gui/tex.debuff_curse_health.png",

    [GUI_COMBAT_ROUND] = CACHE_RESOURCE("Combat_RoundCounter"),
    [GUI_COMBAT_BANNER] = CACHE_RESOURCE("Bg_InitiativeMessaging"),
    [GUI_COMBAT_PREBATTLE_BG] = APK_RESOURCE("Bg_InstructionBackground"),
    [GUI_COMBAT_GOLD_BG] = CACHE_RESOURCE("Bg_CombatTimer"),
    [GUI_COMBAT_FLOURISH] = CACHE_RESOURCE("BG_PvPUpperLeftFlouish"),
    [GUI_TEAMBAR_GEOM] = CACHE_RESOURCE("Bg_TeamHealthBarGeom"),
    [GUI_TEAMBAR_FILL] = CACHE_RESOURCE("Bar_FillTeamHealth"),
    [GUI_TEAMBAR_LCAP] = CACHE_RESOURCE("Bg_TeamHealthLeftCap"),
    [GUI_TEAMBAR_RCAP] = CACHE_RESOURCE("Bg_TeamHealthRightCap"),
    [GUI_COMBAT_FF] = CACHE_RESOURCE("ico_fastforward"),
    [GUI_COMBAT_FF_ON] = CACHE_RESOURCE("ico_fastforward_active"),
    [GUI_SUMMARY_VS] = CACHE_RESOURCE("vs_container"),
    [GUI_SUMMARY_PLAQUE] = CACHE_RESOURCE("ftue_container"),
    [GUI_SUMMARY_DAMAGE] = CACHE_RESOURCE("tex.Ico_Damage"),
    [GUI_SUMMARY_FACTION] = CACHE_RESOURCE("ico_factionhits"),
    [GUI_SUMMARY_MVP] = CACHE_RESOURCE("tex.ico_mvp"),
    [GUI_SUMMARY_SURVIVORS] = CACHE_RESOURCE("ico_survivors"),

    [GUI_RESULTS_OVERLAY] = CACHE_RESOURCE("Bg_Overlay"),
    [GUI_RESULTS_BG] = CACHE_RESOURCE("Bg_Battle_Results"),
    [GUI_RESULTS_ROSTER_BG] = APK_RESOURCE("Bg_PureBlack"),
    [GUI_RESULTS_STARBURST] = CACHE_BUNDLE_RESOURCE("BattleResultsRewardSmart", "Victory_starburst"),
    [GUI_RESULTS_MASTERY_BASE] = CACHE_RESOURCE("Bg_MasteryBase"),
    [GUI_RESULTS_MASTERY_PROGRESS] = CACHE_RESOURCE("Bg_MasteryInProgressHighlight"),
    [GUI_RESULTS_MASTERY_HIGHLIGHT] = CACHE_RESOURCE("Bg_MasteryHighlight"),
    [GUI_RESULTS_STORE] = CACHE_RESOURCE("Btn_Store_HalfSize"),
    [GUI_RESULTS_STORE_ON] = CACHE_RESOURCE("Btn_Store_HalfSize_Active"),
    [GUI_RESULTS_HEROES] = CACHE_RESOURCE("Btn_ResultsHeroesDefault2"),
    [GUI_RESULTS_HEROES_ON] = CACHE_RESOURCE("Btn_ResultsHeroesDefault2_active"),
    [GUI_RESULTS_MAP] = CACHE_RESOURCE("Btn_PVEDefault2"),
    [GUI_RESULTS_MAP_ON] = CACHE_RESOURCE("Btn_PVEDefault2_active"),
    [GUI_SPLASH_INKSMEAR] = CACHE_RESOURCE("victory_defeat_inksmear"),
    [GUI_SPLASH_INKSPLAT] = CACHE_RESOURCE("victory_defeat_inksplat"),
    [GUI_SPLASH_GRUNGE] = CACHE_RESOURCE("victory_defeat_texture"),
    [GUI_SPLASH_GLOW] = CACHE_RESOURCE("valor_glow"),
    [GUI_SPLASH_SMOKE] = CACHE_RESOURCE("wispy10"),
};

#define FONT_BASE_SIZE 96
static const char *const font_paths[FNT_COUNT] = {
    [FNT_MENU] = APK_RESOURCE("DAmFont"),
    [FNT_COOL] = APK_RESOURCE("inquisition"),
};

typedef struct {
    const char *path;
    bool looping;
} MusicDef;

static const MusicDef music_defs[MUS_COUNT] = {
    [MUS_HOME1] = {CACHE_RESOURCE("mus_home1"), false},
    [MUS_HOME2] = {CACHE_RESOURCE("mus_home2"), false},
    [MUS_HOME3] = {CACHE_RESOURCE("mus_home3"), false},
    [MUS_BATTLE_INTRO] = {CACHE_RESOURCE("sundermount_trans_in_01"), false},
    [MUS_BATTLE_LOOP] = {CACHE_RESOURCE("WOUNDED COAST COMBAT LOW"), true},
    [MUS_BATTLE_AMBIENCE] = {CACHE_RESOURCE("amb_rain"), true},
    [MUS_COMBAT_INTRO] = {CACHE_RESOURCE("dae_cave_trans_OUT_02"), false},
    [MUS_COMBAT_1] = {CACHE_RESOURCE("dae_caverns_combat_C_05"), true},
    [MUS_COMBAT_2] = {CACHE_RESOURCE("kirkwall_combat_int_C_04"), true},
    [MUS_COMBAT_3] = {CACHE_RESOURCE("WOUNDED COAST COMBAT HIGH"), true},
    [MUS_VICTORY] = {CACHE_RESOURCE("mus_victory"), false},
    [MUS_DEFEAT] = {CACHE_RESOURCE("mus_defeat"), false},
};

static const char *const sound_paths[SND_COUNT] = {
    [SND_UI_CLICK] = EXTRA_ASSET_DIR "audio/gui_gen.wav",
    [SND_UI_BIG_BUTTON] = CACHE_RESOURCE("gui_gen_large"),
    [SND_UI_HOME] = CACHE_RESOURCE("gui_home"),
    [SND_UI_POPUP_CLOSE] = CACHE_RESOURCE("gui_close"),
    [SND_UI_TAB] = CACHE_RESOURCE("gui_tab"),
    [SND_STAMINA_REFRESH] = CACHE_RESOURCE("gui_stamina_refresh"),
    [SND_PVE_NODE_SELECTED] = CACHE_RESOURCE("gui_pve_node"),
    [SND_COMBAT_SPEED] = CACHE_RESOURCE("gui_bonus_speed"),
    [SND_BATTLE_INFO_OPEN] = CACHE_RESOURCE("gui_battle_info_open"),
    [SND_BATTLE_INFO_CLOSE] = CACHE_RESOURCE("gui_battle_info_close"),
    [SND_LEVEL_UP] = CACHE_RESOURCE("mus_complete"),
    [SND_PVE_NODE_TRANSITION] = CACHE_RESOURCE("gui_pve_node_trans"),
    [SND_MAP_TRANSITION] = CACHE_RESOURCE("gui_map_transition"),
    [SND_TALLY_START] = CACHE_RESOURCE("gui_tally_win_start"),
    [SND_TALLY_HIT] = CACHE_RESOURCE("gui_tally_hit"),
    [SND_TALLY_END] = CACHE_RESOURCE("gui_tally_win_end"),
    [SND_TALLY_LOSS_START] = CACHE_RESOURCE("gui_tally_loss_start"),
    [SND_TALLY_LOSS_END] = CACHE_RESOURCE("gui_tally_loss_end"),
    [SND_STORE_OPEN] = CACHE_RESOURCE("gui_store_open"),
    [SND_PACK_SHUFFLE] = CACHE_RESOURCE("gui_pack_open_shuffle"),
    [SND_PACK_REVEAL] = CACHE_RESOURCE("gui_pack_open_seq"),
    [SND_STORE_INFO] = CACHE_RESOURCE("gui_info"),
    [SND_STORE_ERROR] = CACHE_RESOURCE("gui_error"),
    [SND_PACK_COMMON] = CACHE_RESOURCE("gui_levelup_1"),
    [SND_PACK_UNCOMMON] = CACHE_RESOURCE("gui_levelup_2"),
    [SND_PACK_RARE] = CACHE_RESOURCE("gui_purchase_rare"),
    [SND_PACK_EPIC] = CACHE_RESOURCE("gui_purchase_epic"),
    [SND_PACK_LEGENDARY] = CACHE_RESOURCE("gui_purchase_legendary"),
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
    return ResourceLoadFont(path, size, codepoints, n);
}

void AssetsLoad(void) {
    for (int i = 0; i < GUI_COUNT; i++) {
        gui_tex[i] = ResourceLoadTexture(gui_paths[i]);
        if (gui_tex[i].id == 0) TraceLog(LOG_FATAL, "Assets: failed to load %s", gui_paths[i]);
    }
    for (int i = 0; i < FNT_COUNT; i++) {
        gui_fonts[i] = LoadCyrillicFont(font_paths[i], FONT_BASE_SIZE);
        SetTextureFilter(gui_fonts[i].texture, TEXTURE_FILTER_BILINEAR);
    }
    for (int i = 0; i < MUS_COUNT; i++) {
        gui_music[i] = ResourceLoadMusic(music_defs[i].path);
        gui_music[i].looping = music_defs[i].looping;
        if (gui_music[i].ctxData == NULL) TraceLog(LOG_FATAL, "Assets: failed to load %s", music_defs[i].path);
    }
    for (int i = 0; i < SND_COUNT; i++) {
        gui_sounds[i] = ResourceLoadSound(sound_paths[i]);
        if (gui_sounds[i].frameCount == 0) TraceLog(LOG_FATAL, "Assets: failed to load %s", sound_paths[i]);
    }
}

void AssetsUnload(void) {
    for (int i = 0; i < GUI_COUNT; i++) UnloadTexture(gui_tex[i]);
    for (int i = 0; i < FNT_COUNT; i++) UnloadFont(gui_fonts[i]);
    for (int i = 0; i < MUS_COUNT; i++) ResourceUnloadMusic(gui_music[i]);
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

const char *GuiPortraitRef(const char *icon) {
    const char *original = TextFormat(CACHE_RESOURCE("%s"), icon);
    return ResourceExists(original) ? original : TextFormat(POST_4_4_PORTRAIT_DIR "%s.png", icon);
}

Texture2D GuiPortraitLoad(const char *icon) {
    const char *ref = GuiPortraitRef(icon);
    if (!ResourceExists(ref)) TraceLog(LOG_FATAL, "Assets: missing portrait %s", icon);
    Texture2D t = ResourceLoadTexture(ref);
    if (t.id == 0) TraceLog(LOG_FATAL, "Assets: failed to load portrait %s", ref);
    GenTextureMipmaps(&t);
    SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
    return t;
}
