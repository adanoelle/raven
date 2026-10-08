# Sprite Animation

Animation timing comes from Aseprite. Each frame tag in an exported sheet
becomes a named clip; the `Animation` component plays one clip and writes the
current frame into the entity's `Sprite`. The decision is recorded in
[ADR-0024](../decisions/0024-animation-clips-from-aseprite.md).

## From Aseprite to clips

`just export-art` (`tools/export_art.sh`) exports every `art/**/*.aseprite`
(except templates and sketches) to two files in `assets/sprites/`:

- `<name>.png`: one row per frame tag (`--sheet-type rows --split-tags`)
- `<name>.json`: frame rectangles, per-frame durations and the tags
  (`--list-tags --format json-array`)

The sheet's `config.json` entry points at the data:

```json
{ "id": "knight", "path": "assets/sprites/knight.png", "frame_w": 32, "frame_h": 32,
  "animations": "assets/sprites/knight.json" }
```

`Game::load_assets` loads it into the `AnimationLibrary`
(`src/rendering/animation_library.hpp`), which lives in the registry context.
For each tag:

- Every frame must be a cell of the sheet's grid (no border or padding), so
  its rectangle becomes a `frame_x` column and `frame_y` row.
- Each frame keeps its own duration, so per-frame holds survive.
- The tag direction is applied once at load: `reverse` flips the order,
  `pingpong` plays back again without repeating the turnaround frame.
- The tag's repeat count decides looping. No repeat (Aseprite's default)
  loops forever; repeat 1 plays once and holds the last frame; repeat N plays
  N times.

Problems are logged and the bad tag is skipped: a frame off the grid, a
zero-length frame, a tag range past the end, an unknown direction, or a
json-hash export. `tests/test_content.cpp` checks every shipped export
against its PNG.

## Components

```cpp
struct Animation {
    StringId clip;         // Interned clip name (the Aseprite tag).
    int frame = 0;         // Index into the clip's frames.
    float elapsed = 0.f;   // Seconds spent on the current frame.
    int passes = 0;        // Completed passes through the clip.
    bool finished = false; // A repeat-limited clip has shown its last frame in full.
};
```

Change clips with `systems::play_clip(anim, clip)`. It restarts from the
first frame only when the clip actually changes, so calling it every tick is
safe.

## The animation system

`update_animation(reg, dt)` runs once per fixed tick. For each entity with
`Animation` and `Sprite`:

1. Look up the clip for the sprite's sheet. If the sheet has no animation
   data, leave the sprite alone. If it has data but not this clip, log the
   missing tag once and leave the sprite alone.
2. Add `dt` to `elapsed`, and step through frames while `elapsed` covers the
   current frame's duration. Several frames can pass in one tick.
3. At the end of the clip, loop, start another pass, or mark it `finished`.
   `finished` is set only after the last frame has shown for its full
   duration, so follow-through frames aren't cut short.
4. Write the frame's column and row into `Sprite::frame_x` and `frame_y`.

## Player states

`update_player_animation(reg)` picks the player's clip each tick, before
`update_animation`:

| Doing                        | Clip     |
| ---------------------------- | -------- |
| Melee attack or ground slam  | `attack` |
| Dash                         | `dash`   |
| Moving (speed² > 1)          | `walk`   |
| Otherwise                    | `idle`   |

- A one-shot clip plays to its end even after the action itself ends, so the
  wind-up and follow-through always show. A new action interrupts it.
- If the sheet has no clip for the state, the choice falls back to `walk`,
  then `idle`, and the missing tag is logged once. A character with only
  idle and walk drawn still works.
- It also faces the sprite along the aim direction (all art faces right).

Enemies are given the `idle` clip when they spawn, so they animate as soon as
their sheet has an `idle` tag.

## System execution order

```
update_ai
update_player_animation  what the player is doing → clip
update_animation         advance frames, write Sprite::frame_x/frame_y
update_movement          velocity → position
...
```

Running animation before movement means the displayed frame reflects this
tick's input, not the previous tick's.

## Key files

| File                                     | Role                                                  |
| ---------------------------------------- | ----------------------------------------------------- |
| `src/rendering/animation_library.hpp`    | `AnimationLibrary`, `AnimationClip`, clip names        |
| `src/ecs/components.hpp`                 | `Animation`, `Sprite`                                 |
| `src/ecs/systems/animation_system.cpp`   | `update_animation`, `update_player_animation`, `play_clip` |
| `tools/export_art.sh`                    | Aseprite export behind `just export-art`              |
| `tests/test_animation.cpp`               | Loading, playback, one-shots and state fallbacks      |
