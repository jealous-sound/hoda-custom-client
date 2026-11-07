#define EXPEDITIONS_PANEL_WIDTH 940.0f
#define EXPEDITIONS_PANEL_HEIGHT 597.0f
#define EXPEDITIONS_INSET_X 62.0f
#define EXPEDITIONS_CONTENT_TOP 96.0f
#define EXPEDITIONS_FOOTER_HEIGHT 52.0f
#define EXPEDITIONS_FOOTER_BOTTOM 30.0f
#define EXPEDITIONS_ROW_HEIGHT 112.0f
#define EXPEDITIONS_ROW_GAP 6.0f
#define EXPEDITIONS_ART_WIDTH 150.0f
#define EXPEDITIONS_ART_HEIGHT 100.0f
#define EXPEDITIONS_ACTION_WIDTH 150.0f
#define EXPEDITIONS_REWARD_WIDTH 170.0f
#define EXPEDITIONS_ICON_SIZE 22.0f
#define EXPEDITIONS_DROP_ICON_SIZE 34.0f
#define EXPEDITIONS_MINI_PORTRAIT 34.0f
#define EXPEDITIONS_HEADER_HEIGHT 150.0f
#define EXPEDITIONS_HEADER_ART_WIDTH 210.0f
#define EXPEDITIONS_SLOT_SIZE 62.0f
#define EXPEDITIONS_GRID_COLUMNS 9
#define EXPEDITIONS_BUTTON_HEIGHT 46.0f
#define EXPEDITIONS_REWARD_ICON_SIZE 64.0f
#define EXPEDITIONS_TEXT_CAPACITY 192
#define EXPEDITIONS_BOARD_ART "gui/expeditions/board.png"

typedef enum { EXPEDITIONS_NONE = 0, EXPEDITIONS_CLOSE } ExpeditionsAction;
typedef enum { EXPEDITIONS_MODE_BOARD, EXPEDITIONS_MODE_PARTY } ExpeditionsMode;
typedef enum {
    EXPEDITIONS_DIALOG_NONE,
    EXPEDITIONS_DIALOG_REWARD,
    EXPEDITIONS_DIALOG_RECALL,
    EXPEDITIONS_DIALOG_MESSAGE,
} ExpeditionsDialog;

typedef enum {
    EXPEDITION_ROW_LOCKED,
    EXPEDITION_ROW_OPEN,
    EXPEDITION_ROW_NO_SLOT,
    EXPEDITION_ROW_AWAY,
    EXPEDITION_ROW_READY,
} ExpeditionRowState;

static const Color EXPEDITIONS_TEXT = {235, 232, 220, 255};
static const Color EXPEDITIONS_TEXT_DIM = {182, 178, 168, 255};
static const Color EXPEDITIONS_GOLD = {255, 210, 110, 255};
static const Color EXPEDITIONS_GOOD = {150, 225, 120, 255};
static const Color EXPEDITIONS_BAD = {240, 120, 100, 255};
static const Color EXPEDITIONS_SHADE = {0, 0, 0, 120};
static const Color EXPEDITIONS_ART_BORDER = {120, 98, 60, 255};

typedef struct {
    Font font;
    Font body;
    Button close;
    Button rows[EXPEDITION_CAPACITY];
    Button collect_all;
    Button cancel;
    Button auto_fill;
    Button dispatch;
    Button ok;
    Button yes;
    Button no;
    Button send_template;
    Button claim_template;
    Button recall_template;
    ExpeditionsMode mode;
    ExpeditionsDialog dialog;
    int selected;
    int party[EXPEDITION_PARTY_CAPACITY];
    int party_count;
    int *candidates;
    int candidate_count;
    int candidate_capacity;
    GoalsScroll board_scroll;
    GoalsScroll roster_scroll;
    ExpeditionReward reward;
    char message[EXPEDITIONS_TEXT_CAPACITY];
    bool claim_waiting;
    bool reveal_pending;
    int claim_expeditions;
    int claim_great;
    bool scrim_armed;
} ExpeditionsView;

void ExpeditionsInitialize(ExpeditionsView *v) {
    memset(v, 0, sizeof(*v));
    v->font = GuiFont(FNT_COOL);
    v->body = GuiFont(FNT_MENU);
    Vector2 centre = {0.5f, 0.5f};
    v->close = ButtonLoad(GUI_BTN_CLOSE, GUI_BTN_CLOSE);
    ButtonSetClickSound(&v->close, SND_UI_POPUP_CLOSE);
    v->send_template =
        ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("HODA_EXPEDITION_SEND"), v->font, centre, 0.4f);
    v->claim_template = ButtonLoadText(
        GUI_MISSION_CLAIM, GUI_MISSION_CLAIM_ON, LocGet("LEADERBOARD_MISSIONS_CLAIM"), v->font, centre, 0.5f
    );
    ButtonSetClickSound(&v->claim_template, SND_MISSION_COLLECT);
    v->recall_template =
        ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("HODA_EXPEDITION_RECALL"), v->font, centre, 0.36f);
    v->collect_all = ButtonLoadText(
        GUI_MISSION_CLAIM, GUI_MISSION_CLAIM_ON, LocGet("HODA_EXPEDITION_COLLECT_ALL"), v->font, centre, 0.45f
    );
    ButtonSetClickSound(&v->collect_all, SND_MISSION_COLLECT);
    v->cancel = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("FORTIFY_CANCEL_BUTTON"), v->font, centre, 0.4f);
    v->auto_fill =
        ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("HODA_EXPEDITION_AUTO_FILL"), v->font, centre, 0.4f);
    v->dispatch = v->send_template;
    v->ok = ButtonLoadText(
        GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("RUNE_SINGLE_CONFIRMATION_OK_BUTTON"), v->font, centre, 0.4f
    );
    v->yes = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("CONFIRM_DIALOG_YES"), v->font, centre, 0.4f);
    v->no = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("CONFIRM_DIALOG_NO"), v->font, centre, 0.4f);
    v->selected = EXPEDITION_NONE;
}

int ExpeditionsHomeBadge(const Player *p) {
    return ExpeditionsAvailable() ? ExpeditionsReadyCount(&p->expeditions, ServerNow()) : 0;
}

bool ExpeditionsHomeOpen(const Player *p) {
    return ExpeditionsAvailable() && p->level >= ExpeditionUnlockLevel();
}

void ExpeditionsOpen(ExpeditionsView *v) {
    v->mode = EXPEDITIONS_MODE_BOARD;
    if (!v->claim_waiting) v->dialog = EXPEDITIONS_DIALOG_NONE;
    v->selected = EXPEDITION_NONE;
    v->party_count = 0;
    v->scrim_armed = false;
    GoalsScrollReset(&v->board_scroll);
}

