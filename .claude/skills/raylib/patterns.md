# raylib 5.5 Game Patterns & Recipes (macOS / C)

This is the architecture-and-recipes companion to the raylib reference. It assumes raylib 5.5 installed via Homebrew on Apple Silicon. Every recipe compiles with the project Makefile (`make`, `make run`, `make clean`), which expands to:

```
cc -Wall -std=c99 -I/opt/homebrew/opt/raylib/include main.c -o game \
   -L/opt/homebrew/opt/raylib/lib -lraylib -framework Cocoa -framework IOKit -framework OpenGL
```

`pkg-config` is not installed, so the Makefile uses explicit paths. Once you `brew install pkg-config` you can switch to `cc main.c -o game $(pkg-config --cflags --libs raylib)`.

> **Standard-library includes:** `raylib.h` deliberately includes almost nothing (only `<stdarg.h>`). If a file uses `NULL` add `<stddef.h>`; for `malloc`/`free` add `<stdlib.h>`; for `printf` add `<stdio.h>`; for `strlen`/`memcpy` add `<string.h>`. raylib provides `Vector*`/`Clamp`/`Lerp` etc. via `raymath.h`, and `TextFormat`/`TextCopy` so you rarely need `<stdio.h>`/`<string.h>` for text.

Read this top to bottom the first time: each section builds on the structural discipline of the one before it. After that, treat it as a recipe box — every snippet is self-contained and copy-pasteable.

---

## 1. Program structure & the update/draw split

A raylib program has exactly one shape: configure flags, create the window, load every resource **once**, run a loop that updates then draws, unload everything, close. Drawing functions need a live GL context, so they only ever appear between `InitWindow` and `CloseWindow`, inside a `BeginDrawing`/`EndDrawing` pair.

The single most important discipline is the **update/draw split**: `UpdateGame(dt)` mutates state and reads input; `DrawGame()` only renders and never changes state. Keeping them apart makes pausing, fixed-timestep physics, and module splitting trivial, and it stops the classic bug where game logic accidentally depends on draw order.

```c
#include "raylib.h"

typedef struct {
    Vector2 player;
    float   speed;
} Game;

// Mutates state, reads input. NEVER draws.
static void UpdateGame(Game *g, float dt)
{
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) g->player.x += g->speed * dt;
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) g->player.x -= g->speed * dt;
    if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) g->player.y += g->speed * dt;
    if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) g->player.y -= g->speed * dt;
}

// Renders only. Takes state by const pointer so it CAN'T mutate it.
static void DrawGame(const Game *g)
{
    BeginDrawing();
        ClearBackground(RAYWHITE);             // first call inside the pair
        DrawCircleV(g->player, 20.0f, MAROON);
        DrawText("WASD / arrows to move", 10, 10, 20, DARKGRAY);
        DrawFPS(10, 40);
    EndDrawing();                              // swap buffers, poll input, pace FPS
}

int main(void)
{
    SetConfigFlags(FLAG_VSYNC_HINT);           // BEFORE InitWindow (context-time hint)
    InitWindow(800, 450, "structure");         // window + GL context
    SetTargetFPS(60);                          // AFTER InitWindow

    // ---- LOAD ONCE, before the loop ----
    Game game = { .player = { 400, 225 }, .speed = 300.0f };

    // ---- MAIN LOOP: update(dt) then draw ----
    while (!WindowShouldClose())                // ESC or close button
    {
        float dt = GetFrameTime();             // delta seconds (0 on the first frame)
        UpdateGame(&game, dt);
        DrawGame(&game);
    }

    // ---- UNLOAD, then close (no GPU resources here yet) ----
    CloseWindow();                             // destroys the GL context
    return 0;
}
```

Rules baked into this skeleton:

- `SetConfigFlags` / `SetTraceLogLevel` go **before** `InitWindow`. `SetTargetFPS`, `SetTextureFilter`, and any `Load*` go **after** it.
- `GetFrameTime()` returns `0` on the very first frame — never divide by it without guarding.
- `WindowShouldClose()` also pumps input internally; `EndDrawing()` is where buffers swap and the FPS cap is enforced.

---

## 2. Delta-time movement vs a fixed-timestep accumulator

There are two ways to advance the simulation, and they solve different problems.

**Delta-time integration** multiplies every velocity and timer by `GetFrameTime()`. It is simple and frame-rate independent, and it is the right default for movement, animation, UI, and anything cosmetic. Its weakness: when `dt` spikes (you dragged the window, alt-tabbed, hit a GC-like hitch), a fast object can *tunnel* straight through a wall in a single huge step, and the result is not deterministic frame-to-frame.

**Fixed-timestep with an accumulator** runs the physics step at a constant `dt` (e.g. 1/120 s) regardless of render rate. It is deterministic and tunnel-resistant, which is what you want for collision-critical physics, networked sims, or replay. You bank real elapsed time in an accumulator and drain it in fixed slices, clamping the accumulator so a long stall can't trigger a "spiral of death" (more steps than the frame can afford, which makes the next frame longer, which schedules even more steps).

Use delta-time for the player's feel and effects; use the fixed accumulator for the part of the world that must not glitch. They coexist happily.

