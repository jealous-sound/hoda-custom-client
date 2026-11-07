
#define MM_SX (1280.0f / 1003.0f)
#define MM_SY (648.0f / 564.0f)
#define MM_X(v) ((v) * MM_SX)
#define MM_Y(v) (72.0f + (v) * MM_SY)

static const GuiId BATTLE_UI_FACTION_INTRO_ICON = GUI_DETAILS_POWER;

typedef enum { BATTLE_UI_NONE, BATTLE_UI_FIGHT, BATTLE_UI_RETREAT, BATTLE_UI_RUNES, BATTLE_UI_NEXT } BattleUiAction;

typedef struct {
    const Character *ch;
    bool enemy;
    char name[64];
    int level, power, health;
    bool has_border;
    Color left, right;
    Vector3 world;
} BattlePlate;

typedef struct {
    Button info;
    Button retreat;
    Button fight;
    Button fight_zero;
    Button next;
    char next_label[48];

    bool pvp;
    bool pvp_can_next;
    int pvp_nexts;
    int pvp_gain;
    GuiId pvp_stake_icon;
    char pvp_player_name[64];

    char title[128];
    char mastery[64];
    char diff_value[64];
    Color diff_color;
    int energy_cost;
    int gold, xp, gems;
    const CampaignNode *node;
    CampaignNode nexus_node;
    bool new_map_on_win;

    bool tactical;
    BattlePlate plates[2][SQUAD_SLOT_COUNT];
    const BattlePlate *plate_armed;
    bool fast_forward_armed;
    bool detail_close_armed;
    HeroCard detail_card;
    bool detail_open;
} BattleUi;

static void BattleUiParseColored(const char *src, char *out, int cap, Color *col) {
    const char *open = strstr(src, ">");
    const char *close = strstr(src, "</color>");
    if (strncmp(src, "<color=", 7) != 0 || !open || !close || open > close) {
        StrCopy(out, (size_t)cap, src);
        return;
    }
    snprintf(out, (size_t)cap, "%.*s", (int)(close - open - 1), open + 1);
    char name[16] = {0};
    sscanf(src + 7, "%*[\"]%15[^\"]", name);
    for (char *c = name; *c; c++) *c = (char)tolower(*c);
    *col = HeroesRichColor(name, *col);
}

void BattleUiInitialize(BattleUi *ui) {
    memset(ui, 0, sizeof(*ui));
    Font font = GuiFont(FNT_COOL);
    ui->info = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("MORE_INFO"), font, (Vector2){0.5f, 0.5f}, 0.35f);
    ui->info.click = (Sound){0};
    ui->retreat = ButtonLoadText(
        GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("BUTTON_MATCHMAKING_RETURN"), font, (Vector2){0.5f, 0.5f}, 0.3f
    );
    const char *fight = LocGet("BUTTON_MATCHMAKING_FIGHT");
    ui->fight = ButtonLoadText(GUI_BTN_GOLD_COST, GUI_BTN_GOLD_COST_ON, fight, font, (Vector2){0.35f, 0.5f}, 0.3f);
    ui->fight_zero = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, fight, font, (Vector2){0.5f, 0.5f}, 0.3f);
    ui->next = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, ui->next_label, font, (Vector2){0.5f, 0.5f}, 0.3f);
}

static float BattleUiDifficultyRatio(const SimUnit *units) {
    double power[2] = {0}, health[2] = {0};
    for (int side = 0; side < 2; side++) {
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
            const SimUnit *unit = &units[side * SQUAD_SLOT_COUNT + slot];
            if (!unit->present) continue;
            power[side] += unit->start_power;
            health[side] += unit->max_health;
        }
    }
    if (power[0] <= 0.0 || health[0] <= 0.0) return 1.0f;
    if (power[1] <= 0.0 || health[1] <= 0.0) return 0.0f;
    return sqrtf((float)((power[1] / power[0]) * (health[1] / health[0])));
}

static const char *BattleUiDifficultyKey(float ratio) {
    return ratio < 0.85f ? "PVE_COLORED_EASY" : ratio <= 1.25f ? "PVE_COLORED_MEDIUM" : "PVE_COLORED_HARD";
}

static bool BattleUiFactionColors(Faction f, Color *left, Color *right) {
    const Color white = {255, 255, 255, 255}, red = {192, 2, 1, 255}, blue = {0, 126, 255, 255},
                black = {60, 60, 60, 255};
    switch (f) {
        case FACTION_WHITE:
            *left = white;
            *right = white;
            break;
        case FACTION_RED:
            *left = red;
            *right = red;
            break;
        case FACTION_BLUE:
            *left = blue;
            *right = blue;
            break;
        case FACTION_BLACK:
            *left = black;
            *right = black;
            break;
        case FACTION_WHITE_RED:
            *left = white;
            *right = red;
            break;
        case FACTION_WHITE_BLUE:
            *left = white;
            *right = blue;
            break;
        case FACTION_RED_BLUE:
            *left = red;
            *right = blue;
            break;
        case FACTION_WHITE_BLACK:
            *left = black;
            *right = white;
            break;
        case FACTION_RED_BLACK:
            *left = black;
            *right = red;
            break;
        case FACTION_BLUE_BLACK:
            *left = black;
            *right = blue;
            break;
        case FACTION_ALL:
            *left = black;
            *right = blue;
            break;
        default:
            return false;
    }
    return true;
}

static void
BattlePlateBuild(BattlePlate *p, const Character *ch, const SimUnit *setup_unit, bool enemy, const BattleSpot *spot) {
    p->ch = ch;
    p->enemy = enemy;

    StrCopy(p->name, sizeof(p->name), CharacterName(ch));

    p->level = ch->level;
    p->power = setup_unit->start_power;
    p->health = setup_unit->max_health;
    p->has_border = BattleUiFactionColors(ch->faction, &p->left, &p->right);
    p->world = (Vector3){spot->x, BattleFeetY(ch->giant ? 2.0f : 1.0f), spot->z};
}

void BattleUiEnterNode(
    BattleUi *ui,
    const Player *player,
    const BattleRoster *enemy,
    const CampaignNode *node,
    const char *map_id,
    int step
) {
    SimUnit preview[SIM_UNIT_COUNT];
    BattleSimPreview(preview, player, enemy);

    char key[96];
    snprintf(key, sizeof(key), "MAP_%s_NODE_%d_NAME", map_id, step);
    const char *name = LocGet(key);
    if (!name[0] && node) name = node->name;
    StrCopy(ui->title, sizeof(ui->title), name);

    StrCopy(ui->mastery, sizeof(ui->mastery), LocGet("PVE_UNLOCKED"));

    const char *diff_key = BattleUiDifficultyKey(BattleUiDifficultyRatio(preview));
    ui->diff_color = (Color){230, 230, 230, 255};
    BattleUiParseColored(LocGet(diff_key), ui->diff_value, sizeof(ui->diff_value), &ui->diff_color);

    ui->node = node;
    ui->pvp = false;
    ui->energy_cost = node ? node->energy_cost : 1;
    ui->gold = node ? node->gold : 0;
    ui->xp = node ? node->xp : 0;
    ui->gems = node ? node->default_gems : 0;
    ui->new_map_on_win = false;

    ui->tactical = false;
    ui->detail_open = false;
    ui->info.label = LocGet("MORE_INFO");
    memset(ui->plates, 0, sizeof(ui->plates));
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) {
        const Character *ours = PlayerSquadCharacter(player, s);
        if (ours) BattlePlateBuild(&ui->plates[0][s], ours, &preview[s], false, &BATTLE_PLAYER_SPOTS[s]);
        if (enemy->present[s])
            BattlePlateBuild(
                &ui->plates[1][s], &enemy->chars[s], &preview[SQUAD_SLOT_COUNT + s], true, &BATTLE_ENEMY_SPOTS[s]
            );
    }
}

