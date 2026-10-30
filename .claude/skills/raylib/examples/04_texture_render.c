// 04_texture_render.c
// Generate a texture procedurally (no asset files), then draw it with
// DrawTexturePro (rotation + scale + tint) and animate a source rectangle
// stepping across a "sprite sheet".
//
// Key teaching points:
//   - Image lives in CPU RAM; Texture2D lives in GPU VRAM. Build/edit the Image,
//     upload it ONCE with LoadTextureFromImage, then UnloadImage the RAM copy.
//   - A sprite sheet is one texture; "animation" = moving a source Rectangle
//     across it over time.
//   - DrawTexturePro maps a source rect onto a dest rect; 'origin' is the pivot
//     measured from the dest's top-left. Set origin = {w/2, h/2} to rotate about
//     the center.
//   - Every Load* has a matching Unload*: UnloadImage after upload, UnloadTexture
//     before CloseWindow.
//
// Build: cc -Wall -std=c99 -I/opt/homebrew/opt/raylib/include 04_texture_render.c \
//        -o game -L/opt/homebrew/opt/raylib/lib -lraylib \
//        -framework Cocoa -framework IOKit -framework OpenGL
#include "raylib.h"

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib 5.5 - procedural texture + DrawTexturePro");
    SetTargetFPS(60);

    // --- Build a procedural sprite sheet: 4 frames of 64x64 laid out in a row ---
    const int frameCount = 4;
    const int frameSize  = 64;

    // Start from a checkerboard so each frame has visible internal detail
    Image sheet = GenImageChecked(frameSize*frameCount, frameSize, 8, 8, LIGHTGRAY, GRAY);

    // Stamp a distinct colored block into each frame cell so stepping the source
    // rectangle is obvious to the eye (CPU-side ImageDraw* edits, no GPU needed)
    Color frameTints[4] = { RED, GREEN, BLUE, ORANGE };
    for (int f = 0; f < frameCount; f++)
        ImageDrawRectangle(&sheet, f*frameSize + 10, 10, frameSize - 20, frameSize - 20, frameTints[f]);

    Texture2D texture = LoadTextureFromImage(sheet); // upload to GPU
    UnloadImage(sheet);                              // free the CPU copy
    SetTextureFilter(texture, TEXTURE_FILTER_POINT); // crisp pixel edges

    // Animation + rotation state
    int   currentFrame  = 0;
    float frameTimer    = 0.0f;
    const float frameDuration = 0.25f; // seconds shown per frame
    float rotation      = 0.0f;

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        // Advance the animation frame on a fixed cadence
        frameTimer += dt;
        if (frameTimer >= frameDuration)
        {
            frameTimer -= frameDuration;
            currentFrame = (currentFrame + 1) % frameCount;
        }

        rotation += 40.0f * dt; // degrees per second (DrawTexturePro uses degrees)

        // The source rect selects the current frame from the sheet
        Rectangle source = { (float)(currentFrame*frameSize), 0.0f,
                             (float)frameSize, (float)frameSize };

        BeginDrawing();
            ClearBackground(RAYWHITE);

            // (1) Show the full sheet and highlight the active frame
            DrawText("Sprite sheet (active frame outlined):", 20, 25, 20, DARKGRAY);
            DrawTexture(texture, 20, 55, WHITE);
            DrawRectangleLines(20 + currentFrame*frameSize, 55, frameSize, frameSize, BLACK);

            // (2) DrawTexturePro: draw the current frame scaled 2x, rotating about
            //     its center, with a white (no-op) tint. Change WHITE to e.g.
            //     SKYBLUE to see channel-wise tinting.
            const float scale = 2.0f;
            Rectangle dest = { screenWidth*0.68f, screenHeight*0.58f,
                               frameSize*scale, frameSize*scale };
            Vector2 origin = { dest.width/2.0f, dest.height/2.0f }; // pivot = center
            DrawTexturePro(texture, source, dest, origin, rotation, WHITE);

            DrawText("DrawTexturePro: 2x scale, rotating about center",
                     360, 25, 18, DARKGRAY);
            DrawText(TextFormat("Frame %i / %i", currentFrame + 1, frameCount),
                     20, screenHeight - 30, 20, MAROON);
        EndDrawing();
    }

    UnloadTexture(texture); // free GPU resource BEFORE CloseWindow destroys the GL context
    CloseWindow();
    return 0;
}
