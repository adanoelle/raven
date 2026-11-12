# Roguelike Game — Architecture

## Technology Stack

| Layer                 | Choice                             | Rationale                                                                                         |
| --------------------- | ---------------------------------- | ------------------------------------------------------------------------------------------------- |
| Language              | **C++20**                          | EnTT requires modern C++, good perf for high tick rates                                           |
| ECS                   | **EnTT**                           | Header-only, cache-friendly, battle-tested in indie games                                         |
| Windowing/Input/Audio | **SDL3 (>= 3.4) + SDL3_image**     | Cross-platform, Steam Deck native. Sound effects use SDL3's own audio streams, no mixer library   |
| Rendering             | **SDL_Renderer** (initially)       | Perfect for pixel art — hardware-accelerated 2D. Can swap to a custom GPU backend later if needed |
| Build System          | **CMake 3.25+** via **Ninja**      | Industry standard, CPM for dependency management                                                  |
| Debug UI              | **Dear ImGui** (SDL3 backends)     | Invaluable for tuning hitboxes, spawn rates in real-time                                          |
| Serialization         | **nlohmann/json**                  | Level data, config                                                                                |
| Logging               | **spdlog**                         | Fast, fmt-based                                                                                   |
| Testing               | **Catch2 v3**                      | Modern, header-friendly, good CMake integration                                                   |
| Packaging             | **CPack** (CMake)                  | Produces .zip and .tar.gz packages with assets/ next to the binary                                |

## Platform Distribution Strategy

### Steam (Primary Target)

- **Steamworks SDK**: Added as a git-ignored vendor dependency (NDA-restricted,
  can't be in public repos), enabled with `-DRAVEN_ENABLE_STEAM=ON`
- Wrapped by the `raven::Steam` class (`src/platform/steam.hpp`), a no-op
  when Steam is not compiled in
- Achievement, overlay, cloud save abstractions
- CPack target produces Steam-compatible build

### Nintendo Switch (Future)

- Nintendo DevKitPro SDK is proprietary — requires registered developer status
- **Key architectural decision**: All platform-specific code lives behind
  abstract interfaces
  - `platform::input`, `platform::audio`, `platform::filesystem`
  - SDL3 already sits between the game and the OS for windowing, input, audio
    and file I/O, which keeps the platform-specific surface small
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
│   ├── Sanitizers.cmake          # ASan + UBSan flags (RAVEN_ENABLE_ASAN)
│   ├── ShippingFlags.cmake       # Flags for optimised (shipping) builds
│   ├── Steamworks.cmake          # Imports the SDK from vendor/steamworks/sdk
│   └── Dependencies.cmake        # SDL3 lookup + all CPM dependency declarations
│
├── src/
│   ├── main.cpp                  # Entry point: creates Game, runs it
│   │
│   ├── core/
│   │   ├── game.hpp / .cpp       # SDL init, owns subsystems, main loop
│   │   ├── clock.hpp             # Fixed timestep + interpolation
│   │   ├── input.hpp / .cpp      # Input abstraction (keyboard/mouse/gamepad)
│   │   └── ...                   # paths, fs, settings, save_data, string_id
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
│   │       ├── cleanup_system.hpp / .cpp      # Lifetime tick, despawn off-screen / expired
│   │       └── ...                            # One pair per system: ai, shooting, melee,
│   │                                          # dash, wave, pickup, tile_collision, hud, ...
│   │
│   ├── rendering/
│   │   ├── renderer.hpp / .cpp    # SDL_Renderer wrapper, virtual resolution scaling
│   │   ├── sprite_sheet.hpp / .cpp # Texture atlas + frame rects
│   │   └── ...                    # tilemap (LDtk), bitmap_font, animation_library
│   │
│   ├── scenes/
│   │   ├── scene.hpp              # Abstract scene interface + SceneManager
│   │   ├── scene_manager.cpp      # Stack-based scene management
│   │   ├── title_scene.hpp / .cpp
│   │   ├── game_scene.hpp / .cpp
│   │   └── ...                    # char_select, pause, options, game_over, victory
│   │
│   ├── audio/                     # audio_engine: SDL3 audio streams, .wav effects
│   ├── patterns/                  # pattern_library: bullet patterns from JSON
│   ├── debug/                     # debug_overlay: Dear ImGui (RAVEN_ENABLE_IMGUI)
│   │
│   └── platform/
│       ├── platform.hpp           # Compile-time flags (RAVEN_PLATFORM_CONSOLE)
│       └── steam.hpp / .cpp       # Optional Steamworks wrapper
│
├── assets/
│   ├── sprites/                   # .png spritesheets (Aseprite export)
│   ├── audio/
│   │   ├── music/                 # .ogg for BGM
│   │   └── sfx/                   # .wav for sound effects
│   ├── fonts/                     # font.png bitmap font
│   ├── maps/                      # raven.ldtk level project
│   ├── tilesets/                  # .png tilesets used by the LDtk project
│   └── data/
│       ├── stages/                # .json stage/wave scripts
│       ├── patterns/              # .json bullet patterns
│       ├── enemies.json           # Named enemy definitions
│       └── config.json            # Read-only game data
│
├── tests/
│   ├── CMakeLists.txt             # Lists every test source
│   └── test_<area>.cpp            # One Catch2 file per system or area
│
├── tools/
│   ├── export_art.sh              # Aseprite -> .png + animation .json (just export-art)
│   ├── lint_files.sh              # Picks sources for just lint-changed
│   └── gen_*.py, gen_*.lua        # Placeholder and template generators
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
- SDL3, SDL3_image
- clang-tools (clang-format, clang-tidy, clangd)
- gdb, valgrind, renderdoc (graphics debugging)
- libresprite (pixel art)
- ldtk (level editor)
- mdbook, doxygen (docs), just (command runner)

### Via CPM (CMake Package Manager) — pulled at configure time

- EnTT, Dear ImGui, nlohmann/json, spdlog (bundles fmt), LDtkLoader, Catch2
- SDL3 and SDL3_image too, but only when no system package is found
- This keeps the Nix shell focused on system deps and tools