void BattleUiEnterNexusNode(
    BattleUi *ui,
    const Player *player,
    const BattleRoster *enemy,
    const NexusEvent *event,
    const NexusDifficulty *difficulty,
    int step,
    int energy_cost
) {
    int index = (int)Clamp((float)(step - 1), 0.0f, (float)(difficulty->node_count - 1));
    const NexusNode *node = &difficulty->nodes[index];
    ui->nexus_node = (CampaignNode){
        .energy_cost = energy_cost,
        .gold = node->gold,
        .xp = node->xp,
        .wins_to_master = 1,
    };
    BattleUiEnterNode(ui, player, enemy, &ui->nexus_node, difficulty->id, step);
    ui->node = &ui->nexus_node;
    snprintf(ui->title, sizeof(ui->title), "%s — %s", event->name, NexusDifficultyTitle(difficulty));
    LocFormat2(
        ui->mastery,
        sizeof(ui->mastery),
        "PVE_HUB_NEXUS_MAP_PROGRESS_BAR",
        TextFormat("%d", step - 1),
        TextFormat("%d", difficulty->node_count)
    );
}

void BattleUiEnterPvp(
    BattleUi *ui,
    const Player *player,
    const BattleRoster *enemy,
    const PvpOpponent *opponent,
    int stamina_cost,
    int nexts,
    bool event
) {
    BattleUiEnterNode(ui, player, enemy, NULL, "", 0);
    ui->pvp = true;
    ui->pvp_can_next = nexts >= 0;
    ui->pvp_nexts = nexts;
    ui->pvp_gain = event ? PvpEventBannerReward(opponent, player->pvp.event.banners, true) : opponent->gain;
    ui->pvp_stake_icon = event ? GUI_PVP_BANNER : GUI_TAB_TROPHY;
    ui->energy_cost = stamina_cost;
    ui->gold = PvpGoldReward(opponent, true);
    ui->xp = PvpXpReward(opponent, true);
    StrCopy(ui->title, sizeof(ui->title), PvpOpponentName(opponent));
    StrCopy(ui->pvp_player_name, sizeof(ui->pvp_player_name), player->name);
    snprintf(ui->next_label, sizeof(ui->next_label), "%s (%d)", LocGet("BUTTON_MATCHMAKING_NEXT"), nexts);
}

void BattleUiSetNewMapOnWin(BattleUi *ui, bool enabled) {
    ui->new_map_on_win = enabled;
}

void BattleUiSetNodeWins(BattleUi *ui, int wins) {
    CampaignRewards rewards = CampaignNodeRewardsForWins(ui->node, wins);
    ui->gold = rewards.gold;
    ui->xp = rewards.xp;
    ui->gems = rewards.gems;
    CampaignNodeMasteryLabel(ui->node, wins, ui->mastery, sizeof(ui->mastery));
}

static void BattleUiSetTacticalEx(BattleUi *ui, Battle *battle, bool on, bool play_sound) {
    if (ui->tactical == on) return;
    ui->tactical = on;
    ui->detail_open = false;
    ui->plate_armed = NULL;
    ui->detail_close_armed = false;
    ui->info.label = LocGet(on ? "LESS_INFO" : "MORE_INFO");
    BattleSetTactical(battle, on);
    if (play_sound) GuiSoundPlay(on ? SND_BATTLE_INFO_OPEN : SND_BATTLE_INFO_CLOSE);
}

void BattleUiSetTactical(BattleUi *ui, Battle *battle, bool on) {
    BattleUiSetTacticalEx(ui, battle, on, true);
}

static void BattleUiOpenPlateDetails(BattleUi *ui, const BattlePlate *plate) {
    ui->detail_card = (HeroCard){0};
    HeroCardBuild(&ui->detail_card, plate->ch);
    if (plate->enemy) {
        ui->detail_card.max_level = 0;
        ui->detail_card.xp_frac = 1.0f;
    }
    ui->detail_open = true;
}

static bool BattleUiDrawPlate(const BattlePlate *p, const Camera3D *cam, bool interactive) {
    float k = MM_SY;
    float w = 135 * k, h = 75 * k;

    Vector2 sp = GetWorldToScreen(p->world, *cam);
    float x = sp.x / UiScale() - w * 0.5f;
    float y = sp.y / UiScale() - h * 1.5f;
    x = fminf(fmaxf(x, 2.4f), UiWidth() - w - 2.4f);
    y = fminf(fmaxf(y, 74.0f), UiHeight() - h - 2.4f);

    Texture2D bg = GuiTex(GUI_MM_PLATE);
    UiDrawTexture(bg, (Rectangle){x, y, w, 35 * k}, WHITE);

    if (p->has_border) {
        Texture2D border = GuiTex(GUI_MM_PLATE_BORDER);
        float cw = 129 * k, ch = 31 * k, bw = cw * 0.52f;
        Rectangle src = {0, 0, (float)border.width, (float)border.height};
        DrawTexturePro(border, src, (Rectangle){x + 3 * k, y + 2 * k, bw, ch}, (Vector2){0, 0}, 0.0f, p->left);
        src.width = -src.width;
        DrawTexturePro(
            border, src, (Rectangle){x + 3 * k + cw - bw, y + 2 * k, bw, ch}, (Vector2){0, 0}, 0.0f, p->right
        );
    }

    Font cool = GuiFont(FNT_COOL), menu = GuiFont(FNT_MENU);

    Texture2D badge = GuiTex(GUI_HUD_XP);
    Rectangle br = {x - 5.25f * k, y + 0.9f * k, 30 * k, 30 * k};
    UiDrawTexture(badge, br, WHITE);
    char text[16];
    snprintf(text, sizeof(text), "%d", p->level);
    float fs = 15.0f;
    UiDrawTextCentered(cool, text, br, fs, WHITE);

    fs = 13.0f;
    Vector2 tsz = MeasureTextEx(cool, p->name, fs, 1.0f);
    float name_w = 99 * k;
    if (tsz.x > name_w) {
        fs *= name_w / tsz.x;
        tsz = MeasureTextEx(cool, p->name, fs, 1.0f);
    }
    DrawTextEx(
        cool,
        p->name,
        (Vector2){x + 23.4f * k + (name_w - tsz.x) * 0.5f, y + (34 * k - tsz.y) * 0.5f},
        fs,
        1.0f,
        (Color){230, 230, 230, 255}
    );

    Texture2D icons[2] = {GuiTex(GUI_DETAILS_POWER), GuiTex(GUI_MM_HEALTH)};
    int values[2] = {p->power, p->health};
    for (int i = 0; i < 2; i++) {
        Rectangle ir = {x + 34 * k, y + (32.4f + i * 19.6f) * k, 24 * k, 23 * k};
        UiDrawTexture(icons[i], ir, WHITE);
        snprintf(text, sizeof(text), "%d", values[i]);
        fs = 17.0f;
        tsz = MeasureTextEx(menu, text, fs, 1.0f);
        DrawTextEx(menu, text, (Vector2){ir.x + ir.width + 6.0f, ir.y + (ir.height - tsz.y) * 0.5f}, fs, 1.0f, WHITE);
    }

    return interactive && CheckCollisionPointRec(UiMouse(), (Rectangle){x, y, w, h});
}