bool ExpeditionsHandleBack(ExpeditionsView *v) {
    if (v->claim_waiting) return true;
    if (v->dialog != EXPEDITIONS_DIALOG_NONE) {
        v->dialog = EXPEDITIONS_DIALOG_NONE;
        return true;
    }
    if (v->mode == EXPEDITIONS_MODE_PARTY) {
        v->mode = EXPEDITIONS_MODE_BOARD;
        return true;
    }
    return false;
}

static void ExpeditionsShowMessage(ExpeditionsView *v, const char *text) {
    StrCopy(v->message, sizeof(v->message), text);
    v->dialog = EXPEDITIONS_DIALOG_MESSAGE;
    GuiSoundPlay(SND_STORE_ERROR);
}

static void ExpeditionsDrawCover(Texture2D texture, Rectangle dst, Color tint) {
    if (texture.id == 0) {
        DrawRectangleRec(dst, (Color){28, 24, 20, 255});
        return;
    }
    float scale = fmaxf(dst.width / texture.width, dst.height / texture.height);
    float width = dst.width / scale, height = dst.height / scale;
    Rectangle source = {(texture.width - width) * 0.5f, (texture.height - height) * 0.5f, width, height};
    DrawTexturePro(texture, source, dst, (Vector2){0, 0}, 0.0f, tint);
}

static void ExpeditionsDrawArt(const ExpeditionDef *def, Rectangle dst, bool dim) {
    ExpeditionsDrawCover(ExpeditionTexture(def->art), dst, dim ? (Color){90, 90, 90, 255} : WHITE);
    DrawRectangleLinesEx(dst, 2.0f, EXPEDITIONS_ART_BORDER);
}

static void ExpeditionsDrawFit(Texture2D texture, Rectangle dst, Color tint) {
    if (texture.id == 0 || texture.width <= 0 || texture.height <= 0) return;
    float scale = fminf(dst.width / texture.width, dst.height / texture.height);
    float width = texture.width * scale, height = texture.height * scale;
    UiDrawTexture(
        texture,
        (Rectangle){dst.x + (dst.width - width) * 0.5f, dst.y + (dst.height - height) * 0.5f, width, height},
        tint
    );
}

static void ExpeditionsDuration(char *out, size_t size, const ExpeditionDef *def) {
    QuestsShortTime(out, size, ExpeditionDurationSeconds(def));
}

static const char *ExpeditionsRarityName(Rarity rarity) {
    static const char *const keys[] = {
        [RARITY_COMMON] = "RARITY_COMMON",
        [RARITY_UNCOMMON] = "RARITY_UNCOMMON",
        [RARITY_RARE] = "RARITY_RARE",
        [RARITY_EPIC] = "RARITY_EPIC",
        [RARITY_LEGENDARY] = "RARITY_LEGENDARY",
        [RARITY_MYTHIC] = "HODA_RARITY_MYTHIC",
    };
    return rarity >= RARITY_COMMON && rarity <= RARITY_MYTHIC ? LocGet(keys[rarity]) : "";
}

static const char *ExpeditionsFactionKey(Faction faction) {
    if (FactionIsWhite(faction)) return "ROSTER_FILTER_WHITE_FACTION";
    if (FactionIsRed(faction)) return "ROSTER_FILTER_RED_FACTION";
    if (FactionIsBlue(faction)) return "ROSTER_FILTER_BLUE_FACTION";
    return "ROSTER_FILTER_BLACK_FACTION";
}

static void ExpeditionsTraitText(char *out, int size, const ExpeditionTrait *trait) {
    char name[EXPEDITIONS_TEXT_CAPACITY] = "";
    char key[EXPEDITIONS_TEXT_CAPACITY];
    switch (trait->kind) {
        case EXPEDITION_TRAIT_GROUP:
            GalleryUpperKey(key, sizeof(key), "GALLERY_%s_TITLE", trait->group);
            StrCopy(name, sizeof(name), LocGet(key));
            break;
        case EXPEDITION_TRAIT_FACTION:
            StrCopy(name, sizeof(name), LocGet(ExpeditionsFactionKey(trait->faction)));
            break;
        case EXPEDITION_TRAIT_RARITY:
            LocFormat1(name, sizeof(name), "HODA_EXPEDITION_RARITY_OR_BETTER", ExpeditionsRarityName(trait->rarity));
            break;
        case EXPEDITION_TRAIT_GIANT:
            StrCopy(name, sizeof(name), LocGet("ROSTER_FILTER_LARGE_CREATURES"));
            break;
        default:
            break;
    }
    LocFormat2(out, size, "HODA_EXPEDITION_TRAIT_COUNT", TextFormat("%d", trait->count), name);
}

static void ExpeditionsPartySizeText(char *out, int size, const ExpeditionDef *def) {
    if (def->party == 1)
        StrCopy(out, size, LocGet("HODA_EXPEDITION_PARTY_SIZE_ONE"));
    else
        LocFormat1(out, size, "HODA_EXPEDITION_PARTY_SIZE", TextFormat("%d", def->party));
}

static const char *ExpeditionsRangeText(int low, int high) {
    return low == high ? TextFormat("%d", low) : TextFormat("%d-%d", low, high);
}

static Texture2D ExpeditionsDropIcon(const ExpeditionDrop *drop) {
    static const GuiId rune_frames[] = {
        GUI_STORE_RUNE_RARITY_COMMON,
        GUI_STORE_RUNE_RARITY_UNCOMMON,
        GUI_STORE_RUNE_RARITY_RARE,
        GUI_STORE_RUNE_RARITY_EPIC,
        GUI_STORE_RUNE_RARITY_LEGENDARY,
    };
    static const GuiId hero_frames[] = {
        GUI_STORE_UNIT_RARITY_COMMON,
        GUI_STORE_UNIT_RARITY_UNCOMMON,
        GUI_STORE_UNIT_RARITY_RARE,
        GUI_STORE_UNIT_RARITY_EPIC,
        GUI_STORE_UNIT_RARITY_LEGENDARY,
    };
    int last = (int)(sizeof(rune_frames) / sizeof(rune_frames[0])) - 1;
    if (drop->kind == EXPEDITION_DROP_GEAR) return GearRarityFrame(drop->max_rarity);
    if (drop->kind == EXPEDITION_DROP_RUNE) {
        int frame = drop->max_rarity - 1;
        return GuiTex(rune_frames[frame < 0 ? 0 : frame > last ? last : frame]);
    }
    int best = RARITY_COMMON;
    for (int rarity = RARITY_COMMON; rarity <= RARITY_LEGENDARY; rarity++)
        if (drop->hero_weights[rarity] > 0) best = rarity;
    return GuiTex(hero_frames[best - 1 > last ? last : best - 1]);
}

static Texture2D ExpeditionsGearPreview(int rarity) {
    for (int i = 0; i < GearCatalogCount(); i++)
        if (GearDefAt(i)->rarity == rarity) return GearIconLoad(GearDefAt(i));
    return (Texture2D){0};
}

