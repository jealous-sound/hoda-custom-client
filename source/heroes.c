
#define HEROES_CARD_COLS 2
#define HEROES_DRAG_THRESHOLD 51.0f
#define HEROES_BADGE_MAX 99

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
    const char *name_ru;
    const char *rarity_ru;
    const char *speed_ru;
    Color rarity_color;
    char info_ru[320];
    char desc_ru[1280];
    int level, max_level;
    int power, health, crit;
    float xp_frac;
    float desc_fs;
} HeroCard;

typedef struct {
    Button gray;
    Button gold;
    Button train;
    Button fortify;
    Button runes;
    Button gallery;
    Button squad;

    HeroCard *cards;
    int card_count;
    int card_capacity;
    int selected;
    float card_scroll;

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

    char key[96];
    snprintf(key, sizeof(key), "%s%s", ch->id, TIER_NAMES[ch->tier]);
    card->name_ru = LocGet(key);
    if (!card->name_ru[0]) card->name_ru = ch->name_ru[0] ? ch->name_ru : ch->name;

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
    card->rarity_ru = LocGet(rarity_keys[ch->rarity]);
    card->rarity_color = rarity_colors[ch->rarity];

    card->speed_ru = LocGet(
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
        for (int g = 0; g < ch->group_count; g++) {
            const char *title = ch->group_names_ru[g];
            if (!title[0]) {
                char gkey[64];
                snprintf(gkey, sizeof(gkey), "GALLERY_%s_TITLE", ch->groups[g]);
                for (char *c = gkey; *c; c++) *c = (char)toupper(*c);
                title = LocGet(gkey);
            }
            HeroesAppendList(groups, sizeof(groups), title[0] ? title : ch->groups[g]);
        }
        LocFormat2(card->info_ru, sizeof(card->info_ru), "UNIT_DETAIL_FACTION_GROUP_LABEL_FORMAT", factions, groups);
    }

    {
        const char *ability = ch->ability->text_ru;
        if (!ability[0]) ability = LocGetAbility(ch->ability->key);
        if (!ability[0]) ability = ch->ability->text;
        StrCopy(card->desc_ru, sizeof(card->desc_ru), ability);
        if (ch->equipped_rune[0]) {
            const char *rune = ch->equipped_rune_text_ru;
            if (!rune[0]) rune = LocGet(ch->equipped_rune);
            HeroesAppendSentence(card->desc_ru, sizeof(card->desc_ru), rune);
        }
        if (ch->target_weight[0]) {
            const char *target = ch->target_weight_text_ru;
            if (!target[0]) target = LocGet(ch->target_weight);
            HeroesAppendSentence(card->desc_ru, sizeof(card->desc_ru), target);
        }
    }

    const TierStats *stats = &ch->stats[ch->tier];
    card->level = ch->level;
    card->max_level = stats->max_level;
    card->power = CharacterStatPower(ch);
    card->health = CharacterStatHealth(ch);
    card->crit = 1;
    card->xp_frac = ch->level >= stats->max_level ? 1.0f : 0.0f;
    card->desc_fs = 0.0f;
}

void HeroesInitialize(Heroes *h, const Player *p) {
    Font font = GuiFont(FNT_COOL);
    Vector2 anchor = {0.5f, 0.5f};
    float scale = 0.35f;
    Vector2 icon_anchor = {0.5f, 0.72f};
    float icon_scale = 0.15f;
    h->gray = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, "Задание", font, anchor, scale);
    h->gold = ButtonLoadText(GUI_BTN_GOLD, GUI_BTN_GOLD_ON, "PvP", font, anchor, scale);
    h->train = ButtonLoadText(GUI_BTN_TRAIN, GUI_BTN_TRAIN_ON, "Поглотить", font, icon_anchor, icon_scale);
    h->fortify = ButtonLoadText(GUI_BTN_FORTIFY, GUI_BTN_FORTIFY_ON, "Поддержка", font, icon_anchor, icon_scale);
    h->runes = ButtonLoadText(GUI_BTN_RUNES, GUI_BTN_RUNES_ON, "Руны", font, icon_anchor, icon_scale);
    h->gallery = ButtonLoadText(GUI_BTN_GALLERY, GUI_BTN_GALLERY_ON, "Галерея", font, icon_anchor, icon_scale);
    h->squad = ButtonLoadText(GUI_BTN_SQUAD, GUI_BTN_SQUAD_ON, "Отряд", font, icon_anchor, icon_scale);
    ButtonSetClickSound(&h->train, SND_UI_BIG_BUTTON);
    ButtonSetClickSound(&h->fortify, SND_UI_BIG_BUTTON);
    ButtonSetClickSound(&h->runes, SND_UI_BIG_BUTTON);
    ButtonSetClickSound(&h->gallery, SND_UI_BIG_BUTTON);
    ButtonDisableClickSound(&h->squad);

    h->card_capacity = p->roster_capacity > 0 ? p->roster_capacity : p->roster_count;
    h->cards = (HeroCard *)MemAlloc((size_t)h->card_capacity * sizeof(HeroCard));
    h->card_count = p->roster_count;
    h->selected = -1;
    h->card_scroll = 0.0f;
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
    return true;
}

