# Project Tour

This chapter walks through the directory structure so you know where everything
lives. It names the parts and what they are for; the architecture chapters
explain how they work.

## Root

```
raven/
├── CMakeLists.txt       Root build configuration
├── cmake/               Build helpers (dependencies, warnings, shipping flags)
├── flake.nix            Nix dev environment definition
├── justfile             Build command recipes
├── src/                 Game source
├── tests/               Catch2 tests
├── assets/              What ships with the game
├── art/                 Aseprite sources, exported into assets/sprites/
├── tools/               Art export and asset generators
├── docs/                This book, the API reference config, design notes
├── .github/workflows/   CI, the docs check and the docs site
├── CLAUDE.md            AI assistant context file
├── ARCHITECTURE.md      Detailed architecture document
└── README.md            Quick start guide
```

## `src/` — Source Code

### `src/core/`

The engine foundation.

- **`game.hpp / .cpp`** — Top-level `Game` class. Owns all subsystems (renderer,
  input, clock, scene manager, sprite sheets, audio, settings) and the EnTT
  registry. Contains the main loop.
- **`clock.hpp`** — Fixed timestep clock at 120 Hz. Uses an accumulator pattern
  with a spiral-of-death cap (max 4 steps per frame).
- **`input.hpp / .cpp`** — Keyboard, mouse and gamepad input. Provides
  normalised movement axes, latched button presses and menu directions
  ([Input](../architecture/input.md)).
- **`settings.hpp / .cpp`**, **`save_data.hpp / .cpp`** — User settings and
  save data, stored as JSON under SDL's pref path.
- **`fs.hpp / .cpp`** — Whole-file read and write on SDL I/O. The only place
  that touches the filesystem.
- **`paths.hpp / .cpp`** — Resolves asset paths next to the executable.
- **`string_id.hpp`** — `StringId` and the `StringInterner` behind sheet,
  clip and pattern names.
- **`json_keys.hpp`** — The "unknown key, is it a typo?" check shared by the
  data loaders.

### `src/ecs/`

Entity Component System layer.

- **`components.hpp`** — All component structs in one header, small POD types
  grouped by area (transform and rendering, collision, gameplay, weapons,
  classes, AI). See [ECS](../architecture/ecs.md).
- **`enemy_library.hpp / .cpp`** — Loads named enemy definitions from
  `assets/data/enemies.json`.
- **`player_class.hpp / .cpp`** — The class recipes (`apply_brawler`,
  `apply_knight`, `apply_sharpshooter`) that turn the base player into a
  class.

### `src/ecs/systems/`

Systems are free functions that operate on registry views, one system per
file. `GameScene::update` calls them in a fixed order
([Architecture Overview](../architecture/overview.md)).

| Area | Files |
| ---- | ----- |
| Player input and actions | `input_system`, `shooting_system`, `charged_shot_system`, `melee_system`, `dash_system`, `ground_slam_system`, `concussion_shot_system` |
| Enemies | `ai_system`, `emitter_system`, `wave_system` (also the stage loader and exits) |
| Movement and collision | `movement_system`, `tile_collision_system`, `collision_system` |
| Damage, pickups, cleanup | `damage_system`, `pickup_system`, `cleanup_system` |
| Animation and drawing | `animation_system`, `render_system`, `tilemap_render_system`, `hud_system` |
| Shared helpers | `hitbox_math.hpp`, `ability_hits.hpp`, `bullet_spawn`, `player_utils.hpp` |

### `src/rendering/`

SDL3 rendering abstraction.

- **`renderer.hpp / .cpp`** — Window management and virtual resolution scaling
  (480x270 render target scaled to window).
- **`sprite_sheet.hpp / .cpp`** — Texture atlas loading and frame-based drawing.
- **`sheet_ids.hpp`** — The sheet and clip names the code uses.
- **`animation_library.hpp / .cpp`** — Animation clips loaded from Aseprite's
  JSON export.
- **`tilemap.hpp / .cpp`**, **`tilemap_loader.cpp`** — Rooms loaded from the
  LDtk map: tiles, the collision grid and spawn points.
- **`bitmap_font.hpp / .cpp`** — The pixel font used by menus and the HUD.

### `src/audio/`

