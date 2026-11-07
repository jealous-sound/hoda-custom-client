
#define HEROES_GEAR_COLUMN_WIDTH 64.0f
#define HEROES_GEAR_BUTTON_X 284.0f
#define HEROES_DRAG_THRESHOLD 51.0f
#define ROSTER_PREFAB_WIDTH 2030.0f
#define ROSTER_PREFAB_HEIGHT 938.0f
#define ROSTER_PREFAB_TOP 72.0f
#define ROSTER_PANEL_WIDTH 278.0f
#define ROSTER_GRID_COLS 2
#define ROSTER_FILTER_ROW 65.0f
#define ROSTER_SCROLL_STEP 50.0f

typedef enum {
    HEROES_NONE,
    HEROES_QUEST,
    HEROES_PVP,
    HEROES_CONSUME,
    HEROES_FORTIFY,
    HEROES_RUNES,
    HEROES_GALLERY,
    HEROES_SQUAD
} HeroesButton;

typedef struct {
    const Character *ch;
    Texture2D portrait;
    Texture2D coin;
    Texture2D init_icon;
    const char *name;
    const char *rarity;
    const char *speed;
    Color rarity_color;
    char info[320];
    char description[1280];
    int level, max_level;
    int power, health, crit;
    float xp_frac;
    float desc_fs;
} HeroCard;

typedef enum {
    ROSTER_FILTER_ALL,
    ROSTER_FILTER_NEWEST,
    ROSTER_FILTER_RED,
    ROSTER_FILTER_BLUE,
    ROSTER_FILTER_WHITE,
    ROSTER_FILTER_BLACK,
    ROSTER_FILTER_LARGE,
    ROSTER_FILTER_COMMON,
    ROSTER_FILTER_UNCOMMON,
    ROSTER_FILTER_RARE,
    ROSTER_FILTER_TUTORIAL,
    ROSTER_FILTER_COUNT
} RosterFilter;

#define ROSTER_FILTER_PICKABLE_COUNT ROSTER_FILTER_TUTORIAL

static const char *const ROSTER_FILTER_KEYS[ROSTER_FILTER_COUNT] = {
    "ROSTER_FILTER_ALL_HEROES",
    "ROSTER_FILTER_NEWEST",
    "ROSTER_FILTER_RED_FACTION",
    "ROSTER_FILTER_BLUE_FACTION",
    "ROSTER_FILTER_WHITE_FACTION",
    "ROSTER_FILTER_BLACK_FACTION",
    "ROSTER_FILTER_LARGE_CREATURES",
    "ROSTER_FILTER_COMMON",
    "ROSTER_FILTER_UNCOMMON",
    "ROSTER_FILTER_RARE",
    "ROSTER_FILTER_TUTORIAL",
};

typedef struct {
    int order[PLAYER_ROSTER_CAPACITY];
    int order_count;
    bool sort_reversed;
    RosterFilter filter;
    bool filter_open;
    float scroll;
    bool pin_squad;
    int pinned_squad;
    int pinned_units[SQUAD_SLOT_COUNT];
    Button category, sort;
} Roster;

typedef struct {
    float progress;
    bool maxed;
    bool grey_bar;
    Texture2D unlock_icon;
    bool in_squad;
    bool in_use;
    bool selected;
    bool fortified;
} RosterCardState;

typedef void (*RosterCardDrawFn)(const HeroCard *card, Rectangle cell, int roster_index, const void *context);

typedef struct {
    Button gray;
    Button gold;
    Button gear;
    Button train;
    Button fortify;
    Button runes;
    Button gallery;
    Button squad;

    HeroCard *cards;
    int card_count;
    int card_capacity;
    int selected;
    Roster roster;

    float tab_scroll;
    float tab_drag_start, tab_scroll_start;
    bool tab_dragging;
    bool tab_dragged;
    bool tab_snap;

    int press_card;
    Vector2 press_pos;
    bool details_close_armed;
    int drag_request;
    int drag_icon;
} Heroes;

bool GearViewIsOpen(void);

int HeroesTakeDragRequest(Heroes *h) {
    int req = h->drag_request;
    h->drag_request = -1;
    return req;
}

static void HeroesFormat2(char *dst, int cap, const char *fmt, const char *a0, const char *a1) {
    int n = 0;
    for (const char *c = fmt; *c && n < cap - 1;) {
        if (c[0] == '{' && (c[1] == '0' || c[1] == '1') && c[2] == '}') {
            const char *src = c[1] == '0' ? a0 : a1;
            while (*src && n < cap - 1) dst[n++] = *src++;
            c += 3;
        } else {
            dst[n++] = *c++;
        }
    }
    dst[n] = '\0';
}

static void HeroesAppendList(char *dst, int cap, const char *item) {
    int len = (int)strlen(dst);
    snprintf(dst + len, cap - len, "%s%s", dst[0] ? ", " : "", item);
}

static void HeroesAppendSentence(char *dst, int cap, const char *sentence) {
    if (!sentence[0]) return;
    int len = (int)strlen(dst);
    snprintf(dst + len, cap - len, "%s%s", len ? "  " : "", sentence);
}