static void ExpeditionsDrawDrop(const ExpeditionsView *v, const ExpeditionDrop *drop, Rectangle box, bool great) {
    if (drop->kind == EXPEDITION_DROP_GEAR)
        ExpeditionsDrawFit(ExpeditionsGearPreview(drop->max_rarity), UiRect(box, 0.5f, 0.5f, 0.78f, 0.78f), WHITE);
    ExpeditionsDrawFit(ExpeditionsDropIcon(drop), box, WHITE);
    int chance = great ? EXPEDITION_PERCENT : drop->chance;
    Rectangle label = {box.x - 6.0f, box.y + box.height - 4.0f, box.width + 12.0f, 16.0f};
    UiDrawTextCentered(v->body, TextFormat("%d%%", chance), label, 15.0f, great ? EXPEDITIONS_GOLD : EXPEDITIONS_TEXT);
}

static void ExpeditionsDrawCurrency(const ExpeditionsView *v, GuiId icon, const char *text, Rectangle line) {
    ExpeditionsDrawFit(GuiTex(icon), (Rectangle){line.x, line.y, line.height, line.height}, WHITE);
    UiDrawTextLeftMiddle(
        v->font,
        text,
        (Rectangle){line.x + line.height + 6.0f, line.y, line.width - line.height - 6.0f, line.height},
        line.height * 0.9f,
        EXPEDITIONS_TEXT
    );
}

static void ExpeditionsDrawRewards(const ExpeditionsView *v, const ExpeditionDef *def, Rectangle area, bool great) {
    Rectangle line = {area.x, area.y, area.width, EXPEDITIONS_ICON_SIZE};
    int gold_low = ExpeditionGreatSuccessAmount(def->gold_min, great);
    int gold_high = ExpeditionGreatSuccessAmount(def->gold_max, great);
    ExpeditionsDrawCurrency(v, GUI_MISSION_GOLD, ExpeditionsRangeText(gold_low, gold_high), line);
    line.y += EXPEDITIONS_ICON_SIZE + 4.0f;
    if (def->gems_max > 0) {
        int gems_low = ExpeditionGreatSuccessAmount(def->gems_min, great);
        int gems_high = ExpeditionGreatSuccessAmount(def->gems_max, great);
        ExpeditionsDrawCurrency(v, GUI_MISSION_GEM, ExpeditionsRangeText(gems_low, gems_high), line);
        line.y += EXPEDITIONS_ICON_SIZE + 4.0f;
    }
    Rectangle drop = {area.x, line.y + 2.0f, EXPEDITIONS_DROP_ICON_SIZE, EXPEDITIONS_DROP_ICON_SIZE};
    for (int i = 0; i < def->drop_count; i++) {
        ExpeditionsDrawDrop(v, &def->drops[i], drop, great);
        drop.x += EXPEDITIONS_DROP_ICON_SIZE + 12.0f;
    }
}

static ExpeditionRowState ExpeditionsRowState(const Player *p, int def_index, long long now) {
    const ExpeditionDef *def = ExpeditionAt(def_index);
    int active = ExpeditionsFind(&p->expeditions, def_index);
    if (active >= 0)
        return ExpeditionReady(&p->expeditions.active[active], now) ? EXPEDITION_ROW_READY : EXPEDITION_ROW_AWAY;
    if (!ExpeditionUnlocked(def, p->level)) return EXPEDITION_ROW_LOCKED;
    return ExpeditionCanStart(p, def_index) ? EXPEDITION_ROW_OPEN : EXPEDITION_ROW_NO_SLOT;
}

static void ExpeditionsDrawPartyPortraits(
    const Heroes *heroes, const Player *p, const PlayerExpedition *expedition, Rectangle area
) {
    Rectangle portrait = {area.x, area.y, EXPEDITIONS_MINI_PORTRAIT, EXPEDITIONS_MINI_PORTRAIT};
    for (int i = 0; i < expedition->hero_count; i++) {
        int index = ExpeditionRosterIndexOf(p, expedition->heroes[i]);
        if (index >= 0 && index < heroes->card_count) {
            UiDrawTexture(heroes->cards[index].portrait, portrait, WHITE);
            DrawRectangleLinesEx(portrait, 1.0f, EXPEDITIONS_ART_BORDER);
        }
        portrait.x += EXPEDITIONS_MINI_PORTRAIT + 4.0f;
    }
}

static void
ExpeditionsDrawProgressBar(const ExpeditionsView *v, const PlayerExpedition *expedition, Rectangle bar, long long now) {
    float progress = ExpeditionProgress(expedition, now);
    DrawRectangleRec(bar, (Color){0, 0, 0, 170});
    Rectangle fill = {bar.x + 2.0f, bar.y + 2.0f, (bar.width - 4.0f) * progress, bar.height - 4.0f};
    DrawRectangleRec(fill, progress >= 1.0f ? EXPEDITIONS_GOOD : EXPEDITIONS_GOLD);
    DrawRectangleLinesEx(bar, 1.0f, EXPEDITIONS_ART_BORDER);
    char text[EXPEDITIONS_TEXT_CAPACITY];
    if (ExpeditionReady(expedition, now)) {
        StrCopy(text, sizeof(text), LocGet("HODA_EXPEDITION_READY"));
    } else {
        char remaining[64];
        QuestsShortTime(remaining, sizeof(remaining), ExpeditionRemaining(expedition, now));
        LocFormat1(text, sizeof(text), "HODA_EXPEDITION_TIME_LEFT", remaining);
    }
    UiDrawTextCentered(v->body, text, bar, bar.height * 0.8f, WHITE);
}

static void ExpeditionsDrawRowDetails(const ExpeditionsView *v, const ExpeditionDef *def, Rectangle text, bool dim) {
    Color body = dim ? EXPEDITIONS_TEXT_DIM : EXPEDITIONS_TEXT;
    char duration[64], party[64], line[EXPEDITIONS_TEXT_CAPACITY], trait[EXPEDITIONS_TEXT_CAPACITY];
    ExpeditionsDuration(duration, sizeof(duration), def);
    ExpeditionsPartySizeText(party, sizeof(party), def);
    Rectangle info = {text.x, text.y + 32.0f, text.width, 20.0f};
    ExpeditionsDrawFit(GuiTex(GUI_STORE_TIMER_ICON), (Rectangle){info.x, info.y, 20.0f, 20.0f}, WHITE);
    UiDrawTextLeftMiddle(
        v->body,
        TextFormat("%s   %s", duration, party),
        (Rectangle){info.x + 26.0f, info.y, info.width - 26.0f, info.height},
        18.0f,
        body
    );
    float y = info.y + 22.0f;
    for (int i = 0; i < def->requirement_count; i++) {
        ExpeditionsTraitText(trait, sizeof(trait), &def->requirements[i]);
        LocFormat1(line, sizeof(line), "HODA_EXPEDITION_NEEDS", trait);
        UiDrawTextLeftMiddle(v->body, line, (Rectangle){text.x, y, text.width, 20.0f}, 17.0f, body);
        y += 20.0f;
    }
    ExpeditionsTraitText(trait, sizeof(trait), &def->bonus);
    LocFormat1(line, sizeof(line), "HODA_EXPEDITION_BONUS", trait);
    UiDrawTextLeftMiddle(v->body, line, (Rectangle){text.x, y, text.width, 20.0f}, 17.0f, EXPEDITIONS_GOLD);
}

