/*
 * 05_3d_basics.c — Minimal raylib 5.5 3D scene with mouse picking.
 *
 * Demonstrates the 3D essentials:
 *   - Camera3D setup (position / target / up / fovy / projection)
 *   - The BeginMode3D / EndMode3D draw bracket (HUD is drawn AFTER it, in 2D)
 *   - An auto-orbiting camera via UpdateCamera(&cam, CAMERA_ORBITAL)
 *   - Mouse picking: cast a ray from the cursor with GetScreenToWorldRay and
 *     test it against each cube's BoundingBox with GetRayCollisionBox.
 *
 * Asset-free. Build (from this folder):
 *   cc -Wall -std=c99 -I/opt/homebrew/opt/raylib/include 05_3d_basics.c -o demo \
 *      -L/opt/homebrew/opt/raylib/lib -lraylib -framework Cocoa -framework IOKit -framework OpenGL
 */
#include "raylib.h"

#define CUBE_COUNT 5

int main(void)
{
    const int screenWidth = 900;
    const int screenHeight = 600;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);  // context-time hints, before InitWindow
    InitWindow(screenWidth, screenHeight, "raylib [3d] - basics + mouse picking");

    // Camera3D: a perspective camera looking at the origin. fovy is the vertical
    // field of view in DEGREES; projection picks perspective vs orthographic.
    Camera3D camera = { 0 };
    camera.position   = (Vector3){ 8.0f, 8.0f, 8.0f };
    camera.target     = (Vector3){ 0.0f, 0.5f, 0.0f };
    camera.up         = (Vector3){ 0.0f, 1.0f, 0.0f };   // world "up" axis
    camera.fovy       = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // A row of cubes; store positions and a shared size.
    Vector3 cubePos[CUBE_COUNT];
    for (int i = 0; i < CUBE_COUNT; i++)
        cubePos[i] = (Vector3){ (float)(i * 2 - (CUBE_COUNT - 1)), 0.5f, 0.0f };
    Vector3 cubeSize = { 1.0f, 1.0f, 1.0f };

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        // Auto-orbit the camera around its target.
        UpdateCamera(&camera, CAMERA_ORBITAL);

        // --- Mouse picking: ray from the cursor into the 3D world ---
        Ray ray = GetScreenToWorldRay(GetMousePosition(), camera);
        int hovered = -1;
        float nearest = 1e30f;
        for (int i = 0; i < CUBE_COUNT; i++)
        {
            // Build the cube's axis-aligned bounding box (corner-based: min/max).
            BoundingBox box = {
                (Vector3){ cubePos[i].x - cubeSize.x/2, cubePos[i].y - cubeSize.y/2, cubePos[i].z - cubeSize.z/2 },
                (Vector3){ cubePos[i].x + cubeSize.x/2, cubePos[i].y + cubeSize.y/2, cubePos[i].z + cubeSize.z/2 },
            };
            RayCollision hit = GetRayCollisionBox(ray, box);
            // ALWAYS check .hit before trusting .distance / .point / .normal.
            if (hit.hit && hit.distance < nearest) { nearest = hit.distance; hovered = i; }
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(camera);            // all 3D draws go between BeginMode3D / EndMode3D
                for (int i = 0; i < CUBE_COUNT; i++)
                {
                    Color fill = (i == hovered) ? GOLD : SKYBLUE;
                    DrawCube(cubePos[i], cubeSize.x, cubeSize.y, cubeSize.z, fill);
                    DrawCubeWires(cubePos[i], cubeSize.x, cubeSize.y, cubeSize.z, DARKBLUE);
                }
                DrawGrid(20, 1.0f);        // ground reference grid (slices, spacing)
            EndMode3D();                   // back to 2D screen space

            // HUD: drawn AFTER EndMode3D, in screen coordinates.
            DrawText("Orbital camera. Hover a cube to pick it (ray vs bounding box).", 10, 10, 20, DARKGRAY);
            if (hovered >= 0) DrawText(TextFormat("Picked cube #%d", hovered), 10, 36, 20, MAROON);
            DrawFPS(10, screenHeight - 30);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
