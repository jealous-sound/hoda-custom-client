#define main hod_game_main
#include "../source/main.c"
#undef main

#include <assert.h>

#define TEST_ROW_COUNT 4
#define TEST_NO_ROW (-1)
#define TEST_ROW_STRIDE 64.0f
#define TEST_BUTTON_WIDTH 124.0f
#define TEST_BUTTON_HEIGHT 48.0f

static const Vector2 TEST_OUTSIDE_EVERY_ROW = {400.0f, 400.0f};

static Rectangle TestRowButton(int row) {
    return (Rectangle){0.0f, row * TEST_ROW_STRIDE, TEST_BUTTON_WIDTH, TEST_BUTTON_HEIGHT};
}

static Vector2 TestRowCentre(int row) {
    Rectangle button = TestRowButton(row);
    return (Vector2){button.x + button.width * 0.5f, button.y + button.height * 0.5f};
}

static int TestDrawRows(Button *shared, ButtonPointer pointer, bool interactive) {
    int clicked = TEST_NO_ROW;
    for (int row = 0; row < TEST_ROW_COUNT; row++)
        if (ButtonTouchAt(shared, TestRowButton(row), interactive, pointer).clicked) clicked = row;
    return clicked;
}

static void TestPress(Button *shared, Vector2 at) {
    assert(TestDrawRows(shared, (ButtonPointer){.position = at, .pressed = true, .down = true}, true) == TEST_NO_ROW);
}

static void TestHold(Button *shared, Vector2 at, bool interactive) {
    assert(TestDrawRows(shared, (ButtonPointer){.position = at, .down = true}, interactive) == TEST_NO_ROW);
}

static int TestRelease(Button *shared, Vector2 at, bool interactive) {
    int clicked = TestDrawRows(shared, (ButtonPointer){.position = at, .released = true}, interactive);
    assert(TestDrawRows(shared, (ButtonPointer){.position = at}, interactive) == TEST_NO_ROW);
    return clicked;
}

static void TestEveryRowOfASharedButtonClicks(void) {
    Button shared = {0};
    for (int row = 0; row < TEST_ROW_COUNT; row++) {
        TestPress(&shared, TestRowCentre(row));
        TestHold(&shared, TestRowCentre(row), true);
        assert(TestRelease(&shared, TestRowCentre(row), true) == row);
    }
}

static void TestOnlyThePressedRowIsHeld(void) {
    Button shared = {0};
    ButtonPointer pressing = {.position = TestRowCentre(2), .pressed = true, .down = true};
    TestDrawRows(&shared, pressing, true);
    ButtonPointer holding = {.position = TestRowCentre(2), .down = true};
    for (int row = 0; row < TEST_ROW_COUNT; row++)
        assert(ButtonTouchAt(&shared, TestRowButton(row), true, holding).held == (row == 2));
}

static void TestAPressReleasedOnAnotherRowClicksNothing(void) {
    Button shared = {0};
    TestPress(&shared, TestRowCentre(1));
    TestHold(&shared, TestRowCentre(3), true);
    assert(TestRelease(&shared, TestRowCentre(3), true) == TEST_NO_ROW);
}

static void TestAPressFromOutsideClicksNothing(void) {
    Button shared = {0};
    TestPress(&shared, TEST_OUTSIDE_EVERY_ROW);
    TestHold(&shared, TestRowCentre(0), true);
    assert(TestRelease(&shared, TestRowCentre(0), true) == TEST_NO_ROW);
}

static void TestAListThatStartsScrollingDropsThePress(void) {
    Button shared = {0};
    TestPress(&shared, TestRowCentre(1));
    TestHold(&shared, TestRowCentre(1), false);
    assert(TestRelease(&shared, TestRowCentre(1), true) == TEST_NO_ROW);
}

static void TestAnAbandonedPressDoesNotClickLater(void) {
    Button shared = {0};
    TestPress(&shared, TestRowCentre(0));
    assert(TestRelease(&shared, TEST_OUTSIDE_EVERY_ROW, true) == TEST_NO_ROW);
    TestPress(&shared, TEST_OUTSIDE_EVERY_ROW);
    TestHold(&shared, TestRowCentre(0), true);
    assert(TestRelease(&shared, TestRowCentre(0), true) == TEST_NO_ROW);
}

static void TestARestyledButtonKeepsItsPress(void) {
    Button restyled = {0};
    Button style = {.label = "Send"};
    for (int row = 0; row < TEST_ROW_COUNT; row++) {
        ButtonRestyle(&restyled, style);
        TestPress(&restyled, TestRowCentre(row));
        ButtonRestyle(&restyled, style);
        TestHold(&restyled, TestRowCentre(row), true);
        ButtonRestyle(&restyled, style);
        assert(TestRelease(&restyled, TestRowCentre(row), true) == row);
        assert(restyled.label == style.label);
    }
}

int main(void) {
    TestEveryRowOfASharedButtonClicks();
    TestOnlyThePressedRowIsHeld();
    TestAPressReleasedOnAnotherRowClicksNothing();
    TestAPressFromOutsideClicksNothing();
    TestAListThatStartsScrollingDropsThePress();
    TestAnAbandonedPressDoesNotClickLater();
    TestARestyledButtonKeepsItsPress();
    printf("button tests passed\n");
    return 0;
}
