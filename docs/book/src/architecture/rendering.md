# Rendering Pipeline

Raven renders to a 480x270 virtual resolution target and scales it to the window
using SDL3's logical presentation system. Sprites are drawn from texture
atlases, sorted by layer, with float-precision positioning for smooth
interpolated movement.

Key files: `src/rendering/renderer.hpp`, `src/rendering/renderer.cpp`,
`src/rendering/sprite_sheet.hpp`, `src/rendering/sprite_sheet.cpp`,
`src/ecs/systems/render_system.cpp`.

---

## Virtual Resolution and Logical Presentation

The game renders at a fixed 480x270 internal resolution (1920/4 x 1080/4),
regardless of the actual window size. This is configured in `Renderer::init()`:

1. A window is created at `480 * scale` x `270 * scale` (default scale = 2).
2. A render target texture is created at the virtual resolution:
   ```cpp
   render_target_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888,
                                      SDL_TEXTUREACCESS_TARGET,
                                      VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
   ```
3. The render target is assigned the pixel-art scale mode:
   ```cpp
   SDL_SetTextureScaleMode(render_target_, SDL_SCALEMODE_PIXELART);
   ```
4. SDL's logical presentation system handles the scaling:
   ```cpp
   SDL_SetRenderLogicalPresentation(renderer_, VIRTUAL_WIDTH, VIRTUAL_HEIGHT,
                                    SDL_LOGICAL_PRESENTATION_LETTERBOX);
   ```

Letterbox mode preserves the 16:9 aspect ratio and adds black bars if the window
proportions differ. The game content is always rendered at exactly 480x270
pixels, then scaled up by the presentation layer.

---

## Pixel-Art Scale Mode

SDL3 introduces `SDL_SCALEMODE_PIXELART`, a per-texture scale mode designed for
pixel art. It ensures clean integer-multiple upscaling without blurring or
sub-pixel artifacts.

This mode is applied in two places:

- **Render target** — `Renderer::init()` and `Renderer::recreate_target()` set
  it on the virtual resolution texture (`renderer.cpp:42,89`).
- **Sprite sheet textures** — `SpriteSheet::load()` sets it on every loaded
  sprite atlas (`sprite_sheet.cpp:33`).

In SDL2, the equivalent was a global hint (`SDL_HINT_RENDER_SCALE_QUALITY "0"`)
that applied nearest-neighbour filtering to all textures. The SDL3 per-texture
approach gives explicit control — if future UI elements or particle textures
need bilinear filtering, they can use `SDL_SCALEMODE_LINEAR` without affecting
sprite art.

---

## Float-Precision Rendering with SDL_FRect

All draw calls use `SDL_FRect` (float rectangles) instead of SDL2's integer
`SDL_Rect`. This matters for the render system's interpolation.

The fixed timestep runs at 120 Hz. Between ticks, the render system interpolates
entity positions:

```cpp
render_x = prev.x + (tf.x - prev.x) * interpolation_alpha;
render_y = prev.y + (tf.y - prev.y) * interpolation_alpha;
```

With SDL2's integer rects, these sub-pixel positions were truncated to whole
pixels, causing occasional 1px jitter on slow-moving entities. Float rects
preserve the fractional position all the way to the GPU, producing smooth motion
at any speed.

The sprite sheet draw function constructs float rects for both source and
destination (`sprite_sheet.cpp:49-53`):

```cpp
SDL_FRect src{static_cast<float>(frame_x * frame_w_),
              static_cast<float>(frame_y * frame_h_),
              static_cast<float>(frame_w_), static_cast<float>(frame_h_)};

SDL_FRect dst{static_cast<float>(dest_x), static_cast<float>(dest_y),
              static_cast<float>(frame_w_), static_cast<float>(frame_h_)};
```

---

## Render Target Lifecycle

The virtual resolution render target is an `SDL_Texture` with
`SDL_TEXTUREACCESS_TARGET`. Its lifecycle:

1. **Creation** — During `Renderer::init()`, after the SDL renderer is created.
2. **Begin frame** — `Renderer::begin_frame()` sets the render target as active
   and clears it to black. All game drawing (sprites, tiles, HUD) goes to this
   texture.
3. **End frame** — `Renderer::end_frame()` switches back to the default render
   target (the window), clears it, and draws the virtual resolution texture
   full-screen via `SDL_RenderTexture`. The logical presentation system handles
   scaling and letterboxing.
4. **Present** — `Renderer::present()` calls `SDL_RenderPresent` to flip the
   backbuffer to the display. The ImGui debug overlay draws between end_frame
   and present (directly to the window, at window resolution).
5. **Recreation** — If SDL fires `SDL_EVENT_RENDER_TARGETS_RESET`,
   `SDL_EVENT_RENDER_DEVICE_RESET`, or `SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED`,
   the renderer destroys and recreates the target texture, re-applying
   `SDL_SCALEMODE_PIXELART`.

---

## Layer-Sorted Sprite Drawing

The render system (`src/ecs/systems/render_system.cpp`) draws all entities with
`Transform2D` and `Sprite` components. The process each frame:

1. **Collect** — Iterate all `(Transform2D, Sprite)` entities. Interpolate
   position if `PreviousTransform` is present. Look up the `SpriteSheet` by
   string ID. If not found, draw a coloured placeholder rectangle.
2. **Sort** — Sort collected entries by `Sprite::layer` (ascending). Lower
   layers draw first, higher layers draw on top.
3. **Draw** — Iterate sorted entries and call `SpriteSheet::draw()` for each.
   Sprites are centred on the entity position (offset by half frame size).

Layer conventions:

| Layer | Usage                        |
| ----- | ---------------------------- |
| 0     | Background tiles             |
| 10    | Floor decorations            |
| 20    | Items and pickups            |
| 30    | Enemies and player           |
| 40    | Projectiles                  |
| 50    | VFX (explosions, hit sparks) |
| 60    | UI overlays                  |

The sort uses a persistent scratch buffer stored in the EnTT registry context,
avoiding per-frame allocation.

---

## Horizontal Flip

When `Sprite::flip_x` is true, the sprite is drawn mirrored via the
`SDL_FlipMode` enum:

```cpp
SDL_FlipMode flip = flip_x ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
SDL_RenderTextureRotated(renderer, texture_, &src, &dst, 0.0, nullptr, flip);
```

All art faces right by convention; the engine mirrors for leftward movement.

---

## Future: SDL_GPU Shader Path

SDL3's `SDL_GPU` API exposes a cross-platform GPU abstraction with fragment
shader support. This is not currently used — the renderer relies entirely on
`SDL_Renderer` — but it opens a path for post-processing effects:

- **Bloom / glow** — Apply a blur + additive composite pass to bright pixels.
  The concussion shot VFX ([ADR-0012](../decisions/0012-concussion-shot-vfx.md))
  currently uses additive blending for glow; a shader could add real bloom.
- **CRT scanline filter** — A fragment shader simulating scanlines, slight
  barrel distortion, and colour fringing. Popular for retro pixel-art
  aesthetics.
- **Screen-space effects** — Vignette, colour grading, chromatic aberration.

The migration path would involve rendering the 480x270 scene to a GPU texture,
then running a shader pass before presenting. The existing render target pattern
makes this straightforward — the virtual resolution texture is already a
separate render pass.

See [ADR-0013](../decisions/0013-sdl3-migration.md) for the full migration
rationale.