static void HeroCardBuild(HeroCard *card, const Character *ch) {
    card->ch = ch;
    card->coin = GuiTex(ch->giant || ch->faction == FACTION_ALL ? GUI_COIN_LARGE : GUI_COIN_LARGE + (int)ch->faction);

    card->name = CharacterName(ch);

    static const char *const rarity_keys[] = {
        "", "RARITY_COMMON", "RARITY_UNCOMMON", "RARITY_RARE", "RARITY_EPIC", "RARITY_LEGENDARY", "RARITY_MYTHIC"
    };
    static const Color rarity_colors[] = {
        {255, 255, 255, 255},
        {195, 141, 77, 255},
        {173, 171, 163, 255},
        {255, 242, 154, 255},
        {236, 78, 68, 255},
        {182, 252, 240, 255},
        {255, 0, 255, 255},
    };
    card->rarity = ch->rarity == RARITY_MYTHIC ? LocGet("HODA_RARITY_MYTHIC") : LocGet(rarity_keys[ch->rarity]);
    card->rarity_color = rarity_colors[ch->rarity];

    card->speed = LocGet(
        ch->speed == SPEED_QUICK  ? "INITIATIVE_FAST"
        : ch->speed == SPEED_SLOW ? "INITIATIVE_SLOW"
                                  : "INITIATIVE_MEDIUM"
    );
    card->init_icon = GuiTex(
        ch->speed == SPEED_QUICK  ? GUI_DETAILS_INIT_QUICK
        : ch->speed == SPEED_SLOW ? GUI_DETAILS_INIT_SLOW
                                  : GUI_DETAILS_INIT_NORMAL
    );

    {
        char factions[128] = "";
        if (ch->giant) HeroesAppendList(factions, sizeof(factions), LocGet("UNIT_DETAIL_FACTION_GIANT"));
        if (FactionIsBlack(ch->faction))
            HeroesAppendList(factions, sizeof(factions), LocGet("UNIT_DETAIL_FACTION_BLACK"));
        if (FactionIsWhite(ch->faction))
            HeroesAppendList(factions, sizeof(factions), LocGet("UNIT_DETAIL_FACTION_WHITE"));
        if (FactionIsRed(ch->faction)) HeroesAppendList(factions, sizeof(factions), LocGet("UNIT_DETAIL_FACTION_RED"));
        if (FactionIsBlue(ch->faction))
            HeroesAppendList(factions, sizeof(factions), LocGet("UNIT_DETAIL_FACTION_BLUE"));

        char groups[256] = "";
        for (int g = 0; g < ch->group_count; g++)
            HeroesAppendList(groups, sizeof(groups), CharacterGroupTitle(ch->groups[g]));
        LocFormat2(card->info, sizeof(card->info), "UNIT_DETAIL_FACTION_GROUP_LABEL_FORMAT", factions, groups);
    }

    {
        StrCopy(card->description, sizeof(card->description), CharacterAbilityText(ch->ability));
        if (ch->equipped_rune[0])
            HeroesAppendSentence(card->description, sizeof(card->description), CharacterLocText(ch->equipped_rune));
        if (ch->target_weight[0])
            HeroesAppendSentence(card->description, sizeof(card->description), CharacterLocText(ch->target_weight));
    }

    const TierStats *stats = &ch->stats[ch->tier];
    card->level = ch->level;
    card->max_level = stats->max_level;
    card->power = CharacterStatPower(ch);
    card->health = CharacterStatHealth(ch);
    card->crit = ch->crit_chance > CONSUME_BASE_CRIT_CHANCE ? ch->crit_chance : CONSUME_BASE_CRIT_CHANCE;
    card->xp_frac = ch->level >= stats->max_level
                        ? 1.0f
                        : ConsumeBandFraction(ConsumeProfileFor(ch)->level_table, ch->level, ch->xp);
    card->desc_fs = 0.0f;
}

static Rectangle RosterPx(float x, float y, float w, float h) {
    float sx = UiWidth() / ROSTER_PREFAB_WIDTH;
    float sy = (UiHeight() - ROSTER_PREFAB_TOP) / ROSTER_PREFAB_HEIGHT;
    return (Rectangle){x * sx, ROSTER_PREFAB_TOP + y * sy, w * sx, h * sy};
}

static Rectangle RosterGridView(void) {
    return RosterPx(8.0f, 120.0f, 375.0f, 704.0f);
}

static Rectangle RosterTileRect(const Roster *r, int index) {
    Rectangle view = RosterGridView();
    float cell = view.width / ROSTER_GRID_COLS;
    return (Rectangle){
        view.x + (index % ROSTER_GRID_COLS) * cell, view.y + (index / ROSTER_GRID_COLS) * cell - r->scroll, cell, cell
    };
}

static bool RosterFilterAccepts(RosterFilter filter, const Character *ch) {
    switch (filter) {
        case ROSTER_FILTER_RED:
            return !ch->giant && FactionIsRed(ch->faction);
        case ROSTER_FILTER_BLUE:
            return !ch->giant && FactionIsBlue(ch->faction);
        case ROSTER_FILTER_WHITE:
            return !ch->giant && FactionIsWhite(ch->faction);
        case ROSTER_FILTER_BLACK:
            return !ch->giant && FactionIsBlack(ch->faction);
        case ROSTER_FILTER_LARGE:
            return ch->giant;
        case ROSTER_FILTER_COMMON:
            return ch->rarity == RARITY_COMMON;
        case ROSTER_FILTER_UNCOMMON:
            return ch->rarity == RARITY_UNCOMMON;
        case ROSTER_FILTER_RARE:
            return ch->rarity == RARITY_RARE;
        case ROSTER_FILTER_TUTORIAL:
            return TutorialMockStoreUnit(ch);
        default:
            return true;
    }
}

static int RosterCompareOrder(const Character *a, const Character *b) {
    if (a->rarity != b->rarity) return (int)b->rarity - (int)a->rarity;
    if (a->tier != b->tier) return (int)b->tier - (int)a->tier;
    if (a->level != b->level) return b->level - a->level;
    return strcmp(a->id, b->id);
}

static int RosterPinRank(const Roster *r, const Player *p, int index) {
    return r->pin_squad && PlayerSquadFindUnit(p, p->current_squad, index) < 0;
}

static int RosterCompareEntries(const Roster *r, const Player *p, int a, int b) {
    int pin_order = RosterPinRank(r, p, a) - RosterPinRank(r, p, b);
    if (pin_order != 0) return pin_order;
    const Character *first = &p->roster[a], *second = &p->roster[b];
    int order = r->filter == ROSTER_FILTER_NEWEST ? (first->instance_id < second->instance_id ? 1 : -1)
                                                  : RosterCompareOrder(first, second);
    return r->sort_reversed ? -order : order;
}

static void RosterRefreshOrder(Roster *r, const Player *p) {
    r->order_count = 0;
    for (int i = 0; i < p->roster_count; i++) {
        if (!RosterFilterAccepts(r->filter, &p->roster[i])) continue;
        int slot = r->order_count++;
        while (slot > 0 && RosterCompareEntries(r, p, r->order[slot - 1], i) > 0) {
            r->order[slot] = r->order[slot - 1];
            slot--;
        }
        r->order[slot] = i;
    }
}

