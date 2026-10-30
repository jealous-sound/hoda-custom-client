---
name: raylib
description: Building games with raylib 5.5 in C on macOS — compiling/running, the game loop, drawing, input, textures, fonts, audio, 2D/3D cameras, collision, shaders, and avoiding common pitfalls. Use when writing or debugging raylib code in this project.
---

# raylib 5.5 (C, macOS / Apple Silicon)

raylib is a small C library for games and graphics. It is **immediate-mode**: there is no retained scene graph and no widgets to update. Every frame you wipe the framebuffer and redraw the entire world from your own game state, between `BeginDrawing()` and `EndDrawing()`. Your job is to keep that state in plain C structs, advance it each frame (using delta time), and paint it. Everything — input polling, textures, audio, 3D — lives inside one loop bracketed by `InitWindow()` / `CloseWindow()`.

## Build & run

A `Makefile` already exists in the project root with these targets:

```sh
make        # compile main.c -> ./game
make run    # build, then run ./game
make clean  # remove ./game
```

The exact compile command (Homebrew raylib, explicit paths — `pkg-config` is NOT installed):

```sh
cc -Wall -std=c99 -I/opt/homebrew/opt/raylib/include main.c -o game \
   -L/opt/homebrew/opt/raylib/lib -lraylib \
   -framework Cocoa -framework IOKit -framework OpenGL
```

- Headers: `/opt/homebrew/opt/raylib/include/` (`raylib.h`, `raymath.h`, `rlgl.h`).
- Library: `/opt/homebrew/opt/raylib/lib` (`libraylib.a`, `libraylib.dylib`).
- Required macOS frameworks when linking: **Cocoa, IOKit, OpenGL**.
- If you install pkg-config later, the shortcut is: `cc main.c -o game $(pkg-config --cflags --libs raylib)`.

## Program skeleton

A complete, minimal, correct loop. Copy this as a starting point:

```c
#include "raylib.h"

int main(void)
{
    // Context-time flags (MSAA/VSync/HighDPI) MUST be set BEFORE InitWindow.
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    InitWindow(800, 450, "heroes-of-dragons");   // create window + GL context first
    SetTargetFPS(60);                            // software frame cap

    // --- LOAD resources ONCE here (textures, sounds, fonts) ---

    while (!WindowShouldClose())                 // ESC or window close button
    {
        float dt = GetFrameTime();               // delta seconds (0 on first frame)

        // --- UPDATE: read input, advance game state, scale motion by dt ---

        BeginDrawing();
            ClearBackground(RAYWHITE);           // first call inside the pair
            // --- DRAW: paint the world from your state ---
            DrawFPS(10, 10);
        EndDrawing();                            // swaps buffers, polls input, paces FPS
    }

    // --- UNLOAD every resource you loaded, BEFORE CloseWindow ---

    CloseWindow();                               // destroys the GL context
    return 0;
}
```

> **Headers note:** `raylib.h` includes almost no standard library (only `<stdarg.h>`). Add `<stddef.h>` yourself for `NULL`, `<stdlib.h>` for `malloc`/`free`, `<stdio.h>` for `printf`. Use `<raymath.h>` for vector/matrix math and `TextFormat`/`TextCopy` for strings, and you rarely need the C stdlib at all.

## The three cardinal rules

1. **Load once, unload after — never inside the loop.** Call `LoadTexture`/`LoadSound`/`LoadFont`/`LoadModel`/`LoadRenderTexture` above the `while`, store the handles, and call the matching `Unload*` after the loop. Loading per frame leaks VRAM/RAM every frame and stutters until it crashes.
2. **Every `BeginX` has exactly one matching `EndX`, and all drawing happens between `BeginDrawing`/`EndDrawing`.** `BeginMode2D`/`BeginMode3D`/`BeginTextureMode`/`BeginShaderMode`/`BeginBlendMode`/`BeginScissorMode` each pair up and unwind LIFO. A missing `EndX` leaves the renderer in the wrong camera/shader/blend state for the rest of the frame; drawing outside the `BeginDrawing` pair is silently dropped.
3. **Move with delta time so speed is frame-rate independent.** `pos.x += speed * GetFrameTime();` — `SetTargetFPS` only caps the rate, it does not normalize motion. Without `dt`, the game runs faster on a 144 Hz display than on a 60 Hz one.

## Essentials cheat sheet

### Coordinate system
- Origin is **top-left**; **+X is right, +Y is DOWN.** Moving "up" on screen means *decreasing* y. Lerp/clamp math must respect this.
- Angles are in **degrees** for raylib draw calls (`DrawPoly`, `DrawRectanglePro`, `DrawTexturePro` rotation, `Camera2D.rotation`); raymath helpers use radians (convert with `DEG2RAD`/`RAD2DEG`).
- On Retina/HighDPI, `GetScreenWidth/Height` return logical units while `GetRenderWidth/Height` return physical pixels; the ratio is `GetWindowScaleDPI()`. Do gameplay/layout in screen units; use render units only for pixel-exact work (render textures, shaders).

