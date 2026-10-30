typedef struct {
    Texture2D idle;
    Texture2D active;
    const char *label;
    Font font;
    Vector2 label_anchor;
    float label_scale;
    Color label_color;
    Sound click;
    bool click_allow_duplicate;
    bool armed;
} Button;

Button ButtonLoadText(GuiId idle, GuiId active, const char *label, Font font, Vector2 label_anchor, float label_scale) {
    return (Button){
        .idle = GuiTex(idle),
        .active = GuiTex(active),
        .label = label,
        .font = font,
        .label_anchor = label_anchor,
        .label_scale = label_scale,
        .label_color = (Color){235, 232, 220, 255},
        .click = GuiSound(SND_UI_CLICK),
        .click_allow_duplicate = false,
    };
}

Button ButtonLoad(GuiId idle, GuiId active) {
    return ButtonLoadText(idle, active, NULL, (Font){0}, (Vector2){0, 0}, 0.0f);
}

void ButtonSetClickSound(Button *button, SoundId sound) {
    button->click = GuiSound(sound);
    button->click_allow_duplicate = GuiSoundAllowsDuplicate(sound);
}

void ButtonDisableClickSound(Button *button) {
    button->click = (Sound){0};
    button->click_allow_duplicate = false;
}

void ButtonSetLabelColor(Button *button, Color color) {
    button->label_color = color;
}

static void ButtonDrawVisual(const Button *button, Rectangle dst, bool selected) {
    if (dst.width == -1) dst.width = button->idle.width;
    if (dst.height == -1) dst.height = button->idle.height;

    Texture2D texture = selected ? button->active : button->idle;
    UiDrawTexture(texture, dst, WHITE);

    if (button->label) {
        float size = dst.height * button->label_scale;
        Vector2 measured = MeasureTextEx(button->font, button->label, size, 1.0f);
        Vector2 position = {
            dst.x + dst.width * button->label_anchor.x - measured.x * 0.5f,
            dst.y + dst.height * button->label_anchor.y - measured.y * 0.5f,
        };
        DrawTextEx(button->font, button->label, position, size, 1.0f, button->label_color);
    }
}

void ButtonDrawSelected(Button *button, Rectangle dst) {
    button->armed = false;
    ButtonDrawVisual(button, dst, true);
}

bool ButtonDrawEx(Button *b, Rectangle dst, bool interactive) {
    if (dst.width == -1) {
        dst.width = b->idle.width;
    }
    if (dst.height == -1) {
        dst.height = b->idle.height;
    }

    if (!interactive) {
        b->armed = false;
    }

    bool hover = interactive && CheckCollisionPointRec(UiMouse(), dst);

    if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        b->armed = true;
    }

    ButtonDrawVisual(b, dst, hover && b->armed);

    bool clicked = hover && b->armed && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        b->armed = false;
    }
    if (clicked && b->click.frameCount > 0) {
        GuiSoundPlayHandle(b->click, b->click_allow_duplicate);
    }
    return clicked;
}

bool ButtonDraw(Button *b, Rectangle dst) {
    return ButtonDrawEx(b, dst, true);
}