static void RosterSyncPinnedSquad(Roster *r, const Player *p) {
    int units[SQUAD_SLOT_COUNT];
    for (int s = 0; s < SQUAD_SLOT_COUNT; s++) units[s] = PlayerSquadUnitAt(p, p->current_squad, s);
    bool squad_switched = r->pinned_squad != p->current_squad;
    if (!squad_switched && memcmp(units, r->pinned_units, sizeof(units)) == 0) return;
    if (squad_switched) r->scroll = 0.0f;
    r->pinned_squad = p->current_squad;
    memcpy(r->pinned_units, units, sizeof(units));
    RosterRefreshOrder(r, p);
}

static void RosterInitialize(Roster *r, const Player *p) {
    memset(r, 0, sizeof(*r));
    Font font = GuiFont(FNT_COOL);
    Vector2 anchor = {0.5f, 0.5f};
    r->category = ButtonLoadText(GUI_CONSUME_CATEGORY, GUI_CONSUME_CATEGORY_ON, "", font, anchor, 0.42f);
    r->sort = ButtonLoadText(
        GUI_CONSUME_CATEGORY, GUI_CONSUME_CATEGORY_ON, LocGet("HODA_HEROES_SORT_BUTTON"), font, anchor, 0.34f
    );
    RosterRefreshOrder(r, p);
}

static void RosterDrawFilterList(Roster *r, const Player *p, bool input) {
    Font font = GuiFont(FNT_COOL);
    for (int i = 0; i < ROSTER_FILTER_PICKABLE_COUNT; i++) {
        Rectangle row = RosterPx(8.0f, 124.0f + i * ROSTER_FILTER_ROW, 378.0f, 70.0f);
        bool selected = r->filter == (RosterFilter)i;
        UiDrawTexture(GuiTex(selected ? GUI_CONSUME_CATEGORY_ON : GUI_CONSUME_CATEGORY), row, WHITE);
        UiDrawTextCentered(font, LocGet(ROSTER_FILTER_KEYS[i]), row, row.height * 0.42f, WHITE);
        if (input && IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(UiMouse(), row)) {
            r->filter = (RosterFilter)i;
            r->filter_open = false;
            r->scroll = 0.0f;
            RosterRefreshOrder(r, p);
            GuiSoundPlay(SND_UI_TAB);
        }
    }
}

static int
RosterDraw(Roster *r, const HeroCard *cards, const Player *p, bool input, RosterCardDrawFn draw, const void *context) {
    UiDrawTexture(GuiTex(GUI_CONSUME_TRAY), RosterPx(-7.0f, 0.0f, 455.0f, 938.0f), WHITE);

    r->category.label = LocGet(ROSTER_FILTER_KEYS[r->filter]);
    if (ButtonDrawEx(&r->category, RosterPx(16.0f, 56.0f, 276.0f, 68.0f), input)) {
        r->filter_open = !r->filter_open;
        GuiSoundPlay(SND_UI_TAB);
    }
    if (ButtonDrawEx(&r->sort, RosterPx(296.0f, 56.0f, 100.0f, 68.0f), input && !r->filter_open)) {
        r->sort_reversed = !r->sort_reversed;
        r->scroll = 0.0f;
        RosterRefreshOrder(r, p);
    }

    if (r->filter_open) {
        RosterDrawFilterList(r, p, input);
        return -1;
    }

    Rectangle view = RosterGridView();
    int rows = (r->order_count + ROSTER_GRID_COLS - 1) / ROSTER_GRID_COLS;
    float cell = view.width / ROSTER_GRID_COLS;
    float max_scroll = fmaxf(rows * cell - view.height, 0.0f);
    bool over = input && CheckCollisionPointRec(UiMouse(), view);
    if (over) r->scroll -= GetMouseWheelMove() * ROSTER_SCROLL_STEP;
    r->scroll = Clamp(r->scroll, 0.0f, max_scroll);

    int hovered = -1;
    float ui = UiScale();
    BeginScissorMode(0, (int)(view.y * ui), (int)(ROSTER_PANEL_WIDTH * ui), (int)(view.height * ui));
    for (int i = 0; i < r->order_count; i++) {
        Rectangle tile = RosterTileRect(r, i);
        if (tile.y + tile.height < view.y || tile.y > view.y + view.height) continue;
        int index = r->order[i];
        draw(&cards[index], tile, index, context);
        if (over && CheckCollisionPointRec(UiMouse(), tile)) hovered = index;
    }
    EndScissorMode();
    return hovered;
}

void HeroesInitialize(Heroes *h, const Player *p) {
    Font font = GuiFont(FNT_COOL);
    Vector2 anchor = {0.5f, 0.5f};
    float scale = 0.35f;
    Vector2 icon_anchor = {0.5f, 0.72f};
    float icon_scale = 0.15f;
    h->gray = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("NEXUS_BUTTON_LOC_STRING"), font, anchor, scale);
    h->gold = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, LocGet("BUTTON_MAINSCREEN_BATTLE"), font, anchor, scale);
    h->gear =
        ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("HODA_GEAR_REVEAL_WINDOW_TITLE"), font, anchor, 0.25f);
    h->train =
        ButtonLoadText(GUI_BTN_TRAIN, GUI_BTN_TRAIN_ON, LocGet("TRAIN_BUTTON_LABEL"), font, icon_anchor, icon_scale);
    h->fortify = ButtonLoadText(
        GUI_BTN_FORTIFY, GUI_BTN_FORTIFY_ON, LocGet("FORTIFY_BUTTON_LABEL"), font, icon_anchor, icon_scale
    );
    h->runes =
        ButtonLoadText(GUI_BTN_RUNES, GUI_BTN_RUNES_ON, LocGet("RUNES_BUTTON_LABEL"), font, icon_anchor, icon_scale);
    h->gallery = ButtonLoadText(
        GUI_BTN_GALLERY, GUI_BTN_GALLERY_ON, LocGet("BUTTON_MAINSCREEN_GALLERY"), font, icon_anchor, icon_scale
    );
    h->squad =
        ButtonLoadText(GUI_BTN_SQUAD, GUI_BTN_SQUAD_ON, LocGet("SQUAD_BUTTON_LABEL"), font, icon_anchor, icon_scale);
    ButtonSetClickSound(&h->train, SND_UI_BIG_BUTTON);
    ButtonSetClickSound(&h->fortify, SND_UI_BIG_BUTTON);
    ButtonSetClickSound(&h->runes, SND_UI_BIG_BUTTON);
    ButtonSetClickSound(&h->gallery, SND_UI_BIG_BUTTON);
    ButtonDisableClickSound(&h->squad);

    h->card_capacity = p->roster_capacity > 0 ? p->roster_capacity : p->roster_count;
    h->cards = (HeroCard *)MemAlloc((size_t)h->card_capacity * sizeof(HeroCard));
    h->card_count = p->roster_count;
    h->selected = -1;
    h->tab_scroll = 0.0f;
    h->tab_dragging = false;
    h->tab_dragged = false;
    h->tab_snap = true;
    h->press_card = -1;
    h->details_close_armed = false;
    h->drag_request = -1;
    h->drag_icon = -1;
    for (int i = 0; i < p->roster_count; i++) {
        HeroCard *card = &h->cards[i];
        const Character *ch = &p->roster[i];
        HeroCardBuild(card, ch);
        card->portrait = GuiPortraitLoad(ch->icon_prefab[ch->tier]);
    }
    RosterInitialize(&h->roster, p);
    h->roster.pin_squad = true;
    h->roster.pinned_squad = -1;
}

