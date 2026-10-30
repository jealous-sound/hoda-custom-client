# raylib 5.5 — API Quick Reference

> Targets **raylib 5.5** (headers at `/opt/homebrew/opt/raylib/include` — `raylib.h`, `raymath.h`, `rlgl.h`). Signatures verified against the installed v5.5 headers. The default font is auto-loaded by `InitWindow`; a valid GL context exists only between `InitWindow` and `CloseWindow`.

---

## Core structs

Field layouts as declared in `raylib.h` v5.5.

| Struct | Fields |
|---|---|
| `Vector2` | `float x, y` |
| `Vector3` | `float x, y, z` |
| `Vector4` / `Quaternion` | `float x, y, z, w` |
| `Matrix` | `float m0,m4,m8,m12, m1,m5,m9,m13, m2,m6,m10,m14, m3,m7,m11,m15` (column-major, OpenGL) |
| `Rectangle` | `float x, y, width, height` (all float) |
| `Color` | `unsigned char r, g, b, a` (8-bit per channel) |
| `Image` | `void *data; int width; int height; int mipmaps; int format` (pixels in **CPU/RAM**) |
| `Texture` / `Texture2D` / `TextureCubemap` | `unsigned int id; int width; int height; int mipmaps; int format` (pixels in **GPU/VRAM**) |
| `RenderTexture` / `RenderTexture2D` | `unsigned int id; Texture2D texture; Texture2D depth` (FBO; `.texture` is Y-flipped) |
| `NPatchInfo` | `Rectangle source; int left; int top; int right; int bottom; int layout` |
| `GlyphInfo` | `int value; int offsetX; int offsetY; int advanceX; Image image` |
| `Font` | `int baseSize; int glyphCount; int glyphPadding; Texture2D texture; Rectangle *recs; GlyphInfo *glyphs` |
| `Camera2D` | `Vector2 offset; Vector2 target; float rotation; float zoom` (rotation in **degrees**, zoom 1.0 = none) |
| `Camera3D` / `Camera` | `Vector3 position; Vector3 target; Vector3 up; float fovy; int projection` (`Camera` is a typedef alias of `Camera3D`) |
| `Mesh` | `int vertexCount; int triangleCount; float *vertices; float *texcoords; float *texcoords2; float *normals; float *tangents; unsigned char *colors; unsigned short *indices; ... unsigned int vaoId; unsigned int *vboId` |
| `Shader` | `unsigned int id; int *locs` (`locs` is a heap array of `RL_MAX_SHADER_LOCATIONS`=32 ints, owned by the Shader) |
| `MaterialMap` | `Texture2D texture; Color color; float value` |
| `Material` | `Shader shader; MaterialMap *maps; float params[4]` |
| `Model` | `Matrix transform; int meshCount; int materialCount; Mesh *meshes; Material *materials; int *meshMaterial; int boneCount; BoneInfo *bones; Transform *bindPose` |
| `ModelAnimation` | `int boneCount; int frameCount; BoneInfo *bones; Transform **framePoses; char name[32]` |
| `Ray` | `Vector3 position; Vector3 direction` (direction should be normalized) |
| `RayCollision` | `bool hit; float distance; Vector3 point; Vector3 normal` (read `hit` before trusting the rest) |
| `BoundingBox` | `Vector3 min; Vector3 max` (corner-based, NOT center/extents) |
| `Wave` | `unsigned int frameCount; unsigned int sampleRate; unsigned int sampleSize; unsigned int channels; void *data` |
| `AudioStream` | `rAudioBuffer *buffer; rAudioProcessor *processor; unsigned int sampleRate; unsigned int sampleSize; unsigned int channels` |
| `Sound` | `AudioStream stream; unsigned int frameCount` |
| `Music` | `AudioStream stream; unsigned int frameCount; bool looping; int ctxType; void *ctxData` (`looping` defaults true) |

Predefined `Color` constants (`CLITERAL(Color){r,g,b,a}`): `RAYWHITE, WHITE, BLACK, BLANK` (transparent), `LIGHTGRAY, GRAY, DARKGRAY, RED, MAROON, GREEN, LIME, DARKGREEN, BLUE, SKYBLUE, DARKBLUE, YELLOW, GOLD, ORANGE, PINK, PURPLE, VIOLET, DARKPURPLE, BEIGE, BROWN, DARKBROWN, MAGENTA`.

Construct inline with compound literals: `(Vector2){x,y}`, `(Rectangle){x,y,w,h}`, `(Color){r,g,b,a}`.

---

## Window & Timing (core)

```c
void  InitWindow(int width, int height, const char *title);   // create window + GL context; call first
void  CloseWindow(void);                                       // destroy window/GL context; call last
bool  WindowShouldClose(void);                                 // loop condition; true on close button or ESC
void  BeginDrawing(void);                                      // begin a frame
void  EndDrawing(void);                                        // swap buffers, poll input, pace FPS
void  ClearBackground(Color color);                            // clear framebuffer (first call after BeginDrawing)
void  SetTargetFPS(int fps);                                   // software FPS cap (0 = uncapped); not VSync
float GetFrameTime(void);                                      // delta time (s) of last frame; 0 on first frame
double GetTime(void);                                          // seconds since InitWindow (double)
int   GetFPS(void);                                            // current measured FPS (smoothed)
void  SetConfigFlags(unsigned int flags);                      // set ConfigFlags BEFORE InitWindow
bool  IsWindowResized(void);                                   // true only the frame size changed (needs RESIZABLE)
int   GetScreenWidth(void);                                    // logical screen width (screen units)
int   GetScreenHeight(void);
int   GetRenderWidth(void);                                    // physical framebuffer width (pixels, HiDPI)
int   GetRenderHeight(void);
void  SetWindowState(unsigned int flags);                      // toggle window state at runtime (ConfigFlags)
Vector2 GetWindowScaleDPI(void);                               // DPI scale (e.g. {2,2} on Retina)
int   GetCurrentMonitor(void);                                 // monitor index for GetMonitor* queries
void  SetRandomSeed(unsigned int seed);                        // seed RNG for reproducible runs
int   GetRandomValue(int min, int max);                        // uniform int in [min,max] INCLUSIVE
int  *LoadRandomSequence(unsigned int count, int min, int max);// non-repeating ints; free with Unload*
void  UnloadRandomSequence(int *sequence);                     // free LoadRandomSequence result (mandatory)
void  TakeScreenshot(const char *fileName);                    // save framebuffer (.png) to working dir
void  SetTraceLogLevel(int logLevel);                          // filter console log severity
void  SetExitKey(int key);                                     // change/disable ESC quit; KEY_NULL = disable
```

