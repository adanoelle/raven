# Art Integration Guide

Developer reference for integrating sprites and animation into the Raven engine.
Read the [Art Specification](art-spec.md) first for asset format requirements.

---

## 1. Pipeline Overview

Data flows from PNG files on disk through the config and rendering systems to
the screen:

```
assets/sprites/*.png
        │
        ▼
assets/data/config.json       sprite_sheets array defines id, path, frame size
        │
        ▼
SpriteSheetManager::load()    loads texture + frame grid at startup (Game::load_assets)
        │
        ▼
Sprite component               attached to an entity, references sheet_id + frame coords
        │
        ▼
render_sprites()               queries Transform2D + Sprite, interpolates, draws
        │
        ▼
SDL_Renderer → screen          480×270 virtual resolution, SDL_SCALEMODE_PIXELART upscale
```

**Key files:**

| File                                | Role                                                  |
| ----------------------------------- | ----------------------------------------------------- |
| `assets/data/config.json`           | Sprite sheet and sprite definition registry           |
| `src/rendering/sprite_sheet.hpp`    | `SpriteSheet` and `SpriteSheetManager` classes        |
| `src/ecs/components.hpp`            | `Sprite`, `Animation`, and all other components       |
| `src/ecs/systems/render_system.cpp` | `render_sprites()` system                             |
| `src/core/game.cpp`                 | `Game::load_assets()` — reads config and loads sheets |

---

## 2. Adding a Sprite Sheet

### Step 1: Place the PNG

Add the exported sprite sheet to `assets/sprites/`. Follow the art-spec format:
32-bit RGBA PNG, no padding between frames, uniform frame grid, lowercase
underscore filename.

### Step 2: Register in config.json

Add an entry to the `sprite_sheets` array:

```json
{
  "id": "player",
  "path": "assets/sprites/player.png",
  "frame_w": 16,
  "frame_h": 16
}
```

| Field     | Type   | Description                                       |
| --------- | ------ | ------------------------------------------------- |
| `id`      | string | Unique identifier used by `Sprite::sheet_id`      |
| `path`    | string | Path to the PNG relative to the working directory |
| `frame_w` | int    | Width of one frame in pixels                      |
| `frame_h` | int    | Height of one frame in pixels                     |

### Step 3: Optionally add sprite definitions

Named sprite definitions map a human-readable name to a specific frame:

```json
"sprite_defs": {
    "projectile_small_red": { "sheet": "projectiles", "frame_x": 0, "frame_y": 0 }
}
```

These are for convenience — you can also reference frames directly by column/row
index in the `Sprite` component.

### Step 4: Loading

`Game::load_assets()` reads `config.json` at startup and calls
`SpriteSheetManager::load()` for each entry. Loading is non-fatal: if a sheet
fails to load, the engine logs a warning and continues with placeholder
rendering.

Each loaded texture automatically gets
`SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_PIXELART)` applied during
`SpriteSheet::load()`. This ensures clean pixel-art upscaling without blurring —
no manual setup is needed per sheet.

---

## 3. The Sprite Component

Defined in `src/ecs/components.hpp`:

```cpp
struct Sprite {
    std::string sheet_id;  // Identifier of the SpriteSheet to draw from.
    int frame_x = 0;      // Frame column index in the sheet.
    int frame_y = 0;      // Frame row index in the sheet.
    int width = 16;        // Pixel width of one frame.
    int height = 16;       // Pixel height of one frame.
    int layer = 0;         // Render order (higher values draw on top).
    bool flip_x = false;   // Flip the sprite horizontally when drawing.
};
```

### Attaching to an entity

```cpp
auto entity = reg.create();
reg.emplace<Transform2D>(entity, 100.f, 50.f);
reg.emplace<Sprite>(entity, "player", 0, 0, 16, 16, 0, false);
```

### Field reference

| Field      | Meaning                                                         |
| ---------- | --------------------------------------------------------------- |
| `sheet_id` | Must match an `id` in config.json's `sprite_sheets`             |
| `frame_x`  | Column index — which frame within the current animation row     |
| `frame_y`  | Row index — which animation state                               |
| `width`    | Frame width in pixels (must match `frame_w` in config)          |
| `height`   | Frame height in pixels (must match `frame_h` in config)         |
| `layer`    | Drawing order. Higher values render on top of lower values      |
| `flip_x`   | `true` draws the sprite mirrored horizontally (leftward facing) |

