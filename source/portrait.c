#define PORTRAIT_SIZE 128
#define PORTRAIT_FRAME_BORDER 8
#define PORTRAIT_ART_BOTTOM 92
#define PORTRAIT_STAR_BAND_TOP 76
#define PORTRAIT_PLAQUE_TOP 104
#define PORTRAIT_PLAQUE_BOTTOM 120
#define PORTRAIT_STAR_MIN_GREEN 80
#define PORTRAIT_STAR_GREEN_LEAD 15
#define PORTRAIT_STAR_HALO 2
#define PORTRAIT_KEEPS_PAINTED_FRAME ((Rarity)0)

static const char *const PORTRAIT_STARLESS_FRAME_DONORS[RARITY_MYTHIC + 1] = {
    [RARITY_COMMON] = CACHE_RESOURCE("tex.c_avvargoat_common_t1_pt"),
    [RARITY_UNCOMMON] = CACHE_RESOURCE("tex.c_avvargoat_uncommon_t1_pt"),
    [RARITY_RARE] = CACHE_RESOURCE("tex.c_abomination_t1_pt"),
    [RARITY_EPIC] = CACHE_RESOURCE("tex.c_ashwraith_t1_pt"),
};

static Image portrait_frames[RARITY_MYTHIC + 1];

static bool PortraitHasPaintedLayout(Image image) {
    return image.data && image.width == PORTRAIT_SIZE && image.height == PORTRAIT_SIZE &&
           image.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
}

static Color *PortraitPixel(Image image, int x, int y) {
    return &((Color *)image.data)[y * image.width + x];
}

static bool PortraitInsideArt(int x, int y) {
    return x >= PORTRAIT_FRAME_BORDER && x < PORTRAIT_SIZE - PORTRAIT_FRAME_BORDER && y >= PORTRAIT_FRAME_BORDER &&
           y < PORTRAIT_ART_BOTTOM;
}

static bool PortraitIsLegendaryGreen(int red, int green, int blue) {
    return green >= PORTRAIT_STAR_MIN_GREEN && green - red >= PORTRAIT_STAR_GREEN_LEAD &&
           green - blue >= PORTRAIT_STAR_GREEN_LEAD;
}

static bool PortraitPixelIsLegendaryGreen(Image image, int x, int y) {
    Color pixel = *PortraitPixel(image, x, y);
    return PortraitIsLegendaryGreen(pixel.r, pixel.g, pixel.b);
}

static bool PortraitHasLegendaryPlaque(Image portrait) {
    long red = 0, green = 0, blue = 0, count = 0;
    for (int y = PORTRAIT_PLAQUE_TOP; y < PORTRAIT_PLAQUE_BOTTOM; y++) {
        for (int x = PORTRAIT_FRAME_BORDER; x < PORTRAIT_SIZE - PORTRAIT_FRAME_BORDER; x++) {
            Color pixel = *PortraitPixel(portrait, x, y);
            red += pixel.r;
            green += pixel.g;
            blue += pixel.b;
            count++;
        }
    }
    return PortraitIsLegendaryGreen((int)(red / count), (int)(green / count), (int)(blue / count));
}

typedef struct {
    bool covered[PORTRAIT_ART_BOTTOM][PORTRAIT_SIZE];
} PortraitStarMask;

static bool PortraitInsideStarBand(int x, int y) {
    return y >= PORTRAIT_STAR_BAND_TOP && PortraitInsideArt(x, y);
}

static void PortraitStarMaskCoverHalo(PortraitStarMask *mask, int star_x, int star_y) {
    for (int y = star_y - PORTRAIT_STAR_HALO; y <= star_y + PORTRAIT_STAR_HALO; y++)
        for (int x = star_x - PORTRAIT_STAR_HALO; x <= star_x + PORTRAIT_STAR_HALO; x++)
            if (PortraitInsideStarBand(x, y)) mask->covered[y][x] = true;
}

static bool PortraitIsCleanArt(const PortraitStarMask *mask, int x, int y) {
    return PortraitInsideArt(x, y) && !mask->covered[y][x];
}

typedef struct {
    float red, green, blue, weight;
} PortraitBlend;

