#define GALLERY_DESIGN_WIDTH 1003.0f
#define GALLERY_DESIGN_SCALE (720.0f / 564.0f)
#define GALLERY_TOP_HUD_HEIGHT 72.0f
#define GALLERY_PORTRAITS_PER_PAGE 9
#define GALLERY_PORTRAIT_COLUMNS 3
#define GALLERY_SWIPE_DISTANCE 40.0f
#define GALLERY_SPIN_RADIANS_PER_UI_PIXEL 0.012f
#define GALLERY_SPIN_DRAG_THRESHOLD 8.0f
#define GALLERY_NAME_CAP 192
#define GALLERY_LINE_CAP 128
#define GALLERY_LORE_FONT_SIZE 17.0f
#define GALLERY_LABEL_FONT_SIZE 18.0f
#define GALLERY_TITLE_FONT_SIZE 34.0f
#define GALLERY_HEADER_FONT_SIZE 21.0f
#define GALLERY_NAME_FONT_SIZE 14.0f
#define GALLERY_POPUP_BORDER 34.0f
#define GALLERY_TIER_BUTTON_SIZE 75.0f
#define GALLERY_TIER_BUTTON_STRIDE 90.0f
#define GALLERY_TIER_BUTTONS_X 17.0f
#define GALLERY_BOTTOM_BUTTONS_Y 629.0f
#define GALLERY_NAV_BUTTON_W 236.0f
#define GALLERY_NAV_BUTTON_X 620.0f
#define GALLERY_NAV_BUTTON_GAP 16.0f
#define GALLERY_DETAILS_RECT ((Rectangle){16.0f, 80.0f, 420.0f, 536.0f})
#define GALLERY_SPIN_AREA ((Rectangle){448.0f, 80.0f, 700.0f, 536.0f})

typedef enum { GALLERY_ACTION_NONE, GALLERY_ACTION_SHOW_UNIT } GalleryAction;

typedef enum {
    GALLERY_SCREEN_ACTION_NONE,
    GALLERY_SCREEN_ACTION_SQUAD,
    GALLERY_SCREEN_ACTION_RUNES,
    GALLERY_SCREEN_ACTION_CONSUME,
    GALLERY_SCREEN_ACTION_POPUP
} GalleryScreenAction;

typedef enum {
    GALLERY_CONTROL_NONE,
    GALLERY_CONTROL_CATEGORY_LEFT,
    GALLERY_CONTROL_CATEGORY_RIGHT,
    GALLERY_CONTROL_SUBCATEGORY_LEFT,
    GALLERY_CONTROL_SUBCATEGORY_RIGHT,
    GALLERY_CONTROL_PAGE_LEFT,
    GALLERY_CONTROL_PAGE_RIGHT,
    GALLERY_CONTROL_FILTER_OWNED,
    GALLERY_CONTROL_FILTER_FOUGHT,
    GALLERY_CONTROL_FILTER_DEFEATED,
    GALLERY_CONTROL_CLOSE,
    GALLERY_CONTROL_SCRIM
} GalleryControl;

typedef struct {
    int unit;
    Texture2D portrait;
    bool loaded_portrait;
    bool highlighted, in_squad, currently_owned;
    char name[GALLERY_NAME_CAP];
} GalleryCell;

typedef struct {
    GalleryCatalog catalog;
    int *matches, *unit_list;
    int match_count, unit_count;
    GalleryProgress progress;
    GalleryTotals totals;

    bool popup_open;
    GallerySelection selection;
    GalleryFilter filter;
    int page;
    GalleryCell cells[GALLERY_PORTRAITS_PER_PAGE];
    int cell_count;
    GalleryControl armed;
    bool panel_pressed;
    int pressed_cell;
    Vector2 press_pos;

    int unit_position;
    Tier selected_level;
    Character shown_unit;
    Ability shown_ability;
    HeroCard card;
    bool spin_pressed, spin_dragged;
    float spin_press_x, spin_yaw_start, spin_yaw;

    Button previous, next, tiers[TIER_COUNT], close;
} Gallery;

static Rectangle GalleryRect(float x, float y, float w, float h) {
    float k = GALLERY_DESIGN_SCALE;
    float origin = (UiWidth() - GALLERY_DESIGN_WIDTH * k) * 0.5f;
    return (Rectangle){origin + x * k, y * k, w * k, h * k};
}