static void BattleUiRewardRow(float right, float y, Texture2D icon, bool mirror_icon, int value) {
    Font font = GuiFont(FNT_COOL);
    Texture2D tag = GuiTex(GUI_MM_TAG);
    float row_h = 44 * MM_SY, icon_s = 53 * MM_SY, tag_w = MM_X(110);
    Rectangle icon_dst = {right - icon_s, y + (row_h - icon_s) * 0.5f, icon_s, icon_s};
    Rectangle tag_dst = {icon_dst.x + icon_s * 0.5f - tag_w, y, tag_w, row_h};
    UiDrawTexture(tag, tag_dst, WHITE);

    char text[16];
    snprintf(text, sizeof(text), "+%d", value);
    float fs = row_h * 0.6f;
    UiDrawTextRight(font, text, (Rectangle){0.0f, y, icon_dst.x - 6.0f, row_h}, fs, (Color){245, 244, 238, 255});
    Rectangle src = {0, 0, mirror_icon ? -icon.width : icon.width, icon.height};
    DrawTexturePro(icon, src, icon_dst, (Vector2){0, 0}, 0.0f, WHITE);
}

static const float RIBBON_LEFT[14][4] = {
    {2.9581f, -1.0548f, 0.1963f, 0.0000f},
    {4.2316f, -0.9812f, 0.0000f, 0.0419f},
    {2.9581f, -0.9812f, 0.1963f, 0.0419f},
    {4.2316f, -1.0548f, 0.0000f, 0.0000f},
    {2.3389f, -1.0548f, 0.2918f, 0.0000f},
    {2.3614f, -0.9812f, 0.2883f, 0.0419f},
    {1.7381f, -0.3748f, 0.3844f, 0.3875f},
    {1.6432f, -0.3748f, 0.3990f, 0.3875f},
    {2.3389f, 0.3052f, 0.2918f, 0.7751f},
    {2.3614f, 0.2316f, 0.2883f, 0.7331f},
    {2.9581f, 0.3052f, 0.1963f, 0.7751f},
    {2.9581f, 0.2316f, 0.1963f, 0.7331f},
    {4.2316f, 0.3052f, 0.0000f, 0.7751f},
    {4.2316f, 0.2316f, 0.0000f, 0.7331f}
};
static const unsigned char RIBBON_I[36] = {0, 1, 2, 0, 3, 1, 2, 4,  0, 2, 5,  4,  6,  4,  5,  6,  7,  4,
                                           6, 8, 7, 6, 9, 8, 9, 10, 8, 9, 11, 10, 11, 12, 10, 11, 13, 12};

static void BattleUiSplashChevron(float xpoint, float xopen, float bcy, Color tint) {
    const float dxp = 1.6432f, dxo = 4.2316f, dycy = -0.3748f;
    float xs = (xopen - xpoint) / (dxo - dxp);
    float ys = fabsf(xs);
    Texture2D grunge = GuiTex(GUI_SPLASH_GRUNGE);
    rlSetTexture(grunge.id);
    rlBegin(RL_TRIANGLES);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);
    for (int k = 0; k < 36; k++) {
        const float *r = RIBBON_LEFT[RIBBON_I[k]];
        rlTexCoord2f(r[2], r[3]);
        rlVertex2f(xpoint + (r[0] - dxp) * xs, bcy - (r[1] - dycy) * ys);
    }
    rlEnd();
    rlSetTexture(0);
}

static void BattleUiResultSplash(BattleUi *ui, BattlePlay *bp, float W, float H, float cx, Font cool) {
    bool won = bp->won;
    float fade = bp->state == BP_END_SPLASH ? bp->t / 0.30f : 1.0f;
    if (fade > 1.0f) fade = 1.0f;
    if (fade < 0.0f) fade = 0.0f;
    fade = fade * fade * (3.0f - 2.0f * fade);

    float bw = W * 0.92f, bh = bw * 0.25f, bcy = H * 0.435f;
    Rectangle banner = {cx - bw * 0.5f, bcy - bh * 0.5f, bw, bh};

    if (won) {
        Texture2D t = GuiTex(GUI_SPLASH_GLOW);
        float gw = bw * 0.6f, gh = gw * 0.42f;
        BeginBlendMode(BLEND_ADDITIVE);
        UiDrawTexture(
            t,
            (Rectangle){cx - gw * 0.5f, bcy - gh * 0.5f, gw, gh},
            (Color){255, 205, 110, (unsigned char)(fade * 205.0f)}
        );
        EndBlendMode();
    } else {
        Texture2D t = GuiTex(GUI_SPLASH_SMOKE);
        UiDrawTexture(
            t,
            (Rectangle){cx - bw * 0.5f, bcy - bh * 0.72f, bw, bh * 1.44f},
            (Color){16, 14, 18, (unsigned char)(fade * 165.0f)}
        );
    }

    {
        Texture2D t = GuiTex(GUI_SPLASH_INKSMEAR);
        UiDrawTexture(t, banner, (Color){0, 0, 0, (unsigned char)(fade * 255.0f)});
    }

    {
        Texture2D t = GuiTex(GUI_SPLASH_INKSPLAT);
        UiDrawTexture(
            t,
            (Rectangle){cx - bw * 0.52f, bcy - bh * 0.60f, bw * 1.04f, bh * 1.20f},
            (Color){0, 0, 0, (unsigned char)(fade * 205.0f)}
        );
    }

    {
        Color arr = won ? (Color){40, 220, 70, (unsigned char)(fade * 235.0f)}
                        : (Color){225, 34, 22, (unsigned char)(fade * 235.0f)};
        rlDisableBackfaceCulling();
        BattleUiSplashChevron(cx - bw * 0.47f, cx - bw * 0.29f, bcy, arr);
        BattleUiSplashChevron(cx + bw * 0.47f, cx + bw * 0.29f, bcy, arr);
        rlDrawRenderBatchActive();
        rlEnableBackfaceCulling();
    }

    {
        const char *msg = LocGet(won ? "BATTLE_RESULTS_VICTORY_MESSAGE" : "BATTLE_RESULTS_DEFEAT_MESSAGE");
        float fs = 120.0f;
        Vector2 m = MeasureTextEx(cool, msg, fs, 1.0f);
        if (m.x > 1.0f) fs *= (bw * 0.5f) / m.x;
        if (fs > 132.0f) fs = 132.0f;
        Vector2 ts = MeasureTextEx(cool, msg, fs, 1.0f);
        Vector2 pos = {cx - ts.x * 0.5f, bcy - ts.y * 0.5f};
        Color col = won ? (Color){240, 240, 240, (unsigned char)(fade * 255.0f)}
                        : (Color){139, 0, 0, (unsigned char)(fade * 255.0f)};
        DrawTextEx(
            cool, msg, (Vector2){pos.x + 2.5f, pos.y + 2.5f}, fs, 1.0f, (Color){0, 0, 0, (unsigned char)(fade * 140.0f)}
        );
        DrawTextEx(cool, msg, pos, fs, 1.0f, col);
    }

    if (won && ui->new_map_on_win) {
        const char *msg = LocGet("MAP_PVE_NEWMAPUNLOCK");
        float fs = 34.0f;
        Vector2 ts = MeasureTextEx(cool, msg, fs, 1.0f);
        Vector2 pos = {cx - ts.x * 0.5f, bcy + bh * 0.58f};
        DrawTextEx(
            cool, msg, (Vector2){pos.x + 2.0f, pos.y + 2.0f}, fs, 1.0f, (Color){0, 0, 0, (unsigned char)(fade * 190.0f)}
        );
        DrawTextEx(cool, msg, pos, fs, 1.0f, (Color){244, 204, 99, (unsigned char)(fade * 255.0f)});
    }
}

