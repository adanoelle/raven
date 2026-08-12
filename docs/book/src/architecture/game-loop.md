# Game Loop and Fixed Timestep

All game logic in Raven advances in fixed steps of **1/120 of a second**,
regardless of how fast the machine renders frames. This chapter covers why,
and walks through the accumulator pattern that makes it work.

Key files: `src/core/clock.hpp`, `src/core/game.cpp` (`Game::run`).
Decision record: [ADR-0002](../decisions/0002-fixed-timestep-120hz.md).

---

## Why a Fixed Timestep

If systems advanced by the raw frame delta, gameplay would depend on frame
rate: bullet patterns would drift apart on slow machines, collision could
tunnel through thin hitboxes at large deltas, and no two runs would behave
identically. With dense bullet patterns as a core mechanic, that's
unacceptable.

A fixed tick gives:

- **Determinism** — the same inputs produce the same simulation, on any
  machine.
- **Stable physics** — movement and collision math sees a constant `dt`, so
  tuning done at 120 Hz is valid everywhere.
- **A precision budget** — 120 Hz is fine-grained enough for fast bullets
  and 8-pixel hitboxes, and an integer multiple of common 60 Hz displays.

The cost: render frames and simulation ticks are decoupled, so rendering
must *interpolate* between ticks (below), and input must be careful not to
lose button presses that arrive between ticks (covered in
[Smooth Character Movement](smooth-movement.md)).

## The Accumulator Pattern

`Clock` (`src/core/clock.hpp`) carries unprocessed wall-clock time in an
accumulator. Each frame deposits its delta; the simulation withdraws it in
fixed-size ticks:

```cpp
int Clock::advance(float frame_delta_seconds) {
    // Clamp to prevent huge deltas (e.g., after a breakpoint or OS stall)
    if (frame_delta_seconds > 0.25f) {
        frame_delta_seconds = 0.25f;
    }

    accumulator += frame_delta_seconds;

    int steps = 0;
    while (accumulator >= TICK_RATE && steps < MAX_STEPS_PER_FRAME) {
        accumulator -= TICK_RATE;
        ++steps;
        ++tick_count;
    }
    // ...
    interpolation_alpha = accumulator / TICK_RATE;
    return steps;
}
```

The frame rate no longer matters to the simulation:

```
60 Hz display   (16.6ms frames):  2 ticks most frames, sometimes 3
120 Hz display  (8.3ms frames):   1 tick per frame
240 Hz display  (4.2ms frames):   1 tick every other frame, 0 in between
```

A frame that runs zero ticks is normal on fast displays — the accumulator
just hasn't filled yet. A frame that runs several is normal after a hitch.

## Spiral-of-Death Prevention

If a frame takes longer than the ticks it triggers, each frame owes more
simulation than the last and the game locks up trying to catch up — the
classic *spiral of death*. Two guards prevent it:

1. **Delta clamp** — a frame delta above 250ms (a debugger break, a laptop
   suspend) is clamped rather than simulated.
2. **Step cap** — at most `MAX_STEPS_PER_FRAME = 4` ticks run per frame.
   Beyond that, the game slows down rather than freezing.

When the cap is hit, the leftover accumulator is drained rather than kept.
Otherwise `interpolation_alpha` (see next section) could exceed 1.0, turning
interpolation into *extrapolation* and flinging rendered sprites past their
actual positions.

## Render Interpolation

Rendering runs once per frame, between ticks. Drawing entities at their
last-tick positions would stutter on displays that aren't exactly 120 Hz, so
the renderer blends between the previous and current tick:

```
tick N ────────●───────────────● tick N+1
               ▲
        render frame here
        alpha = accumulator / TICK_RATE   (0.0 → at tick N, 1.0 → at N+1)

render_pos = prev_pos + (curr_pos - prev_pos) * alpha
```

`Clock::interpolation_alpha` is that blend factor: how far into the *next*
tick the unprocessed accumulator time has already progressed. The full
mechanics — including the `PreviousTransform` component and the input
edge-latching this design requires — are in
[Smooth Character Movement](smooth-movement.md).

## The Frame in `Game::run`

Putting it together, each iteration of the main loop
(`src/core/game.cpp`):

1. Measure the frame delta with SDL's performance counter.
2. Pump SDL events and poll input state once per frame.
3. `clock_.advance(delta)` → run that many `fixed_update()` calls, each
   stepping the active scene's systems by exactly `TICK_RATE`. Input press
   edges are consumed after the first tick so one press fires exactly one
   tick.
4. Housekeeping: reap finished audio streams, pump Steam callbacks.
5. Render with the current interpolation alpha, then present.
6. If vsync is off (unavailable or user-disabled), sleep to cap the loop at
   240 fps — bounding CPU/GPU spin while keeping input latency low.

The result: simulation at exactly 120 Hz, rendering as fast as the display
allows, and neither one caring what the other is doing.