**Enums:**
- `ConfigFlags` (OR-combinable, for `SetConfigFlags`/`SetWindowState`): `FLAG_VSYNC_HINT=0x40`, `FLAG_FULLSCREEN_MODE=0x02`, `FLAG_WINDOW_RESIZABLE=0x04`, `FLAG_WINDOW_UNDECORATED=0x08`, `FLAG_WINDOW_HIDDEN=0x80`, `FLAG_WINDOW_MINIMIZED=0x200`, `FLAG_WINDOW_MAXIMIZED=0x400`, `FLAG_WINDOW_UNFOCUSED=0x800`, `FLAG_WINDOW_TOPMOST=0x1000`, `FLAG_WINDOW_ALWAYS_RUN=0x100`, `FLAG_WINDOW_TRANSPARENT=0x10`, `FLAG_WINDOW_HIGHDPI=0x2000`, `FLAG_WINDOW_MOUSE_PASSTHROUGH=0x4000`, `FLAG_BORDERLESS_WINDOWED_MODE=0x8000`, `FLAG_MSAA_4X_HINT=0x20`, `FLAG_INTERLACED_HINT=0x10000`
- `TraceLogLevel` (priority order): `LOG_ALL=0, LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_FATAL, LOG_NONE`

**Key notes:** Context-time hints (`MSAA_4X_HINT`, `VSYNC_HINT`, `HIGHDPI`) MUST precede `InitWindow` — calling `SetConfigFlags` after silently no-ops; use `SetWindowState`/`ClearWindowState` for runtime state. `GetScreenWidth/Height` (logical) ≠ `GetRenderWidth/Height` (pixels) on Retina; scale = `GetWindowScaleDPI()`. `GetFrameTime()` is 0 the first frame. Unload all GPU resources before `CloseWindow` (it destroys the GL context). Canonical loop: `InitWindow → SetTargetFPS → while(!WindowShouldClose()){ update; BeginDrawing(); ClearBackground(); draw; EndDrawing(); } → CloseWindow`.

---

## Input (Keyboard / Mouse / Gamepad / Touch & Gestures)

```c
// Keyboard
bool IsKeyDown(int key);          // held (continuous; scale by GetFrameTime)
bool IsKeyPressed(int key);       // transition to down (one frame; discrete actions)
bool IsKeyReleased(int key);      // transition to up
bool IsKeyUp(int key);            // not held
bool IsKeyPressedRepeat(int key); // auto-repeat fires while held
int  GetKeyPressed(void);         // pop next keycode (0 when empty); loop for remapping
int  GetCharPressed(void);        // pop next Unicode codepoint (0 when empty); loop for text input
void SetExitKey(int key);         // override ESC quit; KEY_NULL disables

// Mouse
Vector2 GetMousePosition(void);                 // logical screen coords
Vector2 GetMouseDelta(void);                    // movement since last frame (mouselook)
bool IsMouseButtonPressed(int button);          // first-press edge
bool IsMouseButtonDown(int button);             // held (dragging)
float GetMouseWheelMove(void);                  // scroll this frame (larger of X/Y)
Vector2 GetMouseWheelMoveV(void);               // scroll as separate X and Y (trackpads)
void SetMouseCursor(int cursor);                // system cursor shape

// Gamepad
bool IsGamepadAvailable(int gamepad);                       // check 0-based index before reading
bool IsGamepadButtonPressed(int gamepad, int button);       // edge-triggered
float GetGamepadAxisMovement(int gamepad, int axis);        // [-1,1]; triggers rest at -1; apply deadzone

// Touch & gestures
int     GetTouchPointCount(void);               // active touch points
Vector2 GetTouchPosition(int index);            // screen pos of touch index
void    SetGesturesEnabled(unsigned int flags); // OR of Gesture flags to detect
int     GetGestureDetected(void);               // last detected gesture (single Gesture value)
```

**Enums:**
- `KeyboardKey` (US layout, GLFW codes): `KEY_NULL=0`; letters `KEY_A..KEY_Z` (65..90); digits `KEY_ZERO..KEY_NINE` (48..57); `KEY_SPACE=32, KEY_ESCAPE=256, KEY_ENTER=257, KEY_TAB=258, KEY_BACKSPACE=259, KEY_DELETE=261`; arrows `KEY_RIGHT=262, KEY_LEFT=263, KEY_DOWN=264, KEY_UP=265`; `KEY_LEFT_SHIFT=340, KEY_LEFT_CONTROL=341, KEY_LEFT_ALT=342, KEY_LEFT_SUPER=343` (Cmd on macOS), `KEY_RIGHT_*=344..347`; `KEY_F1..KEY_F12` (290..301); keypad `KEY_KP_0..KEY_KP_9` (320..329), `KEY_KP_ENTER=335`; Android: `KEY_BACK=4, KEY_MENU=5, KEY_VOLUME_UP=24, KEY_VOLUME_DOWN=25`
- `MouseButton`: `MOUSE_BUTTON_LEFT=0, MOUSE_BUTTON_RIGHT=1, MOUSE_BUTTON_MIDDLE=2, MOUSE_BUTTON_SIDE=3, MOUSE_BUTTON_EXTRA=4, MOUSE_BUTTON_FORWARD=5, MOUSE_BUTTON_BACK=6` (aliases `MOUSE_LEFT_BUTTON`/`MOUSE_RIGHT_BUTTON`/`MOUSE_MIDDLE_BUTTON`)
- `MouseCursor`: `MOUSE_CURSOR_DEFAULT=0, MOUSE_CURSOR_ARROW=1, MOUSE_CURSOR_IBEAM=2, MOUSE_CURSOR_CROSSHAIR=3, MOUSE_CURSOR_POINTING_HAND=4, MOUSE_CURSOR_RESIZE_EW=5, _RESIZE_NS=6, _RESIZE_NWSE=7, _RESIZE_NESW=8, _RESIZE_ALL=9, MOUSE_CURSOR_NOT_ALLOWED=10`
- `GamepadButton`: `GAMEPAD_BUTTON_UNKNOWN=0`, then sequential `LEFT_FACE_UP/RIGHT/DOWN/LEFT` (dpad), `RIGHT_FACE_UP/RIGHT/DOWN/LEFT` (Xbox Y/B/A/X), `LEFT_TRIGGER_1, LEFT_TRIGGER_2, RIGHT_TRIGGER_1, RIGHT_TRIGGER_2, MIDDLE_LEFT` (Select), `MIDDLE` (guide), `MIDDLE_RIGHT` (Start), `LEFT_THUMB, RIGHT_THUMB`
- `GamepadAxis`: `GAMEPAD_AXIS_LEFT_X=0, GAMEPAD_AXIS_LEFT_Y=1, GAMEPAD_AXIS_RIGHT_X=2, GAMEPAD_AXIS_RIGHT_Y=3, GAMEPAD_AXIS_LEFT_TRIGGER=4, GAMEPAD_AXIS_RIGHT_TRIGGER=5` (triggers rest at -1, full = +1)
- `Gesture` (bit flags): `GESTURE_NONE=0, GESTURE_TAP=1, GESTURE_DOUBLETAP=2, GESTURE_HOLD=4, GESTURE_DRAG=8, GESTURE_SWIPE_RIGHT=16, GESTURE_SWIPE_LEFT=32, GESTURE_SWIPE_UP=64, GESTURE_SWIPE_DOWN=128, GESTURE_PINCH_IN=256, GESTURE_PINCH_OUT=512`