static void BattleUiIntroText(const BpIntroEvent *event, char *dst, int cap) {
    char value[16];
    if (fabsf(event->value - roundf(event->value)) < 0.05f)
        snprintf(value, sizeof(value), "%.0f", event->value);
    else
        snprintf(value, sizeof(value), "%.1f", event->value);

    HeroesFormat2(dst, cap, LocGet("FACTION_BONUS"), value, "");
}

static void BattleUiDrawIntroBanner(const BpIntroEvent *event, float cx, Font cool) {
    Texture2D bg = GuiTex(GUI_COMBAT_PREBATTLE_BG);
    UiDrawTexture(bg, (Rectangle){cx - 237.5f, 198.0f, 475.0f, 65.0f}, WHITE);

    char text[128];
    const RuneDef *rune = event->rune_def >= 0 ? RuneDefAt(event->rune_def) : NULL;
    if (event->event_bonus)
        StrCopy(text, sizeof(text), LocGet("NEXUS_EVENT_BONUS"));
    else if (event->no_event_bonus)
        StrCopy(text, sizeof(text), LocGet("PVP_EVENT_NO_BONUS"));
    else if (rune)
        snprintf(text, sizeof(text), "%s %s", RuneName(rune), RuneValueText(rune));
    else
        BattleUiIntroText(event, text, sizeof(text));
    float fs = 24.0f;
    Vector2 size = MeasureTextEx(cool, text, fs, 1.0f);
    if (size.x > 620.0f) {
        fs *= 620.0f / size.x;
        size = MeasureTextEx(cool, text, fs, 1.0f);
    }
    DrawTextEx(cool, text, (Vector2){cx - size.x * 0.5f, 198.0f + (65.0f - size.y) * 0.5f}, fs, 1.0f, WHITE);

    Texture2D icon = rune ? RuneIconLoad(rune->buff_icon) : GuiTex(BATTLE_UI_FACTION_INTRO_ICON);
    UiDrawTexture(icon, (Rectangle){cx - 36.0f, 252.0f, 72.0f, 72.0f}, WHITE);
}

static Rectangle BattleUiCombatMessageRect(float canvas_width, int slot, float rise) {
    float cx = canvas_width * 0.5f;
    float x = slot == 1 ? cx - 468.0f : slot == 0 ? cx - 216.0f : cx + 36.0f;
    return (Rectangle){x, 270.0f - rise, 432.0f, 216.0f};
}

static Vector2 BattleUiRewardCoinStart(float canvas_width) {
    Rectangle actor = BattleUiCombatMessageRect(canvas_width, 0, 72.0f);
    float x_max = actor.x + actor.width;
    float y_max = actor.y + actor.height;
    return (Vector2){0.75f * x_max, 0.625f * y_max};
}

static float BattleUiRewardCoinAlpha(float age) {
    if (age <= 0.0f || age >= BP_REWARD_COIN_MOVE_TIME) return 0.0f;
    if (age < BP_REWARD_COIN_FADE_IN) return age / BP_REWARD_COIN_FADE_IN;
    float fade_out_at = BP_REWARD_COIN_MOVE_TIME - BP_REWARD_COIN_FADE_OUT;
    if (age > fade_out_at) return (BP_REWARD_COIN_MOVE_TIME - age) / BP_REWARD_COIN_FADE_OUT;
    return 1.0f;
}