static Button *ExpeditionsRowButton(ExpeditionsView *v, int def_index, ExpeditionRowState state) {
    Button *button = &v->rows[def_index];
    const Button *source = state == EXPEDITION_ROW_READY  ? &v->claim_template
                           : state == EXPEDITION_ROW_AWAY ? &v->recall_template
                                                          : &v->send_template;
    bool armed = button->armed;
    *button = *source;
    button->armed = armed;
    return button;
}

typedef enum {
    EXPEDITION_ROW_PRESS_NONE,
    EXPEDITION_ROW_PRESS_SEND,
    EXPEDITION_ROW_PRESS_RECALL,
    EXPEDITION_ROW_PRESS_CLAIM,
} ExpeditionRowPress;

static ExpeditionRowPress ExpeditionsDrawRow(
    ExpeditionsView *v,
    const Heroes *heroes,
    const Player *p,
    int def_index,
    Rectangle row,
    Rectangle clip,
    bool interactive,
    long long now
) {
    const ExpeditionDef *def = ExpeditionAt(def_index);
    ExpeditionRowState state = ExpeditionsRowState(p, def_index, now);
    bool locked = state == EXPEDITION_ROW_LOCKED;
    UiDrawTexture(GuiTex(GUI_MISSION_ROW), row, locked ? (Color){150, 150, 150, 255} : WHITE);
    Rectangle art = {
        row.x + 8.0f,
        row.y + (row.height - EXPEDITIONS_ART_HEIGHT) * 0.5f,
        EXPEDITIONS_ART_WIDTH,
        EXPEDITIONS_ART_HEIGHT
    };
    ExpeditionsDrawArt(def, art, locked);
    if (locked) {
        char level[64];
        LocFormat1(level, sizeof(level), "HODA_EXPEDITION_LEVEL_LOCK", TextFormat("%d", def->level));
        UiDrawTextCentered(v->font, level, art, 22.0f, EXPEDITIONS_TEXT);
    }

    float text_x = art.x + art.width + 14.0f;
    float action_x = row.x + row.width - EXPEDITIONS_ACTION_WIDTH - 10.0f;
    float reward_x = action_x - EXPEDITIONS_REWARD_WIDTH;
    Rectangle text = {text_x, row.y + 6.0f, reward_x - text_x - 10.0f, row.height - 12.0f};
    UiDrawTextLeftMiddle(
        v->font, LocGet(def->name_key), (Rectangle){text.x, text.y, text.width, 28.0f}, 23.0f, EXPEDITIONS_TEXT
    );

    int active = ExpeditionsFind(&p->expeditions, def_index);
    bool great = active >= 0 && ExpeditionActiveGreatSuccess(p, &p->expeditions.active[active]);
    if (active >= 0) {
        const PlayerExpedition *expedition = &p->expeditions.active[active];
        ExpeditionsDrawPartyPortraits(
            heroes, p, expedition, (Rectangle){text.x, text.y + 32.0f, text.width, EXPEDITIONS_MINI_PORTRAIT}
        );
        Rectangle bar = {text.x, text.y + text.height - 24.0f, text.width, 22.0f};
        ExpeditionsDrawProgressBar(v, expedition, bar, now);
        if (great)
            UiDrawTextRight(
                v->font,
                LocGet("HODA_EXPEDITION_GREAT_SUCCESS"),
                (Rectangle){text.x, text.y + 32.0f, text.width, EXPEDITIONS_MINI_PORTRAIT},
                18.0f,
                EXPEDITIONS_GOLD
            );
    } else {
        ExpeditionsDrawRowDetails(v, def, text, locked);
    }
    ExpeditionsDrawRewards(
        v, def, (Rectangle){reward_x, row.y + 10.0f, EXPEDITIONS_REWARD_WIDTH - 10.0f, row.height - 20.0f}, great
    );

    if (locked) return EXPEDITION_ROW_PRESS_NONE;
    Rectangle button = {
        action_x,
        row.y + (row.height - EXPEDITIONS_BUTTON_HEIGHT) * 0.5f,
        EXPEDITIONS_ACTION_WIDTH,
        EXPEDITIONS_BUTTON_HEIGHT
    };
    if (state == EXPEDITION_ROW_NO_SLOT) {
        UiDrawTexture(GuiTex(GUI_BTN_GREY), button, (Color){120, 120, 120, 255});
        QuestsDrawFit(
            v->body,
            LocGet("HODA_EXPEDITION_NO_SLOT"),
            UiRect(button, 0.5f, 0.5f, 0.86f, 1.0f),
            16.0f,
            EXPEDITIONS_TEXT_DIM
        );
        return EXPEDITION_ROW_PRESS_NONE;
    }
    bool visible = CheckCollisionPointRec(UiMouse(), clip);
    Button *control = ExpeditionsRowButton(v, def_index, state);
    if (!ButtonDrawEx(control, button, interactive && visible && !v->board_scroll.drag_scrolled))
        return EXPEDITION_ROW_PRESS_NONE;
    if (state == EXPEDITION_ROW_READY) return EXPEDITION_ROW_PRESS_CLAIM;
    return state == EXPEDITION_ROW_AWAY ? EXPEDITION_ROW_PRESS_RECALL : EXPEDITION_ROW_PRESS_SEND;
}

static int ExpeditionsCandidateRank(const ExpeditionDef *def, const Character *ch) {
    bool wanted = ExpeditionTraitMatches(&def->bonus, ch);
    for (int i = 0; i < def->requirement_count && !wanted; i++)
        wanted = ExpeditionTraitMatches(&def->requirements[i], ch);
    return (wanted ? 0 : 1 << 24) + ExpeditionHeroWorth(ch);
}

static void ExpeditionsBuildCandidates(ExpeditionsView *v, const Player *p) {
    if (v->candidate_capacity < p->roster_count) {
        MemFree(v->candidates);
        v->candidate_capacity = p->roster_capacity > p->roster_count ? p->roster_capacity : p->roster_count;
        v->candidates = (int *)MemAlloc((unsigned int)((size_t)v->candidate_capacity * sizeof(int)));
    }
    const ExpeditionDef *def = ExpeditionAt(v->selected);
    v->candidate_count = 0;
    for (int i = 0; i < p->roster_count; i++) {
        if (!ExpeditionHeroAvailable(p, i)) continue;
        int slot = v->candidate_count++;
        int rank = ExpeditionsCandidateRank(def, &p->roster[i]);
        while (slot > 0 && ExpeditionsCandidateRank(def, &p->roster[v->candidates[slot - 1]]) > rank) {
            v->candidates[slot] = v->candidates[slot - 1];
            slot--;
        }
        v->candidates[slot] = i;
    }
}

