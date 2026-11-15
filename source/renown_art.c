#define RENOWN_ART_EMBLEM "emblem"
#define RENOWN_ART_STAR "star"
#define RENOWN_NUMERAL_CENTRE_Y 0.447f
#define RENOWN_PLAIN_STAR_CENTRE_Y 0.5f
#define RENOWN_PLAIN_STAR_SCALE 0.6f
#define RENOWN_NUMERAL_WIDTH 0.34f
#define RENOWN_NUMERAL_HEIGHT 0.23f
#define RENOWN_NUMERAL_SIZE 0.18f
#define RENOWN_NUMERAL_OUTLINE 0.04f
#define RENOWN_NUMERAL_OUTLINE_COLOR ((Color){30, 18, 6, 230})
#define RENOWN_NAME_STAR_SIDE 1.3f
#define RENOWN_NAME_STAR_GAP 0.2f
#define RENOWN_NAME_LEFT 0.0f
#define RENOWN_NAME_CENTRE 0.5f

typedef struct {
    bool asked;
    Texture2D texture;
} RenownArtSlot;

typedef struct {
    Rectangle star;
    Rectangle text;
    float text_size;
} RenownNameLayout;

typedef struct {
    int renown_rank;
    int vault_tier;
} NameBadges;

static RenownArtSlot renown_emblems[RENOWN_TIER_COUNT];
static RenownArtSlot renown_stars[RENOWN_TIER_COUNT];

static Texture2D RenownArtLoad(RenownArtSlot *slot, const char *kind, RenownTier tier) {
    if (slot->asked) return slot->texture;
    slot->asked = true;
    const char *path = TextFormat(EXTRA_ASSET_DIR "gui/renown/%s_%s.png", kind, RENOWN_TIER_IDS[tier]);
    if (tier == RENOWN_TIER_NONE || !ResourceExists(path)) return slot->texture;
    slot->texture = ResourceLoadTexture(path);
    if (slot->texture.id) {
        GenTextureMipmaps(&slot->texture);
        SetTextureFilter(slot->texture, TEXTURE_FILTER_TRILINEAR);
    }
    return slot->texture;
}

Texture2D RenownStarTexture(RenownTier tier) {
    return RenownArtLoad(&renown_stars[tier], RENOWN_ART_STAR, tier);
}

static bool RenownHasEmblemArt(RenownTier tier) {
    return RenownArtLoad(&renown_emblems[tier], RENOWN_ART_EMBLEM, tier).id != 0;
}

Texture2D RenownEmblemTexture(RenownTier tier) {
    return RenownHasEmblemArt(tier) ? renown_emblems[tier].texture : GuiTex(GUI_CARD_LEVEL);
}

static void RenownDrawFit(Texture2D texture, Rectangle dst, Color tint) {
    if (texture.id == 0 || texture.width <= 0 || texture.height <= 0) return;
    float scale = fminf(dst.width / texture.width, dst.height / texture.height);
    float width = texture.width * scale, height = texture.height * scale;
    UiDrawTexture(
        texture,
        (Rectangle){dst.x + (dst.width - width) * 0.5f, dst.y + (dst.height - height) * 0.5f, width, height},
        tint
    );
}

static float RenownFittedTextSize(Font font, const char *text, float width, float size) {
    float measured = MeasureTextEx(font, text, size, 1.0f).x;
    return measured > width && measured > 0.0f ? size * width / measured : size;
}

static void RenownDrawNumeral(const char *text, Rectangle rect, float size, Color tint) {
    static const Vector2 directions[8] = {
        {-1.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, -1.0f},
        {0.0f, 1.0f},
        {-0.75f, -0.75f},
        {0.75f, -0.75f},
        {-0.75f, 0.75f},
        {0.75f, 0.75f},
    };
    Font font = GuiFont(FNT_COOL);
    size = RenownFittedTextSize(font, text, rect.width, size);
    float outline = size * RENOWN_NUMERAL_OUTLINE;
    Color shade = RENOWN_NUMERAL_OUTLINE_COLOR;
    shade.a = (unsigned char)((int)shade.a * tint.a / 255);
    for (int i = 0; i < 8; i++) {
        Rectangle shifted = rect;
        shifted.x += directions[i].x * outline;
        shifted.y += directions[i].y * outline;
        UiDrawTextCentered(font, text, shifted, size, shade);
    }
    UiDrawTextCentered(font, text, rect, size, tint);
}

