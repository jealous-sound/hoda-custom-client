// 01_input_movement.c
// Frame-rate-independent player movement with WASD + arrow keys.
//
// Key teaching points:
//   - Multiply movement by GetFrameTime() (delta seconds) so the player moves
//     at the same real-world speed at 30, 60, or 144 FPS.
//   - Clamp the player rectangle so it never leaves the screen.
//   - Normalize diagonal movement so moving on two axes isn't ~41% faster.
//
// Build: cc -Wall -std=c99 -I/opt/homebrew/opt/raylib/include 01_input_movement.c \
//        -o game -L/opt/homebrew/opt/raylib/lib -lraylib \
//        -framework Cocoa -framework IOKit -framework OpenGL
#include "raylib.h"

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib 5.5 - frame-rate independent movement");
    SetTargetFPS(60); // software cap; movement stays correct at any rate thanks to dt

    // Player as a Rectangle (float fields), centered on screen
    Rectangle player = { screenWidth/2.0f - 25, screenHeight/2.0f - 25, 50, 50 };
    const float speed = 300.0f; // pixels PER SECOND (not per frame)

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime(); // seconds elapsed since last frame (0 on frame 1)

        // Build a movement direction from held keys (level-triggered: IsKeyDown)
        Vector2 dir = { 0.0f, 0.0f };
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dir.x += 1.0f;
        if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) dir.x -= 1.0f;
        if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) dir.y += 1.0f;
        if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) dir.y -= 1.0f;

        // Normalize diagonals so (1,1) isn't longer than (1,0). 0.7071 = 1/sqrt(2).
        if (dir.x != 0.0f && dir.y != 0.0f)
        {
            dir.x *= 0.7071f;
            dir.y *= 0.7071f;
        }

        // Frame-rate independent integration: distance = speed * dt
        player.x += dir.x * speed * dt;
        player.y += dir.y * speed * dt;

        // Clamp to screen bounds (remember: +Y is DOWN in raylib 2D)
        if (player.x < 0) player.x = 0;
        if (player.y < 0) player.y = 0;
        if (player.x + player.width  > screenWidth)  player.x = screenWidth  - player.width;
        if (player.y + player.height > screenHeight) player.y = screenHeight - player.height;

        BeginDrawing();
            ClearBackground(RAYWHITE);

            DrawRectangleRec(player, MAROON);

            DrawText("Move with WASD or the Arrow keys", 10, 10, 20, DARKGRAY);
            DrawText("Movement is frame-rate independent (speed * GetFrameTime())",
                     10, 35, 20, DARKGRAY);
            DrawText(TextFormat("Player: %.0f, %.0f", player.x, player.y),
                     10, screenHeight - 30, 20, GRAY);
            DrawFPS(screenWidth - 90, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