**Key notes:** Input refreshes at the END of the loop via `EndDrawing()` (internally `PollInputEvents()`); poll only inside the main loop. `IsKeyPressed`/`IsMouseButtonPressed` are true for exactly one frame. Drain text queues until 0: `while ((c=GetCharPressed())!=0)`. `GetCharPressed` yields Unicode codepoints only for printable chars; handle arrows/F-keys/Enter/Backspace via `IsKeyPressed`. Gamepad Y axes are typically inverted (up = negative); apply a deadzone (`|axis| < 0.1..0.2`). On macOS, Cmd = `KEY_LEFT_SUPER`. For FPS mouselook call `DisableCursor()` then read `GetMouseDelta()`. 3D picking: `GetScreenToWorldRay(GetMousePosition(), camera)`.

---

## Shapes & Colors (rshapes)

```c
void DrawPixelV(Vector2 position, Color color);                                              // SLOW: one pixel = geometry
void DrawLine(int startX, int startY, int endX, int endY, Color color);                      // 1px GL line
void DrawLineEx(Vector2 startPos, Vector2 endPos, float thick, Color color);                 // portable thickness
void DrawLineStrip(const Vector2 *points, int pointCount, Color color);                      // connected segments
void DrawCircle(int centerX, int centerY, float radius, Color color);
void DrawCircleV(Vector2 center, float radius, Color color);                                 // float center (moving objs)
void DrawCircleSector(Vector2 center, float radius, float startAngle, float endAngle, int segments, Color color);
void DrawCircleGradient(int centerX, int centerY, float radius, Color inner, Color outer);   // no Vector2 variant
void DrawRectangle(int posX, int posY, int width, int height, Color color);                  // int args
void DrawRectangleRec(Rectangle rec, Color color);                                           // float rect
void DrawRectanglePro(Rectangle rec, Vector2 origin, float rotation, Color color);           // rotate about origin (deg)
void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color);        // roundness 0..1 of shorter side
void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color);                      // border drawn INWARD
void DrawRectangleGradientV(int posX, int posY, int width, int height, Color top, Color bottom);
void DrawTriangle(Vector2 v1, Vector2 v2, Vector2 v3, Color color);                          // CCW winding or culled
void DrawTriangleFan(const Vector2 *points, int pointCount, Color color);                    // first point = center
void DrawPoly(Vector2 center, int sides, float radius, float rotation, Color color);         // regular n-gon (deg)
void DrawSplineCatmullRom(const Vector2 *points, int pointCount, float thick, Color color);  // min 4 points
Color ColorAlpha(Color color, float alpha);                                                  // set alpha 0..1 (modern Fade)
```

**Enums:**
- `BlendMode`: `BLEND_ALPHA=0` (default), `BLEND_ADDITIVE, BLEND_MULTIPLIED, BLEND_ADD_COLORS, BLEND_SUBTRACT_COLORS, BLEND_ALPHA_PREMULTIPLY, BLEND_CUSTOM, BLEND_CUSTOM_SEPARATE`
- Spline draw families (all `(const Vector2*, int pointCount, float thick, Color)`): `DrawSplineLinear` (min 2), `DrawSplineBasis` (min 4), `DrawSplineCatmullRom` (min 4), `DrawSplineBezierQuadratic` (min 3), `DrawSplineBezierCubic` (min 4)
- Color helpers: `Fade(c,a)` ≡ `ColorAlpha(c,a)`; `ColorTint(c,t)`, `ColorBrightness(c,f)`, `ColorContrast(c,f)`, `ColorLerp(a,b,t)` (clamp+round — recompute from base each frame).

**Key notes:** Immediate-mode painter's algorithm — draw background first. `DrawTriangle` requires **counter-clockwise** winding (CW filled triangles are back-face culled and vanish); `DrawTriangleLines` is documented CCW but as a GL-line outline is not culled. `Rectangle` is float but `DrawRectangle` takes int (truncation) — use `DrawRectangleRec`. `DrawRectanglePro` origin is an OFFSET from top-left and is the rotation pivot ({w/2,h/2} = center). `DrawRectangleRounded` roundness is a fraction of the shorter side, clamped 0..1. Angles are DEGREES. `BeginBlendMode(BLEND_ADDITIVE)`...`EndBlendMode()` for glows; `EndBlendMode` always resets to `BLEND_ALPHA` (no stack).

---

## Textures & Images (rtextures)

`Image` = CPU/RAM (editable, not drawable); `Texture2D` = GPU/VRAM (drawable). Edit the `Image`, then upload; CPU edits do NOT update an existing GPU texture.