void GalleryInitialize(Gallery *g, const char *catalog_path) {
    memset(g, 0, sizeof(*g));
    GalleryCatalogLoad(&g->catalog, catalog_path);
    g->matches = (int *)MemAlloc((unsigned int)g->catalog.count * sizeof(int));
    g->unit_list = (int *)MemAlloc((unsigned int)g->catalog.count * sizeof(int));
    g->selection.category = GALLERY_CATEGORY_ALL;
    g->selection.rarity = RARITY_COMMON;
    g->selection.faction = GALLERY_FACTION_BLACK;

    Font font = GuiFont(FNT_COOL);
    Vector2 anchor = {0.5f, 0.5f};
    g->previous = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("GALLERY_PREVIOUS"), font, anchor, 0.35f);
    g->next = ButtonLoadText(GUI_BTN_GREY, GUI_BTN_GREY_ON, LocGet("GALLERY_NEXT"), font, anchor, 0.35f);
    for (int tier = 0; tier < TIER_COUNT; tier++)
        g->tiers[tier] = ButtonLoadText(GUI_TAB_SQUAD, GUI_TAB_SQUAD_ON, TIER_NAMES[tier], font, anchor, 0.42f);
    g->close = ButtonLoad(GUI_BTN_CLOSE, GUI_BTN_CLOSE);
    ButtonSetClickSound(&g->close, SND_UI_POPUP_CLOSE);
}

static void GalleryPageUnload(Gallery *g) {
    for (int i = 0; i < g->cell_count; i++)
        if (g->cells[i].loaded_portrait) UnloadTexture(g->cells[i].portrait);
    g->cell_count = 0;
}

void GalleryDestroy(Gallery *g) {
    GalleryPageUnload(g);
    MemFree(g->matches);
    MemFree(g->unit_list);
    GalleryCatalogUnload(&g->catalog);
}

static void GalleryUnitDisplayName(char *out, int capacity, const Character *unit, Tier tier) {
    char key[96];
    snprintf(key, sizeof(key), "%s%s", unit->id, TIER_NAMES[tier]);
    const char *name = LocGetOr(key, unit->name_ru[0] ? unit->name_ru : unit->name);
    int end = (int)strlen(name);
    while (end > 0 && (name[end - 1] == 'I' || name[end - 1] == 'V')) end--;
    if (end == (int)strlen(name) || end == 0 || name[end - 1] != ' ')
        end = (int)strlen(name);
    else
        end--;
    int written = 0;
    for (int i = 0; i < end && written < capacity - 4; i++) {
        if (name[i] == '-') {
            memcpy(out + written, " - ", 3);
            written += 3;
        } else {
            out[written++] = name[i];
        }
    }
    out[written] = '\0';
}

static bool GalleryUnitInAnySquad(const Player *p, const char *id) {
    for (int squad = 0; squad < PLAYER_MAX_SQUADS; squad++) {
        for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
            int unit = PlayerSquadUnitAt(p, squad, slot);
            if (unit >= 0 && !strcmp(p->roster[unit].id, id)) return true;
        }
    }
    return false;
}

static Tier GalleryPortraitTier(const Character *unit, const GalleryRecord *record) {
    for (int tier = 0; record && tier < TIER_COUNT; tier++)
        if (record->owned_tiers & GalleryTierBit((Tier)tier)) return (Tier)tier;
    return GalleryLowestTier(unit);
}

static void GalleryCellBuild(GalleryCell *cell, int unit_index, const GalleryCatalog *catalog, const Player *p) {
    const Character *unit = &catalog->units[unit_index];
    const GalleryRecord *record = GalleryRecordFind(&p->gallery, unit->id);
    *cell = (GalleryCell){.unit = unit_index};
    Tier tier = GalleryPortraitTier(unit, record);
    const char *icon = unit->icon_prefab[tier];
    if (record && record->owned_tiers && ResourceExists(GuiPortraitRef(icon))) {
        cell->portrait = GuiPortraitLoad(icon);
        cell->loaded_portrait = true;
    } else {
        cell->portrait = GuiTex(GUI_GALLERY_SHADOW_COMMON + (unit->rarity - RARITY_COMMON));
    }
    cell->highlighted = record && (record->new_owned || record->new_fought || record->new_defeated);
    cell->in_squad = GalleryUnitInAnySquad(p, unit->id);
    cell->currently_owned = PlayerRosterDefinitionCount(p, unit->id) > 0;
    GalleryUnitDisplayName(cell->name, sizeof(cell->name), unit, GalleryLowestTier(unit));
}

static int GalleryPageCount(const Gallery *g) {
    return (g->match_count + GALLERY_PORTRAITS_PER_PAGE - 1) / GALLERY_PORTRAITS_PER_PAGE;
}

