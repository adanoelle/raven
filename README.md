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