```c
#include "raylib.h"
#include "raymath.h"   // Vector2Add, Vector2Scale

typedef struct { Vector2 pos, vel; } Body;

// --- Fixed-timestep physics: deterministic, tunnel-resistant ---
static void PhysicsStep(Body *b, float fixedDt)
{
    b->vel.y += 980.0f * fixedDt;                              // gravity
    b->pos = Vector2Add(b->pos, Vector2Scale(b->vel, fixedDt));
    if (b->pos.y > 400.0f) { b->pos.y = 400.0f; b->vel.y *= -0.7f; } // bounce
}

int main(void)
{
    InitWindow(800, 450, "delta vs fixed timestep");
    SetTargetFPS(60);

    Body ball   = { { 400, 50 }, { 0, 0 } };
    Vector2 cursor = { 400, 225 };

    const float FIXED_DT = 1.0f / 120.0f;   // 120 Hz physics
    float accumulator = 0.0f;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // (A) Delta-time movement: cosmetic / input-driven, frame-rate independent
        float speed = 350.0f;
        if (IsKeyDown(KEY_RIGHT)) cursor.x += speed * dt;
        if (IsKeyDown(KEY_LEFT))  cursor.x -= speed * dt;

        // (B) Fixed-timestep physics: bank time, drain in constant slices
        accumulator += dt;
        if (accumulator > 0.25f) accumulator = 0.25f;          // clamp the spiral of death
        while (accumulator >= FIXED_DT)
        {
            PhysicsStep(&ball, FIXED_DT);
            accumulator -= FIXED_DT;
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawCircleV(ball.pos, 18.0f, RED);                 // fixed-step body
            DrawCircleV(cursor, 10.0f, BLUE);                  // delta-step cursor
            DrawText("RED = fixed physics, BLUE = delta-time input", 10, 10, 20, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
```

Note that `SetTargetFPS(60)` only *caps* the rate — it does not pin it. Under load the loop runs slower, so never assume a frame is exactly 1/60 s; always read `GetFrameTime()`.

---

## 3. Game state machine — `GameScreen`

Model game flow as an `enum` and run two parallel `switch` statements: one in update (logic + transitions), one in draw (rendering). This is the canonical raylib `core_basic_screen_manager` pattern. The cardinal rule is that **screen transitions happen only in update** — draw never changes `currentScreen`.

```c
#include "raylib.h"

typedef enum { LOGO = 0, TITLE, GAMEPLAY, ENDING } GameScreen;

typedef struct {
    GameScreen screen;
    int        framesCounter;   // for timed LOGO auto-advance
    int        score;
} Game;

static void UpdateGame(Game *g)
{
    switch (g->screen)
    {
        case LOGO:
            g->framesCounter++;
            if (g->framesCounter > 120) g->screen = TITLE;        // auto-advance ~2s @60fps
            break;

        case TITLE:
            if (IsKeyPressed(KEY_ENTER) || IsGestureDetected(GESTURE_TAP))
                g->screen = GAMEPLAY;
            break;

        case GAMEPLAY:
            if (IsKeyPressed(KEY_SPACE)) g->score += 10;          // gameplay logic
            if (IsKeyPressed(KEY_ENTER)) g->screen = ENDING;
            break;

        case ENDING:
            if (IsKeyPressed(KEY_ENTER)) { g->screen = TITLE; g->score = 0; }
            break;

        default: break;
    }
}

static void DrawGame(const Game *g)
{
    BeginDrawing();
        ClearBackground(RAYWHITE);
        switch (g->screen)
        {
            case LOGO:
                DrawText("LOGO", 20, 20, 40, LIGHTGRAY);
                break;
            case TITLE:
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), GREEN);
                DrawText("TITLE SCREEN", 20, 20, 40, DARKGREEN);
                DrawText("PRESS ENTER to PLAY", 120, 220, 20, DARKGREEN);
                break;
            case GAMEPLAY:
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), PURPLE);
                DrawText(TextFormat("SCORE: %i", g->score), 20, 20, 40, MAROON);
                DrawText("ENTER to end", 130, 220, 20, MAROON);
                break;
            case ENDING:
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLUE);
                DrawText("ENDING SCREEN", 20, 20, 40, DARKBLUE);
                DrawText(TextFormat("Final score %i - ENTER for title", g->score),
                         120, 220, 20, DARKBLUE);
                break;
            default: break;
        }
    EndDrawing();
}

int main(void)
{
    InitWindow(800, 450, "screen manager");
    SetTargetFPS(60);

    Game game = { .screen = LOGO };

    while (!WindowShouldClose())
    {
        UpdateGame(&game);
        DrawGame(&game);
    }

    CloseWindow();
    return 0;
}
```

For anything larger than this, split each screen into its own module (`screen_title.c/.h`, `screen_gameplay.c/.h`) exposing `InitX/UpdateX/DrawX/UnloadX`, and let `main.c` own only the loop and the dispatch `switch`.

---

## 4. Entity management with no per-frame allocation

C games do not want a `malloc` per bullet. Use a **fixed-size struct array** plus an integer `count` of the live prefix, or a **pool with a `bool active` flag**. Spawn appends; removal uses **swap-and-pop** — copy the last live element into the freed slot and shrink `count`. That's O(1) and never touches the heap during play.

The one subtlety: when you swap-and-pop during a forward iteration, the element you just moved into slot `i` has not been processed yet, so **do not advance `i` on the frame you remove**.