### Layer conventions

| Layer | Usage                        |
| ----- | ---------------------------- |
| 0     | Background tiles             |
| 10    | Floor decorations            |
| 20    | Items and pickups            |
| 30    | Enemies and player           |
| 40    | Projectiles                  |
| 50    | VFX (explosions, hit sparks) |
| 60    | UI overlays                  |

These are conventions, not hard rules. Use intermediate values for fine-grained
ordering within a category.

---

## 4. Frame Addressing

Sprite sheets use a uniform grid. The pixel coordinates of a frame are computed
from the column and row indices:

```
pixel_x = frame_x * frame_w
pixel_y = frame_y * frame_h
```

This matches the art-spec sheet layout:

- **Rows** = animation states (idle, walk, attack, ...)
- **Columns** = frames within a state (left to right)

```
             col 0    col 1    col 2    col 3    col 4    col 5
row 0 Idle:  [frm 0]  [frm 1]  [frm 2]  [frm 3]  [     ]  [     ]
row 1 Walk:  [frm 0]  [frm 1]  [frm 2]  [frm 3]  [frm 4]  [frm 5]
row 2 Attack:[frm 0]  [frm 1]  [frm 2]  [frm 3]  [     ]  [     ]
```

To display walk frame 3: `frame_x = 3`, `frame_y = 1`.

---

## 5. The Render System

`render_sprites()` in `src/ecs/systems/render_system.cpp` handles all sprite
drawing each frame.

### Query

The system views all entities with `Transform2D` and `Sprite` components.

### Interpolation

If the entity also has a `PreviousTransform` component, the render position is
interpolated between the previous and current tick positions:

```cpp
render_x = prev.x + (tf.x - prev.x) * interpolation_alpha;
render_y = prev.y + (tf.y - prev.y) * interpolation_alpha;
```

This produces smooth movement between the 120 Hz fixed-timestep ticks regardless
of display refresh rate. SDL3's `SDL_FRect` float-precision rectangles preserve
these sub-pixel positions all the way to the GPU, eliminating the 1px jitter
that occurred with SDL2's integer `SDL_Rect` on slow-moving entities.

### Draw order

Entities are sorted by `Sprite::layer` (ascending). Lower layers draw first,
higher layers draw on top.

### Centering

Sprites are centered on the entity position. The draw destination is offset by
half the frame size:

```cpp
dest_x = render_x - width / 2;
dest_y = render_y - height / 2;
```

The entity's `Transform2D` position represents the center of the sprite, not the
top-left corner.

### Horizontal flip

When `Sprite::flip_x` is `true`, the sprite is drawn mirrored via the
`SDL_FlipMode` enum (`SDL_FLIP_HORIZONTAL`). All art faces right; the engine
mirrors for leftward movement.

### Placeholder fallback

If the sprite sheet is not loaded (sheet ID not found), the system draws a
colored rectangle instead:

| Entity type | Color   |
| ----------- | ------- |
| Player      | Cyan    |
| Bullet      | Red     |
| Enemy       | Magenta |
| Other       | Gray    |

This allows development and testing without final art assets.

---

## 6. The Animation Component

Defined in `src/ecs/components.hpp`:

```cpp
struct Animation {
    int start_frame = 0;         // First frame index in the animation.
    int end_frame = 0;           // Last frame index in the animation.
    float frame_duration = 0.1f; // Seconds per frame.
    float elapsed = 0.f;         // Time elapsed in the current frame.
    int current_frame = 0;       // Currently displayed frame index.
    bool looping = true;         // Whether the animation loops or stops at end.
};
```

### Design intent

An animation system (to be implemented) ticks `elapsed` forward each update.
When `elapsed >= frame_duration`, it advances `current_frame` and writes it back
to `Sprite::frame_x`. This drives the visual frame displayed by the render
system.

### Field mapping

| Animation field  | Sprite field | Meaning                                   |
| ---------------- | ------------ | ----------------------------------------- |
| `current_frame`  | `frame_x`    | Column index — which frame within the row |
| (set externally) | `frame_y`    | Row index — which animation state row     |

`frame_y` is not driven by the Animation component. It is set when switching
animation states (e.g., from idle to walk).

### Converting art-spec timing