static void GalleryPageLoad(Gallery *g, const Player *p) {
    GalleryPageUnload(g);
    int first = g->page * GALLERY_PORTRAITS_PER_PAGE;
    for (int i = first; i < g->match_count && g->cell_count < GALLERY_PORTRAITS_PER_PAGE; i++)
        GalleryCellBuild(&g->cells[g->cell_count++], g->matches[i], &g->catalog, p);
}

static void GalleryRefresh(Gallery *g, const Player *p, bool reset_page) {
    g->match_count = GalleryQuery(&g->catalog, &p->gallery, &g->selection, g->filter, g->matches, &g->progress);
    int pages = GalleryPageCount(g);
    if (reset_page || g->page >= pages) g->page = 0;
    GalleryPageLoad(g, p);
}

void GalleryOpenPopup(Gallery *g, Player *p) {
    if (p->gallery.badge) {
        p->gallery.badge = 0;
        ProgressionSave(p);
    }
    g->popup_open = true;
    g->filter = GALLERY_FILTER_NONE;
    g->armed = GALLERY_CONTROL_NONE;
    g->panel_pressed = false;
    g->totals = GalleryTotalsCompute(&g->catalog, &p->gallery);
    GalleryRefresh(g, p, false);
}

void GalleryClosePopup(Gallery *g) {
    g->popup_open = false;
    GalleryPageUnload(g);
}

bool GalleryPopupOpen(const Gallery *g) {
    return g->popup_open;
}

bool GalleryHandleBack(Gallery *g) {
    if (!g->popup_open) return false;
    GalleryClosePopup(g);
    GuiSoundPlay(SND_UI_POPUP_CLOSE);
    return true;
}

void GalleryRecordBattle(PlayerGallery *gallery, const BattleRoster *enemies, const BattleSim *sim) {
    for (int slot = 0; slot < SQUAD_SLOT_COUNT; slot++) {
        if (!enemies->present[slot]) continue;
        bool defeated = sim->units[SQUAD_SLOT_COUNT + slot].health <= 0;
        GalleryRecordBattleUnit(gallery, enemies->chars[slot].id, defeated);
    }
}

static bool GalleryControlPressed(Gallery *g, GalleryControl control, Rectangle r, bool input) {
    bool hover = input && CheckCollisionPointRec(UiMouse(), r);
    if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) g->armed = control;
    return hover && g->armed == control && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

