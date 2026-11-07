#define BUTTON_BADGE_MAX 99
#define BUTTON_LABEL_MAX_WIDTH 0.84f

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

Button ButtonFromTextures(
    Texture2D idle, Texture2D active, const char *label, Font font, Vector2 label_anchor, float label_scale
) {
    return (Button){
        .idle = idle,
        .active = active,
        .label = label,
        .font = font,
        .label_anchor = label_anchor,
        .label_scale = label_scale,
        .label_color = (Color){235, 232, 220, 255},
        .click_allow_duplicate = false,
    };
}

Button ButtonLoadText(GuiId idle, GuiId active, const char *label, Font font, Vector2 label_anchor, float label_scale) {
    Button button = ButtonFromTextures(GuiTex(idle), GuiTex(active), label, font, label_anchor, label_scale);
    button.click = GuiSound(SND_UI_CLICK);
    return button;
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
        float room = dst.width * BUTTON_LABEL_MAX_WIDTH;
        if (measured.x > room) {
            size *= room / measured.x;
            measured = MeasureTextEx(button->font, button->label, size, 1.0f);
        }
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

void ButtonDrawBadge(Rectangle button, int count) {
    if (count <= 0) return;
    char text[8];
    snprintf(text, sizeof(text), "%d", count > BUTTON_BADGE_MAX ? BUTTON_BADGE_MAX : count);
    Vector2 center = {button.x + button.width * 0.8f, button.y + button.height * 0.2f};
    DrawCircleV(center, 14.0f, (Color){190, 30, 30, 255});
    UiDrawTextCentered(
        GuiFont(FNT_MENU), text, (Rectangle){center.x - 14.0f, center.y - 14.0f, 28.0f, 28.0f}, 18.0f, WHITE
    );
}

bool ButtonDraw(Button *b, Rectangle dst) {
    return ButtonDrawEx(b, dst, true);
}