### Input (poll every frame; updated by `EndDrawing`)
```c
bool  IsKeyDown(int key);            // held — continuous (movement); scale by dt
bool  IsKeyPressed(int key);         // one frame on key-down — discrete (jump, toggle)
int   GetCharPressed(void);          // drain in a loop for typed Unicode text
Vector2 GetMousePosition(void);      // screen (logical) coords
bool  IsMouseButtonPressed(int button);   // MOUSE_BUTTON_LEFT, ...
float GetMouseWheelMove(void);       // zoom/scroll (+up / -down)
bool  IsGamepadAvailable(int gamepad);    // gate before reading a pad
float GetGamepadAxisMovement(int gamepad, int axis);  // [-1,1]; apply a deadzone
```
ESC closes the window by default — call `SetExitKey(KEY_NULL)` if ESC is a gameplay key.

### 2D drawing & colors
```c
void DrawRectangleRec(Rectangle rec, Color color);          // float rect; use for game state
void DrawRectanglePro(Rectangle rec, Vector2 origin, float rotation, Color color);
void DrawCircleV(Vector2 center, float radius, Color color);
void DrawLineEx(Vector2 start, Vector2 end, float thick, Color color);  // wide lines
void DrawTriangle(Vector2 v1, Vector2 v2, Vector2 v3, Color color);     // CCW winding!
Color ColorAlpha(Color color, float alpha);                 // fade: alpha 0..1
```
Constants: `RAYWHITE WHITE BLACK BLANK RED MAROON GREEN LIME BLUE SKYBLUE GOLD ORANGE PURPLE DARKGRAY ...`. Draw order is painter's algorithm (background first). `DrawTriangle` vertices must be counter-clockwise or the face is culled and vanishes.

### Textures / sprites
Two tiers: **Image** = pixels in RAM (CPU edits), **Texture2D** = pixels in VRAM (the only thing you can draw).
```c
Texture2D LoadTexture(const char *fileName);                // file -> GPU
Texture2D LoadTextureFromImage(Image image);               // upload a CPU Image, then UnloadImage
void DrawTextureRec(Texture2D tex, Rectangle source, Vector2 pos, Color tint);   // atlas frame
void DrawTexturePro(Texture2D tex, Rectangle src, Rectangle dst,
                    Vector2 origin, float rotation, Color tint);                 // scale+rotate
void SetTextureFilter(Texture2D tex, int filter);          // TEXTURE_FILTER_POINT for pixel art
void UnloadTexture(Texture2D texture);
```
`DrawTexturePro` rotates about `origin`, an offset from `dst`'s top-left — set `origin = {dst.width/2, dst.height/2}` to spin about center. `tint` is a multiply; pass `WHITE` for true colors. RenderTexture2D color buffers are y-flipped: draw with a negative source height.

### Text (default font is ready right after `InitWindow`)
```c
void DrawText(const char *text, int posX, int posY, int fontSize, Color color);  // default font
int  MeasureText(const char *text, int fontSize);          // width, for centering
void DrawTextEx(Font font, const char *text, Vector2 pos, float size, float spacing, Color tint);
const char *TextFormat(const char *text, ...);             // printf-style; USE IMMEDIATELY
```
`TextFormat` returns a pointer into a small rotating internal buffer — pass it straight into a draw call; never store it across frames or it aliases/overwrites.

### Audio (one device, init once)
```c
void  InitAudioDevice(void);                               // before any load/play
Sound LoadSound(const char *fileName);                     // short SFX, fully in RAM
void  PlaySound(Sound sound);                              // re-call RESTARTS (no overlap)
Music LoadMusicStream(const char *fileName);               // long track, streamed from disk
void  UpdateMusicStream(Music music);                      // call EVERY frame or it stalls
void  CloseAudioDevice(void);                              // after unloading all audio
```
The classic bug: forgetting `UpdateMusicStream` every frame makes music play one buffer then go silent. For the same SFX overlapping itself, use `LoadSoundAlias`.

## Going deeper

This file is the entry point. For more, open the sibling files in this skill directory:

- **`reference.md`** — full categorized API quick-reference for raylib 5.5: every domain (core, input, shapes, textures, text, audio, camera2d, camera3d, collision/math, shaders, architecture, performance) with exact signatures, enums, and gotchas verified against the installed headers.
- **`patterns.md`** — game architecture and recipes: the `GameScreen` state machine, fixed-timestep accumulator vs. delta-time, entity pools with swap-and-pop, follow/clamp/zoom/shake cameras, collision response, **3D camera + mouse picking**, sprite-sheet animation, render-to-texture post-processing, persistence, runtime window control, the raygui/ecosystem note, and a performance/pitfalls checklist.
- **`examples/`** — seven self-contained, **asset-free** programs, each compiled & linked against this raylib 5.5: input/movement, 2D camera + collision, state machine, textures, **3D + picking**, text input, and tilemap. See `examples/README.md`; build each with the `cc` command above, swapping `main.c` for the example file.

Official references:
- Cheatsheet: https://www.raylib.com/cheatsheet/cheatsheet.html
- Examples gallery: https://www.raylib.com/examples.html
- Wiki: https://github.com/raysan5/raylib/wiki