The art spec defines animation speed in FPS. Convert to `frame_duration`:

```
frame_duration = 1.0 / anim_fps
```

| Anim FPS | frame_duration |
| -------- | -------------- |
| 4        | 0.250s         |
| 8        | 0.125s         |
| 10       | 0.100s         |
| 12       | 0.083s         |
| 15       | 0.067s         |

---

## 7. Animation State Management (Design)

This section describes the intended design for switching animation states.

### Switching states

When an entity changes animation state (e.g., idle → walk), update these fields:

```cpp
auto& anim = reg.get<Animation>(entity);
auto& sprite = reg.get<Sprite>(entity);

// Switch to walk state (row 1, 6 frames, 10 fps, looping)
sprite.frame_y = 1;           // Walk row
anim.start_frame = 0;
anim.end_frame = 5;           // 6 frames: 0..5
anim.frame_duration = 0.1f;   // 10 fps
anim.looping = true;
anim.current_frame = 0;
anim.elapsed = 0.f;
```

### One-shot animations

For non-looping animations (attack, hurt, death), set `looping = false`. The
animation system should stop advancing when `current_frame == end_frame`.

Game logic can detect completion by checking:

```cpp
bool finished = !anim.looping && anim.current_frame == anim.end_frame;
```

### Avoiding magic numbers

Rather than scattering row indices and frame counts through the code, consider a
helper function or enum approach:

```cpp
enum class AnimState { Idle, Walk, Run, Attack, Dodge, Hurt, Death };

void set_anim_state(entt::registry& reg, entt::entity e, AnimState state) {
    auto& anim = reg.get<Animation>(e);
    auto& sprite = reg.get<Sprite>(e);

    switch (state) {
        case AnimState::Idle:
            sprite.frame_y = 0;
            anim.start_frame = 0;  anim.end_frame = 3;
            anim.frame_duration = 0.25f;  anim.looping = true;
            break;
        case AnimState::Walk:
            sprite.frame_y = 1;
            anim.start_frame = 0;  anim.end_frame = 5;
            anim.frame_duration = 0.1f;   anim.looping = true;
            break;
        // ... etc
    }

    anim.current_frame = anim.start_frame;
    anim.elapsed = 0.f;
}
```

This centralizes animation data and makes state transitions readable.

---

## 8. Step-by-Step: Adding an Animated Entity

Complete walkthrough from receiving art to seeing it animate in-game.

### 1. Receive and place the sprite sheet

Save the PNG to `assets/sprites/`. Verify it follows the art-spec:

- Uniform frame grid, no padding
- Rows = animation states, columns = frames
- Faces right
- 32-bit RGBA, transparent background

### 2. Add a config.json entry

```json
{
  "id": "goblin",
  "path": "assets/sprites/goblin.png",
  "frame_w": 16,
  "frame_h": 16
}
```

### 3. Create the entity with components

```cpp
auto goblin = reg.create();

reg.emplace<Transform2D>(goblin, spawn_x, spawn_y);
reg.emplace<PreviousTransform>(goblin, spawn_x, spawn_y);

reg.emplace<Sprite>(goblin, "goblin",
    0,      // frame_x: first frame
    0,      // frame_y: idle row
    16, 16, // width, height
    30,     // layer: enemy layer
    false   // flip_x
);

reg.emplace<Animation>(goblin,
    0,      // start_frame
    3,      // end_frame (4 idle frames: 0..3)
    0.25f,  // frame_duration (4 fps idle)
    0.f,    // elapsed
    0,      // current_frame
    true    // looping
);
```

### 4. Set initial animation state

The entity starts in idle (row 0). The animation system will cycle `frame_x`
through 0..3 at 4 fps.

### 5. Verify in-game

Run the game. The goblin should appear at the spawn position with its idle
animation playing. If the sprite sheet is not found, you will see a colored
placeholder rectangle instead — check the console for loading warnings.

To switch to walk animation when the goblin starts moving:

```cpp
auto& anim = reg.get<Animation>(goblin);
auto& sprite = reg.get<Sprite>(goblin);
sprite.frame_y = 1;          // Walk row
anim.start_frame = 0;
anim.end_frame = 3;          // 4 walk frames
anim.frame_duration = 0.125f; // 8 fps
anim.looping = true;
anim.current_frame = 0;
anim.elapsed = 0.f;
```