static bool
GalleryArrow(Gallery *g, GalleryControl control, Rectangle r, GuiId idle, GuiId active, bool mirrored, bool input) {
    bool clicked = GalleryControlPressed(g, control, r, input);
    bool held = g->armed == control && input && CheckCollisionPointRec(UiMouse(), r);
    Texture2D texture = GuiTex(held ? active : idle);
    float width = (float)texture.width;
    Rectangle source = {0.0f, 0.0f, mirrored ? -width : width, (float)texture.height};
    DrawTexturePro(texture, source, r, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
    return clicked;
}

static void GalleryDrawCenteredWrapped(Font font, const char *text, Rectangle r, float size, Color color) {
    char lines[4][GALLERY_LINE_CAP];
    int line_count = 0, line_length = 0;
    lines[0][0] = '\0';
    const char *cursor = text;
    while (*cursor && line_count < 4) {
        const char *word_end = cursor;
        while (*word_end == ' ') word_end++;
        while (*word_end && *word_end != ' ') word_end++;
        int length = (int)(word_end - cursor);
        if (line_length + length >= GALLERY_LINE_CAP) break;
        char candidate[GALLERY_LINE_CAP];
        memcpy(candidate, lines[line_count], (size_t)line_length);
        memcpy(candidate + line_length, cursor, (size_t)length);
        candidate[line_length + length] = '\0';
        if (line_length > 0 && MeasureTextEx(font, candidate, size, 1.0f).x > r.width) {
            line_count++;
            line_length = 0;
            while (*cursor == ' ') cursor++;
            if (line_count < 4) lines[line_count][0] = '\0';
            continue;
        }
        memcpy(lines[line_count], candidate, (size_t)(line_length + length + 1));
        line_length += length;
        cursor = word_end;
    }
    if (line_length > 0 && line_count < 4) line_count++;
    float line_height = size * 1.1f;
    float y = r.y + (r.height - line_count * line_height) * 0.5f;
    for (int i = 0; i < line_count; i++) {
        UiDrawTextCentered(font, lines[i], (Rectangle){r.x, y, r.width, line_height}, size, color);
        y += line_height;
    }
}

static void GalleryDrawCountRow(Rectangle row, const char *caption, int owned, int total) {
    Font font = GuiFont(FNT_MENU);
    DrawRectangleRec(row, Fade(BLACK, 0.8f));
    float fraction = total > 0 ? (float)owned / (float)total : 0.0f;
    Rectangle fill = {row.x, row.y, row.width * fraction, row.height};
    UiDrawTexture(GuiTex(GUI_GALLERY_ROW_FILL), fill, Fade(WHITE, 0.65f));
    UiDrawTextRight(font, TextFormat("(%d/%d)", owned, total), row, GALLERY_LABEL_FONT_SIZE, WHITE);
    if (caption) {
        Rectangle label = {row.x - 30.0f, row.y, 60.0f, row.height};
        UiDrawTextCentered(GuiFont(FNT_COOL), caption, label, GALLERY_LABEL_FONT_SIZE, WHITE);
    }
}

static void GalleryDrawStats(const Gallery *g) {
    UiDrawTexture(GuiTex(GUI_GALLERY_HOME_BG), GalleryRect(436.0f, 132.0f, 387.0f, 387.0f), WHITE);
    static const float tier_rows[TIER_COUNT] = {261.0f, 230.0f, 199.0f, 167.0f};
    static const float rarity_rows[RARITY_LEGENDARY] = {457.0f, 426.0f, 395.0f, 366.0f, 334.0f};
    const GalleryTotals *totals = &g->totals;
    for (int tier = 0; tier < TIER_COUNT; tier++)
        GalleryDrawCountRow(
            GalleryRect(591.0f, tier_rows[tier], 155.0f, 20.0f),
            TIER_NAMES[tier],
            totals->tier_owned[tier],
            totals->tier_total[tier]
        );
    for (int rarity = 0; rarity < RARITY_LEGENDARY; rarity++)
        GalleryDrawCountRow(
            GalleryRect(591.0f, rarity_rows[rarity], 155.0f, 20.0f),
            NULL,
            totals->rarity_owned[rarity],
            totals->rarity_total[rarity]
        );
}

static void GalleryFormatCount(char *out, int capacity, const char *key, int value, int total) {
    char left[16], right[16];
    snprintf(left, sizeof(left), "%d", value);
    snprintf(right, sizeof(right), "%d", total);
    LocFormat2(out, (size_t)capacity, key, left, right);
}

static void GallerySetFilter(Gallery *g, const Player *p, GalleryFilter filter) {
    GuiClickSoundPlay();
    g->filter = g->filter == filter ? GALLERY_FILTER_NONE : filter;
    GalleryRefresh(g, p, true);
}

static void GalleryDrawFilter(
    Gallery *g,
    const Player *p,
    GalleryControl control,
    GalleryFilter filter,
    float y,
    const char *key,
    int value,
    bool is_new,
    bool input
) {
    Font font = GuiFont(FNT_MENU);
    Rectangle button = GalleryRect(188.0f, y, 243.0f, 45.0f);
    if (GalleryControlPressed(g, control, button, input)) GallerySetFilter(g, p, filter);
    UiDrawTexture(GuiTex(g->filter == filter ? GUI_GALLERY_FILTER_ON : GUI_GALLERY_FILTER), button, WHITE);

    int total = g->progress.total;
    float fraction = total > 0 ? (float)value / (float)total : 0.0f;
    UiDrawTexture(GuiTex(GUI_GALLERY_FILTER_FILL), GalleryRect(213.0f, y + 13.0f, 194.0f * fraction, 19.0f), WHITE);
    char label[64];
    GalleryFormatCount(label, sizeof(label), key, value, total);
    Rectangle text = GalleryRect(217.0f, y, 187.0f, 45.0f);
    UiDrawTextCentered(font, label, text, GALLERY_LABEL_FONT_SIZE, WHITE);
    if (is_new) UiDrawTextRight(font, LocGet("GALLERY_NEW"), text, GALLERY_NAME_FONT_SIZE, (Color){56, 207, 41, 255});
}

static void GalleryDrawProgress(Gallery *g, const Player *p, bool input) {
    const GalleryProgress *progress = &g->progress;
    GalleryDrawFilter(
        g,
        p,
        GALLERY_CONTROL_FILTER_OWNED,
        GALLERY_FILTER_OWNED,
        379.0f,
        "GALLERY_SUMMONED",
        progress->owned,
        progress->new_owned,
        input
    );
    GalleryDrawFilter(
        g,
        p,
        GALLERY_CONTROL_FILTER_FOUGHT,
        GALLERY_FILTER_FOUGHT,
        427.0f,
        "GALLERY_FOUGHT",
        progress->fought,
        progress->new_fought,
        input
    );
    GalleryDrawFilter(
        g,
        p,
        GALLERY_CONTROL_FILTER_DEFEATED,
        GALLERY_FILTER_DEFEATED,
        474.0f,
        "GALLERY_DEFEATED",
        progress->defeated,
        progress->new_defeated,
        input
    );
}

static void GalleryChangeCategory(Gallery *g, const Player *p, int delta) {
    GuiSoundPlay(SND_UI_TAB);
    g->selection.category = GalleryCategoryStep(g->selection.category, delta);
    GalleryRefresh(g, p, true);
}

static void GalleryChangeSubcategory(Gallery *g, const Player *p, int delta) {
    GuiSoundPlay(SND_UI_TAB);
    GallerySelectionStepSubcategory(&g->selection, &g->catalog, delta);
    GalleryRefresh(g, p, true);
}

static void GalleryDrawFittedTitle(Font font, const char *text, Rectangle r) {
    float size = GALLERY_HEADER_FONT_SIZE;
    while (size > GALLERY_NAME_FONT_SIZE && MeasureTextEx(font, text, size, 1.0f).x > r.width) size -= 0.5f;
    UiDrawTextCentered(font, text, r, size, WHITE);
}

static void GalleryDrawNavigator(Gallery *g, const Player *p, bool input) {
    Font font = GuiFont(FNT_COOL);
    bool all = g->selection.category == GALLERY_CATEGORY_ALL;
    char key[GALLERY_KEY_CAP];

    UiDrawTexture(GuiTex(GUI_GALLERY_CATEGORY), GalleryRect(216.0f, 135.0f, 189.0f, 44.0f), WHITE);
    GalleryCategoryTitleKey(key, sizeof(key), g->selection.category);
    GalleryDrawFittedTitle(font, LocGet(key), GalleryRect(236.0f, 132.0f, 148.0f, 49.0f));
    if (GalleryArrow(
            g,
            GALLERY_CONTROL_CATEGORY_LEFT,
            GalleryRect(189.0f, 132.0f, 49.0f, 49.0f),
            GUI_GALLERY_ARROW,
            GUI_GALLERY_ARROW_ON,
            true,
            input
        ))
        GalleryChangeCategory(g, p, -1);
    if (GalleryArrow(
            g,
            GALLERY_CONTROL_CATEGORY_RIGHT,
            GalleryRect(382.0f, 132.0f, 49.0f, 49.0f),
            GUI_GALLERY_ARROW,
            GUI_GALLERY_ARROW_ON,
            false,
            input
        ))
        GalleryChangeCategory(g, p, 1);

    if (all) {
        char summoned[64];
        GalleryFormatCount(summoned, sizeof(summoned), "GALLERY_SUMMONED", g->totals.owned, g->totals.total);
        UiDrawTextCentered(
            GuiFont(FNT_MENU), summoned, GalleryRect(224.0f, 191.0f, 172.0f, 31.0f), GALLERY_HEADER_FONT_SIZE, WHITE
        );
        return;
    }

    UiDrawTexture(GuiTex(GUI_GALLERY_CATEGORY), GalleryRect(213.0f, 182.0f, 193.0f, 45.0f), WHITE);
    GallerySubcategoryTitleKey(key, sizeof(key), &g->catalog, &g->selection);
    GalleryDrawFittedTitle(font, LocGet(key), GalleryRect(236.0f, 180.0f, 148.0f, 49.0f));
    if (GalleryArrow(
            g,
            GALLERY_CONTROL_SUBCATEGORY_LEFT,
            GalleryRect(189.0f, 180.0f, 49.0f, 49.0f),
            GUI_GALLERY_ARROW,
            GUI_GALLERY_ARROW_ON,
            true,
            input
        ))
        GalleryChangeSubcategory(g, p, -1);
    if (GalleryArrow(
            g,
            GALLERY_CONTROL_SUBCATEGORY_RIGHT,
            GalleryRect(382.0f, 180.0f, 49.0f, 49.0f),
            GUI_GALLERY_ARROW,
            GUI_GALLERY_ARROW_ON,
            false,
            input
        ))
        GalleryChangeSubcategory(g, p, 1);
}

static Rectangle GalleryCellRect(int index) {
    int column = index % GALLERY_PORTRAIT_COLUMNS, row = index / GALLERY_PORTRAIT_COLUMNS;
    return GalleryRect(446.0f + column * 125.5f, 132.0f + row * 129.0f, 126.0f, 129.0f);
}

static void GalleryDrawCell(const GalleryCell *cell, Rectangle r) {
    float side = r.height * 0.8f;
    Rectangle portrait = {r.x + (r.width - side) * 0.5f, r.y + (r.height - side) * 0.5f, side, side};
    if (cell->highlighted) {
        float glow = r.height * 1.1f;
        UiDrawTexture(
            GuiTex(GUI_CARD_SELECT),
            (Rectangle){r.x + (r.width - glow) * 0.5f, r.y + (r.height - glow) * 0.5f, glow, glow},
            WHITE
        );
    }
    UiDrawTexture(cell->portrait, portrait, WHITE);

    Rectangle plate = UiRect(r, 0.5f, 1.15f, 1.0f, 0.45f);
    UiDrawTexture(GuiTex(GUI_MM_PLATE), plate, WHITE);
    GalleryDrawCenteredWrapped(
        GuiFont(FNT_MENU), cell->name, UiRect(r, 0.5f, 1.05f, 0.9f, 0.35f), GALLERY_NAME_FONT_SIZE, WHITE
    );
    if (cell->in_squad) {
        Rectangle badge = UiRect(r, 0.175f, 0.1f, 0.2f, 0.2f);
        badge.height = badge.width;
        UiDrawTexture(GuiTex(GUI_GALLERY_IN_SQUAD), badge, WHITE);
    }
    if (cell->currently_owned) {
        Rectangle check = UiRect(r, 0.5f, 0.4f, 0.5f, 0.5f);
        UiDrawTexture(GuiTex(GUI_MAP_ABOUT_COLLECTED), check, Fade(WHITE, 0.8f));
    }
}

static void GalleryChangePage(Gallery *g, const Player *p, int page) {
    if (page < 0 || page >= GalleryPageCount(g) || page == g->page) return;
    g->page = page;
    GalleryPageLoad(g, p);
}

static int GalleryCellAt(const Gallery *g, Vector2 point) {
    for (int i = 0; i < g->cell_count; i++)
        if (CheckCollisionPointRec(point, GalleryCellRect(i))) return i;
    return -1;
}

static int GalleryDrawUnits(Gallery *g, const Player *p, bool input) {
    int selected = -1;
    Rectangle panel = GalleryRect(446.0f, 132.0f, 377.0f, 387.0f);
    Vector2 mouse = UiMouse();
    bool over = input && CheckCollisionPointRec(mouse, panel);
    if (over) {
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) GalleryChangePage(g, p, g->page + (wheel < 0.0f ? 1 : -1));
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            g->panel_pressed = true;
            g->press_pos = mouse;
            g->pressed_cell = GalleryCellAt(g, mouse);
        }
    }
    if (g->panel_pressed && input && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        float swipe = mouse.x - g->press_pos.x;
        if (fabsf(swipe) >= GALLERY_SWIPE_DISTANCE) {
            GalleryChangePage(g, p, g->page + (swipe < 0.0f ? 1 : -1));
        } else if (g->pressed_cell >= 0 && g->pressed_cell == GalleryCellAt(g, mouse)) {
            GuiClickSoundPlay();
            selected = g->cells[g->pressed_cell].unit;
        }
        g->panel_pressed = false;
    }

    for (int i = 0; i < g->cell_count; i++) GalleryDrawCell(&g->cells[i], GalleryCellRect(i));

    int pages = GalleryPageCount(g);
    if (pages > 0) {
        char page[16], count[16], label[64];
        snprintf(page, sizeof(page), "%d", g->page + 1);
        snprintf(count, sizeof(count), "%d", pages);
        LocFormat2(label, sizeof(label), "GALLERY_PAGE", page, count);
        UiDrawTextCentered(
            GuiFont(FNT_MENU), label, GalleryRect(462.0f, 524.0f, 342.0f, 28.0f), GALLERY_LABEL_FONT_SIZE, WHITE
        );
    }
    if (g->page > 0 && GalleryArrow(
                           g,
                           GALLERY_CONTROL_PAGE_LEFT,
                           GalleryRect(433.0f, 520.0f, 29.0f, 36.0f),
                           GUI_GALLERY_PAGE_ARROW,
                           GUI_GALLERY_PAGE_ARROW,
                           true,
                           input
                       ))
        GalleryChangePage(g, p, g->page - 1);
    if (g->page < pages - 1 && GalleryArrow(
                                   g,
                                   GALLERY_CONTROL_PAGE_RIGHT,
                                   GalleryRect(804.0f, 520.0f, 29.0f, 36.0f),
                                   GUI_GALLERY_PAGE_ARROW,
                                   GUI_GALLERY_PAGE_ARROW,
                                   false,
                                   input
                               ))
        GalleryChangePage(g, p, g->page + 1);
    return selected;
}

