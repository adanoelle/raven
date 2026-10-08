# Art Integration Guide

Developer reference for integrating sprites and animation into the Raven engine.
Read the [Art Specification](art-spec.md) first for asset format requirements.

---

## 1. Pipeline Overview

Data flows from PNG files on disk through the config and rendering systems to
the screen:

```
art/**/*.aseprite
        │  just export-art
        ▼
assets/sprites/*.png + *.json  sheet (one row per tag) + frame tags and durations
        │
        ▼
assets/data/config.json       sprite_sheets array defines id, path, frame size, animations
        │
        ▼
SpriteSheetManager::load()    loads texture + frame grid at startup (Game::load_assets)
AnimationLibrary::load_file() loads the sheet's tags as clips
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
| `src/rendering/animation_library.hpp` | `AnimationLibrary`: clips from Aseprite JSON        |
| `src/ecs/components.hpp`            | `Sprite`, `Animation`, and all other components       |
| `src/ecs/systems/render_system.cpp` | `render_sprites()` system                             |
| `src/core/game.cpp`                 | `Game::load_assets()` — reads config and loads sheets |

---

## 2. Adding a Sprite Sheet

### Step 1: Export the PNG

Run `just export-art` to export Aseprite sources to `assets/sprites/` (a PNG
plus its animation JSON). A PNG made another way can be added there directly,
but has no animations. Follow the art-spec format: 32-bit RGBA PNG, no
padding between frames, uniform frame grid, lowercase underscore filename.

### Step 2: Register in config.json

Add an entry to the `sprite_sheets` array:

```json
{
  "id": "player",
  "path": "assets/sprites/player.png",
  "frame_w": 32,
  "frame_h": 32
}
```

| Field     | Type   | Description                                                  |
| --------- | ------ | ------------------------------------------------------------ |
| `id`      | string | Unique identifier used by `Sprite::sheet_id`                 |
| `path`    | string | Path to the PNG, resolved via `paths::asset()` (relative to the executable, never the CWD) |
| `frame_w` | int    | Width of one frame in pixels                                 |
| `frame_h` | int    | Height of one frame in pixels                                |
| `animations` | string | Optional. Aseprite JSON export with the sheet's frame tags (see [Animation Clips](#6-animation-clips)) |

If C++ refers to the sheet by id, add the id to `src/rendering/sheet_ids.hpp`
too. `tests/test_content.cpp` fails if any id there is missing from
`config.json`, if a registered file doesn't exist, or if a frame the code
addresses by index is outside the image. A sheet that is used but not
registered draws as a grey rectangle and logs a warning once.

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
rendering. Each entry loads on its own, so a malformed entry (a missing
`frame_h`, say) is reported by its index and skipped without affecting the
others.

Each loaded texture automatically gets
`SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_PIXELART)` applied during
`SpriteSheet::load()`. This ensures clean pixel-art upscaling without blurring —
no manual setup is needed per sheet.

---

## 3. The Sprite Component

Defined in `src/ecs/components.hpp`:

```cpp
struct Sprite {
    StringId sheet_id;    // Interned identifier of the SpriteSheet to draw from.
    int frame_x = 0;      // Frame column index in the sheet.
    int frame_y = 0;      // Frame row index in the sheet.
    int width = 32;       // Rendered width in pixels.
    int height = 32;      // Rendered height in pixels.
    int layer = 0;        // Render order (higher values draw on top).
    bool flip_x = false;  // Flip the sprite horizontally when drawing.
    float offset_x = 0.f; // Horizontal render offset from entity center in pixels.
    float offset_y = 0.f; // Vertical render offset from entity center in pixels.
};
```

### Attaching to an entity

Sheet ids are interned through the registry's `StringInterner` context
variable:

```cpp
auto& interner = reg.ctx().get<StringInterner>();

