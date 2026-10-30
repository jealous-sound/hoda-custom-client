// 02_camera2d_collision.c
// A Camera2D that follows a player around a world larger than the screen,
// with solid wall rectangles the player collides against.
//
// Key teaching points:
//   - camera.offset is a SCREEN-space anchor (screen center) and camera.target
//     is the WORLD-space point that lands on that anchor -> centered follow.
//   - Resolve AABB collisions one axis at a time: move X, push out of any wall
//     along X; then move Y, push out along Y. This avoids corner sticking.
//   - Draw the world inside BeginMode2D/EndMode2D, then draw the HUD in plain
//     screen space AFTER EndMode2D so it doesn't scroll with the camera.
//   - GetScreenToWorld2D maps the mouse (screen pixels) back into world space.
//
// Build: cc -Wall -std=c99 -I/opt/homebrew/opt/raylib/include 02_camera2d_collision.c \
//        -o game -L/opt/homebrew/opt/raylib/lib -lraylib \
//        -framework Cocoa -framework IOKit -framework OpenGL
#include "raylib.h"

#define MAX_WALLS 5

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib 5.5 - camera2d follow + collision");
    SetTargetFPS(60);

    // World is much bigger than the window
    const float worldWidth  = 2000.0f;
    const float worldHeight  = 1500.0f;

    Rectangle player = { worldWidth/2.0f, worldHeight/2.0f, 40, 40 };
    const float speed = 300.0f; // pixels per second

    // Solid walls in world space
    Rectangle walls[MAX_WALLS] = {
        {  300,  300, 200,  60 },
        {  800,  500,  80, 400 },
        { 1200,  200, 300,  80 },
        {  600,  900, 500,  70 },
        { 1400, 1000,  90, 300 }
    };

    // Centered-follow camera: target (world) is pinned to offset (screen center)
    Camera2D camera = { 0 };
    camera.offset   = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    camera.target   = (Vector2){ player.x + player.width/2.0f, player.y + player.height/2.0f };
    camera.rotation = 0.0f;
    camera.zoom     = 1.0f; // MUST be non-zero or nothing renders

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // --- Move along X, then resolve X-axis collisions ---
        float dx = 0.0f;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dx += 1.0f;
        if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) dx -= 1.0f;
        player.x += dx * speed * dt;
        for (int i = 0; i < MAX_WALLS; i++)
        {
            if (CheckCollisionRecs(player, walls[i]))
            {
                if (dx > 0)      player.x = walls[i].x - player.width;         // hit left face
                else if (dx < 0) player.x = walls[i].x + walls[i].width;       // hit right face
            }
        }

        // --- Move along Y, then resolve Y-axis collisions ---
        float dy = 0.0f;
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) dy += 1.0f;
        if (IsKeyDown(KEY_UP)   || IsKeyDown(KEY_W)) dy -= 1.0f;
        player.y += dy * speed * dt;
        for (int i = 0; i < MAX_WALLS; i++)
        {
            if (CheckCollisionRecs(player, walls[i]))
            {
                if (dy > 0)      player.y = walls[i].y - player.height;        // hit top face
                else if (dy < 0) player.y = walls[i].y + walls[i].height;      // hit bottom face
            }
        }

        // Keep the player inside the world bounds
        if (player.x < 0) player.x = 0;
        if (player.y < 0) player.y = 0;
        if (player.x + player.width  > worldWidth)  player.x = worldWidth  - player.width;
        if (player.y + player.height > worldHeight) player.y = worldHeight - player.height;

        // Camera follows the player's center
        camera.target = (Vector2){ player.x + player.width/2.0f, player.y + player.height/2.0f };

        // Coordinate readout: screen pixels -> world units
        Vector2 mouseScreen = GetMousePosition();
        Vector2 mouseWorld  = GetScreenToWorld2D(mouseScreen, camera);

        BeginDrawing();
            ClearBackground((Color){ 30, 30, 40, 255 });

            BeginMode2D(camera); // everything here is in WORLD space
                // Reference grid
                for (int x = 0; x <= (int)worldWidth; x += 100)
                    DrawLine(x, 0, x, (int)worldHeight, (Color){ 50, 50, 60, 255 });
                for (int y = 0; y <= (int)worldHeight; y += 100)
                    DrawLine(0, y, (int)worldWidth, y, (Color){ 50, 50, 60, 255 });

                // World border
                DrawRectangleLines(0, 0, (int)worldWidth, (int)worldHeight, DARKGRAY);

                // Walls
                for (int i = 0; i < MAX_WALLS; i++)
                    DrawRectangleRec(walls[i], BROWN);

                // Player
                DrawRectangleRec(player, RED);
            EndMode2D();

            // HUD in SCREEN space (after EndMode2D)
            DrawText("WASD / Arrows to move. Brown walls are solid.", 10, 10, 20, RAYWHITE);
            DrawText(TextFormat("Player world: %.0f, %.0f", player.x, player.y), 10, 35, 20, RAYWHITE);
            DrawText(TextFormat("Mouse screen: %.0f, %.0f", mouseScreen.x, mouseScreen.y), 10, 60, 20, RAYWHITE);
            DrawText(TextFormat("Mouse world:  %.0f, %.0f", mouseWorld.x,  mouseWorld.y),  10, 85, 20, RAYWHITE);
            DrawFPS(screenWidth - 90, 10);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
