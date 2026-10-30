/*
 * 07_tilemap.c — Tilemap rendering from a single atlas (raylib 5.5).
 *
 * The bread-and-butter 2D-game pattern:
 *   - Keep every tile in ONE texture (the atlas) and draw cells with
 *     DrawTextureRec so the whole map renders from one bound texture (one batch).
 *   - The map is a grid of indices into the atlas strip.
 *   - Screen<->tile math: tileX = mouseX / TILE.
 *
 * Asset-free (atlas is generated procedurally). Build (from this folder):
 *   cc -Wall -std=c99 -I/opt/homebrew/opt/raylib/include 07_tilemap.c -o demo \
 *      -L/opt/homebrew/opt/raylib/lib -lraylib -framework Cocoa -framework IOKit -framework OpenGL
 */
#include "raylib.h"

#define TILE        32
#define MAP_W       20
#define MAP_H       12
#define ATLAS_TILES 4     // 4 tile types in a horizontal strip

int main(void)
{
    InitWindow(MAP_W * TILE, MAP_H * TILE + 40, "raylib - tilemap");
    SetTargetFPS(60);

    // Procedural atlas: a horizontal strip of ATLAS_TILES colored, outlined tiles.
    Image atlasImg = GenImageColor(TILE * ATLAS_TILES, TILE, BLANK);
    Color tileColors[ATLAS_TILES] = { DARKGREEN, LIME, BROWN, GRAY };
    for (int t = 0; t < ATLAS_TILES; t++)
    {
        ImageDrawRectangle(&atlasImg, t * TILE, 0, TILE, TILE, tileColors[t]);
        ImageDrawRectangleLines(&atlasImg, (Rectangle){ (float)(t * TILE), 0, TILE, TILE }, 1, BLACK);
    }
    Texture2D atlas = LoadTextureFromImage(atlasImg);
    UnloadImage(atlasImg);                       // CPU copy no longer needed
    SetTextureFilter(atlas, TEXTURE_FILTER_POINT);

    // The map: each cell stores an index into the atlas strip.
    int map[MAP_H][MAP_W];
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++)
            map[y][x] = (x + y) % ATLAS_TILES;   // simple checker-ish pattern

    while (!WindowShouldClose())
    {
        // Screen position -> tile coordinate
        Vector2 m = GetMousePosition();
        int tx = (int)(m.x / TILE);
        int ty = (int)(m.y / TILE);
        bool onMap = (tx >= 0 && tx < MAP_W && ty >= 0 && ty < MAP_H);

        // Left click cycles the hovered tile's type.
        if (onMap && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            map[ty][tx] = (map[ty][tx] + 1) % ATLAS_TILES;

        BeginDrawing();
            ClearBackground(BLACK);

            // Draw the whole map from the single bound atlas texture.
            for (int y = 0; y < MAP_H; y++)
            {
                for (int x = 0; x < MAP_W; x++)
                {
                    Rectangle src = { (float)(map[y][x] * TILE), 0, TILE, TILE };
                    DrawTextureRec(atlas, src, (Vector2){ (float)(x * TILE), (float)(y * TILE) }, WHITE);
                }
            }

            if (onMap)   // highlight the hovered tile
                DrawRectangleLinesEx((Rectangle){ (float)(tx * TILE), (float)(ty * TILE), TILE, TILE }, 2.0f, YELLOW);

            DrawRectangle(0, MAP_H * TILE, MAP_W * TILE, 40, (Color){ 20, 20, 20, 255 });
            DrawText(TextFormat("tile (%i, %i)  -  left-click to change", tx, ty),
                     8, MAP_H * TILE + 10, 20, RAYWHITE);
        EndDrawing();
    }

    UnloadTexture(atlas);
    CloseWindow();
    return 0;
}