static void BattleUiDrawCombat(BattleUi *ui, BattlePlay *bp) {
    Font cool = GuiFont(FNT_COOL), menu = GuiFont(FNT_MENU);
    Color off_white = {230, 230, 230, 255};
    const Color style_red = {232, 43, 43, 255};
    float W = UiWidth(), H = UiHeight(), cx = W * 0.5f;

    for (int side = 0; side < 2; side++) {
        float pct = BattlePlayTeamPct(bp, side);
        Texture2D geom = GuiTex(GUI_TEAMBAR_GEOM), fill = GuiTex(GUI_TEAMBAR_FILL);
        Texture2D lcap = GuiTex(GUI_TEAMBAR_LCAP), rcap = GuiTex(GUI_TEAMBAR_RCAP);
        bool mir = side == 1;
#define MMX(x, w) (mir ? W - (x) - (w) : (x))
        Rectangle g = {MMX(2.9f, 570), 7.6f, 570, 65.1f};
        Rectangle gs = {0, 0, mir ? -geom.width : geom.width, geom.height};
        DrawTexturePro(geom, gs, g, (Vector2){0, 0}, 0.0f, WHITE);
        float fw = 541.5f * pct;
        Rectangle f = {mir ? W - 11.5f - fw : 11.5f, 17.4f, fw, 26};
        Rectangle fsrc = {0, 0, mir ? -fill.width : fill.width, fill.height};
        DrawTexturePro(fill, fsrc, f, (Vector2){0, 0}, 0.0f, WHITE);
        Rectangle lc = {MMX(-6.0f, 122.5f), -0.2f, 122.5f, 58.6f};
        Rectangle lcs = {0, 0, mir ? -lcap.width : lcap.width, lcap.height};
        DrawTexturePro(lcap, lcs, lc, (Vector2){0, 0}, 0.0f, WHITE);
        Rectangle rc = {MMX(509.6f, 73.2f), 7.6f, 73.2f, 48.8f};
        Rectangle rcs = {0, 0, mir ? -rcap.width : rcap.width, rcap.height};
        DrawTexturePro(rcap, rcs, rc, (Vector2){0, 0}, 0.0f, WHITE);
#undef MMX
    }

    {
        Texture2D flourish = GuiTex(GUI_COMBAT_FLOURISH);
        Rectangle src = {0, 0, flourish.width, flourish.height};
        DrawTexturePro(flourish, src, (Rectangle){2.9f, 49.9f, 570.0f, 152.0f}, (Vector2){0}, 0.0f, WHITE);
        src.width = -src.width;
        DrawTexturePro(flourish, src, (Rectangle){W - 572.9f, 49.9f, 570.0f, 152.0f}, (Vector2){0}, 0.0f, WHITE);
    }

    Rectangle gold_counter = {180.0f, 65.0f, 216.0f, 72.0f};
    Rectangle gold_coin = {315.0f, 73.0f, 54.0f, 54.0f};
    if (bp->gold_counter_alpha > 0.001f) {
        unsigned char ga = (unsigned char)(255.0f * bp->gold_counter_alpha);
        Color gt = {255, 255, 255, ga};
        Texture2D bg = GuiTex(GUI_COMBAT_GOLD_BG), coin = GuiTex(GUI_HUD_GOLD);
        UiDrawTexture(bg, gold_counter, gt);
        UiDrawTexture(coin, gold_coin, gt);
        char amount[16];
        snprintf(amount, sizeof(amount), "%d", bp->total_bonus_gold);
        UiDrawTextRight(
            cool, amount, (Rectangle){0.0f, gold_counter.y, gold_coin.x - 14.0f, gold_counter.height}, 27.0f, gt
        );
    }

    {
        Texture2D plaque = GuiTex(GUI_COMBAT_ROUND);
        Rectangle dst = {cx - 92.7f, 0, 185.5f, 115.2f};
        UiDrawTexture(plaque, dst, WHITE);
        char num[8];
        snprintf(num, sizeof(num), "%d", bp->cur_round);
        float fs = 34.0f;
        UiDrawTextCentered(cool, num, (Rectangle){cx, 8.0f, 0.0f, 69.0f}, fs, off_white);
    }

    if (bp->state == BP_INTRO_BANNER && bp->intro_event >= 0 && bp->intro_event < bp->intro_event_count)
        BattleUiDrawIntroBanner(&bp->intro_events[bp->intro_event], cx, cool);

    int actor = BattlePlayCurrentActor(bp);
    if (actor >= 0) {
        int init_class = BattlePlayActorInitiativeClass(bp);
        Texture2D banner = GuiTex(GUI_COMBAT_BANNER);
        Rectangle dst = {cx - 172.8f + 26.0f, 98.0f, 293.8f, 76.5f};
        UiDrawTexture(banner, dst, WHITE);
        int icon_id = init_class == SIM_INIT_CLASS_FAST   ? GUI_DETAILS_INIT_QUICK
                      : init_class == SIM_INIT_CLASS_SLOW ? GUI_DETAILS_INIT_SLOW
                                                          : GUI_DETAILS_INIT_NORMAL;
        Texture2D icon = GuiTex(icon_id);
        UiDrawTexture(icon, (Rectangle){cx + 86.0f, 91.0f, 81, 90}, WHITE);
        const char *label = LocGet(
            init_class == SIM_INIT_CLASS_FAST   ? "INITIATIVE_FAST"
            : init_class == SIM_INIT_CLASS_SLOW ? "INITIATIVE_SLOW"
                                                : "INITIATIVE_MEDIUM"
        );
        float fs = 30.0f;
        Vector2 tsz = MeasureTextEx(cool, label, fs, 1.0f);
        DrawTextEx(
            cool,
            label,
            (Vector2){dst.x + (dst.width - tsz.x) * 0.5f - 20.0f, dst.y + (dst.height - tsz.y) * 0.5f},
            fs,
            1.0f,
            off_white
        );
    }

    if ((bp->state == BP_ACT_MESSAGES || bp->state == BP_ACT_FLIGHT) && bp->message_count > 0) {
        int idx = (int)(bp->message_t / BP_MESSAGE_TIME);
        if (idx >= 0 && idx < bp->message_count && bp->state == BP_ACT_MESSAGES) {
            BpMessage *m = &bp->messages[idx];
            float within = bp->message_t - idx * BP_MESSAGE_TIME;
            float alpha = within < 0.15f ? within / 0.15f : within > 0.91f ? (BP_MESSAGE_TIME - within) / 0.15f : 1.0f;
            if (alpha < 0) alpha = 0;
            if (alpha > 1) alpha = 1;
            float rise = (within < 0.15f ? within / 0.15f : 1.0f) * 72.0f;
            Rectangle cell = BattleUiCombatMessageRect(W, m->slot, rise);
            if (m->icon >= 0) {
                Texture2D icon = GuiTex((GuiId)m->icon);
                Color tint = {255, 255, 255, (unsigned char)(alpha * 255)};
                UiDrawTexture(icon, cell, tint);
            }
            float fs = 28.0f * m->big;
            Color col = m->color;
            col.a = (unsigned char)(alpha * 255);
            Vector2 tsz = MeasureTextEx(cool, m->text, fs, 1.0f);
            DrawTextEx(
                cool,
                m->text,
                (Vector2){cell.x + cell.width * 0.55f - tsz.x, cell.y + cell.height * 0.42f},
                fs,
                1.0f,
                col
            );
            if (m->bonus[0]) {
                Color bc = m->bonus_color.a ? m->bonus_color : style_red;
                bc.a = col.a;
                DrawTextEx(
                    cool,
                    m->bonus,
                    (Vector2){cell.x + cell.width * 0.55f - tsz.x, cell.y + cell.height * 0.42f + fs},
                    22.0f,
                    1.0f,
                    bc
                );
            }
        }
    }

    if (bp->reward_anim_active) {
        Texture2D coin = GuiTex(GUI_HUD_GOLD);
        Vector2 start = BattleUiRewardCoinStart(W);
        Vector2 end = {gold_coin.x, gold_coin.y};
        for (int i = 0; i < BP_REWARD_COIN_COUNT; i++) {
            float age = bp->reward_anim_t - i * BP_REWARD_COIN_STAGGER;
            float alpha = BattleUiRewardCoinAlpha(age);
            if (alpha <= 0.0f) continue;
            float q = age / BP_REWARD_COIN_MOVE_TIME;
            Vector2 p = Vector2Lerp(start, end, q * q);
            float size = 26.0f;
            unsigned char ca = (unsigned char)(255.0f * alpha);
            UiDrawTexture(
                coin, (Rectangle){p.x - size * 0.5f, p.y - size * 0.5f, size, size}, (Color){255, 255, 255, ca}
            );
        }
    }

    if (bp->state != BP_END_SPLASH && bp->state != BP_SUMMARY) {
        Rectangle r = {12.8f, 356.4f, 72, 72};
        Texture2D ff = GuiTex(BattlePlayFast(bp) ? GUI_COMBAT_FF_ON : GUI_COMBAT_FF);
        UiDrawTexture(ff, r, WHITE);
        bool hover = CheckCollisionPointRec(UiMouse(), r);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui->fast_forward_armed = hover;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            bool clicked = ui->fast_forward_armed && hover;
            ui->fast_forward_armed = false;
            if (clicked) {
                GuiSoundPlay(SND_COMBAT_SPEED);
                BattlePlayToggleSpeed(bp);
            }
        }
    } else
        ui->fast_forward_armed = false;

    if (bp->state == BP_END_SPLASH || bp->state == BP_SUMMARY) {
        BattleUiResultSplash(ui, bp, W, H, cx, cool);

        if (bp->state == BP_SUMMARY) {
            BattleSummary sum;
            BattlePlaySummary(bp, &sum);
            float sfade = bp->t / 0.20f;
            if (sfade > 1.0f) sfade = 1.0f;
            if (sfade < 0.0f) sfade = 0.0f;
            unsigned char summary_alpha = (unsigned char)(255.0f * sfade);
            Color summary_white = {255, 255, 255, summary_alpha};
            Color summary_off_white = off_white;
            summary_off_white.a = summary_alpha;
            DrawRectangle(0, 0, (int)W + 1, (int)H + 1, (Color){0, 0, 0, (unsigned char)(178.0f * sfade)});

            Texture2D plaque = GuiTex(GUI_SUMMARY_PLAQUE);
            Rectangle prect = {cx - 648.0f, 230.0f, 1296.0f, 432.0f};
            UiDrawTexture(plaque, prect, summary_white);
            for (int i = 0; i < 3; i++)
                DrawRectangle(
                    (int)(cx - 450.0f), 271 + i * 143, 900, 3, (Color){0, 0, 0, (unsigned char)(178.0f * sfade)}
                );

            const Color green = {34, 204, 43, summary_alpha}, red2 = {255, 0, 0, summary_alpha};
            const char *pname = LocGet("BATTLE_SUMMARY_PLAYER");
            float fs = 30.0f;
            Vector2 psz = MeasureTextEx(cool, pname, fs, 1.0f);
            DrawTextEx(cool, pname, (Vector2){cx - 80.0f - psz.x, 170.0f}, fs, 1.0f, bp->won ? green : red2);
            DrawTextEx(cool, ui->title, (Vector2){cx + 80.0f, 170.0f}, fs, 1.0f, bp->won ? red2 : green);
            Texture2D vs = GuiTex(GUI_SUMMARY_VS);
            UiDrawTexture(vs, (Rectangle){cx - 38.7f, 162, 77.5f, 72}, summary_white);
            const char *vs_text = LocGet("BATTLE_SUMMARY_VS");
            Vector2 vtsz = MeasureTextEx(cool, vs_text, 22.0f, 1.0f);
            DrawTextEx(cool, vs_text, (Vector2){cx - vtsz.x * 0.5f, 187.0f}, 22.0f, 1.0f, summary_off_white);

            const char *labels[6] = {
                LocGet("BATTLE_SUMMARY_DAMAGE_DEALT"),
                LocGet("BATTLE_SUMMARY_STARTING_HEALTH"),
                LocGet("BATTLE_SUMMARY_CRIT_HITS"),
                LocGet("BATTLE_SUMMARY_FACTION_HITS"),
                LocGet("BATTLE_SUMMARY_MVP"),
                LocGet("BATTLE_SUMMARY_SURVIVORS")
            };
            const GuiId icons[6] = {
                GUI_SUMMARY_DAMAGE,
                GUI_MM_HEALTH,
                GUI_DETAILS_CRIT,
                GUI_SUMMARY_FACTION,
                GUI_SUMMARY_MVP,
                GUI_SUMMARY_SURVIVORS
            };
            char pv[6][64], ev[6][64];
            snprintf(pv[0], 64, "%d", sum.dmg[0]);
            snprintf(ev[0], 64, "%d", sum.dmg[1]);
            snprintf(pv[1], 64, "%ld", sum.start_hp[0]);
            snprintf(ev[1], 64, "%ld", sum.start_hp[1]);
            snprintf(pv[2], 64, "%d", sum.crits[0]);
            snprintf(ev[2], 64, "%d", sum.crits[1]);
            snprintf(pv[3], 64, "%d", sum.fac_hits[0]);
            snprintf(ev[3], 64, "%d", sum.fac_hits[1]);
            snprintf(pv[4], 64, "%s", sum.mvp[0]);
            snprintf(ev[4], 64, "%s", sum.mvp[1]);
            snprintf(pv[5], 64, "%d", sum.survivors[0]);
            snprintf(ev[5], 64, "%d", sum.survivors[1]);
            for (int i = 0; i < 6; i++) {
                float y = 276.0f + i * 47.0f;
                DrawTextEx(
                    menu, labels[i], (Vector2){cx - 449.0f, y}, 20.0f, 1.0f, (Color){191, 191, 191, summary_alpha}
                );
                Vector2 vsz = MeasureTextEx(cool, pv[i], 22.0f, 1.0f);
                DrawTextEx(cool, pv[i], (Vector2){cx - 45.0f - vsz.x, y}, 22.0f, 1.0f, summary_off_white);
                float icon_size = i == 3 ? 28.0f : 32.0f;
                Texture2D icon = GuiTex(icons[i]);
                UiDrawTexture(icon, (Rectangle){cx - icon_size * 0.5f, y - 3.0f, icon_size, icon_size}, summary_white);
                DrawTextEx(cool, ev[i], (Vector2){cx + 46.0f, y}, 22.0f, 1.0f, summary_off_white);
            }
            const char *tap = LocGet("TAP_TO_CONTINUE");
            Vector2 tsz = MeasureTextEx(menu, tap, 22.0f, 1.0f);
            DrawTextEx(menu, tap, (Vector2){cx - tsz.x * 0.5f, 600.0f}, 22.0f, 1.0f, summary_off_white);
        }
    }
}