static void GalleryPrepareUnitList(Gallery *g, int unit_index) {
    g->unit_count = g->match_count;
    memcpy(g->unit_list, g->matches, (size_t)g->match_count * sizeof(int));
    g->unit_position = 0;
    for (int i = 0; i < g->unit_count; i++)
        if (g->unit_list[i] == unit_index) g->unit_position = i;
}

static void GalleryScrimUpdate(Gallery *g, Rectangle popup, bool input) {
    Vector2 mouse = UiMouse();
    bool outside = input && mouse.y >= GALLERY_TOP_HUD_HEIGHT && !CheckCollisionPointRec(mouse, popup);
    if (outside && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) g->armed = GALLERY_CONTROL_SCRIM;
    if (outside && g->armed == GALLERY_CONTROL_SCRIM && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        GuiSoundPlay(SND_UI_POPUP_CLOSE);
        GalleryClosePopup(g);
    }
}

GalleryAction GalleryPopupDraw(Gallery *g, const Player *p) {
    if (!g->popup_open) return GALLERY_ACTION_NONE;
    bool input = !UiInputBlocked();
    Rectangle popup = GalleryRect(126.0f, 56.0f, 753.0f, 508.0f);
    Rectangle scrim = {0.0f, GALLERY_TOP_HUD_HEIGHT, UiWidth(), UiHeight() - GALLERY_TOP_HUD_HEIGHT};
    DrawRectangleRec(scrim, Fade(BLACK, 0.6f));
    StoreDrawNineSlice(GuiTex(GUI_STORE_POPUP), popup, GALLERY_POPUP_BORDER, WHITE);
    UiDrawTexture(GuiTex(GUI_GALLERY_DETAILS), GalleryRect(179.0f, 125.0f, 260.0f, 402.0f), WHITE);
    UiDrawTextCentered(
        GuiFont(FNT_COOL),
        LocGet("GALLERY_TITLE"),
        GalleryRect(126.0f, 62.0f, 753.0f, 51.0f),
        GALLERY_TITLE_FONT_SIZE,
        WHITE
    );

    bool all = g->selection.category == GALLERY_CATEGORY_ALL;
    int selected = -1;
    if (all) {
        GalleryDrawStats(g);
        UiDrawTexture(GuiTex(GUI_GALLERY_HOME_ART), GalleryRect(242.0f, 380.0f, 136.0f, 136.0f), WHITE);
    } else {
        selected = GalleryDrawUnits(g, p, input);
        GalleryDrawProgress(g, p, input);
    }
    GalleryDrawNavigator(g, p, input);

    char key[GALLERY_KEY_CAP];
    GallerySubcategoryLoreKey(key, sizeof(key), &g->catalog, &g->selection);
    HeroesDrawRich(
        GuiFont(FNT_MENU),
        LocGet(key),
        GalleryRect(193.0f, 240.0f, 233.0f, 136.0f),
        GALLERY_LORE_FONT_SIZE,
        (Color){191, 191, 191, 255}
    );

    if (ButtonDrawEx(&g->close, GalleryRect(811.0f, 86.0f, 58.0f, 58.0f), input)) {
        GalleryClosePopup(g);
        return GALLERY_ACTION_NONE;
    }
    GalleryScrimUpdate(g, popup, input);
    if (input && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) g->armed = GALLERY_CONTROL_NONE;

    if (selected < 0) return GALLERY_ACTION_NONE;
    GalleryPrepareUnitList(g, selected);
    GalleryClosePopup(g);
    return GALLERY_ACTION_SHOW_UNIT;
}