```c
#include "raylib.h"
#include "raymath.h"   // Vector2Add, Vector2Scale

#define MAX_BULLETS 256

typedef struct { Vector2 pos, vel; float life; } Bullet;

typedef struct {
    Bullet bullets[MAX_BULLETS];   // fixed storage, zero per-frame allocation
    int    count;                  // live slots are [0 .. count)
} Bullets;

static void SpawnBullet(Bullets *b, Vector2 pos, Vector2 vel)
{
    if (b->count >= MAX_BULLETS) return;                  // pool full: drop, never grow
    b->bullets[b->count++] = (Bullet){ pos, vel, 2.0f };
}

// Swap-and-pop: O(1) removal, does NOT preserve order.
static void RemoveBullet(Bullets *b, int i)
{
    b->bullets[i] = b->bullets[--b->count];
}

static void UpdateBullets(Bullets *b, float dt)
{
    for (int i = 0; i < b->count; )                       // note: i advanced manually
    {
        Bullet *bl = &b->bullets[i];
        bl->pos = Vector2Add(bl->pos, Vector2Scale(bl->vel, dt));
        bl->life -= dt;
        if (bl->life <= 0.0f || bl->pos.y < 0)
            RemoveBullet(b, i);                           // do NOT i++ here:
        else                                              // a fresh element now sits at i
            i++;
    }
}

int main(void)
{
    InitWindow(800, 450, "entity pool");
    SetTargetFPS(60);

    Bullets bullets = { 0 };
    Vector2 gun = { 400, 420 };

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        if (IsKeyDown(KEY_A)) gun.x -= 300 * dt;
        if (IsKeyDown(KEY_D)) gun.x += 300 * dt;
        if (IsKeyPressed(KEY_SPACE)) SpawnBullet(&bullets, gun, (Vector2){ 0, -500 });
        UpdateBullets(&bullets, dt);

        BeginDrawing();
            ClearBackground(BLACK);
            DrawRectangleV((Vector2){ gun.x - 15, gun.y }, (Vector2){ 30, 12 }, SKYBLUE);
            for (int i = 0; i < bullets.count; i++)
                DrawCircleV(bullets.bullets[i].pos, 4.0f, GOLD);
            DrawText(TextFormat("alive: %i / %i", bullets.count, MAX_BULLETS),
                     10, 10, 20, RAYWHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
```

If you need **stable IDs** (e.g. a bullet references a target), don't swap-and-pop — keep entities in place and clear `active` instead, iterating `for (i...) if (active)`. Pick swap-and-pop when order/identity don't matter (cheaper iteration), and the active-flag pool when handles must stay valid.

---

## 5. Resource lifetime discipline

The hardest rule for beginners, and the most damaging to violate: **load every GPU/audio/font resource exactly once, before the loop, and unload it once, after the loop.** A `LoadTexture`/`LoadSound`/`LoadFont`/`LoadModel` inside the loop allocates a brand-new VRAM/RAM object every frame and never frees the old one — you get visible stutter and climbing memory until a crash.

Every `Load*` has a matching `Unload*`. The pairing is not interchangeable: `RenderTexture2D` is freed with `UnloadRenderTexture`, *not* `UnloadTexture`. CPU `Image`s are freed with `UnloadImage` and can be released the instant they're uploaded to the GPU.

```c
#include "raylib.h"

// One struct that owns all loaded assets: load it once, unload it once.
typedef struct {
    Texture2D      sprite;
    Texture2D      background;
    RenderTexture2D scene;
    Font           ui;
} Assets;

static Assets LoadAssets(void)
{
    Assets a = { 0 };

    // Procedural texture (no asset file needed): Image -> Texture, free the Image
    Image img = GenImageChecked(32, 32, 8, 8, DARKBLUE, SKYBLUE);
    a.sprite = LoadTextureFromImage(img);
    UnloadImage(img);                         // RAM copy no longer needed after upload
    SetTextureFilter(a.sprite, TEXTURE_FILTER_POINT);   // crisp pixel art

    Image bg = GenImageGradientLinear(800, 450, 0, RAYWHITE, LIGHTGRAY);
    a.background = LoadTextureFromImage(bg);
    UnloadImage(bg);

    a.scene = LoadRenderTexture(800, 450);    // offscreen framebuffer (its own Unload!)
    a.ui    = GetFontDefault();               // default font: safe no-op to UnloadFont
    return a;
}

static void UnloadAssets(Assets *a)
{
    UnloadTexture(a->sprite);
    UnloadTexture(a->background);
    UnloadRenderTexture(a->scene);            // NOT UnloadTexture
    UnloadFont(a->ui);                        // no-op for the default font
}

int main(void)
{
    InitWindow(800, 450, "resource discipline");
    SetTargetFPS(60);

    Assets assets = LoadAssets();             // ONCE, before the loop

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawTexture(assets.background, 0, 0, WHITE);
            DrawTexture(assets.sprite, 384, 209, WHITE);
        EndDrawing();
    }

    UnloadAssets(&assets);                     // unload all GPU resources...
    CloseWindow();                             // ...THEN destroy the GL context
    return 0;
}
```

Two lifetime traps worth memorizing: a failed load returns a zeroed struct (id 0) rather than crashing, so bad asset paths fail *silently* — gate user-supplied files with `IsTextureValid`/`IsImageValid`. And `UnloadModel` frees a model's meshes but deliberately **not** the textures/shaders you assigned to its materials — unload those yourself.

---

## 6. 2D camera recipes

`Camera2D` has four fields: `offset` (where the target lands on **screen**, in pixels), `target` (the **world** point that maps to offset), `rotation` (degrees), and `zoom` (1.0 = no scale). Wrap world drawing in `BeginMode2D`/`EndMode2D`; draw the HUD **after** `EndMode2D` so it stays in screen space. The number-one camera bug: a `{0}`-initialized camera has `zoom == 0`, which collapses the world to a point and renders nothing — always set `cam.zoom = 1.0f`.

This recipe combines smooth follow, world-bounds clamping, mouse-centered zoom, and screen shake.