static void BattleUiDrawCombatRunes(const BattlePlay *bp) {
    float size = 54.0f;
    for (int i = 0; i < bp->sim.rune_def_count; i++) {
        Texture2D icon = RuneIconLoad(RuneDefAt(bp->sim.rune_defs[i])->icon);
        if (icon.id != 0) UiDrawTexture(icon, (Rectangle){8.0f, 92.0f + i * (size + 4.0f), size * 0.78f, size}, WHITE);
    }
}

static bool BattleUiDrawRuneSlots(const Player *player, bool input_ok) {
    long long now = RuneNow();
    int squad = player->current_squad;
    int slots = player->runes.prestige_unlocked[squad] ? RUNE_SLOT_COUNT : RUNE_NORMAL_SLOT_COUNT;
    Rectangle area = {0, MM_Y(4), MM_X(76), 97 * MM_SY * slots};
    for (int slot = 0; slot < slots; slot++) {
        Rectangle rect = {0, MM_Y(4) + slot * 97 * MM_SY, MM_X(76), 98 * MM_SY};
        const PlayerRune *rune = RuneInSlot(&player->runes, squad, slot);
        Texture2D texture = rune ? RuneIconLoad(RuneDefFor(rune)->icon) : GuiTex(GUI_MM_RUNE_SLOT);
        if (rune && !RuneIsExpired(rune, now) && texture.id != 0)
            UiDrawTexture(texture, rect, WHITE);
        else
            UiDrawTexture(GuiTex(GUI_MM_RUNE_SLOT), rect, WHITE);
    }
    return input_ok && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(UiMouse(), area);
}