bool HeroesSyncRoster(Heroes *h, const Player *p) {
    if (p->roster_count < h->card_count || p->roster_count > h->card_capacity) {
        TraceLog(
            LOG_ERROR,
            "Heroes: cannot sync %d roster entries into card capacity %d (currently %d)",
            p->roster_count,
            h->card_capacity,
            h->card_count
        );
        return false;
    }
    for (int i = h->card_count; i < p->roster_count; i++) {
        HeroCard *card = &h->cards[i];
        const Character *ch = &p->roster[i];
        HeroCardBuild(card, ch);
        card->portrait = GuiPortraitLoad(ch->icon_prefab[ch->tier]);
    }
    h->card_count = p->roster_count;
    RosterRefreshOrder(&h->roster, p);
    return true;
}

bool HeroesBoardInputBlocked(const Heroes *h) {
    return h->selected >= 0 || GearViewIsOpen();
}

void HeroesRebuildCard(Heroes *h, const Player *p, int index) {
    Texture2D portrait = h->cards[index].portrait;
    HeroCardBuild(&h->cards[index], &p->roster[index]);
    h->cards[index].portrait = portrait;
    RosterRefreshOrder(&h->roster, p);
}

void HeroesApplyConsume(Heroes *h, const Player *p, const int *seeds, int seed_count, int target) {
    bool *removed = (bool *)MemAlloc((unsigned int)h->card_count);
    for (int i = 0; i < seed_count; i++) {
        removed[seeds[i]] = true;
        UnloadTexture(h->cards[seeds[i]].portrait);
    }
    int kept = 0;
    for (int i = 0; i < h->card_count; i++) {
        if (removed[i]) continue;
        if (kept != i) h->cards[kept] = h->cards[i];
        h->cards[kept].ch = &p->roster[kept];
        kept++;
    }
    MemFree(removed);
    h->card_count = kept;
    h->selected = -1;
    h->press_card = -1;
    h->drag_icon = -1;

    HeroCard *card = &h->cards[target];
    const Character *ch = &p->roster[target];
    UnloadTexture(card->portrait);
    HeroCardBuild(card, ch);
    card->portrait = GuiPortraitLoad(ch->icon_prefab[ch->tier]);
    RosterRefreshOrder(&h->roster, p);
}

void HeroesDestroy(Heroes *h) {
    for (int i = 0; i < h->card_count; i++) UnloadTexture(h->cards[i].portrait);
    MemFree(h->cards);
    h->cards = NULL;
    h->card_count = 0;
    h->card_capacity = 0;
}

static Color HeroesRichColor(const char *name, Color base) {
    if (name[0] == '#') {
        unsigned value = 0;
        int digits = (int)strlen(name + 1);
        if ((digits == 6 || digits == 8) && sscanf(name + 1, "%x", &value) == 1) {
            if (digits == 8) value >>= 8;
            return (Color){(value >> 16) & 255, (value >> 8) & 255, value & 255, 255};
        }
        return base;
    }
    if (!strcasecmp(name, "green")) return (Color){0, 128, 0, 255};
    if (!strcasecmp(name, "red")) return (Color){255, 0, 0, 255};
    if (!strcasecmp(name, "orange")) return (Color){255, 165, 0, 255};
    if (!strcasecmp(name, "grey") || !strcasecmp(name, "gray")) return (Color){128, 128, 128, 255};
    if (!strcasecmp(name, "yellow")) return (Color){255, 255, 0, 255};
    if (!strcasecmp(name, "white")) return WHITE;
    return base;
}

typedef enum {
    HEROES_RICH_END,
    HEROES_RICH_SPACE,
    HEROES_RICH_NEWLINE,
    HEROES_RICH_COLOR_OPEN,
    HEROES_RICH_COLOR_CLOSE,
    HEROES_RICH_TEXT,
} HeroesRichTokenKind;

typedef struct {
    HeroesRichTokenKind kind;
    int bytes;
    Color color;
} HeroesRichToken;

static bool HeroesAsciiPrefix(const char *text, const char *prefix) {
    while (*prefix) {
        char a = *text++, b = *prefix++;
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a != b) return false;
    }
    return true;
}

static HeroesRichToken HeroesRichNextToken(const char *text, Color base) {
    HeroesRichToken token = {.kind = HEROES_RICH_END, .color = base};
    if (!text || !text[0]) return token;
    if (text[0] == ' ') return (HeroesRichToken){HEROES_RICH_SPACE, 1, base};
    if (text[0] == '\n') return (HeroesRichToken){HEROES_RICH_NEWLINE, 1, base};
    if (text[0] == '\\' && text[1] == 'n') return (HeroesRichToken){HEROES_RICH_NEWLINE, 2, base};
    if (HeroesAsciiPrefix(text, "</color>")) return (HeroesRichToken){HEROES_RICH_COLOR_CLOSE, 8, base};
    if (HeroesAsciiPrefix(text, "<color=")) {
        const char *end = strchr(text + 7, '>');
        if (end) {
            const char *value = text + 7;
            char quote = 0;
            if (*value == '\'' || *value == '"') quote = *value++;
            char name[16];
            int n = 0;
            while (value < end && (!quote || *value != quote) && n < (int)sizeof(name) - 1) name[n++] = *value++;
            name[n] = '\0';
            token.kind = HEROES_RICH_COLOR_OPEN;
            token.bytes = (int)(end - text) + 1;
            token.color = HeroesRichColor(name, base);
            return token;
        }
    }

    token.kind = HEROES_RICH_TEXT;
    if (text[0] == '<') {
        token.bytes = 1;
        return token;
    }
    const char *end = UiBreakUnitEnd(text);
    const char *tag = memchr(text, '<', (size_t)(end - text));
    token.bytes = (int)((tag ? tag : end) - text);
    if (token.bytes == 0) token.bytes = 1;
    return token;
}