```c
#include "raylib.h"
#include "raymath.h"   // Vector2Lerp, Vector2Clamp, Vector2Add, Vector2Subtract, Clamp
#include <math.h>      // powf

int main(void)
{
    const int sw = 800, sh = 450;
    InitWindow(sw, sh, "camera2d recipes");
    SetTargetFPS(60);

    const float worldW = 2400, worldH = 1400;
    Vector2 player = { worldW / 2, worldH / 2 };

    Camera2D cam = { 0 };
    cam.offset = (Vector2){ sw / 2.0f, sh / 2.0f };   // target appears at screen center
    cam.target = player;
    cam.zoom   = 1.0f;                                // MUST be non-zero

    float shake = 0.0f;                               // seconds of shake remaining

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // Move player (remember: +Y is DOWN in raylib 2D)
        float v = 360.0f * dt;
        if (IsKeyDown(KEY_D)) player.x += v;
        if (IsKeyDown(KEY_A)) player.x -= v;
        if (IsKeyDown(KEY_S)) player.y += v;
        if (IsKeyDown(KEY_W)) player.y -= v;
        player = Vector2Clamp(player, (Vector2){ 0, 0 }, (Vector2){ worldW, worldH });
        if (IsKeyPressed(KEY_SPACE)) shake = 0.4f;

        // Mouse-wheel zoom centered on the cursor: pin the world point under the mouse
        float wheel = GetMouseWheelMove();
        if (wheel != 0)
        {
            Vector2 before = GetScreenToWorld2D(GetMousePosition(), cam);
            cam.zoom = Clamp(cam.zoom + wheel * 0.15f, 0.25f, 6.0f);
            Vector2 after  = GetScreenToWorld2D(GetMousePosition(), cam);
            // Nudge the follow target toward the cursor. Because cam.target is
            // smoothed (lerped) below, the pin is APPROXIMATE — it converges over
            // a few frames rather than holding the point exactly. See note below
            // for the exact, snap-follow version.
            player = Vector2Add(player, Vector2Subtract(before, after));
        }

        // Smooth follow (frame-rate independent: 0.0001 reached per second)
        cam.target = Vector2Lerp(cam.target, player, 1.0f - powf(0.0001f, dt));

        // Clamp camera so the view never shows past the world edges
        float halfW = (sw * 0.5f) / cam.zoom, halfH = (sh * 0.5f) / cam.zoom;
        Vector2 mn = { halfW, halfH }, mx = { worldW - halfW, worldH - halfH };
        if (mn.x > mx.x) mn.x = mx.x = worldW * 0.5f;     // world narrower than view
        if (mn.y > mx.y) mn.y = mx.y = worldH * 0.5f;
        cam.target = Vector2Clamp(cam.target, mn, mx);

        // Screen shake: jitter the screen-space offset, decay over time
        Vector2 baseOffset = { sw / 2.0f, sh / 2.0f };
        if (shake > 0.0f)
        {
            shake -= dt;
            float mag = 12.0f * (shake > 0 ? shake : 0);
            baseOffset.x += GetRandomValue(-100, 100) / 100.0f * mag;
            baseOffset.y += GetRandomValue(-100, 100) / 100.0f * mag;
        }
        cam.offset = baseOffset;

        BeginDrawing();
            ClearBackground((Color){ 24, 24, 32, 255 });
            BeginMode2D(cam);                              // ---- world space ----
                for (int x = 0; x <= worldW; x += 100)
                    DrawLine(x, 0, x, worldH, (Color){ 45, 45, 55, 255 });
                for (int y = 0; y <= worldH; y += 100)
                    DrawLine(0, y, worldW, y, (Color){ 45, 45, 55, 255 });
                DrawRectangleLines(0, 0, worldW, worldH, DARKGRAY);
                DrawCircleV(player, 16, RED);
            EndMode2D();                                   // ---- back to screen ----

            DrawText("WASD move | wheel zoom | SPACE shake", 10, 10, 20, RAYWHITE);
            DrawText(TextFormat("zoom %.2f", cam.zoom), 10, 35, 20, RAYWHITE);
            // Pin a label above the player using world->screen projection
            Vector2 lbl = GetWorldToScreen2D(player, cam);
            DrawText("player", (int)lbl.x - 24, (int)lbl.y - 34, 16, YELLOW);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
```

Gotchas to keep in mind: `offset` is screen pixels and `target` is world units — mixing them is a classic error. Drag-to-pan must divide `GetMouseDelta()` by `cam.zoom` or it moves the world too fast when zoomed in. And `GetScreenToWorld2D` / `GetWorldToScreen2D` are exact inverses only for the camera state of *this* frame — re-sample after you change zoom or target.

**Exact cursor-anchored zoom** (no smooth follow): operate on `cam.target` directly so the world point under the cursor is pinned the same frame —

```c
Vector2 before = GetScreenToWorld2D(GetMousePosition(), cam);
cam.zoom = Clamp(cam.zoom + wheel * 0.15f, 0.25f, 6.0f);
Vector2 after  = GetScreenToWorld2D(GetMousePosition(), cam);
cam.target = Vector2Add(cam.target, Vector2Subtract(before, after));  // point held exactly
```

The recipe above instead nudges `player` and then lerps `cam.target` toward it, which trades that exact pin for a smoother feel. Use this snap version when zooming must keep a point dead-still (e.g. a level editor); use the lerp version for gameplay.

---

## 7. Collision & response

raylib ships fast immediate-mode overlap tests. All 2D rect tests are **axis-aligned only** — there is no built-in rotated-rect (OBB) collision (use `CheckCollisionPointPoly` for arbitrary convex shapes). For UI, `CheckCollisionPointRec(GetMousePosition(), bounds)` plus `IsMouseButtonPressed(MOUSE_BUTTON_LEFT)` is the whole button-click recipe.

