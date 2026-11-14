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

typedef struct {
    bool asked;
    Texture2D texture;
} RenownArtSlot;

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