void HeroesDestroy(Heroes *h) {
    for (int i = 0; i < h->card_count; i++) UnloadTexture(h->cards[i].portrait);
    MemFree(h->cards);
    h->cards = NULL;
    h->card_count = 0;
    h->card_capacity = 0;
}

static Color HeroesRichColor(const char *name, Color base) {
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
    while (text[token.bytes] && text[token.bytes] != ' ' && text[token.bytes] != '\n' &&
           !(text[token.bytes] == '\\' && text[token.bytes + 1] == 'n') && text[token.bytes] != '<')
        token.bytes++;
    if (token.bytes == 0) token.bytes = 1;
    return token;
}

static float HeroesRichLayout(Font font, const char *text, Rectangle r, float fs, Color base, bool draw) {
    Color col = base;
    float x = 0.0f, y = 0.0f, spacing = 1.0f, line_h = fs * 1.25f;
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
        if (plaque_id > GUI_PLAQUE_LEGENDARY) plaque_id = GUI_PLAQUE_LEGENDARY;
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
        Vector2 tsz = MeasureTextEx(title_font, card->name_ru, fs, 1.0f);
        DrawTextEx(
            title_font, card->name_ru, (Vector2){head.x + (w - tsz.x) * 0.5f, head.y + 26.0f}, fs, 1.0f, off_white
        );
        fs = 18.0f;
        tsz = MeasureTextEx(title_font, card->rarity_ru, fs, 1.0f);
        DrawTextEx(
            title_font,
            card->rarity_ru,
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
            {card->init_icon, card->speed_ru, 260, 466, 181},
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
        float used = HeroesDrawRich(body_font, card->info_ru, text_area, 17.0f, off_white);
        Rectangle desc_area = {text_area.x, text_area.y + used + 4.0f, text_area.width, text_area.height - used};
        if (card->desc_fs <= 0.0f) {
            float fs = 17.0f;
            while (fs > 10.0f &&
                   HeroesRichLayout(body_font, card->desc_ru, desc_area, fs, off_white, false) > desc_area.height) {
                fs -= 0.5f;
            }
            card->desc_fs = fs;
        }
        HeroesDrawRich(body_font, card->desc_ru, desc_area, card->desc_fs, off_white);
    }
}

static void HeroesDrawDetails(HeroCard *card, float span_x, float span_w) {
    HeroesDrawDetailsAt(card, HeroesDetailsBounds(span_x, span_w));
}