#define HEROES_RICH_LINE_HEIGHT 1.25f

static float HeroesRichLayout(Font font, const char *text, Rectangle r, float fs, Color base, bool draw) {
    Color col = base;
    float x = 0.0f, y = 0.0f, spacing = 1.0f, line_h = fs * HEROES_RICH_LINE_HEIGHT;
    float space_w = MeasureTextEx(font, " ", fs, spacing).x;
    const char *c = text;
    for (;;) {
        HeroesRichToken token = HeroesRichNextToken(c, base);
        if (token.kind == HEROES_RICH_END) break;
        if (token.kind == HEROES_RICH_SPACE) {
            c += token.bytes;
            x += space_w;
            continue;
        }
        if (token.kind == HEROES_RICH_NEWLINE) {
            c += token.bytes;
            x = 0.0f;
            y += line_h;
            continue;
        }
        if (token.kind == HEROES_RICH_COLOR_CLOSE) {
            c += token.bytes;
            col = base;
            continue;
        }
        if (token.kind == HEROES_RICH_COLOR_OPEN) {
            c += token.bytes;
            col = token.color;
            continue;
        }

        char word[1280];
        int n = token.bytes;
        if (n >= (int)sizeof(word)) {
            n = (int)sizeof(word) - 1;
            while (n > 0 && ((unsigned char)c[n] & 0xC0) == 0x80) n--;
            if (n == 0) n = 1;
        }
        memcpy(word, c, (size_t)n);
        word[n] = '\0';
        c += n;
        float w = MeasureTextEx(font, word, fs, spacing).x;
        if (x > 0.0f && x + w > r.width) {
            x = 0.0f;
            y += line_h;
        }
        if (draw) DrawTextEx(font, word, (Vector2){r.x + x, r.y + y}, fs, spacing, col);
        x += w;
    }
    return y + line_h;
}

static float HeroesDrawRich(Font font, const char *text, Rectangle r, float fs, Color base) {
    return HeroesRichLayout(font, text, r, fs, base, true);
}

static Rectangle HeroesDetailsBounds(float span_x, float span_w) {
    float width = 440.0f;
    float header_height = 93.0f;
    float body_height = width / 0.93f;
    return (Rectangle){
        span_x + (span_w - width) * 0.5f,
        72.0f + (UiHeight() - 72.0f - header_height - body_height) * 0.5f,
        width,
        header_height + body_height,
    };
}