static void GalleryShowCurrentUnit(Gallery *g, Player *p, HomeBackground *scene) {
    const Character *definition = &g->catalog.units[g->unit_list[g->unit_position]];
    Tier tier = definition->has_tier[g->selected_level] ? g->selected_level : GalleryLowestTier(definition);
    g->shown_unit = *definition;
    g->shown_ability = *definition->ability;
    g->shown_unit.ability = &g->shown_ability;
    g->shown_unit.tier = tier;
    g->shown_unit.level = definition->stats[tier].max_level;
    Texture2D portrait = g->card.portrait;
    HeroCardBuild(&g->card, &g->shown_unit);
    g->card.portrait = portrait;

    if (GalleryRecordClearNew(&p->gallery, definition->id)) ProgressionSave(p);
    const GalleryRecord *record = GalleryRecordFind(&p->gallery, definition->id);
    bool owned = record && (record->owned_tiers & GalleryTierBit(tier));
    g->spin_yaw = 0.0f;
    HomeBackgroundGalleryShow(scene, &g->shown_unit, !owned, g->spin_yaw);
}

void GalleryScreenEnter(Gallery *g, Player *p, HomeBackground *scene) {
    g->selected_level = TIER_I;
    g->spin_pressed = false;
    GalleryShowCurrentUnit(g, p, scene);
}

