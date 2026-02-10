# Roguelike Game — Architecture

## Technology Stack

| Layer                 | Choice                             | Rationale                                                                                         |
| --------------------- | ---------------------------------- | ------------------------------------------------------------------------------------------------- |
| Language              | **C++20**                          | EnTT requires modern C++, good perf for high tick rates                                           |
| ECS                   | **EnTT**                           | Header-only, cache-friendly, battle-tested in indie games                                         |
| Windowing/Input/Audio | **SDL2 + SDL2_image + SDL2_mixer** | Cross-platform, Switch-friendly (Nintendo has official SDL2 support), Steam Deck native           |
| Rendering             | **SDL2_Renderer** (initially)      | Perfect for pixel art — hardware-accelerated 2D. Can swap to a custom GPU backend later if needed |
| Build System          | **CMake 3.25+** via **Ninja**      | Industry standard, CPM for dependency management                                                  |
| Debug UI              | **Dear ImGui** (SDL2 backend)      | Invaluable for tuning hitboxes, spawn rates in real-time                                          |
| Serialization         | **nlohmann/json**                  | Level data, config                                                                                |
| Logging               | **spdlog**                         | Fast, fmt-based                                                                                   |
| Testing               | **Catch2 v3**                      | Modern, header-friendly, good CMake integration                                                   |
| Packaging             | **CPack** (CMake)                  | Builds distributables for Steam, can produce .AppImage / .zip                                     |

## Platform Distribution Strategy

### Steam (Primary Target)

- **Steamworks SDK**: Added as a git-ignored vendor dependency (NDA-restricted,
  can't be in public repos)
- Wrap behind `platform::steam` interface
- Achievement, overlay, cloud save abstractions
- CPack target produces Steam-compatible build

### Nintendo Switch (Future)

- Nintendo DevKitPro SDK is proprietary — requires registered developer status
- **Key architectural decision**: All platform-specific code lives behind
  abstract interfaces
  - `platform::input`, `platform::audio`, `platform::filesystem`
  - SDL2 is officially supported on Switch, which makes porting much smoother
- Separate CMake toolchain file for Switch cross-compilation
- Keep draw calls and texture sizes within Switch GPU budget (pixel art helps
  enormously here)

### Architectural Rules for Portability

1. No `std::filesystem` (not available on all console toolchains) — use SDL file
   I/O
2. No exceptions in hot paths (some console compilers disable them)
3. Fixed-point or deterministic math for replays/netplay potential
4. Asset sizes must respect Switch memory constraints (~4GB available)

## Repository Structure

```
raven/
├── flake.nix                     # Full NixOS dev environment
├── flake.lock
├── .envrc                        # direnv integration
├── CMakeLists.txt                # Root CMake
├── cmake/
│   ├── CPM.cmake                 # CMake Package Manager (vendored)
│   ├── CompilerWarnings.cmake    # Strict warning flags
│   ├── Sanitizers.cmake          # ASan/UBSan/TSan toggles
│   └── Dependencies.cmake        # All CPM dependency declarations
│
├── src/
│   ├── main.cpp                  # Entry point, SDL init, game loop bootstrap
│   │
│   ├── core/
│   │   ├── game.hpp / .cpp       # Top-level game state machine
│   │   ├── clock.hpp             # Fixed timestep + interpolation
│   │   └── input.hpp / .cpp      # Input abstraction (keyboard/gamepad)
│   │
│   ├── ecs/
│   │   ├── components.hpp        # All components in one header (they're small)
│   │   │   # Transform2D, Velocity, Sprite, Hitbox (circle + rect),
│   │   │   # Player, Enemy, Bullet, Health,
│   │   │   # Lifetime, DamageOnContact, Invulnerable, ScoreValue
│   │   │
│   │   └── systems/
│   │       ├── movement_system.hpp / .cpp
│   │       ├── collision_system.hpp / .cpp    # Circle-circle + AABB collision
│   │       ├── render_system.hpp / .cpp       # Sprite batching, layer sorting
│   │       ├── input_system.hpp / .cpp        # Maps input -> player intent
│   │       ├── damage_system.hpp / .cpp       # Health, invuln frames, death
│   │       └── cleanup_system.hpp / .cpp      # Lifetime tick, despawn off-screen / expired
│   │
│   ├── rendering/
│   │   ├── renderer.hpp / .cpp    # SDL_Renderer wrapper, virtual resolution scaling
│   │   └── sprite_sheet.hpp / .cpp # Texture atlas + frame rects
│   │
│   ├── scenes/
│   │   ├── scene.hpp              # Abstract scene interface
│   │   ├── scene_manager.hpp      # Stack-based scene management
│   │   ├── title_scene.hpp / .cpp
│   │   └── game_scene.hpp / .cpp
│   │
│   └── platform/
│       ├── platform.hpp           # Abstract platform interface
│       ├── platform_desktop.cpp   # Steam/desktop implementation
│       └── platform_switch.cpp    # Future: Switch-specific (stubbed)
│
├── assets/
│   ├── sprites/                   # .png spritesheets (Aseprite export)
│   ├── audio/
│   │   ├── music/                 # .ogg for BGM
│   │   └── sfx/                   # .wav for sound effects
│   ├── fonts/                     # .ttf bitmap fonts
│   └── data/
│       ├── stages/                # .json stage/wave scripts
│       └── config.json            # Game settings defaults
│
├── tests/
│   ├── CMakeLists.txt
│   ├── test_collision.cpp         # Hitbox tests
│   ├── test_patterns.cpp          # Pattern math verification
│   └── test_ecs.cpp               # Component/system integration
│
├── tools/
│   ├── pattern_editor/            # Future: Dear ImGui editor
│   └── scripts/
│       ├── export_aseprite.sh     # Batch export from .aseprite -> .png
│       └── package_steam.sh       # Steam build packaging
│
├── .clang-format
├── .clang-tidy
├── .gitignore
└── README.md
```

## Key Design Decisions

### Collision System

We use:

1. **Circle-circle** for entity hitbox checks
2. **AABB** for enemy hitboxes vs player projectiles
3. Future: spatial hash for broad-phase optimization

### Fixed Timestep

```
accumulator += frame_delta;
while (accumulator >= TICK_RATE) {
    update(TICK_RATE);  // 1/120s
    accumulator -= TICK_RATE;
}
render(accumulator / TICK_RATE);  // interpolation alpha
```

### Virtual Resolution

Render to a 480x270 (16:9 pixel art) render target, then scale to window. This
gives authentic pixel art scaling and consistent gameplay across resolutions.

## Build & Dev Tools Summary

### In the Nix flake

- gcc14, cmake, ninja, pkg-config
- SDL2, SDL2_image, SDL2_mixer
- clang-tools (clang-format, clang-tidy, clangd)
- gdb, valgrind, renderdoc (graphics debugging)
- libresprite (pixel art)
- ldtk (level editor, optional)
- Catch2 (pulled via CPM, but nice to have system-wide for IDE)

### Via CPM (CMake Package Manager) — pulled at configure time

- EnTT, Dear ImGui, nlohmann/json, spdlog, Catch2, fmt
- This keeps the Nix shell focused on system deps and tools