- **`audio_engine.hpp / .cpp`** — Sound effects on SDL3 audio streams.
  Systems request sounds through an `AudioQueue` in the registry; the scene
  forwards them once per tick.

### `src/scenes/`

Stack-based scene management.

- **`scene.hpp`**, **`scene_manager.cpp`** — Abstract `Scene` interface and
  `SceneManager`.
- **`title_scene`**, **`char_select_scene`** — Title screen and class choice.
- **`game_scene`** — Main gameplay scene, owns system execution order.
- **`pause_scene`**, **`options_scene`** — Overlays pushed on top of whatever
  is running.
- **`game_over_scene`**, **`victory_scene`** — The two ways a run ends.

### `src/patterns/`

Data-driven bullet patterns.

- **`pattern_library.hpp / .cpp`** — Loads `EmitterDef` and `PatternDef` from
  JSON files. Used by enemy emitters and by the weapons the player steals.

### `src/platform/`

- **`platform.hpp`** — Compile-time platform capabilities, such as whether
  the options menu shows window settings.
- **`steam.hpp / .cpp`** — Optional Steamworks wrapper; does nothing unless
  built with `RAVEN_ENABLE_STEAM`.

### `src/debug/`

Development-only tooling.

- **`debug_overlay.hpp / .cpp`** — Dear ImGui overlay with FPS graph, entity
  inspector, and player state panel. Compiled only when `RAVEN_ENABLE_IMGUI` is
  set; toggled with F1.

## `tests/`

Catch2 test suite, one `test_<area>.cpp` per system or area (`test_ai.cpp`,
`test_input.cpp`, `test_waves.cpp` and so on). Each test file is listed in
`tests/CMakeLists.txt` along with the `src/` files it needs.

`test_content.cpp` is different in kind: it loads the shipped stages,
patterns, enemies, config and LDtk map and checks them against each other, so
a typo in content fails a test.

## `assets/`

```
assets/
├── sprites/     PNG sprite sheets, plus Aseprite's JSON for animated ones
├── tilesets/    Tileset images used by the map
├── maps/        raven.ldtk, the LDtk project holding every room
├── audio/
│   ├── sfx/     .wav sound effects
│   └── music/   Empty for now; music playback is not built yet
├── fonts/       font.png, the bitmap font
└── data/
    ├── config.json    Read-only game data: sprite sheets and sounds to load
    ├── enemies.json   Named enemy definitions
    ├── patterns/      Bullet patterns, listed in manifest.json
    └── stages/        Stages (a room and its waves), listed in stage_manifest.json
```

## `art/`

Aseprite sources, grouped by kind (`characters/`, `enemies/`, `bosses/`,
`tiles/`, `ui/`, `vfx/`), with canvas templates in `templates/` and palettes
in `palettes/`. `just export-art` turns them into `assets/sprites/`
([Tools](../tools.md#exporting-art)).

## `tools/`

- `export_art.sh` — the Aseprite export behind `just export-art`.
- `gen_placeholder_sprites.py`, `gen_font.py`, `gen_sfx.py` — generate the
  placeholder sprites, the bitmap font and the placeholder sound effects.
- `gen_templates.lua` — Aseprite script that makes the canvas templates.
- `lint_files.sh` — picks the sources clang-tidy runs on for a set of
  changed files (CI and `just lint-changed`).

## `docs/`

```
docs/
├── book/            mdBook (this book)
│   └── src/
│       ├── guide/           Getting set up and finding your way around
│       ├── architecture/    How each part of the game works
│       ├── art/             Art spec and guides
│       ├── decisions/       Architecture Decision Records (ADRs)
│       └── devlog/          Date-prefixed development log entries
├── design/          Design matrices and context notes
├── Doxyfile         Doxygen configuration for the API reference
└── book.just, api.just      Recipes behind `just book` and `just api`
```

## `cmake/`

Build system helpers:

- `CPM.cmake` — vendored CMake Package Manager
- `Dependencies.cmake` — all dependency declarations
- `CompilerWarnings.cmake` — strict warning flags
- `Sanitizers.cmake` — ASan and UBSan toggle
- `ShippingFlags.cmake` — release build flags (LTO, stripping)
- `Steamworks.cmake` — locates the Steamworks SDK when Steam is enabled