static void HeroesDrawDetailsAt(HeroCard *card, Rectangle bounds) {
    Font title_font = GuiFont(FNT_COOL), body_font = GuiFont(FNT_MENU);
    Color off_white = {230, 230, 230, 255}, grey = {127, 127, 127, 255};

    float w = bounds.width, head_h = 93.0f, body_h = bounds.height - head_h;
    float x = bounds.x, y = bounds.y;
    Rectangle head = {x, y, w, head_h};
    Rectangle body = {x, y + head_h, w, body_h};
    float sx = w / 512.0f, sy = body_h / 512.0f;

    {
        Texture2D bg = GuiTex(GUI_UNIT_DETAILS);
        UiDrawTexture(bg, body, WHITE);

        int plaque_id = GUI_PLAQUE_COMMON + (card->ch->rarity - RARITY_COMMON);
        Texture2D plaque = GuiTex(plaque_id);
        UiDrawTexture(plaque, head, WHITE);
    }

    {
        float cw = head_h * 0.55f;
        Rectangle r = {head.x + (w - cw) * -0.02f, head.y + (head_h - cw) * -0.2f, cw, cw};
        UiDrawTexture(card->coin, r, WHITE);
    }

    {
        float fs = 24.0f;
        Vector2 tsz = MeasureTextEx(title_font, card->name, fs, 1.0f);
        DrawTextEx(title_font, card->name, (Vector2){head.x + (w - tsz.x) * 0.5f, head.y + 26.0f}, fs, 1.0f, off_white);
        fs = 18.0f;
        tsz = MeasureTextEx(title_font, card->rarity, fs, 1.0f);
        DrawTextEx(
            title_font,
            card->rarity,
            (Vector2){head.x + (w - tsz.x) * 0.5f, head.y + 53.0f},
            fs,
            1.0f,
            card->rarity_color
        );
    }

    {
        float bw = 76.0f;
        Rectangle badge = {body.x + 29.0f, body.y + 22.0f, bw, bw};
        Texture2D star = GuiTex(GUI_CARD_LEVEL);
        UiDrawTexture(star, badge, WHITE);

        char text[16];
        snprintf(text, sizeof(text), "%d", card->level);
        float fs = 40.0f;
        UiDrawTextCentered(title_font, text, (Rectangle){badge.x, badge.y, bw, bw}, fs, WHITE);

        if (card->max_level > 0) {
            snprintf(text, sizeof(text), "/%d", card->max_level);
            fs = 30.0f;
            DrawTextEx(
                title_font, text, (Vector2){badge.x + bw + 10.0f, badge.y + bw * 0.5f - fs * 0.5f}, fs, 1.0f, grey
            );
        }

        Texture2D pips = GuiTex(GUI_DETAILS_PIPS);
        int tier = (int)card->ch->tier, max_tier = (int)card->ch->max_tier;
        float pw = 34.0f, ph = 17.0f, gap = 4.0f;
        float px0 = body.x + 232.0f * sx, py0 = body.y + 30.0f * sy;
        for (int i = 0; i < 4; i++) {
            Rectangle src = {i <= tier ? 0.0f : 197.0f, 0, 59, 32};
            Rectangle dst = {px0 + i * (pw + gap), py0, pw, ph};
            DrawTexturePro(pips, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
        }

        fs = 22.0f;
        const char *max_num = TIER_NAMES[max_tier];
        char cur[8];
        snprintf(cur, sizeof(cur), "%s/", TIER_NAMES[tier]);
        Vector2 max_sz = MeasureTextEx(title_font, max_num, fs, 1.0f);
        Vector2 cur_sz = MeasureTextEx(title_font, cur, fs, 1.0f);
        float lx = body.x + 460.0f * sx - max_sz.x - cur_sz.x;
        DrawTextEx(title_font, cur, (Vector2){lx, py0 - 3.0f}, fs, 1.0f, WHITE);
        DrawTextEx(title_font, max_num, (Vector2){lx + cur_sz.x, py0 - 3.0f}, fs, 1.0f, grey);

        Rectangle bar = {px0, body.y + 68.0f * sy, 466.0f * sx - 232.0f * sx, 14.0f};
        Texture2D bar_bg = GuiTex(GUI_DETAILS_BAR);
        UiDrawTexture(bar_bg, bar, (Color){62, 62, 62, 255});
        if (card->xp_frac > 0.0f) {
            Texture2D fill = GuiTex(GUI_CARD_BAR_FILL);
            Rectangle fill_src = {0, 0, fill.width * card->xp_frac, fill.height};
            Rectangle fill_dst = {bar.x, bar.y, bar.width * card->xp_frac, bar.height};
            DrawTexturePro(fill, fill_src, fill_dst, (Vector2){0, 0}, 0.0f, WHITE);
        }
    }

    {
        typedef struct {
            Texture2D icon;
            const char *text;
            float box_l, box_r, box_y;
        } StatCell;
        char power[16], health[16], crit[16];
        snprintf(power, sizeof(power), "%d", card->power);
        snprintf(health, sizeof(health), "%d", card->health);
        snprintf(crit, sizeof(crit), "%d%%", card->crit);
        StatCell cells[4] = {
            {GuiTex(GUI_DETAILS_POWER), power, 43, 247, 127},
            {GuiTex(GUI_DETAILS_HEALTH), health, 260, 466, 127},
            {GuiTex(GUI_DETAILS_CRIT), crit, 43, 247, 181},
            {card->init_icon, card->speed, 260, 466, 181},
        };

        for (int i = 0; i < 4; i++) {
            float box_h = 48.0f * sy;
            float cy = body.y + cells[i].box_y * sy + box_h * 0.5f;
            float ih = 34.0f;
            Rectangle icon = {body.x + cells[i].box_l * sx + 6.0f, cy - ih * 0.5f, ih, ih};
            UiDrawTexture(cells[i].icon, icon, WHITE);
            float fs = 26.0f;
            UiDrawTextRight(
                body_font,
                cells[i].text,
                (Rectangle){0.0f, cy, body.x + cells[i].box_r * sx - 10.0f, 0.0f},
                fs,
                off_white
            );
        }
    }

    {
        Rectangle text_area = {body.x + 50.0f * sx, body.y + 246.0f * sy, 410.0f * sx, 216.0f * sy};
        float used = HeroesDrawRich(body_font, card->info, text_area, 17.0f, off_white);
        Rectangle desc_area = {text_area.x, text_area.y + used + 4.0f, text_area.width, text_area.height - used};
        if (card->desc_fs <= 0.0f) {
            float fs = 17.0f;
            while (fs > 10.0f &&
                   HeroesRichLayout(body_font, card->description, desc_area, fs, off_white, false) > desc_area.height) {
                fs -= 0.5f;
            }
            card->desc_fs = fs;
        }
        HeroesDrawRich(body_font, card->description, desc_area, card->desc_fs, off_white);
    }
}

static void HeroesDrawDetails(HeroCard *card, float span_x, float span_w) {
    HeroesDrawDetailsAt(card, HeroesDetailsBounds(span_x, span_w));
}

static void RosterDrawCard(const HeroCard *card, Rectangle cell, const RosterCardState *state) {
    Font font = GuiFont(FNT_MENU);
    Color tint = state->in_use ? (Color){110, 110, 110, 255} : WHITE;
    if (state->selected) UiDrawTexture(GuiTex(GUI_CARD_SELECT), UiRect(cell, 0.5f, 0.5f, 1.1f, 1.1f), WHITE);
    if (state->fortified)
        UiDrawTexture(GuiTex(GUI_ROSTER_FORTIFIED), HudFitRect(cell, 0.5f, 0.5f, 1.9f, 0.96f, 0.97f), tint);
    UiDrawTexture(card->portrait, UiRect(cell, 0.5f, 0.5f, 0.85f, 0.85f), tint);

    float bh = cell.height * 0.06f, bw = bh * 10.0f;
    Rectangle bar = {cell.x + (cell.width - bw) * 0.68f, cell.y + (cell.height - bh) * 0.855f, bw, bh};
    UiDrawTexture(GuiTex(GUI_CARD_BAR_BG), UiRect(bar, 0.5f, 0.5f, 1.05f, 1.5f), WHITE);
    if (state->maxed) {
        UiDrawTextCentered(font, LocGet("UNIT_PROGRESS_BAR_MAXED"), bar, bh * 1.6f, WHITE);
    } else if (state->progress > 0.0f) {
        Texture2D fill = GuiTex(GUI_CARD_BAR_FILL);
        DrawTexturePro(
            fill,
            (Rectangle){0, 0, fill.width * state->progress, fill.height},
            (Rectangle){bar.x, bar.y, bar.width * state->progress, bar.height},
            (Vector2){0, 0},
            0.0f,
            state->grey_bar ? (Color){170, 170, 170, 255} : WHITE
        );
    }

    float badge_size = cell.height * 0.25f;
    if (state->grey_bar) {
        if (state->unlock_icon.id)
            UiDrawTexture(state->unlock_icon, UiRect(cell, 0.1f, 0.9f, badge_size, badge_size), tint);
    } else {
        Rectangle badge = UiRect(cell, 0.08f, 0.93f, badge_size, badge_size);
        UiDrawTexture(GuiTex(GUI_CARD_LEVEL), badge, WHITE);
        char level[16];
        snprintf(level, sizeof(level), "%d", card->level);
        UiDrawTextCentered(font, level, badge, badge_size * 0.62f, WHITE);
    }

    if (state->in_use) {
        float size = cell.height * 0.6f;
        UiDrawTexture(GuiTex(GUI_CONSUME_IN_USE), UiRect(cell, 0.5f, 0.28f, size, size), WHITE);
    }
    if (state->in_squad) {
        float size = cell.height * 0.2f;
        UiDrawTexture(GuiTex(GUI_GALLERY_IN_SQUAD), UiRect(cell, 0.14f, 0.06f, size, size), WHITE);
    }
}

typedef struct {
    const Heroes *heroes;
    const Player *player;
    int newest_unit;
} HeroesCardContext;

static int HeroesNewestRosterIndex(const Player *p) {
    int newest = -1;
    for (int i = 0; i < p->roster_count; i++)
        if (newest < 0 || p->roster[i].instance_id > p->roster[newest].instance_id) newest = i;
    return newest;
}

static void
HeroesPublishTutorialCard(const HeroesCardContext *c, const HeroCard *card, Rectangle cell, int index, bool in_squad) {
    if (index == c->newest_unit) TutorialAnchorSet(TUTORIAL_ANCHOR_HEROES_NEWEST_UNIT, cell);
    Rectangle published;
    if (!in_squad && TutorialWantsDragFrom(card->ch) &&
        !TutorialAnchorRect(TUTORIAL_ANCHOR_HEROES_DRAG_SOURCE, &published))
        TutorialAnchorSet(TUTORIAL_ANCHOR_HEROES_DRAG_SOURCE, cell);
}

static void HeroesDrawRosterCard(const HeroCard *card, Rectangle cell, int roster_index, const void *context) {
    const HeroesCardContext *c = (const HeroesCardContext *)context;
    bool in_squad = PlayerSquadFindUnit(c->player, c->player->current_squad, roster_index) >= 0;
    RosterCardState state = {
        .progress = card->xp_frac,
        .maxed = card->xp_frac >= 1.0f,
        .in_squad = in_squad,
        .in_use = in_squad,
        .selected = roster_index == c->heroes->selected,
        .fortified = FortifyHasChildren(card->ch),
    };
    RosterDrawCard(card, cell, &state);
    if (TutorialRunning()) HeroesPublishTutorialCard(c, card, cell, roster_index, in_squad);
}

HeroesButton HeroesDrawNav(Heroes *h, HeroesButton active, bool interactive, int runes_badge, int gallery_badge) {
    HeroesButton clicked = HEROES_NONE;
    float w = 112.0f;
    float x = UiWidth() - w;
    float y = 84.0f;
    float gap = 4.0f;

    Rectangle train = {x, y, w, w};
    if (active == HEROES_CONSUME)
        ButtonDrawSelected(&h->train, train);
    else if (ButtonDrawEx(&h->train, train, interactive))
        clicked = HEROES_CONSUME;
    y += w + gap;
    Rectangle fortify = {x, y, w, w};
    if (active == HEROES_FORTIFY)
        ButtonDrawSelected(&h->fortify, fortify);
    else if (ButtonDrawEx(&h->fortify, fortify, interactive))
        clicked = HEROES_FORTIFY;
    y += w + gap;
    Rectangle runes = {x, y, w, w};
    if (active == HEROES_RUNES)
        ButtonDrawSelected(&h->runes, runes);
    else if (ButtonDrawEx(&h->runes, runes, interactive))
        clicked = HEROES_RUNES;
    ButtonDrawBadge(runes, runes_badge);
    y += w + gap;
    Rectangle gallery = {x, y, w, w};
    if (ButtonDrawEx(&h->gallery, gallery, interactive)) clicked = HEROES_GALLERY;
    if (active == HEROES_GALLERY) ButtonDrawVisual(&h->gallery, gallery, true);
    ButtonDrawBadge(gallery, gallery_badge);
    y += w + gap;
    Rectangle squad = {x, y, w, w};
    if (active == HEROES_CONSUME) TutorialAnchorSet(TUTORIAL_ANCHOR_CONSUME_SQUAD, squad);
    if (active == HEROES_SQUAD)
        ButtonDrawSelected(&h->squad, squad);
    else if (ButtonDrawEx(&h->squad, squad, interactive))
        clicked = HEROES_SQUAD;
    return clicked;
}

int HeroesDrawSquadTabs(Heroes *h, const Player *p, Rectangle view) {
    int requested = -1;
    Font font = GuiFont(FNT_MENU);
    float size = view.height, stride = size - 3.0f;
    float content = size + (PLAYER_MAX_SQUADS - 1) * stride;
    float max_scroll = content > view.width ? content - view.width : 0.0f;

    Vector2 mouse = UiMouse();
    bool over = CheckCollisionPointRec(mouse, view);
    bool ui_input = !UiInputBlocked();
    if (ui_input && over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        h->tab_dragging = true;
        h->tab_dragged = false;
        h->tab_drag_start = mouse.x;
        h->tab_scroll_start = h->tab_scroll;
    }
    if (h->tab_dragging) {
        float dx = mouse.x - h->tab_drag_start;
        if (fabsf(dx) > 10.0f) h->tab_dragged = true;
        if (h->tab_dragged) h->tab_scroll = h->tab_scroll_start - dx;
    }
    if (ui_input && over) h->tab_scroll -= GetMouseWheelMove() * stride;
    if (h->tab_snap) {
        float ax = p->current_squad * stride;
        if (ax < h->tab_scroll) h->tab_scroll = ax;
        if (ax + size > h->tab_scroll + view.width) h->tab_scroll = ax + size - view.width;
        h->tab_snap = false;
    }
    if (h->tab_scroll > max_scroll) h->tab_scroll = max_scroll;
    if (h->tab_scroll < 0.0f) h->tab_scroll = 0.0f;

    bool released = ui_input && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    float ui = UiScale();
    BeginScissorMode((int)(view.x * ui), (int)(view.y * ui), (int)(view.width * ui), (int)(view.height * ui));
    for (int q = 0; q < PLAYER_MAX_SQUADS; q++) {
        Rectangle r = {view.x + q * stride - h->tab_scroll, view.y, size, size};
        bool active = p->current_squad == q;

        Texture2D slab = GuiTex(
            q == 0 ? (active ? GUI_TAB_DEFENSE_ON : GUI_TAB_DEFENSE) : (active ? GUI_TAB_SQUAD_ON : GUI_TAB_SQUAD)
        );
        UiDrawTexture(slab, r, WHITE);
        if (q == 1) {
            Texture2D cup = GuiTex(active ? GUI_TAB_TROPHY_ON : GUI_TAB_TROPHY);
            float cw = size * 0.72f;
            Rectangle cr = {r.x + (size - cw) * 0.5f, r.y + (size - cw) * 0.5f, cw, cw};
            UiDrawTexture(cup, cr, WHITE);
        }

        char num[4];
        snprintf(num, sizeof(num), "%d", q + 1);
        float fs = size * 0.55f;
        UiDrawTextCentered(font, num, (Rectangle){r.x, r.y, size, size}, fs, WHITE);

        if (released && h->tab_dragging && !h->tab_dragged && !active && CheckCollisionPointRec(mouse, r)) {
            GuiClickSoundPlay();
            requested = q;
        }
    }
    EndScissorMode();
    if (released) h->tab_dragging = false;
    return requested;
}

void GearViewOpenInventory(void);
void GearViewClose(void);
bool GearViewAcceptsInput(void);
void GearViewDrawSlots(Player *p, int hero, Rectangle details);
void GearViewDrawPanel(Player *p);
void GearViewDrawDialogs(Player *p);
int GearViewTakeChangedHero(void);

static const Rectangle HEROES_SQUAD_TABS = {8.0f, 644.0f, 246.0f, 64.0f};

HeroesButton HeroesDraw(Heroes *h, Player *p) {
    HeroesButton clicked = HEROES_NONE;

    HeroesCardContext card_context = {h, p, TutorialRunning() ? HeroesNewestRosterIndex(p) : -1};
    int selected_before = h->selected;
    bool gear_open = GearViewIsOpen();
    int hovered = -1;
    RosterSyncPinnedSquad(&h->roster, p);
    if (gear_open)
        GearViewDrawPanel(p);
    else
        hovered = RosterDraw(
            &h->roster, h->cards, p, !UiInputBlocked() && h->selected < 0, HeroesDrawRosterCard, &card_context
        );
    {
        int requested_squad = HeroesDrawSquadTabs(h, p, HEROES_SQUAD_TABS);
        if (requested_squad >= 0) PlayerSquadSelect(p, requested_squad);
    }

    {
        Vector2 mouse = UiMouse();
        bool ui_input = !UiInputBlocked() && !gear_open;
        if (h->selected >= 0) {
            Rectangle details = HeroesDetailsBounds(156.0f, UiWidth() - 256.0f);
            TutorialAnchorSet(TUTORIAL_ANCHOR_HEROES_DETAILS, details);
            details.width += HEROES_GEAR_COLUMN_WIDTH;
            bool tap_closes = TutorialRequires(TUTORIAL_ANCHOR_HEROES_DETAILS);
            if (ui_input && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                h->details_close_armed = tap_closes || !CheckCollisionPointRec(mouse, details);
            if (ui_input && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                if (h->details_close_armed && (tap_closes || !CheckCollisionPointRec(mouse, details))) h->selected = -1;
                h->details_close_armed = false;
            }
            h->press_card = -1;
        } else {
            if (ui_input && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hovered >= 0) {
                h->press_card = hovered;
                h->press_pos = mouse;
            }
            if (ui_input && h->press_card >= 0 && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                float dx = fabsf(mouse.x - h->press_pos.x), dy = fabsf(mouse.y - h->press_pos.y);
                if (dx >= HEROES_DRAG_THRESHOLD || dy >= HEROES_DRAG_THRESHOLD) {
                    h->drag_request = h->press_card;
                    h->press_card = -1;
                }
            }
            if (ui_input && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                if (h->press_card >= 0) {
                    GuiClickSoundPlayDuplicate();
                    h->selected = h->press_card;
                    h->press_card = -1;
                }
                h->drag_icon = -1;
            }
        }
    }
    bool buttons_active = h->selected < 0 && !gear_open;

    {
        float gap = 8.0f;
        float x = UiWidth() / 4.0f + 96.0f;
        float y = UiHeight() - h->gray.idle.height - 12.0f;

        Rectangle inventory = {HEROES_GEAR_BUTTON_X, y, x - HEROES_GEAR_BUTTON_X - gap, (float)h->gray.idle.height};
        h->gear.label = gear_open ? LocGet("BUTTON_STORE_HERO_TAB") : LocGet("HODA_GEAR_REVEAL_WINDOW_TITLE");
        if (ButtonDrawEx(&h->gear, inventory, gear_open ? GearViewAcceptsInput() : buttons_active)) {
            if (gear_open)
                GearViewClose();
            else
                GearViewOpenInventory();
        }
        Rectangle quest = {x, y, (float)h->gray.idle.width, (float)h->gray.idle.height};
        TutorialAnchorSet(TUTORIAL_ANCHOR_HEROES_QUEST, quest);
        if (ButtonDrawEx(&h->gray, quest, buttons_active)) clicked = HEROES_QUEST;
        x += h->gray.idle.width + gap;
        ButtonDrawEx(&h->gold, (Rectangle){x, y, -1, -1}, false);
    }

    {
        HeroesButton nav =
            HeroesDrawNav(h, HEROES_SQUAD, buttons_active, RuneUnseenCount(&p->runes, RuneNow()), p->gallery.badge);
        if (nav != HEROES_NONE) clicked = nav;
    }

    if (h->selected >= 0) {
        HeroesDrawDetails(&h->cards[h->selected], 156.0f, UiWidth() - 256.0f);
        GearViewDrawSlots(
            p, (int)(h->cards[h->selected].ch - p->roster), HeroesDetailsBounds(156.0f, UiWidth() - 256.0f)
        );
    }
    GearViewDrawDialogs(p);
    int changed_hero = GearViewTakeChangedHero();
    if (changed_hero >= 0) HeroesRebuildCard(h, p, changed_hero);

    if (h->drag_icon >= 0) {
        Vector2 m = UiMouse();
        if (m.x < ROSTER_PANEL_WIDTH) {
            Texture2D pt = h->cards[h->drag_icon].portrait;
            float sz = 82.0f * 0.85f * 1.9f;
            Rectangle dst = {m.x - sz * 0.5f, m.y - sz * 0.5f, sz, sz};
            UiDrawTexture(pt, dst, WHITE);
        }
    }

    if (selected_before < 0 && h->selected >= 0) TutorialRaise(TUTORIAL_EVENT_UNIT_DETAILS_SHOWING);
    if (selected_before >= 0 && h->selected < 0) TutorialRaise(TUTORIAL_EVENT_UNIT_DETAILS_CLOSED);
    return clicked;
}