static void ExpeditionsOpenParty(ExpeditionsView *v, const Player *p, int def_index) {
    v->mode = EXPEDITIONS_MODE_PARTY;
    v->selected = def_index;
    v->party_count = ExpeditionAutoFill(p, def_index, v->party);
    GoalsScrollReset(&v->roster_scroll);
    ExpeditionsBuildCandidates(v, p);
}

static int ExpeditionsPartySlotOf(const ExpeditionsView *v, int roster_index) {
    for (int i = 0; i < v->party_count; i++)
        if (v->party[i] == roster_index) return i;
    return EXPEDITION_NONE;
}

static void ExpeditionsTogglePartyHero(ExpeditionsView *v, int roster_index) {
    const ExpeditionDef *def = ExpeditionAt(v->selected);
    int slot = ExpeditionsPartySlotOf(v, roster_index);
    if (slot >= 0) {
        for (int i = slot; i + 1 < v->party_count; i++) v->party[i] = v->party[i + 1];
        v->party_count--;
        GuiSoundPlay(SND_UI_TAB);
    } else if (v->party_count < def->party) {
        v->party[v->party_count++] = roster_index;
        GuiSoundPlay(SND_UI_TAB);
    } else {
        GuiSoundPlay(SND_STORE_ERROR);
    }
}

static void
ExpeditionsDrawTraitCheck(const ExpeditionsView *v, const char *text, bool met, Rectangle line, Color met_color) {
    UiDrawTextLeftMiddle(
        v->body,
        met ? "+" : "-",
        (Rectangle){line.x, line.y, 14.0f, line.height},
        20.0f,
        met ? met_color : EXPEDITIONS_BAD
    );
    UiDrawTextLeftMiddle(
        v->body,
        text,
        (Rectangle){line.x + 16.0f, line.y, line.width - 16.0f, line.height},
        18.0f,
        met ? met_color : EXPEDITIONS_TEXT_DIM
    );
}

static void ExpeditionsDrawPartyHeader(const ExpeditionsView *v, const Player *p, Rectangle header) {
    const ExpeditionDef *def = ExpeditionAt(v->selected);
    Rectangle art = {header.x, header.y, EXPEDITIONS_HEADER_ART_WIDTH, header.height};
    ExpeditionsDrawArt(def, art, false);
    float x = art.x + art.width + 16.0f;
    float rewards_x = header.x + header.width - EXPEDITIONS_REWARD_WIDTH;
    Rectangle text = {x, header.y, rewards_x - x - 12.0f, header.height};
    UiDrawTextLeftMiddle(
        v->font, LocGet(def->name_key), (Rectangle){text.x, text.y, text.width, 30.0f}, 26.0f, EXPEDITIONS_TEXT
    );
    HeroesDrawRich(
        v->body,
        LocGet(def->description_key),
        (Rectangle){text.x, text.y + 32.0f, text.width, 40.0f},
        16.0f,
        EXPEDITIONS_TEXT_DIM
    );
    char trait[EXPEDITIONS_TEXT_CAPACITY], line[EXPEDITIONS_TEXT_CAPACITY];
    float y = text.y + 74.0f;
    for (int i = 0; i < def->requirement_count; i++) {
        const Character *members[EXPEDITION_PARTY_CAPACITY];
        int known = ExpeditionPartyCharacters(p, v->party, v->party_count, members);
        ExpeditionsTraitText(trait, sizeof(trait), &def->requirements[i]);
        LocFormat1(line, sizeof(line), "HODA_EXPEDITION_NEEDS", trait);
        ExpeditionsDrawTraitCheck(
            v,
            line,
            ExpeditionTraitMet(&def->requirements[i], members, known),
            (Rectangle){text.x, y, text.width, 20.0f},
            EXPEDITIONS_GOOD
        );
        y += 20.0f;
    }
    bool great = ExpeditionPartyGreatSuccess(p, v->selected, v->party, v->party_count);
    ExpeditionsTraitText(trait, sizeof(trait), &def->bonus);
    LocFormat1(line, sizeof(line), "HODA_EXPEDITION_BONUS", trait);
    ExpeditionsDrawTraitCheck(v, line, great, (Rectangle){text.x, y, text.width, 20.0f}, EXPEDITIONS_GOLD);
    ExpeditionsDrawRewards(v, def, (Rectangle){rewards_x, header.y, EXPEDITIONS_REWARD_WIDTH, header.height}, great);
}

static bool ExpeditionsDrawPartySlots(ExpeditionsView *v, const Heroes *heroes, Rectangle row, bool interactive) {
    const ExpeditionDef *def = ExpeditionAt(v->selected);
    bool changed = false;
    float width = def->party * (EXPEDITIONS_SLOT_SIZE + 8.0f) - 8.0f;
    Rectangle slot = {row.x + (row.width - width) * 0.5f, row.y, EXPEDITIONS_SLOT_SIZE, EXPEDITIONS_SLOT_SIZE};
    for (int i = 0; i < def->party; i++) {
        DrawRectangleRec(slot, (Color){0, 0, 0, 160});
        DrawRectangleLinesEx(slot, 2.0f, EXPEDITIONS_ART_BORDER);
        if (i < v->party_count && v->party[i] < heroes->card_count) {
            UiDrawTexture(heroes->cards[v->party[i]].portrait, UiRect(slot, 0.5f, 0.5f, 0.92f, 0.92f), WHITE);
            bool released =
                interactive && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(UiMouse(), slot);
            if (released && !changed) {
                ExpeditionsTogglePartyHero(v, v->party[i]);
                changed = true;
            }
        } else {
            UiDrawTextCentered(v->font, "+", slot, 34.0f, EXPEDITIONS_TEXT_DIM);
        }
        slot.x += EXPEDITIONS_SLOT_SIZE + 8.0f;
    }
    return changed;
}

