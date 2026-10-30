# raylib 5.5 examples

Self-contained, **asset-free** sample programs. Every one was compiled and linked
against the installed raylib 5.5 (`/opt/homebrew/opt/raylib`) on macOS / Apple Silicon.

## Build & run any example

From this folder, swap in the file you want:

```sh
cc -Wall -std=c99 -I/opt/homebrew/opt/raylib/include 01_input_movement.c -o demo \
   -L/opt/homebrew/opt/raylib/lib -lraylib \
   -framework Cocoa -framework IOKit -framework OpenGL
./demo
```

Or, from the project root, point the existing `Makefile` at one by copying it over
`main.c` (or edit the `SRC` line in the Makefile).

## What each one teaches

| File | Focus |
|---|---|
| `01_input_movement.c` | Frame-rate-independent movement (`GetFrameTime`), keyboard input, screen-bounds clamping |
| `02_camera2d_collision.c` | `Camera2D` follow in a world larger than the screen, AABB wall collision, world↔screen readout |
| `03_game_states.c` | A `GameScreen` state machine (TITLE → GAMEPLAY → GAMEOVER) with input-driven transitions |
| `04_texture_render.c` | Procedural texture (`GenImageChecked`), `DrawTexturePro` rotate/scale/tint, sprite-sheet source stepping |
| `05_3d_basics.c` | `Camera3D`, the `BeginMode3D`/`EndMode3D` bracket, orbital camera, mouse picking (`GetScreenToWorldRay` + `GetRayCollisionBox`) |
| `06_text_input.c` | Text entry: draining `GetCharPressed`, backspace with key-repeat, a blinking caret |
| `07_tilemap.c` | Rendering a tile grid from one atlas with `DrawTextureRec`, screen↔tile math, click-to-edit |

> These demos lay out UI in plain screen pixels and omit `FLAG_WINDOW_HIGHDPI` for
> brevity. On a Retina display add `SetConfigFlags(FLAG_WINDOW_HIGHDPI)` before
> `InitWindow` for a crisp framebuffer (see the HighDPI note in `../patterns.md`).
