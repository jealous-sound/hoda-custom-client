#define main hod_game_main
#include "../source/main.c"
#undef main

#include <assert.h>

#define TEST_STAR_LEFT 58
#define TEST_STAR_RIGHT 70
#define TEST_STAR_TOP 82
#define TEST_STAR_BOTTOM 97
#define TEST_ART_SAMPLE_X 30
#define TEST_ART_SAMPLE_Y 40
#define TEST_PLAQUE_SAMPLE_Y 112

static const Color TEST_ART = {150, 90, 70, 255};
static const Color TEST_GREEN_ART = {60, 200, 70, 255};
static const Color TEST_LEGENDARY_FRAME = {89, 140, 85, 255};
static const Color TEST_STAR = {140, 215, 90, 255};
static const Color TEST_RARE_FRAME = {190, 137, 46, 255};
static const Color TEST_EPIC_FRAME = {98, 33, 18, 255};

static bool TestSameColor(Color a, Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

static Image TestPortrait(Color art, Color frame) {
    Image portrait = GenImageColor(PORTRAIT_SIZE, PORTRAIT_SIZE, frame);
    for (int y = 0; y < PORTRAIT_SIZE; y++)
        for (int x = 0; x < PORTRAIT_SIZE; x++)
            if (PortraitInsideArt(x, y)) *PortraitPixel(portrait, x, y) = art;
    return portrait;
}

static void TestPaintStar(Image portrait) {
    for (int y = TEST_STAR_TOP; y < TEST_STAR_BOTTOM; y++)
        for (int x = TEST_STAR_LEFT; x < TEST_STAR_RIGHT; x++) *PortraitPixel(portrait, x, y) = TEST_STAR;
}

static void TestAssertFrameIs(Image portrait, Color frame) {
    for (int y = 0; y < PORTRAIT_SIZE; y++)
        for (int x = 0; x < PORTRAIT_SIZE; x++)
            if (!PortraitInsideArt(x, y)) assert(TestSameColor(*PortraitPixel(portrait, x, y), frame));
}

static void TestAssertArtIs(Image portrait, Color art) {
    for (int y = 0; y < PORTRAIT_SIZE; y++)
        for (int x = 0; x < PORTRAIT_SIZE; x++)
            if (PortraitInsideArt(x, y)) assert(TestSameColor(*PortraitPixel(portrait, x, y), art));
}

static void TestADemotedLegendaryTakesItsNewFrameAndLosesItsStars(void) {
    Image portrait = TestPortrait(TEST_ART, TEST_LEGENDARY_FRAME);
    Image frame = TestPortrait(TEST_GREEN_ART, TEST_EPIC_FRAME);
    TestPaintStar(portrait);
    PortraitReframe(&portrait, frame);
    TestAssertFrameIs(portrait, TEST_EPIC_FRAME);
    TestAssertArtIs(portrait, TEST_ART);
    UnloadImage(frame);
    UnloadImage(portrait);
}

static void TestGreenArtUnderAnotherFrameIsKept(void) {
    Image portrait = TestPortrait(TEST_GREEN_ART, TEST_EPIC_FRAME);
    Image frame = TestPortrait(TEST_ART, TEST_RARE_FRAME);
    PortraitReframe(&portrait, frame);
    TestAssertFrameIs(portrait, TEST_RARE_FRAME);
    TestAssertArtIs(portrait, TEST_GREEN_ART);
    UnloadImage(frame);
    UnloadImage(portrait);
}

static void TestARarityWithoutAStarlessFrameKeepsThePaintedOne(void) {
    Image portrait = TestPortrait(TEST_ART, TEST_LEGENDARY_FRAME);
    TestPaintStar(portrait);
    PortraitReframe(&portrait, PortraitFrameFor(RARITY_LEGENDARY));
    PortraitReframe(&portrait, PortraitFrameFor(RARITY_MYTHIC));
    PortraitReframe(&portrait, PortraitFrameFor(PORTRAIT_KEEPS_PAINTED_FRAME));
    assert(TestSameColor(*PortraitPixel(portrait, TEST_STAR_LEFT, TEST_STAR_TOP), TEST_STAR));
    assert(TestSameColor(*PortraitPixel(portrait, TEST_ART_SAMPLE_X, TEST_ART_SAMPLE_Y), TEST_ART));
    assert(TestSameColor(*PortraitPixel(portrait, TEST_ART_SAMPLE_X, TEST_PLAQUE_SAMPLE_Y), TEST_LEGENDARY_FRAME));
    UnloadImage(portrait);
}

static void TestAPortraitOfAnotherSizeIsLeftAlone(void) {
    Image portrait = GenImageColor(PORTRAIT_SIZE / 2, PORTRAIT_SIZE / 2, TEST_ART);
    Image frame = TestPortrait(TEST_ART, TEST_RARE_FRAME);
    PortraitReframe(&portrait, frame);
    assert(TestSameColor(GetImageColor(portrait, 0, 0), TEST_ART));
    UnloadImage(frame);
    UnloadImage(portrait);
}

int main(void) {
    SetTraceLogLevel(LOG_WARNING);
    TestADemotedLegendaryTakesItsNewFrameAndLosesItsStars();
    TestGreenArtUnderAnotherFrameIsKept();
    TestARarityWithoutAStarlessFrameKeepsThePaintedOne();
    TestAPortraitOfAnotherSizeIsLeftAlone();
    printf("portrait tests passed\n");
    return 0;
}