static void ExpeditionsDrawCandidateGrid(
    ExpeditionsView *v, const Heroes *heroes, const Player *p, Rectangle grid, bool interactive
) {
    const ExpeditionDef *def = ExpeditionAt(v->selected);
    float cell = grid.width / EXPEDITIONS_GRID_COLUMNS;
    int rows = (v->candidate_count + EXPEDITIONS_GRID_COLUMNS - 1) / EXPEDITIONS_GRID_COLUMNS;
    GoalsScrollUpdate(&v->roster_scroll, grid, rows * cell);
    GoalsBeginClip(grid);
    int clicked = EXPEDITION_NONE;
    for (int i = 0; i < v->candidate_count; i++) {
        Rectangle tile = {
            grid.x + (i % EXPEDITIONS_GRID_COLUMNS) * cell,
            grid.y + (i / EXPEDITIONS_GRID_COLUMNS) * cell - v->roster_scroll.offset,
            cell,
            cell
        };
        if (tile.y + tile.height < grid.y || tile.y > grid.y + grid.height) continue;
        int index = v->candidates[i];
        if (index >= heroes->card_count) continue;
        const HeroCard *card = &heroes->cards[index];
        RosterCardState state = {
            .progress = card->xp_frac,
            .maxed = card->xp_frac >= 1.0f,
            .selected = ExpeditionsPartySlotOf(v, index) >= 0,
            .fortified = FortifyHasChildren(card->ch),
        };
        RosterDrawCard(card, tile, &state);
        if (ExpeditionTraitMatches(&def->bonus, &p->roster[index]))
            ExpeditionsDrawFit(GuiTex(GUI_MISSION_STAR), UiRect(tile, 0.92f, 0.06f, 0.3f, 0.3f), WHITE);
        bool over = CheckCollisionPointRec(UiMouse(), tile) && CheckCollisionPointRec(UiMouse(), grid);
        if (interactive && over && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && !v->roster_scroll.drag_scrolled)
            clicked = index;
    }
    EndScissorMode();
    if (v->candidate_count == 0)
        UiDrawTextCentered(v->body, LocGet("HODA_EXPEDITION_SQUAD_HINT"), grid, 20.0f, EXPEDITIONS_TEXT_DIM);
    if (clicked >= 0) ExpeditionsTogglePartyHero(v, clicked);
}

static void ExpeditionsSend(ExpeditionsView *v, Player *p) {
    long long now = ServerNow();
    cJSON *action = ExpeditionStartAction(p, v->selected, v->party, v->party_count);
    if (!ExpeditionStart(p, v->selected, v->party, v->party_count, now)) {
        cJSON_Delete(action);
        GuiSoundPlay(SND_STORE_ERROR);
        return;
    }
    ProgressionSave(p);
    ServerActionSubmit(action);
    v->mode = EXPEDITIONS_MODE_BOARD;
    v->selected = EXPEDITION_NONE;
    v->party_count = 0;
}

static void ExpeditionsDrawParty(
    ExpeditionsView *v, const Heroes *heroes, Player *p, Rectangle content, Rectangle footer, bool interactive
) {
    Rectangle header = {content.x, content.y, content.width, EXPEDITIONS_HEADER_HEIGHT};
    ExpeditionsDrawPartyHeader(v, p, header);
    Rectangle slots = {content.x, header.y + header.height + 10.0f, content.width, EXPEDITIONS_SLOT_SIZE};
    ExpeditionsDrawPartySlots(v, heroes, slots, interactive);
    Rectangle grid = {
        content.x,
        slots.y + slots.height + 10.0f,
        content.width,
        content.y + content.height - (slots.y + slots.height + 10.0f)
    };
    ExpeditionsDrawCandidateGrid(v, heroes, p, grid, interactive);

    DrawRectangleRec(footer, EXPEDITIONS_SHADE);
    float button_width = 170.0f;
    Rectangle cancel = {footer.x + 8.0f, footer.y + 4.0f, button_width, footer.height - 8.0f};
    Rectangle dispatch = {footer.x + footer.width - button_width - 8.0f, cancel.y, button_width, cancel.height};
    Rectangle fill = {dispatch.x - button_width - 10.0f, cancel.y, button_width, cancel.height};
    UiDrawTextCentered(
        v->body,
        LocGet("HODA_EXPEDITION_SQUAD_HINT"),
        (Rectangle){cancel.x + cancel.width, footer.y, fill.x - cancel.x - cancel.width, footer.height},
        16.0f,
        EXPEDITIONS_TEXT_DIM
    );
    if (ButtonDrawEx(&v->cancel, cancel, interactive)) {
        v->mode = EXPEDITIONS_MODE_BOARD;
        return;
    }
    if (ButtonDrawEx(&v->auto_fill, fill, interactive)) v->party_count = ExpeditionAutoFill(p, v->selected, v->party);
    bool ready = ExpeditionPartyValid(p, v->selected, v->party, v->party_count) && ExpeditionCanStart(p, v->selected);
    if (!ready) {
        UiDrawTexture(GuiTex(GUI_BTN_GOLD), dispatch, (Color){110, 110, 110, 255});
        QuestsDrawFit(v->font, LocGet("HODA_EXPEDITION_SEND"), dispatch, dispatch.height * 0.4f, EXPEDITIONS_TEXT_DIM);
    } else if (ButtonDrawEx(&v->dispatch, dispatch, interactive)) {
        ExpeditionsSend(v, p);
    }
}

static void ExpeditionsShowReward(ExpeditionsView *v) {
    v->reveal_pending = ExpeditionRewardHasItems(&v->reward);
    v->dialog = v->reveal_pending ? EXPEDITIONS_DIALOG_NONE : EXPEDITIONS_DIALOG_REWARD;
}

bool ExpeditionsRevealRewards(ExpeditionsView *v, Store *store) {
    if (!v->reveal_pending) return false;
    v->reveal_pending = false;
    v->dialog = EXPEDITIONS_DIALOG_REWARD;
    const ExpeditionReward *r = &v->reward;
    bool gear_new[EXPEDITION_REWARD_ITEM_CAPACITY];
    for (int i = 0; i < r->gear_count; i++) gear_new[i] = true;
    return StoreRevealRewards(
        store, r->heroes, r->hero_count, r->gear, gear_new, r->gear_count, r->runes, r->rune_new, r->rune_count
    );
}

static void ExpeditionsBeginClaim(ExpeditionsView *v, Heroes *heroes, Player *p, const int *defs, int count) {
    long long now = ServerNow();
    ExpeditionClaimCheck check = ExpeditionCheckClaim(p, defs, count, now);
    if (check == EXPEDITION_CLAIM_ROSTER_FULL) {
        ExpeditionsShowMessage(v, LocGet("HODA_EXPEDITION_ROSTER_FULL"));
        return;
    }
    if (check != EXPEDITION_CLAIM_OK) return;
    int great = 0, ready = 0;
    for (int i = 0; i < count; i++) {
        int active = ExpeditionsFind(&p->expeditions, defs[i]);
        if (active < 0 || !ExpeditionReady(&p->expeditions.active[active], now)) continue;
        ready++;
        great += ExpeditionActiveGreatSuccess(p, &p->expeditions.active[active]);
    }
    if (ServerActionsActive()) {
        v->claim_waiting = true;
        v->claim_expeditions = ready;
        v->claim_great = great;
        ProgressionSave(p);
        ServerActionSubmitTagged(ExpeditionClaimAction(defs, count), SERVER_REPLY_EXPEDITION_CLAIM);
        return;
    }
    v->reward = ExpeditionClaim(p, defs, count, now);
    if (!HeroesSyncRoster(heroes, p)) TraceLog(LOG_WARNING, "Expeditions: granted heroes cannot sync to Heroes");
    ProgressionSave(p);
    ExpeditionsShowReward(v);
}