```c
// Image (CPU)
Image LoadImage(const char *fileName);
Image LoadImageFromMemory(const char *fileType, const unsigned char *fileData, int dataSize); // fileType ".png"
void  UnloadImage(Image image);
Image GenImageColor(int width, int height, Color color);
Image GenImageChecked(int width, int height, int checksX, int checksY, Color col1, Color col2);
Image GenImageGradientLinear(int width, int height, int direction, Color start, Color end);   // direction deg [0..360]
void  ImageDraw(Image *dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint);       // CPU blit
void  ImageResize(Image *image, int newWidth, int newHeight);                                 // bicubic, in place
void  ImageColorTint(Image *image, Color color);                                              // multiply, in place

// Texture (GPU)
Texture2D LoadTexture(const char *fileName);                                                  // file -> VRAM
Texture2D LoadTextureFromImage(Image image);                                                  // Image -> VRAM
TextureCubemap LoadTextureCubemap(Image image, int layout);                                   // CubemapLayout
void  UnloadTexture(Texture2D texture);
RenderTexture2D LoadRenderTexture(int width, int height);                                      // offscreen FBO
void  UpdateTexture(Texture2D texture, const void *pixels);                                    // re-upload full buffer
void  UpdateTextureRec(Texture2D texture, Rectangle rec, const void *pixels);                  // re-upload sub-rect
void  SetTextureFilter(Texture2D texture, int filter);                                         // POINT=pixel art, BILINEAR
void  SetTextureWrap(Texture2D texture, int wrap);
void  GenTextureMipmaps(Texture2D *texture);                                                   // needed for TRILINEAR/aniso

// Drawing textures
void  DrawTexture(Texture2D texture, int posX, int posY, Color tint);                          // int pos
void  DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint);
void  DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint);       // atlas frame
void  DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint);
void  DrawTextureNPatch(Texture2D texture, NPatchInfo nPatchInfo, Rectangle dest, Vector2 origin, float rotation, Color tint);

// Render-to-texture
void  BeginTextureMode(RenderTexture2D target);                                               // redirect draws to FBO
void  EndTextureMode(void);
```

**Enums:**
- `PixelFormat`: `PIXELFORMAT_UNCOMPRESSED_GRAYSCALE=1, ..._R8G8B8` (24bpp), `..._R8G8B8A8` (32bpp, default for most loads), `..._R5G6B5, ..._R32G32B32A32` (float HDR), `..._COMPRESSED_DXT5_RGBA, ...` (used in `Image.format`/`Texture.format`)
- `TextureFilter`: `TEXTURE_FILTER_POINT=0` (nearest, pixel art, raylib default), `TEXTURE_FILTER_BILINEAR, TEXTURE_FILTER_TRILINEAR` (needs mipmaps), `TEXTURE_FILTER_ANISOTROPIC_4X, _8X, _16X`
- `TextureWrap`: `TEXTURE_WRAP_REPEAT=0, TEXTURE_WRAP_CLAMP, TEXTURE_WRAP_MIRROR_REPEAT, TEXTURE_WRAP_MIRROR_CLAMP`
- `CubemapLayout`: `CUBEMAP_LAYOUT_AUTO_DETECT=0, _LINE_VERTICAL, _LINE_HORIZONTAL, _CROSS_THREE_BY_FOUR, _CROSS_FOUR_BY_THREE`
- `NPatchLayout`: `NPATCH_NINE_PATCH=0` (3x3), `NPATCH_THREE_PATCH_VERTICAL` (1x3), `NPATCH_THREE_PATCH_HORIZONTAL` (3x1) → `NPatchInfo.layout`

**Key notes:** You can only draw a `Texture2D`. `DrawTexturePro` origin is subtracted from dest and is the rotation pivot ({0,0} rotates about top-left). RenderTexture color buffer is **Y-flipped** — draw `target.texture` with a NEGATIVE source height `{0,0,w,-h}`. `TEXTURE_FILTER_TRILINEAR`/aniso need `GenTextureMipmaps` first (POT sizes recommended). Pair every `Load*`/`Gen*` with the matching `Unload*` (`UnloadRenderTexture`, NOT `UnloadTexture`, for FBOs). Tint multiplies channel-wise (pass `WHITE` for true colors). Validity: `IsTextureValid`/`IsImageValid`/`IsRenderTextureValid` (failed loads return zero-id structs).

---

## Text & Fonts (rtext)

```c
void  DrawText(const char *text, int posX, int posY, int fontSize, Color color);              // default font
void  DrawTextEx(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint);
void  DrawTextPro(Font font, const char *text, Vector2 position, Vector2 origin, float rotation, float fontSize, float spacing, Color tint);
void  DrawTextCodepoint(Font font, int codepoint, Vector2 position, float fontSize, Color tint);
void  DrawFPS(int posX, int posY);                                                            // color-coded debug FPS
Font  GetFontDefault(void);                                                                   // always available after InitWindow
Font  LoadFont(const char *fileName);                                                         // TTF, size 32, ASCII
Font  LoadFontEx(const char *fileName, int fontSize, int *codepoints, int codepointCount);    // NULL,0 = 95 ASCII chars
Font  LoadFontFromImage(Image image, Color key, int firstChar);                               // XNA sprite-sheet font
Font  LoadFontFromMemory(const char *fileType, const unsigned char *fileData, int dataSize, int fontSize, int *codepoints, int codepointCount);
void  UnloadFont(Font font);                                                                  // safe no-op for default font
int   MeasureText(const char *text, int fontSize);                                            // DEFAULT font width only
Vector2 MeasureTextEx(Font font, const char *text, float fontSize, float spacing);            // width AND height
void  SetTextLineSpacing(int spacing);                                                        // '\n' vertical spacing (px)
const char  *TextFormat(const char *text, ...);                                               // printf into rotating buffer
const char **TextSplit(const char *text, char delimiter, int *count);                         // static array of substrings
int  *LoadCodepoints(const char *text, int *count);                                           // UTF-8 -> heap codepoints; Unload*
int   GetCodepointNext(const char *text, int *codepointSize);                                 // decode next UTF-8 + byte length
```

**Enums:**
- `FontType` (arg to `LoadFontData`): `FONT_DEFAULT=0` (anti-aliased), `FONT_BITMAP` (no AA / pixel font), `FONT_SDF` (signed-distance-field, needs external shader)
- `TextureFilter` on `font.texture`: `TEXTURE_FILTER_POINT` (crisp pixel fonts), `TEXTURE_FILTER_BILINEAR` (REQUIRED for SDF / smooth scaling)
- `GenImageFontAtlas` packMethod (plain int): `0` = default L-to-R, `1` = Skyline rectangle packing

