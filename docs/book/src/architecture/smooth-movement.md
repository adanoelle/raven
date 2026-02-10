# Smooth Character Movement

Raven runs game logic at a fixed 120 Hz tick rate, but the display may refresh
at 60 Hz, 144 Hz, or anything in between. Without careful handling, the player
sprite stutters on monitors that don't align exactly with the tick rate, and
direction changes produce a jarring single-frame snap. This chapter explains the
three techniques that work together to keep movement smooth regardless of
platform.

## The problems

### Stutter from tick/frame misalignment

The `Clock` accumulator produces a fractional leftover after each batch of
fixed-timestep updates. If rendering reads the entity's position directly from
`Transform2D`, the sprite jumps unevenly: some frames it moves one tick's worth,
some frames two, and some frames zero.

### Direction-change snap

When the input system wrote `vel = input * speed` directly, reversing from +200
px/s to -200 px/s happened in a single 8.3 ms tick. Visually the sprite
overshoots by one tick's displacement, then teleports back, reading as a
momentary freeze.

### Input drop-out on eventless frames

SDL's keyboard state pointer is always up to date after `SDL_PollEvent`, but the
game must explicitly read it every frame. If the state-reading code only runs
inside the event loop, frames with no pending SDL events produce zero input
axes. Combined with velocity smoothing, this causes the character to decelerate
toward zero and then re-accelerate — a visible pause when adding a second
movement key.

## Render interpolation

The solution to tick/frame misalignment is to render at a position between the
previous tick and the current tick, weighted by how far the accumulator has
advanced toward the next tick.

### PreviousTransform component

A small POD component stores the entity's position from the start of the current
tick:

```cpp
struct PreviousTransform {
    float x = 0.f;
    float y = 0.f;
};
```

Only entities that need smooth rendering get this component (currently just the
player). Bullets, effects, and other entities render at their exact
`Transform2D` position with no overhead.

### Snapshot before integration

At the top of `update_movement`, before applying velocity, the system copies
every entity's `Transform2D` into its `PreviousTransform`:

```cpp
auto interp_view = reg.view<Transform2D, PreviousTransform>();
for (auto [entity, tf, prev] : interp_view.each()) {
    prev.x = tf.x;
    prev.y = tf.y;
}
```

This runs inside the fixed-timestep loop, so after N ticks the
`PreviousTransform` holds the position from the start of tick N and
`Transform2D` holds the position after tick N.

### Interpolated draw

The render system receives `interpolation_alpha` from `Clock` (the fraction of a
tick remaining in the accumulator) and lerps:

```cpp
float render_x = prev->x + (tf.x - prev->x) * alpha;
float render_y = prev->y + (tf.y - prev->y) * alpha;
```

At `alpha = 0` the sprite draws at the previous tick's position; at `alpha = 1`
it draws at the current tick's position. Any value in between gives a sub-tick
position that appears perfectly smooth to the eye, regardless of display refresh
rate.

The debug hitbox rect uses the same interpolation so it tracks the sprite
exactly.

## Velocity smoothing

Rather than assigning velocity directly from input (`vel = input * speed`), the
input system uses an exponential approach toward the target velocity:

```cpp
float target_dx = mx * speed;
float target_dy = my * speed;

constexpr float approach_rate = 60.f;        // 1/s
float t = 1.f - std::exp(-approach_rate * dt);

vel.dx += (target_dx - vel.dx) * t;
vel.dy += (target_dy - vel.dy) * t;
```

### Why exponential, not linear?

An exponential ease is _frame-rate independent_ by construction. The formula
`1 - exp(-rate * dt)` produces the same physical result whether the tick rate is
120 Hz, 60 Hz, or variable. A linear lerp
(`vel += (target - vel) * fixed_fraction`) depends on being called at a
consistent rate.

### Convergence speed

With `approach_rate = 60` at the 120 Hz tick rate:

| Ticks |    Time | % of target reached |
| ----: | ------: | ------------------: |
|     1 |  8.3 ms |               39.4% |
|     3 | 25.0 ms |               77.7% |
|     5 | 41.7 ms |               91.8% |
|     8 | 66.7 ms |               98.2% |

This is fast enough to feel instant on keyboard while still softening the
single-frame snap on direction reversal. The character arcs through zero
velocity over 2-3 ticks rather than teleporting.

### Tuning

`approach_rate` is the single knob. Higher values make the response snappier (at
the extreme, it degenerates to direct assignment); lower values add more visible
easing. A good range for keyboard-driven movement in a twitch game is 40-120.
Below 40 feels sluggish; above 120 the smoothing becomes imperceptible.

## Per-frame input polling

The `Input` class splits its work into three phases:

1. **`begin_frame()`** — saves the previous state, zeros the current state.
   Called once at the start of the frame.
2. **`process_event(event)`** — handles discrete SDL events (quit, controller
   hot-plug). Called inside the `SDL_PollEvent` loop.
3. **`update()`** — polls `SDL_GetKeyboardState` and gamepad axes, computes
   edge-detected button presses. Called once _after_ the event loop.

The critical invariant: `update()` runs every frame, even when no SDL events are
pending. This guarantees that held keys always produce non-zero movement axes.
Previously, the keyboard poll lived inside `process_event()`, so eventless
frames saw zero input — which the velocity smoothing amplified into a visible
pause.

```
begin_frame()          zero current state
  |
  v
for each SDL event:    handle quit, controller plug/unplug
  process_event()
  |
  v
update()               read keyboard + gamepad, compute edges
  |
  v
fixed_update() x N     input_system reads state().move_x/y
  |
  v
render()               interpolated draw
```

## Resize safety

The renderer draws to a fixed-size offscreen texture (`render_target_`, 480x270)
and blits it to the window at present time. On some platforms (notably Linux
with SDL2-compat on Wayland), resizing the window can invalidate GPU textures.
If the game then calls `SDL_RenderCopy` with the stale texture pointer, it
crashes with SIGSEGV.

The fix has two parts:

1. **Event-driven recreation** — `Renderer::handle_event()` listens for
   `SDL_RENDER_TARGETS_RESET`, `SDL_RENDER_DEVICE_RESET`, and
   `SDL_WINDOWEVENT_SIZE_CHANGED`, and recreates `render_target_` when any of
   them fires.
2. **Null guard** — `end_frame()` skips the `SDL_RenderCopy` if `render_target_`
   is null, so even if recreation fails the game drops a frame instead of
   crashing.

## Key files

| File                                  | Role                                      |
| ------------------------------------- | ----------------------------------------- |
| `src/ecs/components.hpp`              | `PreviousTransform` component             |
| `src/ecs/systems/movement_system.cpp` | Position snapshot before integration      |
| `src/ecs/systems/input_system.cpp`    | Exponential velocity smoothing            |
| `src/ecs/systems/render_system.cpp`   | Interpolated sprite drawing               |
| `src/core/input.cpp`                  | Per-frame keyboard/gamepad polling        |
| `src/core/clock.hpp`                  | `interpolation_alpha` computation         |
| `src/rendering/renderer.cpp`          | Render target recreation on resize        |
| `src/scenes/game_scene.cpp`           | Wires alpha and dt through the call chain |