static void PortraitBlendNearestCleanArt(
    PortraitBlend *blend, Image portrait, const PortraitStarMask *mask, int x, int y, int step_x, int step_y
) {
    int distance = 1;
    while (PortraitInsideArt(x + step_x * distance, y + step_y * distance) &&
           !PortraitIsCleanArt(mask, x + step_x * distance, y + step_y * distance))
        distance++;
    if (!PortraitIsCleanArt(mask, x + step_x * distance, y + step_y * distance)) return;
    Color pixel = *PortraitPixel(portrait, x + step_x * distance, y + step_y * distance);
    float weight = 1.0f / (float)distance;
    blend->red += pixel.r * weight;
    blend->green += pixel.g * weight;
    blend->blue += pixel.b * weight;
    blend->weight += weight;
}

static void PortraitPaintOverLegendaryStars(Image *portrait) {
    static PortraitStarMask mask;
    mask = (PortraitStarMask){0};
    for (int y = PORTRAIT_STAR_BAND_TOP; y < PORTRAIT_ART_BOTTOM; y++)
        for (int x = PORTRAIT_FRAME_BORDER; x < PORTRAIT_SIZE - PORTRAIT_FRAME_BORDER; x++)
            if (PortraitPixelIsLegendaryGreen(*portrait, x, y)) PortraitStarMaskCoverHalo(&mask, x, y);
    Image clean = ImageCopy(*portrait);
    for (int y = PORTRAIT_STAR_BAND_TOP; y < PORTRAIT_ART_BOTTOM; y++) {
        for (int x = PORTRAIT_FRAME_BORDER; x < PORTRAIT_SIZE - PORTRAIT_FRAME_BORDER; x++) {
            if (!mask.covered[y][x]) continue;
            PortraitBlend blend = {0};
            PortraitBlendNearestCleanArt(&blend, clean, &mask, x, y, -1, 0);
            PortraitBlendNearestCleanArt(&blend, clean, &mask, x, y, 1, 0);
            PortraitBlendNearestCleanArt(&blend, clean, &mask, x, y, 0, -1);
            if (blend.weight <= 0.0f) continue;
            Color *pixel = PortraitPixel(*portrait, x, y);
            pixel->r = (unsigned char)(blend.red / blend.weight + 0.5f);
            pixel->g = (unsigned char)(blend.green / blend.weight + 0.5f);
            pixel->b = (unsigned char)(blend.blue / blend.weight + 0.5f);
        }
    }
    UnloadImage(clean);
}

static void PortraitReframe(Image *portrait, Image frame) {
    if (!PortraitHasPaintedLayout(*portrait) || !PortraitHasPaintedLayout(frame)) return;
    if (PortraitHasLegendaryPlaque(*portrait)) PortraitPaintOverLegendaryStars(portrait);
    for (int y = 0; y < PORTRAIT_SIZE; y++)
        for (int x = 0; x < PORTRAIT_SIZE; x++)
            if (!PortraitInsideArt(x, y)) *PortraitPixel(*portrait, x, y) = *PortraitPixel(frame, x, y);
}

static Image PortraitFrameFor(Rarity rarity) {
    if (rarity < RARITY_COMMON || rarity > RARITY_MYTHIC || !PORTRAIT_STARLESS_FRAME_DONORS[rarity]) return (Image){0};
    Image *frame = &portrait_frames[rarity];
    if (!frame->data) {
        *frame = ResourceLoadImage(PORTRAIT_STARLESS_FRAME_DONORS[rarity]);
        if (frame->data) ImageFormat(frame, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    }
    return *frame;
}

void PortraitFramesUnload(void) {
    for (int rarity = 0; rarity <= RARITY_MYTHIC; rarity++) {
        if (portrait_frames[rarity].data) UnloadImage(portrait_frames[rarity]);
        portrait_frames[rarity] = (Image){0};
    }
}

const char *GuiPortraitRef(const char *icon) {
    const char *original = TextFormat(CACHE_RESOURCE("%s"), icon);
    return ResourceExists(original) ? original : TextFormat(POST_4_4_PORTRAIT_DIR "%s.png", icon);
}

Texture2D GuiPortraitLoad(const char *icon, Rarity rarity) {
    const char *ref = GuiPortraitRef(icon);
    if (!ResourceExists(ref)) TraceLog(LOG_FATAL, "Assets: missing portrait %s", icon);
    Image image = ResourceLoadImage(ref);
    if (!image.data) TraceLog(LOG_FATAL, "Assets: failed to load portrait %s", ref);
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    PortraitReframe(&image, PortraitFrameFor(rarity));
    Texture2D t = LoadTextureFromImage(image);
    UnloadImage(image);
    if (t.id == 0) TraceLog(LOG_FATAL, "Assets: failed to load portrait %s", ref);
    GenTextureMipmaps(&t);
    SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
    return t;
}