**Key notes:** `TextFormat` uses an internal ring of 4 buffers × 1024 bytes — use the result immediately, never store across frames (copy with `TextCopy`); long strings truncate at 1024. `TextReplace`/`TextInsert` return heap memory you must `free()`; `TextFormat`/`TextSubtext`/`TextJoin`/`TextToUpper`/`TextToLower`/`TextSplit` return internal buffers you must NOT free. `MeasureText` works only with the default font; use `MeasureTextEx` for custom fonts. Load fonts at/above the largest render size for crispness. `TextToInteger`/`TextToFloat` ignore negative signs. SDF fonts need `BeginShaderMode` + BILINEAR or they render as a blurry blob. Free `LoadCodepoints` with `UnloadCodepoints`.

---

## Audio (raudio)

```c
void  InitAudioDevice(void);                                                                  // call once before any audio
void  CloseAudioDevice(void);                                                                 // after unloading all audio
bool  IsAudioDeviceReady(void);
void  SetMasterVolume(float volume);                                                          // 0..1 global (GetMasterVolume)
Sound LoadSound(const char *fileName);                                                         // short SFX fully in RAM
Sound LoadSoundFromWave(Wave wave);                                                            // copies samples internally
Sound LoadSoundAlias(Sound source);                                                            // shares data; lets SFX overlap
void  PlaySound(Sound sound);                                                                  // restarts if already playing
bool  IsSoundPlaying(Sound sound);
void  SetSoundVolume(Sound sound, float volume);                                               // also SetSoundPitch/SetSoundPan
void  UnloadSound(Sound sound);                                                                // UnloadSoundAlias for aliases
Music LoadMusicStream(const char *fileName);                                                   // long track, streamed from disk
void  PlayMusicStream(Music music);                                                            // Stop/Pause/ResumeMusicStream
void  UpdateMusicStream(Music music);                                                          // MUST call every frame
void  SeekMusicStream(Music music, float position);                                            // seconds; GetMusicTimeLength/Played
void  SetMusicVolume(Music music, float volume);                                               // also SetMusicPitch/SetMusicPan
void  UnloadMusicStream(Music music);
AudioStream LoadAudioStream(unsigned int sampleRate, unsigned int sampleSize, unsigned int channels); // raw PCM
// feed via UpdateAudioStream(AudioStream, const void*, int frameCount) when IsAudioStreamProcessed() is true,
// or SetAudioStreamCallback(stream, AudioCallback)
```

**Enums / magic values:** No dedicated public audio enums — config uses plain ints.
- `sampleSize` (bit depth): `8`, `16`, or `32` (32 = float; **24-bit NOT supported**). 16-bit buffer = `short`; 32-bit = `float` in [-1.0, 1.0].
- `channels`: `1` = mono, `2` = stereo.
- Common `sampleRate` (Hz): `22050, 44100, 48000`.
- `typedef void (*AudioCallback)(void *bufferData, unsigned int frames)` — for `SetAudioStreamCallback`/`AttachAudioStreamProcessor`/`AttachAudioMixedProcessor`.

**Key notes:** Forgetting `UpdateMusicStream()` every frame is the classic bug (track plays one buffer then goes silent). `PlaySound` on an already-playing Sound RESTARTS it — use `LoadSoundAlias` pools to overlap the same SFX (free aliases with `UnloadSoundAlias` BEFORE the source). `UpdateAudioStream` last arg is **frameCount** (samples per channel; stereo buffer holds `frameCount*channels` samples). Pan center is `0.5` (0=left, 1=right). Pitch is coupled to tempo (resample multiplier). Validity checks: `IsSoundValid`/`IsMusicValid`/`IsWaveValid`/`IsAudioStreamValid`.

---

## 2D Camera (Camera2D)

`Camera2D` = `{ Vector2 offset; Vector2 target; float rotation; float zoom; }` — `offset` is SCREEN-space, `target` is WORLD-space, `rotation` in degrees, `zoom` 1.0 = none.

```c
void    BeginMode2D(Camera2D camera);                                  // all draws transformed by camera
void    EndMode2D(void);                                               // restore screen space (draw HUD after)
Vector2 GetWorldToScreen2D(Vector2 position, Camera2D camera);         // world -> screen pixels
Vector2 GetScreenToWorld2D(Vector2 position, Camera2D camera);         // screen -> world (mouse picking, cursor zoom)
Matrix  GetCameraMatrix2D(Camera2D camera);                            // advanced: feed shaders / manual transforms
```

Supporting (timing/input/math used with Camera2D): `GetFrameTime`, `GetMousePosition`, `GetMouseDelta`, `GetMouseWheelMove`, `IsMouseButtonDown(int button)`, `GetScreenWidth`, `GetScreenHeight`, `GetRandomValue`, and raymath `Vector2Lerp`, `Vector2Clamp`, `Clamp`, `Vector2Add`, `Vector2Subtract`, `Vector2Scale`.

**Key notes:** Init with `Camera2D cam = {0}; cam.zoom = 1.0f;` — zoom 0 collapses the world to a point (black screen). Centered follow: `cam.offset = {sw/2, sh/2}`, `cam.target = entity`. Smooth follow frame-rate-independent: `cam.target = Vector2Lerp(cam.target, player, 1.0f - powf(0.0001f, dt))`. Cursor-centered zoom: sample `GetScreenToWorld2D(GetMousePosition(), cam)` before changing zoom, then re-pin. Drag-pan: divide `GetMouseDelta()` by zoom (`Vector2Scale(delta, 1.0f/zoom)`). +Y points DOWN. Always `EndMode2D()` before HUD draws. Clamp accounting for half-viewport `(sw*0.5f)/zoom`.

---

## 3D (Camera / Shapes / Models)

