/*
 * 06_text_input.c — Text entry with raylib 5.5.
 *
 * The text-input recipe every game's name-entry / console / chat box needs:
 *   - GetCharPressed() returns queued Unicode codepoints; drain it in a loop
 *     each frame so fast typing isn't dropped.
 *   - KEY_BACKSPACE deletes; IsKeyPressedRepeat makes held-backspace repeat.
 *   - A blinking caret driven by GetTime().
 *
 * Asset-free (default font). Build (from this folder):
 *   cc -Wall -std=c99 -I/opt/homebrew/opt/raylib/include 06_text_input.c -o demo \
 *      -L/opt/homebrew/opt/raylib/lib -lraylib -framework Cocoa -framework IOKit -framework OpenGL
 */
#include "raylib.h"

#define MAX_INPUT 48

int main(void)
{
    InitWindow(800, 450, "raylib - text input");
    SetTargetFPS(60);

    char text[MAX_INPUT + 1] = "";    // always null-terminated for DrawText
    int  length = 0;
    Rectangle box = { 200, 180, 400, 50 };

    while (!WindowShouldClose())
    {
        // Drain EVERY character typed this frame (handles fast typing).
        int key = GetCharPressed();
        while (key > 0)
        {
            if (key >= 32 && key <= 126 && length < MAX_INPUT)   // printable ASCII
            {
                text[length++] = (char)key;
                text[length] = '\0';
            }
            key = GetCharPressed();    // next queued codepoint (0 when drained)
        }

        // Backspace: one press, and again while held (key-repeat).
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && length > 0)
            text[--length] = '\0';

        bool hover = CheckCollisionPointRec(GetMousePosition(), box);
        SetMouseCursor(hover ? MOUSE_CURSOR_IBEAM : MOUSE_CURSOR_DEFAULT);

        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("Type something (Backspace to delete):", 200, 145, 20, DARKGRAY);
            DrawRectangleRec(box, LIGHTGRAY);
            DrawRectangleLinesEx(box, 2.0f, hover ? RED : DARKGRAY);
            DrawText(text, (int)box.x + 8, (int)box.y + 15, 20, MAROON);

            // Blinking caret positioned right after the current text.
            if (((int)(GetTime() * 2.0)) % 2 == 0)
            {
                int w = MeasureText(text, 20);
                DrawText("_", (int)box.x + 10 + w, (int)box.y + 18, 20, MAROON);
            }
            DrawText(TextFormat("%i / %i", length, MAX_INPUT), 200, 250, 18, GRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