void ExpeditionsTakeClaimReply(ExpeditionsView *v, Heroes *heroes, const Player *p, const cJSON *reply) {
    v->claim_waiting = false;
    if (!reply) {
        ExpeditionsShowMessage(v, LocGet("HODA_EXPEDITION_CLAIM_FAILED"));
        return;
    }
    if (!HeroesSyncRoster(heroes, p)) TraceLog(LOG_WARNING, "Expeditions: granted heroes cannot sync to Heroes");
    v->reward = ExpeditionRewardFromReply(p, reply, v->claim_expeditions, v->claim_great);
    ExpeditionsShowReward(v);
}

static void ExpeditionsRecall(ExpeditionsView *v, Player *p) {
    cJSON *action = ExpeditionRecallAction(v->selected);
    if (!ExpeditionRecall(p, v->selected)) {
        cJSON_Delete(action);
        return;
    }
    ProgressionSave(p);
    ServerActionSubmit(action);
    v->selected = EXPEDITION_NONE;
}

static void ExpeditionsDrawFooter(ExpeditionsView *v, Heroes *heroes, Player *p, Rectangle footer, bool interactive) {
    DrawRectangleRec(footer, EXPEDITIONS_SHADE);
    int slots = ExpeditionSlotsUnlocked(p->level);
    char text[EXPEDITIONS_TEXT_CAPACITY], next[EXPEDITIONS_TEXT_CAPACITY] = "";
    LocFormat2(
        text, sizeof(text), "HODA_EXPEDITION_SLOTS", TextFormat("%d", p->expeditions.count), TextFormat("%d", slots)
    );
    int next_level = ExpeditionNextSlotLevel(p->level);
    if (next_level > 0) LocFormat1(next, sizeof(next), "HODA_EXPEDITION_NEXT_SLOT", TextFormat("%d", next_level));
    UiDrawTextLeftMiddle(
        v->font, text, (Rectangle){footer.x + 16.0f, footer.y, 260.0f, footer.height}, 21.0f, EXPEDITIONS_TEXT
    );
    UiDrawTextLeftMiddle(
        v->body, next, (Rectangle){footer.x + 280.0f, footer.y, 300.0f, footer.height}, 18.0f, EXPEDITIONS_TEXT_DIM
    );
    int defs[EXPEDITION_SLOT_CAPACITY];
    int ready = ExpeditionReadyDefs(p, ServerNow(), defs);
    if (ready < 1) return;
    Rectangle button = {footer.x + footer.width - 200.0f, footer.y + 5.0f, 190.0f, footer.height - 10.0f};
    if (ButtonDrawEx(&v->collect_all, button, interactive)) ExpeditionsBeginClaim(v, heroes, p, defs, ready);
}

static void ExpeditionsDrawBoard(
    ExpeditionsView *v, Heroes *heroes, Player *p, Rectangle content, Rectangle footer, bool interactive
) {
    long long now = ServerNow();
    float stride = EXPEDITIONS_ROW_HEIGHT + EXPEDITIONS_ROW_GAP;
    GoalsScrollUpdate(&v->board_scroll, content, ExpeditionCount() * stride - EXPEDITIONS_ROW_GAP);
    GoalsBeginClip(content);
    int pressed_def = EXPEDITION_NONE;
    ExpeditionRowPress press = EXPEDITION_ROW_PRESS_NONE;
    for (int i = 0; i < ExpeditionCount(); i++) {
        float y = content.y + i * stride - v->board_scroll.offset;
        if (y + EXPEDITIONS_ROW_HEIGHT < content.y || y > content.y + content.height) continue;
        Rectangle row = {content.x, y, content.width, EXPEDITIONS_ROW_HEIGHT};
        ExpeditionRowPress row_press = ExpeditionsDrawRow(v, heroes, p, i, row, content, interactive, now);
        if (row_press != EXPEDITION_ROW_PRESS_NONE) {
            press = row_press;
            pressed_def = i;
        }
    }
    EndScissorMode();
    ExpeditionsDrawFooter(v, heroes, p, footer, interactive);
    if (press == EXPEDITION_ROW_PRESS_SEND) {
        ExpeditionsOpenParty(v, p, pressed_def);
    } else if (press == EXPEDITION_ROW_PRESS_RECALL) {
        v->selected = pressed_def;
        v->dialog = EXPEDITIONS_DIALOG_RECALL;
        GuiSoundPlay(SND_STORE_ERROR);
    } else if (press == EXPEDITION_ROW_PRESS_CLAIM) {
        ExpeditionsBeginClaim(v, heroes, p, &pressed_def, 1);
    }
}

static void ExpeditionsDrawRewardItems(const ExpeditionsView *v, const Heroes *heroes, Rectangle area) {
    const ExpeditionReward *r = &v->reward;
    int count = r->gear_count + r->rune_count + r->hero_count;
    if (count == 0) return;
    float size = fminf(EXPEDITIONS_REWARD_ICON_SIZE, (area.width - (count - 1) * 8.0f) / count);
    float width = count * size + (count - 1) * 8.0f;
    Rectangle box = {area.x + (area.width - width) * 0.5f, area.y, size, size};
    for (int i = 0; i < r->hero_count; i++, box.x += size + 8.0f) {
        if (r->heroes[i] >= heroes->card_count) continue;
        UiDrawTexture(heroes->cards[r->heroes[i]].portrait, box, WHITE);
        if (r->hero_new[i])
            UiDrawTextCentered(
                v->font,
                LocGet("GALLERY_NEW"),
                (Rectangle){box.x, box.y + box.height, box.width, 20.0f},
                17.0f,
                EXPEDITIONS_GOLD
            );
    }
    for (int i = 0; i < r->gear_count; i++, box.x += size + 8.0f) {
        const GearDef *def = GearDefAt(r->gear[i]);
        if (!def) continue;
        ExpeditionsDrawFit(GearIconLoad(def), UiRect(box, 0.5f, 0.5f, 0.78f, 0.78f), WHITE);
        ExpeditionsDrawFit(GearRarityFrame(def->rarity), box, WHITE);
    }
    for (int i = 0; i < r->rune_count; i++, box.x += size + 8.0f) {
        const RuneDef *def = RuneDefAt(r->runes[i]);
        if (def) ExpeditionsDrawFit(RuneIconLoad(def->icon), box, WHITE);
    }
}