```c
// Camera / mode
void  BeginMode3D(Camera3D camera);                                                           // every 3D draw inside this
void  EndMode3D(void);                                                                          // 2D UI goes AFTER this
void  UpdateCamera(Camera *camera, int mode);                                                  // built-in controller (CameraMode)
void  UpdateCameraPro(Camera *camera, Vector3 movement, Vector3 rotation, float zoom);         // movement=(fwd,right,up), rotation=(yaw,pitch,roll) deg
Ray     GetScreenToWorldRay(Vector2 position, Camera camera);                                  // pick ray from screen point
Vector2 GetWorldToScreen(Vector3 position, Camera camera);                                     // world -> screen (labels)

// 3D shapes
void  DrawCube(Vector3 position, float width, float height, float length, Color color);
void  DrawCubeWires(Vector3 position, float width, float height, float length, Color color);
void  DrawSphere(Vector3 centerPos, float radius, Color color);                                // DrawSphereEx/Wires for detail
void  DrawPlane(Vector3 centerPos, Vector2 size, Color color);                                 // XZ-aligned ground
void  DrawGrid(int slices, float spacing);                                                     // XZ reference grid at origin

// Meshes / models
Mesh  GenMeshCube(float width, float height, float length);                                    // also GenMeshSphere/Plane/Cylinder/Torus/Knot
Model LoadModelFromMesh(Mesh mesh);                                                             // Model takes ownership of mesh
Model LoadModel(const char *fileName);                                                          // .obj/.glb/.gltf/.iqm/.m3d/.vox
void  DrawModel(Model model, Vector3 position, float scale, Color tint);
void  DrawModelEx(Model model, Vector3 position, Vector3 rotationAxis, float rotationAngle, Vector3 scale, Color tint);
void  SetMaterialTexture(Material *material, int mapType, Texture2D texture);                   // MaterialMapIndex slot
void  UnloadModel(Model model);                                                                 // frees meshes, NOT textures/shaders
ModelAnimation *LoadModelAnimations(const char *fileName, int *animCount);                      // skeletal anims
void  UpdateModelAnimation(Model model, ModelAnimation anim, int frame);                        // CPU pose (no frame wrap!)
```

**Enums:**
- `CameraProjection`: `CAMERA_PERSPECTIVE=0, CAMERA_ORTHOGRAPHIC` → `Camera3D.projection`
- `CameraMode`: `CAMERA_CUSTOM=0` (UpdateCamera does nothing), `CAMERA_FREE, CAMERA_ORBITAL, CAMERA_FIRST_PERSON, CAMERA_THIRD_PERSON`
- `MaterialMapIndex`: `MATERIAL_MAP_ALBEDO=0` (alias `MATERIAL_MAP_DIFFUSE`), `MATERIAL_MAP_METALNESS` (alias `MATERIAL_MAP_SPECULAR`), `MATERIAL_MAP_NORMAL, MATERIAL_MAP_ROUGHNESS, MATERIAL_MAP_OCCLUSION, MATERIAL_MAP_EMISSION, MATERIAL_MAP_HEIGHT, MATERIAL_MAP_CUBEMAP, MATERIAL_MAP_IRRADIANCE, MATERIAL_MAP_PREFILTER, MATERIAL_MAP_BRDF` → index into `Material.maps[]` (`_DIFFUSE`/`_SPECULAR` are `#define` aliases, not separate members)
- `ShaderLocationIndex` (advanced material shaders): `SHADER_LOC_MATRIX_MVP, SHADER_LOC_MATRIX_MODEL, SHADER_LOC_VECTOR_VIEW, SHADER_LOC_MAP_ALBEDO/_DIFFUSE, SHADER_LOC_MAP_NORMAL, SHADER_LOC_BONE_MATRICES`

**Key notes:** Init `Camera3D camera = {0};` then set `position, target, up={0,1,0}, fovy=45.0f, projection=CAMERA_PERSPECTIVE`. `projection=0` happens to be perspective, so an unset ortho camera renders as perspective. In ORTHOGRAPHIC mode `fovy` is the full vertical view-volume height in world units (not an angle; raylib uses top=fovy/2). `up` must be non-zero. `LoadModelFromMesh` takes ownership — do NOT also `UnloadMesh`. `UnloadModel` does NOT free textures/shaders you assigned. `UpdateModelAnimation` does not wrap the frame — pass `frame % anim.frameCount`; validate with `IsModelAnimationValid`. Filled 3D triangles are back-face culled (CCW winding). For first-person, `DisableCursor()`.

---

## Collision & raymath

```c
// 2D collision
bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2);                                       // AABB only (no rotation)
bool CheckCollisionCircles(Vector2 center1, float radius1, Vector2 center2, float radius2);
bool CheckCollisionCircleRec(Vector2 center, float radius, Rectangle rec);
bool CheckCollisionCircleLine(Vector2 center, float radius, Vector2 p1, Vector2 p2);           // new in 5.5
bool CheckCollisionPointRec(Vector2 point, Rectangle rec);                                      // mouse-over hit test
bool CheckCollisionPointCircle(Vector2 point, Vector2 center, float radius);
bool CheckCollisionPointTriangle(Vector2 point, Vector2 p1, Vector2 p2, Vector2 p3);
bool CheckCollisionPointLine(Vector2 point, Vector2 p1, Vector2 p2, int threshold);            // threshold in px (use >=1)
bool CheckCollisionPointPoly(Vector2 point, const Vector2 *points, int pointCount);            // convex poly / OBB point test
bool CheckCollisionLines(Vector2 startPos1, Vector2 endPos1, Vector2 startPos2, Vector2 endPos2, Vector2 *collisionPoint);
Rectangle GetCollisionRec(Rectangle rec1, Rectangle rec2);                                      // overlap rect; 0-size if none

// 3D / ray
Ray  GetScreenToWorldRay(Vector2 position, Camera camera);                                      // formerly GetMouseRay (#define alias)
bool CheckCollisionBoxes(BoundingBox box1, BoundingBox box2);
bool CheckCollisionBoxSphere(BoundingBox box, Vector3 center, float radius);
bool CheckCollisionSpheres(Vector3 center1, float radius1, Vector3 center2, float radius2);
RayCollision GetRayCollisionBox(Ray ray, BoundingBox box);                                      // hit, distance, point, normal
RayCollision GetRayCollisionSphere(Ray ray, Vector3 center, float radius);
RayCollision GetRayCollisionMesh(Ray ray, Mesh mesh, Matrix transform);                         // per-triangle, CPU, costly
BoundingBox  GetMeshBoundingBox(Mesh mesh);                                                      // local-space AABB
BoundingBox  GetModelBoundingBox(Model model);                                                   // whole-model local AABB
```