static const char VS_LABEL_KEY[] = "BATTLE_SUMMARY_VS";
static const Rectangle VS_PLAYER_PLATE = {-398.5f, 20.0f, 321.0f, 61.0f};
static const Rectangle VS_OPPONENT_PLATE = {78.5f, 20.0f, 321.0f, 61.0f};
static const Rectangle VS_PLAYER_NAME = {-368.5f, 27.0f, 261.0f, 50.0f};
static const Rectangle VS_OPPONENT_NAME = {109.5f, 27.0f, 261.0f, 50.0f};
static const Rectangle VS_LABEL_BOX = {-103.5f, 15.0f, 207.0f, 61.0f};
static const Rectangle VS_INFO_BUTTON = {-125.5f, 71.0f, 251.0f, 61.0f};
static const float VS_NAME_FONT = 15.0f;
static const float VS_LABEL_FONT_FRACTION = 0.77f;
static const Vector2 VS_LABEL_SHADOW = {0.01f, 0.05f};
static const Color VS_LABEL_SHADOW_COLOR = {26, 26, 26, 128};

static Rectangle BattleUiMatchmakingRect(float cx, Rectangle design) {
    return (Rectangle){cx + MM_X(design.x), MM_Y(design.y), MM_X(design.width), design.height * MM_SY};
}

static void BattleUiDrawVsPlate(Rectangle box) {
    Texture2D tex = GuiTex(GUI_MM_VS_PLATE);
    float height = fminf(box.height, box.width * (float)tex.height / (float)tex.width);
    float width = height * (float)tex.width / (float)tex.height;
    Rectangle dst = {box.x + (box.width - width) * 0.5f, box.y + (box.height - height) * 0.5f, width, height};
    UiDrawTexture(tex, dst, WHITE);
}

static void BattleUiDrawVsHeader(BattleUi *ui, Battle *battle, float cx, bool input_ok) {
    Font font = GuiFont(FNT_COOL);
    Color off_white = {230, 230, 230, 255};
    BattleUiDrawVsPlate(BattleUiMatchmakingRect(cx, VS_PLAYER_PLATE));
    BattleUiDrawVsPlate(BattleUiMatchmakingRect(cx, VS_OPPONENT_PLATE));
    float name_size = VS_NAME_FONT * MM_SY;
    const char *player_name = ui->pvp_player_name[0] ? ui->pvp_player_name : LocGet("BATTLE_SUMMARY_PLAYER");
    UiDrawTextCentered(font, player_name, BattleUiMatchmakingRect(cx, VS_PLAYER_NAME), name_size, off_white);
    UiDrawTextCentered(font, ui->title, BattleUiMatchmakingRect(cx, VS_OPPONENT_NAME), name_size, off_white);

    Rectangle vs = BattleUiMatchmakingRect(cx, VS_LABEL_BOX);
    float vs_size = vs.height * VS_LABEL_FONT_FRACTION;
    Rectangle shadow = vs;
    shadow.x += VS_LABEL_SHADOW.x * vs.width;
    shadow.y += VS_LABEL_SHADOW.y * vs.height;
    UiDrawTextCentered(font, LocGet(VS_LABEL_KEY), shadow, vs_size, VS_LABEL_SHADOW_COLOR);
    UiDrawTextCentered(font, LocGet(VS_LABEL_KEY), vs, vs_size, off_white);

    if (ButtonDrawEx(&ui->info, BattleUiMatchmakingRect(cx, VS_INFO_BUTTON), input_ok))
        BattleUiSetTactical(ui, battle, !ui->tactical);
}

static void BattleUiDrawTacticalNote(float cx, float y) {
    const char *note = LocGet("MATCHMAKING_UNIT_DETAILS_NOTE");
    float nfs = 18.0f;
    Vector2 nsz = MeasureTextEx(GuiFont(FNT_MENU), note, nfs, 1.0f);
    DrawTextEx(GuiFont(FNT_MENU), note, (Vector2){cx - nsz.x * 0.5f, y}, nfs, 1.0f, (Color){230, 230, 230, 255});
}