auto entity = reg.create();
reg.emplace<Transform2D>(entity, 100.f, 50.f);
reg.emplace<Sprite>(entity, interner.intern("player"), 0, 0, 32, 32, 10, false, 0.f, -5.f);
```

### Field reference

| Field      | Meaning                                                         |
| ---------- | --------------------------------------------------------------- |
| `sheet_id` | Interned `StringId` of an `id` in config.json's `sprite_sheets` |
| `frame_x`  | Column index — which frame within the current animation row     |
| `frame_y`  | Row index — which animation state                               |
| `width`    | Rendered width in pixels — normally equal to `frame_w`          |
| `height`   | Rendered height in pixels — normally equal to `frame_h`         |
| `layer`    | Drawing order. Higher values render on top of lower values      |
| `flip_x`   | `true` draws the sprite mirrored horizontally (leftward facing) |
| `offset_x` | Horizontal draw offset from the entity center                   |
| `offset_y` | Vertical draw offset — negative shifts the visual up so feet align with the collision center |

When `width`/`height` differ from the sheet's frame size, the engine scales
the frame at draw time. This is reserved for placeholder art (the current
16x16 grunt sheet drawn at 24x24); final art is always authored at its
rendered size and drawn 1:1 — see the
[Art Specification](art-spec.md).

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

Sprites are centered on the entity position, then shifted by the render
offset:

```cpp
dest_x = render_x + offset_x - width / 2;
dest_y = render_y + offset_y - height / 2;
```

The entity's `Transform2D` position represents the center of the sprite, not
the top-left corner. Characters use a negative `offset_y` (the player uses
-5) so the visual body sits higher while the feet line up with the entity's
collision center — see the anchoring section of the
[Aseprite Setup Guide](art-aseprite-guide.md).

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

## 6. Animation Clips

Animations come from Aseprite. Each frame tag in a sheet's export is a clip
named after the tag, with Aseprite's own frame durations. Export with
`just export-art`, which writes `assets/sprites/<name>.png` and
`<name>.json`, then add the data to the sheet's `config.json` entry:

```json
{ "id": "goblin", "path": "assets/sprites/goblin.png", "frame_w": 24, "frame_h": 24,
  "animations": "assets/sprites/goblin.json" }
```

| In Aseprite                 | In the game                                        |
| --------------------------- | -------------------------------------------------- |
| Frame tag name              | Clip name (`idle`, `walk`, `attack`, `dash`)       |
| Frame duration              | How long that frame shows                          |
| Tag direction               | Play order (forward, reverse, pingpong)            |
| Tag repeat: none            | Loops forever                                      |
| Tag repeat: 1               | Plays once and holds the last frame                |

The `Animation` component names the clip it is playing:

```cpp
struct Animation {
    StringId clip;         // Interned clip name (the Aseprite tag).
    int frame = 0;         // Index into the clip's frames.
    float elapsed = 0.f;   // Seconds spent on the current frame.
    int passes = 0;        // Completed passes through the clip.
    bool finished = false; // A repeat-limited clip has shown its last frame in full.
};
```

`update_animation()` advances it and writes the frame's column and row into
`Sprite::frame_x` and `frame_y`. See
[Sprite Animation](../architecture/sprite-animation.md) for the full design.

---

## 7. Switching Clips

Use `systems::play_clip`. It restarts the animation only when the clip
changes, so it is safe to call every tick:

```cpp
auto& interner = reg.ctx().get<StringInterner>();
systems::play_clip(reg.get<Animation>(entity), interner.intern(clips::WALK));
```

A one-shot clip sets `Animation::finished` once its last frame has shown for
its full duration; game logic can wait on that.

The player's clip is chosen by `update_player_animation`: attack (melee or
ground slam), then dash, walk, idle. When a sheet lacks a tag, it falls back
to walk, then idle, and logs the missing tag once.

---

## 8. Step-by-Step: Adding an Animated Entity

Complete walkthrough from receiving art to seeing it animate in-game.

### 1. Export the sprite sheet

Save the source as `art/enemies/goblin.aseprite` with a frame tag per
animation (`idle`, `walk`, ...), then run `just export-art`. That writes
`assets/sprites/goblin.png` (one row per tag) and `goblin.json`. Verify the
sheet follows the art-spec:

- Uniform frame grid, no padding
- Rows = animation states, columns = frames
- Faces right
- 32-bit RGBA, transparent background

### 2. Add a config.json entry

```json
{
  "id": "goblin",
  "path": "assets/sprites/goblin.png",
  "frame_w": 24,
  "frame_h": 24,
  "animations": "assets/sprites/goblin.json"
}
```

The goblin is a grunt, so it uses the small tier (24x24 frame, 20x20 body)
from the [Art Specification](art-spec.md).

### 3. Create the entity with components

```cpp
auto& interner = reg.ctx().get<StringInterner>();
auto goblin = reg.create();

reg.emplace<Transform2D>(goblin, spawn_x, spawn_y);
reg.emplace<PreviousTransform>(goblin, spawn_x, spawn_y);

reg.emplace<Sprite>(goblin, interner.intern("goblin"),
    0,      // frame_x: first frame
    0,      // frame_y: idle row
    24, 24, // width, height (match frame_w/frame_h — drawn 1:1)
    30,     // layer: enemy layer
    false,  // flip_x
    0.f,    // offset_x
    -3.f    // offset_y: feet alignment (bottom-center anchor)
);

reg.emplace<Animation>(goblin, Animation{interner.intern(clips::IDLE)});
```

### 4. Initial animation

The entity starts on its `idle` clip, at the timing set in Aseprite.

### 5. Verify in-game

Run the game. The goblin should appear at the spawn position with its idle
animation playing. If the sprite sheet is not found, you will see a colored
placeholder rectangle instead — check the console for loading warnings.

To switch to the walk clip when the goblin starts moving:

```cpp
systems::play_clip(reg.get<Animation>(goblin), interner.intern(clips::WALK));
```