For response, `GetCollisionRec` returns the overlap rectangle; the smaller of its width/height is the **minimum-penetration axis**, so you push the mover out along that axis. (Guard it: `GetCollisionRec` returns a zero-size rect when there's no overlap, so test `CheckCollisionRecs` first.)

```c
#include "raylib.h"

// Push `mover` out of `solid` along the axis of least penetration (AABB resolve).
static void ResolveAABB(Rectangle *mover, Rectangle solid)
{
    if (!CheckCollisionRecs(*mover, solid)) return;
    Rectangle o = GetCollisionRec(*mover, solid);    // overlap region
    if (o.width < o.height)                           // resolve on X (smaller penetration)
    {
        if (mover->x < solid.x) mover->x -= o.width;  // mover is to the left -> push left
        else                    mover->x += o.width;
    }
    else                                              // resolve on Y
    {
        if (mover->y < solid.y) mover->y -= o.height;
        else                    mover->y += o.height;
    }
}

int main(void)
{
    InitWindow(800, 450, "collision & response");
    SetTargetFPS(60);

    Rectangle player = { 100, 200, 40, 40 };
    Rectangle walls[3] = {
        { 300, 150, 40, 200 },
        { 500, 100, 200, 40 },
        { 450, 320, 250, 40 },
    };
    Rectangle button = { 620, 20, 150, 40 };
    int clicks = 0;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        float s = 220.0f * dt;
        if (IsKeyDown(KEY_RIGHT)) player.x += s;
        if (IsKeyDown(KEY_LEFT))  player.x -= s;
        if (IsKeyDown(KEY_DOWN))  player.y += s;
        if (IsKeyDown(KEY_UP))    player.y -= s;

        for (int i = 0; i < 3; i++) ResolveAABB(&player, walls[i]);

        // UI: point-in-rect hover + click
        Vector2 m = GetMousePosition();
        bool hover = CheckCollisionPointRec(m, button);
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) clicks++;
        SetMouseCursor(hover ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT);

        BeginDrawing();
            ClearBackground(RAYWHITE);
            for (int i = 0; i < 3; i++) DrawRectangleRec(walls[i], DARKGRAY);
            DrawRectangleRec(player, MAROON);
            DrawRectangleRec(button, hover ? SKYBLUE : BLUE);
            DrawText(TextFormat("clicks: %i", clicks), (int)button.x + 12,
                     (int)button.y + 10, 20, RAYWHITE);
            DrawText("Arrows move; collide with the walls", 10, 10, 20, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
```

For radius checks in your own code, prefer the squared-distance helpers (`Vector2DistanceSqr`) to skip the `sqrt`. For 3D picking, cast `Ray ray = GetScreenToWorldRay(GetMousePosition(), camera);` and test it with `GetRayCollisionBox`/`Sphere`/`Mesh` — always read `rc.hit` before trusting `rc.distance`/`point`/`normal`, and broad-phase with `CheckCollisionBoxes` before the per-triangle `GetRayCollisionMesh`.

---

## 8. Sprite sheets & animation

Animation is a moving **source rectangle** sliding across one texture (the atlas). Keep all frames in a single texture so the whole animation draws from one bound texture (one batch). Advance the frame on a timer accumulated from `GetFrameTime()`, not on a raw frame counter, so the animation speed is independent of FPS. `DrawTexturePro` maps a source rect to a destination rect with rotation about an `origin` — set `origin = { dest.width/2, dest.height/2 }` to scale/rotate about the sprite's center.

```c
#include "raylib.h"

#define FRAME_COUNT 6
#define FRAME_W     32
#define FRAME_H     32

int main(void)
{
    InitWindow(800, 450, "sprite animation");
    SetTargetFPS(60);

    // Build a 6-frame horizontal sheet procedurally (no asset file): each frame a
    // circle of a different color, so stepping is visible.
    Image sheet = GenImageColor(FRAME_W * FRAME_COUNT, FRAME_H, BLANK);
    Color cols[FRAME_COUNT] = { RED, ORANGE, GOLD, GREEN, SKYBLUE, VIOLET };
    for (int i = 0; i < FRAME_COUNT; i++)
        ImageDrawCircle(&sheet, i * FRAME_W + FRAME_W / 2, FRAME_H / 2, 14, cols[i]);
    Texture2D tex = LoadTextureFromImage(sheet);
    UnloadImage(sheet);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);

    int   frame = 0;
    float timer = 0.0f;
    const float frameTime = 0.12f;     // seconds per frame -> ~8 fps animation
    float rotation = 0.0f;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // Advance the frame on a time accumulator (FPS-independent)
        timer += dt;
        while (timer >= frameTime)
        {
            timer -= frameTime;
            frame = (frame + 1) % FRAME_COUNT;     // wrap
        }
        rotation += 40.0f * dt;

        // The source rect selects the current frame from the sheet
        Rectangle src  = { (float)(frame * FRAME_W), 0, FRAME_W, FRAME_H };
        Rectangle dest = { 400, 225, FRAME_W * 4, FRAME_H * 4 };   // 4x scale
        Vector2   origin = { dest.width / 2, dest.height / 2 };    // rotate about center

        BeginDrawing();
            ClearBackground(RAYWHITE);
            // DrawTextureRec: simple, axis-aligned, no scale (full sheet 1:1)
            DrawTextureRec(tex, src, (Vector2){ 40, 40 }, WHITE);
            // DrawTexturePro: scale + rotate the same frame about its center
            DrawTexturePro(tex, src, dest, origin, rotation, WHITE);
            DrawText(TextFormat("frame %i/%i", frame + 1, FRAME_COUNT), 40, 90, 20, DARKGRAY);
        EndDrawing();
    }

    UnloadTexture(tex);
    CloseWindow();
    return 0;
}
```

Two correctness notes: `DrawTexturePro`'s `origin` is an offset measured from the destination's top-left and is also the rotation pivot — leave it `{0,0}` and the sprite orbits its corner. For pixel art, pad atlas cells or use `TEXTURE_WRAP_CLAMP`, because `TEXTURE_WRAP_REPEAT` will bleed neighboring frames when you sample a sub-rect.

---

## 9. 3D essentials — camera, the draw bracket, picking

3D mode is the same immediate-mode loop with a perspective camera. A `Camera3D` (the type `Camera` is an alias) has `position`, `target`, `up`, `fovy` (vertical field of view in **degrees** for perspective), and `projection` (`CAMERA_PERSPECTIVE` or `CAMERA_ORTHOGRAPHIC`). Wrap every 3D draw in `BeginMode3D(camera)` / `EndMode3D()`, and draw the 2D HUD **after** `EndMode3D()`. `UpdateCamera(&camera, mode)` gives you a built-in controller: `CAMERA_FREE`, `CAMERA_ORBITAL`, `CAMERA_FIRST_PERSON`, `CAMERA_THIRD_PERSON`.

The number-one 3D footgun mirrors the 2D one: a `{0}` camera has `up = {0,0,0}` and `fovy = 0`, so nothing renders — always set `up = {0,1,0}` and a real `fovy`.

```c
#include "raylib.h"

int main(void)
{
    InitWindow(900, 600, "3d essentials");

    Camera3D cam = { 0 };
    cam.position   = (Vector3){ 8, 8, 8 };
    cam.target     = (Vector3){ 0, 0.5f, 0 };
    cam.up         = (Vector3){ 0, 1, 0 };     // MUST be non-zero
    cam.fovy       = 45.0f;                     // vertical FOV in degrees
    cam.projection = CAMERA_PERSPECTIVE;

    // A model from a procedural mesh (no asset file): mesh -> model, loaded once.
    Model cube = LoadModelFromMesh(GenMeshCube(1.5f, 1.5f, 1.5f));

    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        UpdateCamera(&cam, CAMERA_ORBITAL);     // auto-orbit; swap for FIRST_PERSON

        // Mouse picking: ray from the cursor, tested against an AABB.
        Ray ray = GetScreenToWorldRay(GetMousePosition(), cam);
        BoundingBox box = { (Vector3){ -0.75f, -0.75f, -0.75f }, (Vector3){ 0.75f, 0.75f, 0.75f } };
        RayCollision rc = GetRayCollisionBox(ray, box);   // read rc.hit FIRST

        BeginDrawing();
            ClearBackground(RAYWHITE);
            BeginMode3D(cam);                    // ---- 3D world ----
                DrawModel(cube, (Vector3){ 0, 0, 0 }, 1.0f, rc.hit ? GOLD : SKYBLUE);
                DrawModelWires(cube, (Vector3){ 0, 0, 0 }, 1.0f, DARKBLUE);
                DrawCube((Vector3){ 3, 0.5f, 0 }, 1, 1, 1, RED);   // immediate primitive
                DrawGrid(20, 1.0f);
            EndMode3D();                         // ---- back to 2D HUD ----
            DrawText(rc.hit ? "picked!" : "hover the cube", 10, 10, 20, DARKGRAY);
            DrawFPS(10, 570);
        EndDrawing();
    }

    UnloadModel(cube);     // frees the mesh; does NOT free textures you assigned
    CloseWindow();
    return 0;
}
```

For a **first-person controller**, call `DisableCursor()` once after `InitWindow` (locks + hides the mouse), then `UpdateCamera(&cam, CAMERA_FIRST_PERSON)` each frame; call `EnableCursor()` again when you open a menu. For **picking against real geometry**, broad-phase with `GetRayCollisionBox(ray, GetModelBoundingBox(model))` before the per-triangle `GetRayCollisionMesh(ray, mesh, transform)`. Load authored models with `LoadModel("file.glb")` (`.obj`/`.gltf`/`.glb`/`.iqm` supported) and assign a texture via `SetMaterialTexture(&model.materials[0], MATERIAL_MAP_DIFFUSE, tex)`. A complete runnable version with multiple pickable cubes is in `examples/05_3d_basics.c`.

---

## 10. Shaders & post-processing

macOS picks desktop OpenGL 3.3, so author shaders in GLSL `#version 330` (use `in`/`out`/`texture()`/a named `out vec4 finalColor;` — the old `varying`/`gl_FragColor`/`texture2D` syntax won't compile). The full-screen post-processing pipeline: render the scene into a `RenderTexture2D`, then draw that texture full-screen through a fragment shader. The one trap: render textures are stored bottom-left origin, so draw them with a **negative source height** to undo the vertical flip.

Pass `NULL` as the vertex shader to get raylib's default vertex stage (it provides `fragTexCoord`/`fragColor` and binds `texture0`/`colDiffuse`). Resolve each custom uniform location **once** after loading, then push the value each frame before drawing.

> `raylib.h` pulls in almost no standard library (only `<stdarg.h>`), so any file that uses `NULL`, `malloc`/`free`, or `printf` must include the matching standard header itself — here `<stddef.h>` for `NULL`.

```c
#include "raylib.h"
#include <stddef.h>   // NULL (raylib.h does not define it)

// Fragment-only post effect: horizontal wobble + half desaturation.
static const char *fsCode =
    "#version 330                                              \n"
    "in vec2 fragTexCoord;                                     \n"  // from default VS
    "in vec4 fragColor;                                        \n"
    "out vec4 finalColor;                                      \n"  // required out name
    "uniform sampler2D texture0;                               \n"  // the FBO texture
    "uniform vec4 colDiffuse;                                  \n"  // auto-bound tint
    "uniform float uTime;                                      \n"  // our uniform
    "void main() {                                             \n"
    "    vec2 uv = fragTexCoord;                               \n"
    "    uv.x += 0.01 * sin(uv.y * 30.0 + uTime * 4.0);        \n"
    "    vec4 c = texture(texture0, uv) * colDiffuse * fragColor; \n"
    "    float g = dot(c.rgb, vec3(0.299, 0.587, 0.114));      \n"
    "    finalColor = vec4(mix(c.rgb, vec3(g), 0.5), c.a);     \n"
    "}                                                         \n";

int main(void)
{
    const int W = 800, H = 450;
    InitWindow(W, H, "post-processing");
    SetTargetFPS(60);

    RenderTexture2D target = LoadRenderTexture(W, H);     // offscreen FBO (once)
    Shader shader = LoadShaderFromMemory(NULL, fsCode);   // NULL VS -> default VS
    if (!IsShaderValid(shader)) TraceLog(LOG_WARNING, "shader failed to compile");
    int timeLoc = GetShaderLocation(shader, "uTime");     // cache the uniform location

    float t = 0.0f;
    while (!WindowShouldClose())
    {
        t += GetFrameTime();
        SetShaderValue(shader, timeLoc, &t, SHADER_UNIFORM_FLOAT);  // type must match GLSL

        // 1) Render the scene INTO the render texture
        BeginTextureMode(target);
            ClearBackground(RAYWHITE);
            DrawCircle(400, 225, 80, RED);
            DrawRectangle(150, 150, 120, 120, BLUE);
            DrawText("scene -> FBO", 20, 20, 20, DARKGRAY);
        EndTextureMode();

        // 2) Draw the FBO full-screen THROUGH the shader (negative height = Y flip)
        BeginDrawing();
            ClearBackground(BLACK);
            BeginShaderMode(shader);
                DrawTextureRec(target.texture,
                    (Rectangle){ 0, 0, (float)target.texture.width,
                                       -(float)target.texture.height },
                    (Vector2){ 0, 0 }, WHITE);
            EndShaderMode();
            DrawFPS(10, 10);
        EndDrawing();
    }

    UnloadShader(shader);          // free GPU program
    UnloadRenderTexture(target);   // free FBO + attachments
    CloseWindow();
    return 0;
}
```

Watch for: `GetShaderLocation` returns `-1` if the uniform name is wrong **or** the GLSL compiler stripped an unused uniform (passing `-1` to `SetShaderValue` silently no-ops). `LoadShader*` never returns NULL — on failure it returns a shader using the default program, so check `IsShaderValid`. And if you support window resize / HiDPI, reload the render texture to match the framebuffer size or the post-pass will stretch.

---

## 11. Persistence — save & load

raylib gives you raw file I/O: `SaveFileData`/`LoadFileData` for a binary blob and `SaveFileText`/`LoadFileText` for text. The pointer from `LoadFileData`/`LoadFileText` is heap-allocated **by raylib** and must be freed with `UnloadFileData`/`UnloadFileText` — never plain `free`. Gate the load with `FileExists` so first run falls back to defaults. (Note: raylib 5.5 core no longer ships the old `SaveStorageValue`/`LoadStorageValue` key/value helpers — use these file functions instead.)

```c
#include "raylib.h"

typedef struct {                  // packed save blob
    int   version;
    int   highScore;
    float volume;
} SaveData;

static void SaveGame(const SaveData *s)
{
    // Binary: writes raw struct bytes. Fast, but not portable across architectures.
    SaveFileData("save.dat", (void *)s, sizeof(SaveData));

    // Human-readable mirror (handy for debugging / editing by hand)
    SaveFileText("save.txt",
        (char *)TextFormat("version=%i\nhighScore=%i\nvolume=%.2f\n",
                           s->version, s->highScore, s->volume));
}

static SaveData LoadGame(void)
{
    SaveData s = { .version = 1, .highScore = 0, .volume = 1.0f };  // defaults
    if (!FileExists("save.dat")) return s;                          // first run

    int size = 0;
    unsigned char *bytes = LoadFileData("save.dat", &size);
    if (bytes && size >= (int)sizeof(SaveData))                     // validate size!
        s = *(SaveData *)bytes;
    UnloadFileData(bytes);                                          // raylib's allocator
    return s;
}

int main(void)
{
    InitWindow(800, 450, "persistence");
    SetTargetFPS(60);

    SaveData save = LoadGame();

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_UP))   save.highScore += 100;
        if (IsKeyPressed(KEY_DOWN)) save.highScore -= 100;
        if (IsKeyPressed(KEY_S))    SaveGame(&save);

        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText(TextFormat("High score: %i", save.highScore), 20, 20, 30, MAROON);
            DrawText("UP/DOWN change, S to save", 20, 60, 20, DARKGRAY);
        EndDrawing();
    }

    SaveGame(&save);          // persist on exit
    CloseWindow();
    return 0;
}
```

Binary `SaveFileData` writes raw struct bytes, which is fine for one target but **not portable** across architectures/compilers (struct padding, endianness, type sizes). Keep a `version` field so future builds can detect old saves, and validate the returned `size` before dereferencing. For cross-platform saves, serialize an explicit field-by-field layout or use `SaveFileText`.

---

## 12. Performance & pitfalls — the checklist

Almost every "black screen", stutter, climbing-memory, or silent-audio bug in raylib traces back to one of these structural mistakes rather than to algorithmic cost. Run down the list whenever something misbehaves.

- **Load once, above the loop.** Every `LoadTexture`/`LoadTextureFromImage`/`LoadSound`/`LoadFont`/`LoadMusicStream`/`LoadRenderTexture`/`LoadModel` belongs *before* `while (!WindowShouldClose())`. A `Load*` inside the loop allocates a new GPU/heap object every frame and never frees the old one — stutter plus climbing VRAM until a crash.

- **Unload everything you load, with the matching call.** `UnloadTexture` for textures, `UnloadRenderTexture` for render textures (not `UnloadTexture`), `UnloadImage` for CPU images, `UnloadFont`, `UnloadSound`, `UnloadModel`, `UnloadFileData`/`UnloadFileText` for file buffers, `UnloadSoundAlias` *before* the source `UnloadSound`. `UnloadModel` does **not** free textures/shaders you assigned to its materials.

- **Pair every `BeginX` with its `EndX`, unwound LIFO.** `BeginDrawing`/`EndDrawing`, `BeginMode2D`/`EndMode2D`, `BeginMode3D`/`EndMode3D`, `BeginTextureMode`/`EndTextureMode`, `BeginShaderMode`/`EndShaderMode`, `BeginBlendMode`/`EndBlendMode`, `BeginScissorMode`/`EndScissorMode`. A missing `End*` leaves the renderer in the wrong state for the rest of the frame (stuck camera/shader, inverted geometry). All drawing must be inside `BeginDrawing`/`EndDrawing` or it's silently dropped.

- **`UpdateMusicStream(music)` every single frame, unconditionally.** Skip it (or call it only sometimes) and the track plays one buffer (~a fraction of a second) then stalls. Each playing `Music` needs its own per-frame update.

- **Pick the right texture filter.** raylib's default is already `TEXTURE_FILTER_POINT` (nearest), so **pixel art is crisp by default** — the real pitfall is the opposite: smoothly *scaled* art looks jagged unless you call `SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR)` (or `TRILINEAR` after `GenTextureMipmaps(&tex)` — trilinear/anisotropic do nothing without mipmaps).

- **Batch by texture; group by state.** Pack sprites into one atlas and draw them all with `DrawTextureRec`/`DrawTexturePro` from that single bound texture — one draw call instead of one per sprite. Each `BeginBlendMode`/`BeginShaderMode`/`BeginMode*` switch forces a batch flush, so do all alpha draws, then all additive draws.

- **`SetTargetFPS` vs `FLAG_VSYNC_HINT` — choose one.** VSync (set the flag *before* `InitWindow`) avoids tearing; `SetTargetFPS` (after `InitWindow`) gives a deterministic software cap. Using both double-limits and they can fight. Neither = uncapped, which maxes the GPU and heats the machine. `SetTargetFPS` caps but does not pin the rate — always read `GetFrameTime()`.

- **`TextFormat` returns a rotating internal buffer — use it immediately.** There are only ~4 static slots of 1024 bytes each; making several `TextFormat` calls and using their pointers later makes earlier strings alias/overwrite, and strings over 1024 chars truncate. Copy with `TextCopy` if you must keep one across frames.

- **High-DPI on Apple Silicon.** Set `FLAG_WINDOW_HIGHDPI` *before* `InitWindow` or you get a small/blurry upscaled window. With it, `GetRenderWidth()` is 2x `GetScreenWidth()` on Retina — do gameplay/layout in `GetScreenWidth`/`Height` units, and use `GetRenderWidth`/`Height` or `GetWindowScaleDPI()` only for pixel-accurate work (shaders, render textures, mouse math).

- **Cleanup order: resources, then audio device, then window.** Unload all GPU resources and sounds first, then `CloseAudioDevice()`, then `CloseWindow()` last. Closing the window first destroys the GL/audio context out from under resources you're still freeing, causing warnings or crashes on exit. Conversely, never call any GPU op (`LoadTexture`, `SetTextureFilter`) *before* `InitWindow` — no context exists yet (`SetConfigFlags` is the one call that must precede it).

- **Use `SetTraceLogLevel` deliberately.** `LOG_ALL`/`LOG_DEBUG` while debugging to see raylib's diagnostics, `LOG_WARNING` for a quiet release build.

- **No per-frame heap churn.** Avoid `malloc`/`strdup`/array-rebuilds inside the loop — they cause GC-like hitches. Preallocate buffers and entity pools (Section 4) before the loop and reuse them.

- **ESC closes the window by default.** If ESC is a gameplay/menu key, call `SetExitKey(KEY_NULL)` and handle quitting yourself.

---

## 13. Runtime window control & ecosystem notes

The starter skeleton sets window state once via `SetConfigFlags` before `InitWindow`, but you can change most of it at runtime (handy for an options menu):

```c
ToggleFullscreen();                 // exclusive fullscreen (mode switch)
ToggleBorderlessWindowed();         // borderless fullscreen at desktop res (smoother on macOS)
SetWindowSize(1280, 720);           // resize the client area
SetWindowMinSize(640, 360);         // clamp (needs FLAG_WINDOW_RESIZABLE)
SetWindowPosition(100, 100);
SetWindowTitle("New Title");
MaximizeWindow();                   // also RestoreWindow / MinimizeWindow
bool fs = IsWindowFullscreen();
// Multi-monitor: GetMonitorCount(), GetCurrentMonitor(),
// GetMonitorWidth(m)/GetMonitorHeight(m), GetMonitorRefreshRate(m).
```

On a resizable window, react to `IsWindowResized()` each frame and re-read `GetScreenWidth()`/`GetScreenHeight()` (and re-create any full-screen `RenderTexture2D` to match).

**Clipboard & system:** `const char *GetClipboardText(void)` / `void SetClipboardText(const char *text)` for copy-paste (pair with the text-input recipe in `examples/06_text_input.c`); `OpenURL(const char *url)` to launch a link.

**Ecosystem — what is and isn't in this install.** This Homebrew package ships the core library plus `raymath.h` and `rlgl.h`. It does **not** include the sibling single-header libraries:

- **raygui** (`raygui.h`) — immediate-mode UI (buttons, sliders, checkboxes, text boxes). Not installed here; if you want it, vendor `raygui.h` into the project and `#define RAYGUI_IMPLEMENTATION` in one `.c`. Until then, roll menus from `CheckCollisionPointRec` + `DrawRectangleRec`/`DrawText` (see the button in Section 7).
- **raudio**, **rres**, **physac** — also separate; not part of this install.

For anything beyond this skill: the official cheatsheet (https://www.raylib.com/cheatsheet/cheatsheet.html) lists every function for the installed version, and the examples gallery (https://www.raylib.com/examples.html) has a runnable demo for almost every feature.