BattleUiAction BattleUiDraw(BattleUi *ui, const Player *player, Battle *battle, BattlePlay *bp) {
    Font font = GuiFont(FNT_COOL);
    Color off_white = {230, 230, 230, 255};
    float right = UiWidth();
    float cx = right * 0.5f;
    bool input_ok = !ui->detail_open && !UiInputBlocked();
    BattleUiAction result = BATTLE_UI_NONE;

    if (BattlePlayActive(bp)) {
        ui->detail_open = false;
        if (ui->tactical) BattleUiSetTacticalEx(ui, battle, false, false);
        BattleUiDrawCombat(ui, bp);
        BattleUiDrawCombatRunes(bp);
        return BATTLE_UI_NONE;
    }

    if (ui->tactical) {
        DrawRectangle(0, 72, (int)right + 1, (int)(UiHeight() - 72.0f) + 1, (Color){0, 0, 0, 127});
        const BattlePlate *hovered = NULL;
        for (int side = 0; side < 2; side++) {
            for (int s = 0; s < SQUAD_SLOT_COUNT; s++) {
                const BattlePlate *p = &ui->plates[side][s];
                if (p->ch && BattleUiDrawPlate(p, &battle->cam, input_ok)) hovered = p;
            }
        }
        if (!UiInputBlocked() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui->plate_armed = hovered;
        const BattlePlate *tapped = NULL;
        if (!UiInputBlocked() && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (hovered == ui->plate_armed) tapped = hovered;
            ui->plate_armed = NULL;
        }
        if (tapped) {
            BattleUiOpenPlateDetails(ui, tapped);
            GuiClickSoundPlayDuplicate();
        }
    }

    if (ui->pvp) {
        BattleUiDrawVsHeader(ui, battle, cx, input_ok);
        if (ui->tactical) BattleUiDrawTacticalNote(cx, MM_Y(VS_INFO_BUTTON.y + VS_INFO_BUTTON.height + 4.0f));
    } else {
        Texture2D panel = GuiTex(GUI_MM_PANEL);
        Rectangle dst = {cx - MM_X(266.5f), MM_Y(17), MM_X(534), 113 * MM_SY};
        UiDrawTexture(panel, dst, WHITE);

        float fs = 22.0f;
        UiDrawTextCentered(font, ui->title, (Rectangle){cx, MM_Y(24), 0.0f, 45.0f}, fs, off_white);

        Texture2D trough = GuiTex(GUI_MM_TROUGH);
        Rectangle trough_dst = {cx - MM_X(189.5f), MM_Y(59), MM_X(374), 32 * MM_SY};
        NPatchInfo n9 = {{0, 0, (float)trough.width, (float)trough.height}, 9, 1, 9, 1, NPATCH_NINE_PATCH};
        DrawTextureNPatch(trough, n9, trough_dst, (Vector2){0, 0}, 0.0f, WHITE);
        fs = 17.0f;
        Vector2 tsz = MeasureTextEx(font, ui->mastery, fs, 1.0f);
        DrawTextEx(
            font,
            ui->mastery,
            (Vector2){trough_dst.x + 14.0f, trough_dst.y + (trough_dst.height - tsz.y) * 0.5f},
            fs,
            1.0f,
            off_white
        );

        fs = 20.0f;
        char label[64];
        HeroesFormat2(label, sizeof(label), LocGet("DIFFICULTY_STRING"), "", "");
        Vector2 pos = {cx - MM_X(206.5f), MM_Y(100)};
        DrawTextEx(font, label, pos, fs, 1.0f, (Color){130, 125, 115, 255});
        pos.x += MeasureTextEx(font, label, fs, 1.0f).x + 6.0f;
        DrawTextEx(font, ui->diff_value, pos, fs, 1.0f, ui->diff_color);

        if (ButtonDrawEx(&ui->info, (Rectangle){cx - MM_X(104), MM_Y(132), MM_X(208), 52 * MM_SY}, input_ok))
            BattleUiSetTactical(ui, battle, !ui->tactical);

        if (ui->tactical) BattleUiDrawTacticalNote(cx, MM_Y(190));
    }

    if (BattleUiDrawRuneSlots(player, input_ok)) result = BATTLE_UI_RUNES;

    if (!ui->tactical) {
        float row_h = 44 * MM_SY, stride = 40 * MM_SY;
        float tag_right = right - MM_X(9);
        float y = MM_Y(478) - row_h;
        if (ui->gems > 0) {
            BattleUiRewardRow(tag_right, y, GuiTex(GUI_MAP_GEMS), false, ui->gems);
            y -= stride;
        }
        if (ui->xp > 0) {
            BattleUiRewardRow(tag_right, y, GuiTex(GUI_MM_XP), false, ui->xp);
            y -= stride;
        }
        if (ui->gold > 0) {
            BattleUiRewardRow(tag_right, y, GuiTex(GUI_HUD_GOLD), true, ui->gold);
            y -= stride;
        }
        if (ui->pvp) BattleUiRewardRow(tag_right, y, GuiTex(ui->pvp_stake_icon), false, ui->pvp_gain);
    }

    {
        Texture2D frame = GuiTex(GUI_MM_SQUAD_FRAME);
        Rectangle frame_dst = {MM_X(23), MM_Y(502), MM_X(248), 62 * MM_SY};
        UiDrawTexture(frame, frame_dst, WHITE);

        Font tab_font = GuiFont(FNT_MENU);
        float size = 60 * MM_SY, stride = MM_X(60);
        for (int q = 0; q < PLAYER_MAX_SQUADS; q++) {
            Rectangle r = {MM_X(26) + q * stride, MM_Y(504), stride, size};
            bool active = player->current_squad == q;
            Texture2D tab = GuiTex(
                q == 0 ? (active ? GUI_TAB_DEFENSE_ON : GUI_TAB_DEFENSE) : (active ? GUI_TAB_SQUAD_ON : GUI_TAB_SQUAD)
            );
            UiDrawTexture(tab, r, WHITE);
            if (q == 1) {
                Texture2D cup = GuiTex(active ? GUI_TAB_TROPHY_ON : GUI_TAB_TROPHY);
                float cw = size * 0.72f;
                Rectangle cr = {r.x + (r.width - cw) * 0.5f, r.y + (size - cw) * 0.5f, cw, cw};
                UiDrawTexture(cup, cr, WHITE);
            }

            char num[4];
            snprintf(num, sizeof(num), "%d", q + 1);
            float fs = size * 0.5f;
            UiDrawTextCentered(tab_font, num, (Rectangle){r.x, r.y, r.width, size}, fs, WHITE);
        }

        Rectangle retreat_dst = {right - MM_X(480), MM_Y(497), MM_X(240), 60 * MM_SY};
        if (ui->pvp && ui->pvp_can_next) {
            retreat_dst.x = right - MM_X(600);
            retreat_dst.width = MM_X(180);
            Rectangle next_dst = {right - MM_X(420), MM_Y(497), MM_X(180), 60 * MM_SY};
            if (ButtonDrawEx(&ui->next, next_dst, input_ok && ui->pvp_nexts > 0)) result = BATTLE_UI_NEXT;
        }
        TutorialAnchorSet(TUTORIAL_ANCHOR_BATTLE_RETREAT, retreat_dst);
        if (!TutorialHides(TUTORIAL_ANCHOR_BATTLE_RETREAT) && ButtonDrawEx(&ui->retreat, retreat_dst, input_ok))
            result = BATTLE_UI_RETREAT;

        Rectangle fight_dst = {right - MM_X(240), MM_Y(497), MM_X(240), 60 * MM_SY};
        TutorialAnchorSet(TUTORIAL_ANCHOR_BATTLE_FIGHT, fight_dst);
        bool fight_shown = !TutorialHides(TUTORIAL_ANCHOR_BATTLE_FIGHT);
        if (ui->energy_cost <= 0) {
            if (fight_shown && ButtonDrawEx(&ui->fight_zero, fight_dst, input_ok)) result = BATTLE_UI_FIGHT;
        } else if (fight_shown) {
            if (ButtonDrawEx(&ui->fight, fight_dst, input_ok)) result = BATTLE_UI_FIGHT;

            char cost[8];
            snprintf(cost, sizeof(cost), "-%d", ui->energy_cost);
            float fs = 26.0f;
            Vector2 tsz = MeasureTextEx(font, cost, fs, 1.0f);
            Rectangle cell = {fight_dst.x + MM_X(147), fight_dst.y, MM_X(36), fight_dst.height};
            DrawTextEx(
                font,
                cost,
                (Vector2){cell.x + (cell.width - tsz.x) * 0.5f, cell.y + (cell.height - tsz.y) * 0.5f},
                fs,
                1.0f,
                WHITE
            );

            Texture2D potion = GuiTex(ui->pvp ? GUI_HUD_STAMINA : GUI_MM_POTION);
            float ps = 39 * MM_SY;
            UiDrawTexture(potion, (Rectangle){fight_dst.x + MM_X(185), MM_Y(508), ps, ps}, WHITE);
        }
    }

    if (ui->detail_open) {
        HeroesDrawDetails(&ui->detail_card, 0.0f, right);
        Rectangle details = HeroesDetailsBounds(0.0f, right);
        if (!UiInputBlocked() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            ui->detail_close_armed = !CheckCollisionPointRec(UiMouse(), details);
        if (!UiInputBlocked() && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (ui->detail_close_armed && !CheckCollisionPointRec(UiMouse(), details)) ui->detail_open = false;
            ui->detail_close_armed = false;
        }
    }
    return result;
}
