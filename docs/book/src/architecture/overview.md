# Architecture Overview

Raven is a pixel art roguelike built on three pillars:

1. **EnTT ECS** — all game state lives in an `entt::registry`. Systems are free
   functions that operate on component views.
2. **Fixed timestep at 120 Hz** — deterministic physics via an accumulator
   pattern. Rendering interpolates between ticks.
3. **Virtual resolution (480x270)** — the game renders to a small target and
   scales to the window, giving consistent pixel art at any display size.

## Data Flow

```
Input → Fixed Update (120 Hz) → Render (vsync)
          │
          ├─ input_system      (InputState → Velocity)
          ├─ movement_system   (Velocity → Transform2D)
          ├─ collision_system  (Hitbox checks → flags)
          ├─ damage_system     (Health reduction, death)
          └─ cleanup_system    (Lifetime tick, off-screen despawn)
```

## Ownership

- **Game** owns all subsystems (Renderer, Input, Clock, SceneManager,
  SpriteSheetManager) and the ECS registry.
- **Scenes** define which systems run and in what order. The active scene's
  `update()` is called for each fixed tick.
- **Components** are small POD structs. All defined in `src/ecs/components.hpp`.
- **Systems** are stateless free functions in `raven::systems`. They read/write
  components through EnTT views.

## Platform Portability

- No `std::filesystem` — SDL file I/O only (console toolchain compatibility).
- No exceptions in hot paths — some console compilers disable them.
- Asset loads resolve relative to the executable via `paths::asset()`
  ([ADR-0016](../decisions/0016-executable-relative-asset-paths.md)); user
  settings and save data live in the SDL pref path
  ([ADR-0017](../decisions/0017-settings-pref-path.md)).
- Builds run on Linux and Windows/MSVC, with system-packages-first dependency
  discovery and a CPM source fallback
  ([ADR-0020](../decisions/0020-bundled-dependency-fallback.md)). CI covers
  both platforms.
- Platform-specific code lives in `src/platform/` — currently the optional
  Steamworks wrapper ([ADR-0021](../decisions/0021-optional-steamworks.md)),
  a no-op without the SDK.
- SDL is officially supported on Nintendo Switch, keeping future console
  porting plausible.

See the individual architecture chapters for deep dives into each subsystem.
