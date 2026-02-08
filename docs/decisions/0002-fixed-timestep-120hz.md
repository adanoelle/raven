# 2. Fixed Timestep at 120 Hz

Date: 2026-02-08
Status: Accepted

## Context

Game physics and collision detection need deterministic behaviour regardless of frame rate. Variable timesteps cause tunnelling (fast objects passing through thin colliders) and make replays impossible. We needed a fixed update rate high enough for precise bullet movement.

## Decision

Use a fixed timestep of 1/120 seconds (120 Hz) with an accumulator pattern:

```
accumulator += frame_delta;
while (accumulator >= TICK_RATE && steps < MAX_STEPS) {
    update(TICK_RATE);
    accumulator -= TICK_RATE;
}
alpha = accumulator / TICK_RATE;  // for render interpolation
```

Key parameters:
- **Tick rate: 120 Hz** — high enough for smooth bullet trajectories at pixel scale
- **Max steps per frame: 4** — prevents the spiral-of-death when the game can't keep up
- **Frame delta clamp: 0.25s** — ignores huge deltas from debugger breakpoints

Rendering uses the interpolation alpha to blend between the previous and current physics state, producing smooth visuals even when the display refresh rate differs from the tick rate.

## Consequences

**Positive:**
- Deterministic physics: same inputs always produce same outputs
- No bullet tunnelling at normal speeds
- Enables future replay and netplay systems
- Smooth rendering via interpolation, decoupled from physics rate

**Negative:**
- 120 Hz is CPU-intensive for complex scenes (mitigated by efficient ECS iteration)
- The max-steps cap means the game slows down under extreme load rather than catching up
- Interpolation adds a one-tick latency to visual position (imperceptible at 120 Hz)