static void HeroesDrawCard(const HeroCard *card, Rectangle cell) {
    Font font = GuiFont(FNT_MENU);

    float pw = cell.width * 0.85f;
    Rectangle portrait = {cell.x + (cell.width - pw) * 0.5f, cell.y + (cell.height - pw) * 0.5f, pw, pw};
    UiDrawTexture(card->portrait, portrait, WHITE);

    {
        float bh = cell.height * 0.06f, bw = bh * 10.0f;
        Rectangle bar = {cell.x + (cell.width - bw) * 0.68f, cell.y + (cell.height - bh) * 0.855f, bw, bh};
        Rectangle bg = {bar.x - bw * 0.025f, bar.y - bh * 0.25f, bw * 1.05f, bh * 1.5f};
        Texture2D bar_bg = GuiTex(GUI_CARD_BAR_BG);
        UiDrawTexture(bar_bg, bg, WHITE);
        if (card->xp_frac >= 1.0f) {
            const char *maxed = LocGet("UNIT_PROGRESS_BAR_MAXED");
            float fs = bh * 1.6f;
            Vector2 tsz = MeasureTextEx(font, maxed, fs, 1.0f);
            DrawTextEx(
                font,
                maxed,
                (Vector2){bar.x + (bar.width - tsz.x) * 0.5f, bar.y + (bar.height - tsz.y) * 0.5f},
                fs,
                1.0f,
                WHITE
            );
        } else if (card->xp_frac > 0.0f) {
            Texture2D bar_fill = GuiTex(GUI_CARD_BAR_FILL);
            Rectangle fill = {bar.x, bar.y, bar.width * card->xp_frac, bar.height};
            Rectangle fill_src = {0, 0, bar_fill.width * card->xp_frac, bar_fill.height};
            DrawTexturePro(bar_fill, fill_src, fill, (Vector2){0, 0}, 0.0f, WHITE);
        }

        float lw = cell.height * 0.25f;
        Rectangle badge = {cell.x + (cell.width - lw) * 0.08f, cell.y + (cell.height - lw) * 0.93f, lw, lw};
        Texture2D level_badge = GuiTex(GUI_CARD_LEVEL);
        Rectangle lsrc = {0, 0, level_badge.width, level_badge.height};
        DrawTexturePro(level_badge, lsrc, badge, (Vector2){0, 0}, 0.0f, WHITE);

        char level[16];
        snprintf(level, sizeof(level), "%d", card->level);
        float fs = lw * 0.62f;
        Vector2 tsz = MeasureTextEx(font, level, fs, 1.0f);
        DrawTextEx(
            font,
            level,
            (Vector2){badge.x + (badge.width - tsz.x) * 0.5f, badge.y + (badge.height - tsz.y) * 0.5f},
            fs,
            1.0f,
            WHITE
        );
    }
}

static void HeroesDrawNavBadge(Rectangle button, int count) {
    if (count <= 0) return;
    char text[8];
    snprintf(text, sizeof(text), "%d", count > HEROES_BADGE_MAX ? HEROES_BADGE_MAX : count);
    Vector2 center = {button.x + button.width * 0.8f, button.y + button.height * 0.2f};
    DrawCircleV(center, 14.0f, (Color){190, 30, 30, 255});
    UiDrawTextCentered(
        GuiFont(FNT_MENU), text, (Rectangle){center.x - 14.0f, center.y - 14.0f, 28.0f, 28.0f}, 18.0f, WHITE
    );
}

HeroesButton HeroesDrawNav(Heroes *h, HeroesButton active, bool interactive, int runes_badge) {
    HeroesButton clicked = HEROES_NONE;
    float w = 112.0f;
    float x = UiWidth() - w;
    float y = 84.0f;
    float gap = 4.0f;

    ButtonDrawEx(&h->train, (Rectangle){x, y, w, w}, false);
    y += w + gap;
    ButtonDrawEx(&h->fortify, (Rectangle){x, y, w, w}, false);
    y += w + gap;
    Rectangle runes = {x, y, w, w};
    if (active == HEROES_RUNES)
        ButtonDrawSelected(&h->runes, runes);
    else if (ButtonDrawEx(&h->runes, runes, interactive))
        clicked = HEROES_RUNES;
    HeroesDrawNavBadge(runes, runes_badge);
    y += w + gap;
    ButtonDrawEx(&h->gallery, (Rectangle){x, y, w, w}, false);
    y += w + gap;
    Rectangle squad = {x, y, w, w};
    if (active == HEROES_SQUAD)
        ButtonDrawSelected(&h->squad, squad);
    else if (ButtonDrawEx(&h->squad, squad, interactive))
        clicked = HEROES_SQUAD;
    return clicked;
}

