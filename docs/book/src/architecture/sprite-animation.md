# Sprite Animation

Raven uses a frame-based animation system driven by ECS components.
The `Animation` component ticks elapsed time forward each update,
advancing a frame index that the render system reads to select the
correct sprite sheet column.  A separate `AnimationState` component
tracks high-level states (idle, walk) to avoid redundant transitions.

## Components

### Animation

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

The animation system writes `current_frame` back to `Sprite::frame_x`
each tick, so the render system always draws the correct frame without
any extra coupling.

### AnimationState

```cpp
struct AnimationState {
    enum class State : uint8_t {
        Idle,
        Walk
    };
    State current = State::Idle;
};
```

This component exists to detect state *transitions*.  The game scene
compares the desired state (derived from velocity) against the current
state.  Only when they differ does it reset the animation parameters,
preventing a moving entity from restarting its walk animation every
tick.

## The animation system

`update_animation()` in `src/ecs/systems/animation_system.cpp` is a
free function that runs once per fixed-timestep tick:

```cpp
void update_animation(entt::registry& reg, float dt);
```

For each entity with both `Animation` and `Sprite`:

1. Accumulate `dt` into `elapsed`.
2. While `elapsed >= frame_duration`, advance `current_frame`.
3. If `current_frame > end_frame`:
   - Looping: wrap to `start_frame`.
   - One-shot: clamp to `end_frame` and stop advancing.
4. Write `current_frame` to `Sprite::frame_x`.

The while-loop handles the case where a large `dt` (or a very short
`frame_duration`) requires skipping multiple frames in a single tick.

## State switching

The player's animation state is managed inline in
`GameScene::update()`, after the input system runs but before
`update_animation()`:

```cpp
bool moving = (vel.dx * vel.dx + vel.dy * vel.dy) > 1.f;
auto desired = moving ? AnimationState::State::Walk
                      : AnimationState::State::Idle;

if (state.current != desired) {
    state.current = desired;
    if (desired == AnimationState::State::Walk) {
        sprite.frame_y = 1;       // walk row
        anim.start_frame = 0;
        anim.end_frame = 5;       // 6 frames
        anim.frame_duration = 0.1f;
    } else {
        sprite.frame_y = 0;       // idle row
        anim.start_frame = 0;
        anim.end_frame = 3;       // 4 frames
        anim.frame_duration = 0.25f;
    }
    anim.current_frame = anim.start_frame;
    anim.elapsed = 0.f;
}
```

Key details:

- **`frame_y`** selects the sprite sheet row (animation state).
  The `Animation` component does not touch `frame_y` — that is set
  only on state transitions.
- **`frame_x`** selects the column within the row.  The animation
  system drives this via `current_frame`.
- **Velocity threshold** of `1.f` (squared magnitude) prevents
  flickering between idle and walk when velocity smoothing produces
  near-zero values during deceleration.

### Horizontal flip

After the state check, the sprite's `flip_x` flag is set from the
horizontal velocity direction:

```cpp
if (vel.dx > 1.f)
    sprite.flip_x = false;
else if (vel.dx < -1.f)
    sprite.flip_x = true;
```

All art faces right.  The engine mirrors for leftward movement.  The
dead zone of `[-1, 1]` prevents flipping during velocity smoothing
when the player stops.

## One-shot animations

For non-looping animations (attack, hurt, death), set `looping = false`.
The animation system stops advancing when `current_frame == end_frame`.
Game logic can detect completion:

```cpp
bool finished = !anim.looping && anim.current_frame == anim.end_frame;
```

## System execution order

The animation system runs between input and movement in the
`GameScene::update()` pipeline:

```
update_input           input → target velocity
animation state logic  velocity → idle/walk switch
update_animation       tick frames, write to Sprite::frame_x
update_movement        velocity → position
...
```

Running animation before movement means the displayed frame reflects
the current tick's input, not the previous tick's.

## Timing reference

The art spec defines animation speed in FPS.  Convert to
`frame_duration`:

```
frame_duration = 1.0 / anim_fps
```

| Anim FPS | frame_duration | Use case |
|----------|----------------|----------|
| 4 | 0.250s | Idle breathing |
| 8 | 0.125s | Slow walk |
| 10 | 0.100s | Normal walk |
| 12 | 0.083s | Fast action |
| 15 | 0.067s | Attack / VFX |

## Key files

| File | Role |
|------|------|
| `src/ecs/components.hpp` | `Animation`, `AnimationState`, `Sprite` components |
| `src/ecs/systems/animation_system.cpp` | `update_animation()` — frame advancement |
| `src/scenes/game_scene.cpp` | State switching logic (idle/walk) |
| `src/ecs/systems/render_system.cpp` | Reads `Sprite::frame_x` to select source rect |
