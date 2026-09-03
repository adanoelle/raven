<table width="100%">
<tr>
<td><h1>Raven</h1></td>
<td align="right"><sub>A new game</sub></td>
</tr>
</table>

A pixel art roguelike built with C++20, EnTT ECS, and SDL2. Inspired by Blazing
Beaks.

## Quick Start (NixOS)

```sh
# Enter dev environment
direnv allow   # or: nix develop

# Build and run
just run

# Run tests
just test
```

## Build Commands

| Command        | Description                 |
| -------------- | --------------------------- |
| `just build`   | Debug build                 |
| `just release` | Optimized release build     |
| `just run`     | Build + launch game         |
| `just test`    | Run Catch2 tests            |
| `just asan`    | Build with AddressSanitizer |
| `just fmt`     | Format all source files     |
| `just lint`    | Run clang-tidy              |
| `just clean`   | Remove build directories    |

### Offline / console builds

CPM fetches EnTT, nlohmann_json, spdlog, LDtkLoader, Dear ImGui, and Catch2
from GitHub at configure time. For an isolated build machine (a console
devkit, an air-gapped CI runner), populate a source cache once while online:

```bash
CPM_SOURCE_CACHE=/path/to/cpm-cache cmake -B build -G Ninja
```

then configure with the same `CPM_SOURCE_CACHE` on the isolated machine; every
package is served from the cache and nothing is downloaded. CI already does
this. `-DRAVEN_PLATFORM_CONSOLE=ON` hides desktop-only display options
(fullscreen, window scale, vsync) from the options menu; `just release` and the
Nix package build with `-DRAVEN_ENABLE_IMGUI=OFF`.

## Controls

| Action | Keyboard          | Gamepad            |
| ------ | ----------------- | ------------------ |
| Move   | Arrow keys / WASD | Left stick / D-pad |
| Shoot  | Z / Mouse         | A / Right stick    |
| Pause  | Escape            | Start              |

## Architecture

- **ECS**: EnTT registry with component-based design
- **Fixed timestep**: 120Hz tick rate
- **Virtual resolution**: 480x270 render target (16:9)
- **Top-down twin-stick**: Move and aim independently

See `ARCHITECTURE.md` for full design details.

## Project Structure

```
src/
├── core/       # Game loop, clock, input
├── ecs/        # Components + systems
├── rendering/  # SDL2 renderer, sprite sheets
├── scenes/     # Scene state machine
└── platform/   # Platform abstraction (Steam, Switch)
```

## License

TODO