**raymath.h** (all `RMAPI static inline`, header-only — no extra link step). Macros: `PI=3.14159265358979323846f`, `EPSILON=0.000001f`, `DEG2RAD=(PI/180.0f)`, `RAD2DEG=(180.0f/PI)`. Common helpers: `Clamp(value,min,max)` (scalar), `Lerp`, `Normalize(value,start,end)` (scalar remap to 0..1 — NOT a unit vector), `Vector2Add/Subtract/Scale/Normalize/Distance/DistanceSqr/Angle/Lerp/Rotate/MoveTowards/Clamp`, `Vector3Add/Distance/DistanceSqr/MoveTowards/Lerp/Clamp`, `MatrixRotate`.

**Enums / notes:** No dedicated collision enums — results are `bool`, `Rectangle`, or `RayCollision`. `CameraProjection`: `CAMERA_PERSPECTIVE=0, CAMERA_ORTHOGRAPHIC` (used by the Camera passed to `GetScreenToWorldRay`). All 2D rect tests are AXIS-ALIGNED only (rotate points into local space or use `CheckCollisionPointPoly` for OBB). `GetCollisionRec` returns a 0-width/height rect (not a bool) on no overlap. `CheckCollisionPointLine` threshold 0 rarely registers — use ≥1. `GetScreenToWorldRayEx(position, camera, width, height)` is needed for render-texture/viewport sizes differing from the window. `GetRayCollisionMesh` is per-triangle CPU — broad-phase with boxes/spheres first. Read `rc.hit` before `rc.distance/point/normal`. `BoundingBox` is min/max corners, not center/extents. `Vector2Angle`/`Vector3Angle` return radians (× `RAD2DEG` for degree-based draw params).

---

## Shaders & rlgl

```c
Shader LoadShader(const char *vsFileName, const char *fsFileName);                              // NULL stage = raylib default
Shader LoadShaderFromMemory(const char *vsCode, const char *fsCode);                            // NULL = default vertex/fragment
bool   IsShaderValid(Shader shader);                                                            // shader.id != 0 (was IsShaderReady)
int    GetShaderLocation(Shader shader, const char *uniformName);                               // -1 if not found / stripped
int    GetShaderLocationAttrib(Shader shader, const char *attribName);
void   SetShaderValue(Shader shader, int locIndex, const void *value, int uniformType);         // ShaderUniformDataType
void   SetShaderValueV(Shader shader, int locIndex, const void *value, int uniformType, int count); // uniform array
void   SetShaderValueMatrix(Shader shader, int locIndex, Matrix mat);
void   SetShaderValueTexture(Shader shader, int locIndex, Texture2D texture);                   // extra sampler2D
void   UnloadShader(Shader shader);
void   BeginShaderMode(Shader shader);                                                          // route draws through shader
void   EndShaderMode(void);
RenderTexture2D LoadRenderTexture(int width, int height);                                       // FBO for post-processing
void   BeginTextureMode(RenderTexture2D target);
void   EndTextureMode(void);
void   UnloadRenderTexture(RenderTexture2D target);
void   DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint);       // negative source height to un-flip
void   BeginBlendMode(int mode);                                                                // BlendMode; switching flushes batch
void   EndBlendMode(void);                                                                       // resets to BLEND_ALPHA
void   BeginScissorMode(int x, int y, int width, int height);                                   // clip to screen rect (top-left origin)
void   EndScissorMode(void);
```

**Enums:**
- `ShaderUniformDataType` (the `uniformType` arg): `SHADER_UNIFORM_FLOAT=0, SHADER_UNIFORM_VEC2, SHADER_UNIFORM_VEC3, SHADER_UNIFORM_VEC4, SHADER_UNIFORM_INT, SHADER_UNIFORM_IVEC2, SHADER_UNIFORM_IVEC3, SHADER_UNIFORM_IVEC4, SHADER_UNIFORM_SAMPLER2D` (no `UINT*` members in 5.5)
- `ShaderLocationIndex` (indexes `shader.locs[]`, header order): `SHADER_LOC_VERTEX_POSITION=0, SHADER_LOC_VERTEX_TEXCOORD01, SHADER_LOC_VERTEX_TEXCOORD02, SHADER_LOC_VERTEX_NORMAL, SHADER_LOC_VERTEX_TANGENT, SHADER_LOC_VERTEX_COLOR, SHADER_LOC_MATRIX_MVP, SHADER_LOC_MATRIX_VIEW, SHADER_LOC_MATRIX_PROJECTION, SHADER_LOC_MATRIX_MODEL, SHADER_LOC_MATRIX_NORMAL, SHADER_LOC_VECTOR_VIEW, SHADER_LOC_COLOR_DIFFUSE, SHADER_LOC_COLOR_SPECULAR, SHADER_LOC_COLOR_AMBIENT, SHADER_LOC_MAP_ALBEDO` (==`_MAP_DIFFUSE`), `SHADER_LOC_MAP_METALNESS` (==`_MAP_SPECULAR`), `SHADER_LOC_MAP_NORMAL, SHADER_LOC_MAP_ROUGHNESS, SHADER_LOC_MAP_OCCLUSION, SHADER_LOC_MAP_EMISSION, SHADER_LOC_MAP_HEIGHT, SHADER_LOC_MAP_CUBEMAP, SHADER_LOC_MAP_IRRADIANCE, SHADER_LOC_MAP_PREFILTER, SHADER_LOC_MAP_BRDF, SHADER_LOC_VERTEX_BONEIDS, SHADER_LOC_VERTEX_BONEWEIGHTS, SHADER_LOC_BONE_MATRICES`
- `ShaderAttributeDataType`: `SHADER_ATTRIB_FLOAT=0, SHADER_ATTRIB_VEC2, SHADER_ATTRIB_VEC3, SHADER_ATTRIB_VEC4`
- `BlendMode`: `BLEND_ALPHA=0` (default), `BLEND_ADDITIVE, BLEND_MULTIPLIED, BLEND_ADD_COLORS, BLEND_SUBTRACT_COLORS, BLEND_ALPHA_PREMULTIPLY, BLEND_CUSTOM` (use `rlSetBlendFactors`), `BLEND_CUSTOM_SEPARATE` (use `rlSetBlendFactorsSeparate`)
- rlgl draw modes (arg to `rlBegin`): `RL_LINES=0x0001, RL_TRIANGLES=0x0004, RL_QUADS=0x0007`. `RL_MAX_SHADER_LOCATIONS=32`.

