# Project Tour

This chapter walks through the directory structure so you know where everything lives.

## Root

```
raven/
├── CMakeLists.txt       Root build configuration
├── flake.nix            Nix dev environment definition
├── justfile             Build command recipes
├── CLAUDE.md            AI assistant context file
├── ARCHITECTURE.md      Detailed architecture document
├── README.md            Quick start guide
└── .gitignore
```

## `src/` — Source Code

### `src/core/`
The engine foundation.

- **`game.hpp / .cpp`** — Top-level `Game` class. Owns all subsystems (renderer, input, clock, scene manager, sprite sheets) and the EnTT registry. Contains the main loop.
- **`clock.hpp`** — Fixed timestep clock at 120 Hz. Uses an accumulator pattern with a spiral-of-death cap (max 4 steps per frame).
- **`input.hpp / .cpp`** — Keyboard and gamepad input abstraction. Provides normalised movement axes and edge-detected button presses.

### `src/ecs/`
Entity Component System layer.

- **`components.hpp`** — All component structs in one header. Small POD types: `Transform2D`, `Velocity`, `Sprite`, `Animation`, `CircleHitbox`, `RectHitbox`, `Player`, `Enemy`, `Health`, `Bullet`, `Lifetime`, `DamageOnContact`, `Invulnerable`, `ScoreValue`, `OffScreenDespawn`.

### `src/ecs/systems/`
One file per system. Systems are free functions that operate on registry views.

- **`movement_system`** — Applies velocity to position, clamps player to screen.
- **`collision_system`** — Circle-circle and AABB collision detection.
- **`render_system`** — Draws sprites sorted by layer.
- **`input_system`** — Maps InputState to player velocity and actions.
- **`damage_system`** — Processes contact damage, invulnerability, and death.
- **`cleanup_system`** — Ticks lifetimes and removes expired or off-screen entities.

### `src/rendering/`
SDL2 rendering abstraction.

- **`renderer.hpp / .cpp`** — Window management and virtual resolution scaling (480x270 render target scaled to window).
- **`sprite_sheet.hpp / .cpp`** — Texture atlas loading and frame-based drawing.

### `src/scenes/`
Stack-based scene management.

- **`scene.hpp`** — Abstract `Scene` interface and `SceneManager`.
- **`title_scene.hpp / .cpp`** — Title screen with blinking prompt.
- **`game_scene.hpp / .cpp`** — Main gameplay scene, owns system execution order.

### `src/patterns/`
Data-driven bullet patterns.

- **`pattern_library.hpp / .cpp`** — Loads `EmitterDef` and `PatternDef` from JSON files. Used by enemies and the test suite.

### `src/debug/`
Development-only tooling.

- **`debug_overlay.hpp / .cpp`** — Dear ImGui overlay with FPS graph, entity inspector, and player state panel. Compiled only when `RAVEN_ENABLE_IMGUI` is set.

## `tests/`

Catch2 test suite:
- `test_collision.cpp` — Hitbox intersection tests.
- `test_patterns.cpp` — Pattern loading and parameter validation.
- `test_ecs.cpp` — Component and system integration tests.

## `assets/`

```
assets/
├── sprites/     PNG sprite sheets (Aseprite export)
├── audio/
│   ├── music/   .ogg background music
│   └── sfx/     .wav sound effects
├── fonts/       .ttf bitmap fonts
└── data/
    ├── config.json            Game settings
    └── patterns/
        ├── manifest.json      Lists pattern files to load
        ├── aimed_burst.json   Example aimed pattern
        └── spiral_3way.json   Example radial pattern
```

## `docs/`

```
docs/
├── book/        mdBook source (this book)
├── decisions/   Architecture Decision Records (ADRs)
├── devlog/      Date-prefixed development log entries
└── Doxyfile     Doxygen configuration (XML output)
```

## `cmake/`

Build system helpers:
- `CPM.cmake` — vendored CMake Package Manager
- `Dependencies.cmake` — all CPM dependency declarations
- `CompilerWarnings.cmake` — strict warning flags
- `Sanitizers.cmake` — ASan/UBSan/TSan toggles