int HeroesDrawSquadTabs(Heroes *h, const Player *p) {
    int requested = -1;
    Font font = GuiFont(FNT_MENU);
    float size = 64.0f, stride = size - 3.0f;
    Rectangle view = {8.0f, 612.0f, 216.0f, size};
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

HeroesButton HeroesDraw(Heroes *h, Player *p) {
    HeroesButton clicked = HEROES_NONE;

    {
        Texture2D panel = GuiTex(GUI_HEROES_PANEL);
        Rectangle dst = {0, 72, panel.width / 2.0f, panel.height + 128.0f};
        UiDrawTexture(panel, dst, (Color){255, 255, 255, 236});
    }

    {
        Font font = GuiFont(FNT_COOL);
        const char *title = LocGet("ROSTER_FILTER_ALL_HEROES");
        float fs = 20.0f;
        Vector2 tsz = MeasureTextEx(font, title, fs, 1.0f);
        DrawTextEx(font, title, (Vector2){(150.0f - tsz.x) * 0.5f, 112.0f}, fs, 1.0f, (Color){235, 232, 220, 255});
    }

    int hovered = -1;
    {
        float cell = 112.0f;
        float x0 = 4.0f, y0 = 156.0f;
        Rectangle view = {0.0f, y0, 232.0f, 452.0f};
        int rows = (h->card_count + HEROES_CARD_COLS - 1) / HEROES_CARD_COLS;
        float content = rows * cell;
        float max_scroll = content > view.height ? content - view.height : 0.0f;
        bool over_view = CheckCollisionPointRec(UiMouse(), view);
        if (over_view) h->card_scroll -= GetMouseWheelMove() * 50.0f;
        if (h->card_scroll > max_scroll) h->card_scroll = max_scroll;
        if (h->card_scroll < 0.0f) h->card_scroll = 0.0f;

        float ui = UiScale();
        BeginScissorMode((int)(view.x * ui), (int)(view.y * ui), (int)(view.width * ui), (int)(view.height * ui));
        for (int i = 0; i < h->card_count; i++) {
            Rectangle r = {
                x0 + (i % HEROES_CARD_COLS) * cell, y0 + (i / HEROES_CARD_COLS) * cell - h->card_scroll, cell, cell
            };
            if (r.y + cell < view.y || r.y > view.y + view.height) continue;
            if (i == h->selected) {
                Texture2D glow = GuiTex(GUI_CARD_SELECT);
                Rectangle gr = {r.x - cell * 0.05f, r.y - cell * 0.05f, cell * 1.1f, cell * 1.1f};
                UiDrawTexture(glow, gr, WHITE);
            }
            HeroesDrawCard(&h->cards[i], r);
            if (over_view && CheckCollisionPointRec(UiMouse(), r)) hovered = i;
        }
        EndScissorMode();
    }
    {
        int requested_squad = HeroesDrawSquadTabs(h, p);
        if (requested_squad >= 0) PlayerSquadSelect(p, requested_squad);
    }

    {
        Vector2 mouse = UiMouse();
        bool ui_input = !UiInputBlocked();
        if (h->selected >= 0) {
            Rectangle details = HeroesDetailsBounds(156.0f, UiWidth() - 256.0f);
            if (ui_input && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                h->details_close_armed = !CheckCollisionPointRec(mouse, details);
            if (ui_input && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                if (h->details_close_armed && !CheckCollisionPointRec(mouse, details)) h->selected = -1;
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
    bool buttons_active = h->selected < 0;

    {
        float gap = 8.0f;
        float x = UiWidth() / 4.0f + 96.0f;
        float y = UiHeight() - h->gray.idle.height - 12.0f;

        if (ButtonDrawEx(&h->gray, (Rectangle){x, y, -1, -1}, buttons_active)) clicked = HEROES_QUEST;
        x += h->gray.idle.width + gap;
        ButtonDrawEx(&h->gold, (Rectangle){x, y, -1, -1}, false);
    }

    {
        HeroesButton nav = HeroesDrawNav(h, HEROES_SQUAD, buttons_active, RuneUnseenCount(&p->runes, RuneNow()));
        if (nav != HEROES_NONE) clicked = nav;
    }

    if (h->selected >= 0) HeroesDrawDetails(&h->cards[h->selected], 156.0f, UiWidth() - 256.0f);

    if (h->drag_icon >= 0) {
        Vector2 m = UiMouse();
        if (m.x < 256.0f) {
            Texture2D pt = h->cards[h->drag_icon].portrait;
            float sz = 82.0f * 0.85f * 1.9f;
            Rectangle dst = {m.x - sz * 0.5f, m.y - sz * 0.5f, sz, sz};
            UiDrawTexture(pt, dst, WHITE);
        }
    }

    return clicked;
}