**Key notes:** macOS → desktop OpenGL 3.3 → author GLSL `#version 330` (`in`/`out`/`texture()`, named `out vec4 finalColor;`; old `varying`/`gl_FragColor`/`texture2D` won't compile). Pass NULL vertex shader for post-FX — raylib supplies the default VS. Auto-bound fragment built-ins: `in vec2 fragTexCoord; in vec4 fragColor; out vec4 finalColor; uniform sampler2D texture0; uniform vec4 colDiffuse;`. Default VS attributes (rlgl `RL_DEFAULT_SHADER_ATTRIB_LOCATION_*`, distinct from `ShaderLocationIndex`): position=0, texcoord=1, normal=2, color=3, tangent=4. Resolve uniform locations ONCE and cache; `GetShaderLocation` returns -1 for missing/optimized-out uniforms (`SetShaderValue(-1,...)` silently no-ops). `LoadShader*` never returns NULL — check `IsShaderValid`. RenderTextures are Y-flipped → negative source height in `DrawTextureRec`. Reload the FBO on window resize/HiDPI. `Begin*` calls flush the batch — group draws by shader/blend mode. rlgl immediate mode: `rlBegin(RL_TRIANGLES); rlColor4ub(...); rlVertex3f(...)×3; rlEnd();` with `rlPushMatrix`/`rlTranslatef`/`rlRotatef`/`rlScalef`/`rlPopMatrix`.

---

## Program architecture & performance

```c
void InitWindow(int width, int height, const char *title);
bool WindowShouldClose(void);                                                                   // also polls input
void CloseWindow(void);
void SetTargetFPS(int fps);                                                                      // caps rate; NOT frame-independence
float GetFrameTime(void);                                                                        // dt; multiply movement/timers
double GetTime(void);
void BeginDrawing(void);  void EndDrawing(void);  void ClearBackground(Color color);
void SetConfigFlags(unsigned int flags);                                                         // BEFORE InitWindow
bool IsKeyPressed(int key);   // edge: transitions, pause, fire
bool IsKeyDown(int key);      // level: movement (×dt)

// Persistence
unsigned char *LoadFileData(const char *fileName, int *dataSize);                                // heap byte buffer
bool  SaveFileData(const char *fileName, void *data, int dataSize);                              // raw bytes (not portable)
void  UnloadFileData(unsigned char *data);                                                       // free LoadFileData result
char *LoadFileText(const char *fileName);                                                         // NUL-terminated string
bool  SaveFileText(const char *fileName, char *text);
const char *TextFormat(const char *text, ...);                                                   // do NOT store the pointer
void  SetTraceLogLevel(int logLevel);
```

**Enums:**
- `GameScreen` (your own, per `core_basic_screen_manager`): `LOGO=0, TITLE, GAMEPLAY, ENDING` — drives parallel switches in update and draw.
- `ConfigFlags`, `KeyboardKey`, `MouseButton`, `TraceLogLevel`, `TextureFilter`, `TextureWrap`, `BlendMode`, `PixelFormat` — as listed in their sections above.

**Canonical lifecycle:** `SetConfigFlags → InitWindow → SetTargetFPS → load all resources ONCE → while(!WindowShouldClose()){ UpdateGame(GetFrameTime()); BeginDrawing(); DrawGame(); EndDrawing(); } → unload all resources → CloseWindow`.

**Key notes / pitfalls:**
- **Load once, above the loop.** Calling `LoadTexture`/`LoadFont`/`LoadSound`/`LoadMusicStream`/`LoadRenderTexture`/`LoadModel` inside the loop leaks VRAM/RAM every frame (stutter + climbing memory). Convert a procedural `Image` to `Texture2D` once, then `UnloadImage`.
- **Bracket discipline (LIFO):** `BeginDrawing/EndDrawing`, `BeginMode2D/EndMode2D`, `BeginMode3D/EndMode3D`, `BeginTextureMode/EndTextureMode`, `BeginShaderMode/EndShaderMode`, `BeginBlendMode/EndBlendMode`, `BeginScissorMode/EndScissorMode` each pair exactly. A missing `EndX` leaves the renderer in the wrong state for the rest of the frame; drawing outside `BeginDrawing/EndDrawing` is dropped.
- `SetTargetFPS` only caps the rate — multiply movement by `GetFrameTime()` for frame-rate independence. For deterministic physics use a fixed-timestep accumulator with a clamped max dt (`acc += dt; if (acc>0.25f) acc=0.25f; while(acc>=FIXED_DT){ Step(FIXED_DT); acc-=FIXED_DT; }`) to avoid tunneling and the spiral of death.
- Default texture filter is `TEXTURE_FILTER_POINT` (GL_NEAREST) — pixel art is **crisp by default**; smoothly-scaled images need `TEXTURE_FILTER_BILINEAR` (or `TRILINEAR` after `GenTextureMipmaps`).
- `UpdateMusicStream(music)` must run every frame, unconditionally.
- Batch by texture (sprite atlas + `DrawTextureRec`/`DrawTexturePro`); group draws by blend/shader state — each `Begin*` switch flushes the batch.
- `TextFormat` returns a pointer into a small ring of static buffers — use immediately, never cache; long strings truncate at 1024 bytes.
- VSync (`FLAG_VSYNC_HINT`) and `SetTargetFPS` can double-limit — pick one.
- On Apple Silicon Retina: set `FLAG_WINDOW_HIGHDPI`, then `GetRenderWidth()` = 2× `GetScreenWidth()`; use `GetWindowScaleDPI()` for mouse/layout math.
- Entity management without per-frame allocation: fixed struct arrays / object pools with an `active` flag, integer `count`, and O(1) swap-and-pop removal (`arr[i] = arr[--count]`; do NOT advance the index on the frame you remove). Preallocate buffers before the loop.
- **Cleanup order:** unload all GPU resources (textures, fonts, render textures, models) and sounds, then `CloseAudioDevice()`, then `CloseWindow()` last. Free `LoadFileData`/`LoadFileText` with `UnloadFileData`/`UnloadFileText` (not `free`). Binary `SaveFileData` of raw structs is not portable across architectures (padding/endianness). raylib 5.5 core no longer ships `SaveStorageValue`/`LoadStorageValue` — use file data instead.
