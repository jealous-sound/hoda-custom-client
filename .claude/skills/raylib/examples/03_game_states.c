// 03_game_states.c
// A simple game-state machine: TITLE -> GAMEPLAY -> GAMEOVER -> TITLE.
//
// Key teaching points:
//   - Model flow with an enum and drive TWO parallel switch statements: one in
//     the UPDATE phase (state changes + input) and one in the DRAW phase.
//   - Perform state transitions ONLY in update; the draw phase never mutates
//     game state.
//   - IsKeyPressed (edge-triggered, one frame) is right for menu transitions;
//     IsKeyDown (level-triggered) is right for continuous movement.
//
// The little game: grab gold coins before the timer runs out.
//
// Build: cc -Wall -std=c99 -I/opt/homebrew/opt/raylib/include 03_game_states.c \
//        -o game -L/opt/homebrew/opt/raylib/lib -lraylib \
//        -framework Cocoa -framework IOKit -framework OpenGL
#include "raylib.h"

typedef enum { TITLE = 0, GAMEPLAY, GAMEOVER } GameScreen;

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib 5.5 - game state machine");
    SetTargetFPS(60);

    GameScreen screen = TITLE;

    // Gameplay state
    Rectangle player = { screenWidth/2.0f, screenHeight/2.0f, 40, 40 };
    const float speed = 300.0f;
    Rectangle coin = { 0, 0, 20, 20 };
    bool  coinActive = false;
    float respawnTimer = 0.0f;
    int   score = 0;
    float timeLeft = 0.0f;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // ===================== UPDATE (state + input) =====================
        switch (screen)
        {
            case TITLE:
            {
                if (IsKeyPressed(KEY_ENTER))
                {
                    // (Re)initialize a fresh round
                    player.x = screenWidth/2.0f;
                    player.y = screenHeight/2.0f;
                    score = 0;
                    timeLeft = 15.0f;
                    coinActive = false;
                    respawnTimer = 0.0f;
                    screen = GAMEPLAY;
                }
            } break;

            case GAMEPLAY:
            {
                // Continuous, frame-rate independent movement
                if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) player.x += speed*dt;
                if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) player.x -= speed*dt;
                if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) player.y += speed*dt;
                if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) player.y -= speed*dt;

                // Clamp to screen
                if (player.x < 0) player.x = 0;
                if (player.y < 0) player.y = 0;
                if (player.x + player.width  > screenWidth)  player.x = screenWidth  - player.width;
                if (player.y + player.height > screenHeight) player.y = screenHeight - player.height;

                // Spawn a coin (after a short delay) if none is active
                if (!coinActive)
                {
                    respawnTimer -= dt;
                    if (respawnTimer <= 0.0f)
                    {
                        coin.x = (float)GetRandomValue(0, screenWidth  - (int)coin.width);
                        coin.y = (float)GetRandomValue(45, screenHeight - (int)coin.height);
                        coinActive = true;
                    }
                }
                // Collect it
                if (coinActive && CheckCollisionRecs(player, coin))
                {
                    score += 1;
                    coinActive = false;
                    respawnTimer = 0.4f;
                }

                // Countdown -> game over
                timeLeft -= dt;
                if (timeLeft <= 0.0f)
                {
                    timeLeft = 0.0f;
                    screen = GAMEOVER;
                }
            } break;

            case GAMEOVER:
            {
                if (IsKeyPressed(KEY_ENTER)) screen = TITLE;
            } break;
        }

        // ========================= DRAW (no state changes) =========================
        BeginDrawing();
            ClearBackground(RAYWHITE);

            switch (screen)
            {
                case TITLE:
                {
                    ClearBackground(DARKBLUE);
                    const char *t = "COIN DASH";
                    DrawText(t, screenWidth/2 - MeasureText(t, 60)/2, 120, 60, RAYWHITE);
                    const char *p = "Press ENTER to start";
                    DrawText(p, screenWidth/2 - MeasureText(p, 20)/2, 260, 20, SKYBLUE);
                } break;

                case GAMEPLAY:
                {
                    if (coinActive) DrawRectangleRec(coin, GOLD);
                    DrawRectangleRec(player, MAROON);
                    DrawText(TextFormat("Score: %i", score), 10, 10, 20, DARKGRAY);
                    DrawText(TextFormat("Time: %.1f", timeLeft), screenWidth - 130, 10, 20, DARKGRAY);
                    DrawText("Grab the gold coins! Move with WASD / Arrows.",
                             10, screenHeight - 30, 20, GRAY);
                } break;

                case GAMEOVER:
                {
                    ClearBackground(MAROON);
                    const char *g = "GAME OVER";
                    DrawText(g, screenWidth/2 - MeasureText(g, 60)/2, 110, 60, RAYWHITE);
                    const char *s = TextFormat("Final score: %i", score);
                    DrawText(s, screenWidth/2 - MeasureText(s, 30)/2, 220, 30, RAYWHITE);
                    const char *r = "Press ENTER to return to the title";
                    DrawText(r, screenWidth/2 - MeasureText(r, 20)/2, 300, 20, PINK);
                } break;
            }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