static void GalleryStepUnit(Gallery *g, Player *p, HomeBackground *scene, int delta) {
    g->unit_position = (g->unit_position + delta + g->unit_count) % g->unit_count;
    GalleryShowCurrentUnit(g, p, scene);
}

static void GallerySelectLevel(Gallery *g, Player *p, HomeBackground *scene, Tier level) {
    if (level == g->selected_level) return;
    g->selected_level = level;
    GalleryShowCurrentUnit(g, p, scene);
}

static void GalleryUpdateSpin(Gallery *g, HomeBackground *scene, bool input) {
    Vector2 mouse = UiMouse();
    if (input && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, GALLERY_SPIN_AREA)) {
        g->spin_pressed = true;
        g->spin_dragged = false;
        g->spin_press_x = mouse.x;
        g->spin_yaw_start = g->spin_yaw;
    }
    if (!g->spin_pressed || !input) return;
    float travel = mouse.x - g->spin_press_x;
    if (fabsf(travel) >= GALLERY_SPIN_DRAG_THRESHOLD) g->spin_dragged = true;
    if (g->spin_dragged) {
        g->spin_yaw = g->spin_yaw_start + travel * GALLERY_SPIN_RADIANS_PER_UI_PIXEL;
        HomeBackgroundGallerySetYaw(scene, g->spin_yaw);
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (!g->spin_dragged) HomeBackgroundGalleryPlayMelee(scene);
        g->spin_pressed = false;
    }
}