void RenownDrawEmblem(int rank, Rectangle rect, Color tint) {
    RenownTier tier = RenownTierForRank(rank);
    bool plain_star = !RenownHasEmblemArt(tier);
    float art_scale = plain_star ? RENOWN_PLAIN_STAR_SCALE : 1.0f;
    float numeral_centre = plain_star ? RENOWN_PLAIN_STAR_CENTRE_Y : RENOWN_NUMERAL_CENTRE_Y;
    Rectangle art = UiRect(rect, 0.5f, numeral_centre, rect.width * art_scale, rect.height * art_scale);
    RenownDrawFit(RenownEmblemTexture(tier), art, tint);
    Rectangle numeral = UiRect(rect, 0.5f, numeral_centre, RENOWN_NUMERAL_WIDTH, RENOWN_NUMERAL_HEIGHT);
    RenownDrawNumeral(TextFormat("%d", rank), numeral, rect.height * RENOWN_NUMERAL_SIZE * art_scale, tint);
}

bool RenownNameHasStar(int rank) {
    return RenownStarTexture(RenownTierForRank(rank)).id != 0;
}

RenownNameLayout RenownLayOutName(int badges, Rectangle rect, float size, float name_width, float anchor) {
    float side = badges > 0 ? size * RENOWN_NAME_STAR_SIDE : 0.0f;
    float advance = badges > 0 ? badges * (side + size * RENOWN_NAME_STAR_GAP) : 0.0f;
    float room = fmaxf(rect.width - advance, 0.0f);
    float fitted_width = fminf(name_width, room);
    float left = rect.x + (rect.width - advance - fitted_width) * anchor;
    return (RenownNameLayout){
        .star = {left, rect.y + (rect.height - side) * 0.5f, side, side},
        .text = {left + advance, rect.y, fitted_width, rect.height},
        .text_size = name_width > room && name_width > 0.0f ? size * room / name_width : size,
    };
}

NameBadges PlayerNameBadges(const Player *player) {
    return (NameBadges){RenownRank(player->experience), VaultTierFor(player->vault.locked)};
}

static int NameBadgesTextures(NameBadges badges, Texture2D *out) {
    int count = 0;
    if (RenownNameHasStar(badges.renown_rank)) out[count++] = RenownStarTexture(RenownTierForRank(badges.renown_rank));
    Texture2D seal = VaultSealTexture((VaultTier)badges.vault_tier);
    if (seal.id) out[count++] = seal;
    return count;
}

static void NameBadgesDrawAt(
    NameBadges badges, Font font, const char *name, Rectangle rect, float size, Color color, float anchor
) {
    Texture2D textures[2];
    int count = NameBadgesTextures(badges, textures);
    float name_width = MeasureTextEx(font, name, size, 1.0f).x;
    RenownNameLayout layout = RenownLayOutName(count, rect, size, name_width, anchor);
    Rectangle badge = layout.star;
    for (int i = 0; i < count && badge.width > 0.0f; i++) {
        UiDrawTexture(textures[i], badge, (Color){255, 255, 255, color.a});
        badge.x += badge.width + size * RENOWN_NAME_STAR_GAP;
    }
    UiDrawTextLeftMiddle(font, name, layout.text, layout.text_size, color);
}

void NameBadgesDraw(NameBadges badges, Font font, const char *name, Rectangle rect, float size, Color color) {
    NameBadgesDrawAt(badges, font, name, rect, size, color, RENOWN_NAME_LEFT);
}

void NameBadgesDrawCentred(NameBadges badges, Font font, const char *name, Rectangle rect, float size, Color color) {
    NameBadgesDrawAt(badges, font, name, rect, size, color, RENOWN_NAME_CENTRE);
}