static void ExpeditionsDrawRewardDialog(ExpeditionsView *v, const Heroes *heroes) {
    const ExpeditionReward *r = &v->reward;
    const char *title =
        LocGet(r->great_successes > 0 ? "HODA_EXPEDITION_GREAT_SUCCESS" : "HODA_EXPEDITION_COMPLETE_TITLE");
    Rectangle popup = MessagePopupDrawMessage(GuiTex(GUI_STORE_OVERLAY), v->font, v->body, title, "");
    Rectangle content = MessagePopupContent(popup);
    float line_height = 34.0f;
    Rectangle currencies = UiRect(content, 0.5f, 0.3f, 0.9f, line_height / content.height);
    char gold[32], gems[32], xp[64];
    snprintf(gold, sizeof(gold), "+%d", r->gold);
    snprintf(gems, sizeof(gems), "+%d", r->gems);
    LocFormat1(xp, sizeof(xp), "HODA_EXPEDITION_HERO_XP", TextFormat("%d", r->hero_xp));
    const struct {
        GuiId icon;
        const char *text;
        bool shown;
    } entries[] = {
        {GUI_MISSION_GOLD, gold, r->gold > 0},
        {GUI_MISSION_GEM, gems, r->gems > 0},
        {GUI_HUD_XP, xp, r->hero_xp > 0},
    };
    int shown = 0;
    for (int i = 0; i < (int)(sizeof(entries) / sizeof(entries[0])); i++) shown += entries[i].shown;
    float slot = shown > 0 ? currencies.width / shown : 0.0f;
    float x = currencies.x;
    for (int i = 0; i < (int)(sizeof(entries) / sizeof(entries[0])); i++) {
        if (!entries[i].shown) continue;
        float width = line_height + 6.0f + MeasureTextEx(v->font, entries[i].text, line_height * 0.9f, 1.0f).x;
        Rectangle line = {x + (slot - width) * 0.5f, currencies.y, width, line_height};
        ExpeditionsDrawCurrency(v, entries[i].icon, entries[i].text, line);
        x += slot;
    }
    Rectangle items = {content.x, currencies.y + line_height + 14.0f, content.width, EXPEDITIONS_REWARD_ICON_SIZE};
    ExpeditionsDrawRewardItems(v, heroes, items);
    if (ButtonDraw(&v->ok, MessagePopupOkButton(popup)) || IsKeyPressed(KEY_ENTER)) v->dialog = EXPEDITIONS_DIALOG_NONE;
}

static void ExpeditionsDrawDialogs(ExpeditionsView *v, const Heroes *heroes, Player *p) {
    if (v->dialog == EXPEDITIONS_DIALOG_REWARD) {
        ExpeditionsDrawRewardDialog(v, heroes);
    } else if (v->dialog == EXPEDITIONS_DIALOG_MESSAGE) {
        if (MessagePopupDrawAlert(
                GuiTex(GUI_STORE_OVERLAY), v->font, v->body, LocGet("HODA_EXPEDITIONS_TITLE"), v->message, &v->ok
            ))
            v->dialog = EXPEDITIONS_DIALOG_NONE;
    } else if (v->dialog == EXPEDITIONS_DIALOG_RECALL) {
        MessagePopupChoice choice = MessagePopupDrawChoice(
            GuiTex(GUI_STORE_OVERLAY),
            v->font,
            v->body,
            LocGet("HODA_EXPEDITION_RECALL_TITLE"),
            LocGet("HODA_EXPEDITION_RECALL_MESSAGE"),
            &v->no,
            &v->yes
        );
        if (choice == MESSAGE_POPUP_CHOICE_ACCEPT) ExpeditionsRecall(v, p);
        if (choice != MESSAGE_POPUP_CHOICE_NONE) v->dialog = EXPEDITIONS_DIALOG_NONE;
    }
}

static bool ExpeditionsScrimClosed(ExpeditionsView *v, Rectangle panel, Rectangle close, bool interactive) {
    Vector2 mouse = UiMouse();
    Rectangle top_ui = {0.0f, 0.0f, UiWidth(), 96.0f};
    bool scrim_hover = !CheckCollisionPointRec(mouse, panel) && !CheckCollisionPointRec(mouse, close) &&
                       !CheckCollisionPointRec(mouse, top_ui);
    bool closed = false;
    if (!interactive) {
        v->scrim_armed = false;
    } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        v->scrim_armed = scrim_hover;
    } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        closed = v->scrim_armed && scrim_hover;
        v->scrim_armed = false;
    }
    return closed;
}

ExpeditionsAction ExpeditionsDraw(ExpeditionsView *v, Heroes *heroes, Player *p) {
    Rectangle root = UiRoot();
    DrawRectangle(0, 0, root.width, root.height, (Color){0, 0, 0, 170});
    Rectangle panel = UiRect(root, 0.5f, 0.6f, EXPEDITIONS_PANEL_WIDTH, EXPEDITIONS_PANEL_HEIGHT);
    UiDrawTexture(GuiTex(GUI_QUESTS_BG), panel, WHITE);
    Rectangle inner = {
        panel.x + EXPEDITIONS_INSET_X,
        panel.y + EXPEDITIONS_CONTENT_TOP,
        panel.width - EXPEDITIONS_INSET_X * 2.0f,
        panel.height - EXPEDITIONS_CONTENT_TOP - EXPEDITIONS_FOOTER_BOTTOM
    };
    ExpeditionsDrawCover(ExpeditionTexture(EXPEDITIONS_BOARD_ART), inner, (Color){90, 90, 90, 255});
    UiDrawTextCentered(
        v->font,
        LocGet("HODA_EXPEDITIONS_TITLE"),
        (Rectangle){0.0f, panel.y + 38.0f, UiWidth(), 0.0f},
        42.0f,
        EXPEDITIONS_TEXT
    );

    bool interactive = !UiInputBlocked() && v->dialog == EXPEDITIONS_DIALOG_NONE && !v->claim_waiting;
    Rectangle footer = {
        inner.x, inner.y + inner.height - EXPEDITIONS_FOOTER_HEIGHT, inner.width, EXPEDITIONS_FOOTER_HEIGHT
    };
    Rectangle content = {inner.x, inner.y, inner.width, inner.height - EXPEDITIONS_FOOTER_HEIGHT - 8.0f};
    if (v->mode == EXPEDITIONS_MODE_PARTY)
        ExpeditionsDrawParty(v, heroes, p, content, footer, interactive);
    else
        ExpeditionsDrawBoard(v, heroes, p, content, footer, interactive);

    float close_size = 61.0f;
    Rectangle close = {
        panel.x + panel.width - close_size * 1.25f, panel.y + panel.height * 0.07f, close_size, close_size
    };
    ExpeditionsAction action = EXPEDITIONS_NONE;
    if (ButtonDrawEx(&v->close, close, interactive)) action = EXPEDITIONS_CLOSE;
    if (action == EXPEDITIONS_NONE && ExpeditionsScrimClosed(v, panel, close, interactive)) {
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        action = EXPEDITIONS_CLOSE;
    }
    ExpeditionsDrawDialogs(v, heroes, p);
    return action;
}