GalleryScreenAction GalleryScreenDraw(Gallery *g, Heroes *h, Player *p, HomeBackground *scene) {
    bool input = !UiInputBlocked();
    HeroesDrawDetailsAt(&g->card, GALLERY_DETAILS_RECT);

    Tier requested_level = g->selected_level;
    for (int tier = 0; tier < TIER_COUNT; tier++) {
        Rectangle r = {
            GALLERY_TIER_BUTTONS_X + tier * GALLERY_TIER_BUTTON_STRIDE,
            GALLERY_BOTTOM_BUTTONS_Y,
            GALLERY_TIER_BUTTON_SIZE,
            GALLERY_TIER_BUTTON_SIZE,
        };
        if ((Tier)tier == g->selected_level)
            ButtonDrawSelected(&g->tiers[tier], r);
        else if (ButtonDrawEx(&g->tiers[tier], r, input))
            requested_level = (Tier)tier;
    }

    int step = 0;
    if (g->unit_count > 1) {
        float height = GALLERY_NAV_BUTTON_W / 3.5f;
        Rectangle previous = {GALLERY_NAV_BUTTON_X, GALLERY_BOTTOM_BUTTONS_Y, GALLERY_NAV_BUTTON_W, height};
        Rectangle next = previous;
        next.x += GALLERY_NAV_BUTTON_W + GALLERY_NAV_BUTTON_GAP;
        if (ButtonDrawEx(&g->previous, previous, input)) step = -1;
        if (ButtonDrawEx(&g->next, next, input)) step = 1;
    }

    HeroesButton nav = HeroesDrawNav(h, HEROES_GALLERY, input, RuneUnseenCount(&p->runes, RuneNow()), p->gallery.badge);
    GalleryUpdateSpin(g, scene, input);
    if (requested_level != g->selected_level) GallerySelectLevel(g, p, scene, requested_level);
    if (step) GalleryStepUnit(g, p, scene, step);

    if (nav == HEROES_SQUAD) return GALLERY_SCREEN_ACTION_SQUAD;
    if (nav == HEROES_RUNES) return GALLERY_SCREEN_ACTION_RUNES;
    if (nav == HEROES_CONSUME) return GALLERY_SCREEN_ACTION_CONSUME;
    if (nav == HEROES_GALLERY) return GALLERY_SCREEN_ACTION_POPUP;
    return GALLERY_SCREEN_ACTION_NONE;
}
