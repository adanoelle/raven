# Building and Running

All build commands use [just](https://github.com/casey/just), a modern command
runner. Run `just` with no arguments to see available recipes.

## Quick Start

```bash
just build    # Configure + build (Debug)
just run      # Build + launch the game
just test     # Build + run Catch2 tests
```

## Available Commands

| Command           | Description                                                         |
| ----------------- | ------------------------------------------------------------------- |
| `just build`      | Debug build with Ninja. Creates `compile_commands.json` for clangd. |
| `just release`    | Optimised Release build (tests disabled).                           |
| `just run`        | Build + launch `build/bin/raven`.                                   |
| `just test`       | Build + run tests via ctest.                                        |
| `just asan`       | Debug build with AddressSanitizer enabled.                          |
| `just fmt`        | Format all `.hpp` and `.cpp` files with clang-format.               |
| `just lint`       | Run clang-tidy static analysis.                                     |
| `just book build` | Build the mdBook documentation.                                     |
| `just book serve` | Serve the book locally with live reload.                            |
| `just book clean` | Remove generated book output.                                       |
| `just api build`  | Generate Doxygen API reference (HTML + XML).                        |
| `just api serve`  | Serve API docs locally at `localhost:8080`.                         |
| `just api clean`  | Remove generated API output.                                        |
| `just clean`      | Remove all build directories.                                       |
| `just watch`      | Auto-rebuild on source changes (requires `entr`).                   |

## CMake Options

These are set via `-D` flags during configuration:

| Option                | Default | Description                                                    |
| --------------------- | ------- | -------------------------------------------------------------- |
| `RAVEN_ENABLE_TESTS`  | `ON`    | Build the Catch2 test suite.                                    |
| `RAVEN_ENABLE_ASAN`   | `OFF`   | Enable AddressSanitizer + UBSan.                                |
| `RAVEN_ENABLE_IMGUI`  | `ON`    | Compile the Dear ImGui debug overlay.                           |
| `RAVEN_BUNDLED_DEPS`  | `OFF`   | Build SDL3/SDL3_image from source even if system packages exist. |
| `RAVEN_ENABLE_STEAM`  | `OFF`   | Link the Steamworks SDK from `vendor/steamworks/sdk` (ADR-0021). |

## Building on Windows

No SDL3 distro packages exist on Windows; the build falls back to compiling
SDL from source automatically (see
[ADR-0020](../decisions/0020-bundled-dependency-fallback.md)). With Visual
Studio installed:

```powershell
cmake -B build
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

Assets and runtime DLLs are copied next to the executable post-build, so
`build\bin\Debug\raven.exe` runs in place.

## Continuous Integration

`.github/workflows/ci.yml` runs on every push and pull request:

- **format** — `clang-format --dry-run --Werror` over `src/` and `tests/`
- **build-test (Debug/Release)** — Linux, cached source-built SDL prefix,
  full ctest under dummy video/audio drivers
- **build-test-windows** — MSVC with `RAVEN_BUNDLED_DEPS=ON`, full ctest

## Build Outputs

- `build/bin/raven` — the game executable (Debug)
- `build/bin/assets` — symlink to `assets/` on Linux/macOS; copied on Windows
- `build-release/bin/raven` — Release build
- `build-docs/book/` — generated mdBook HTML
- `build-docs/api/` — generated Doxygen HTML
- `build-docs/doxygen-xml/` — generated Doxygen XML

## Runtime Files

The game writes per-user files to the SDL pref path
(`~/.local/share/adanoelle/raven/` on Linux, `%APPDATA%` on Windows):

- `settings.json` — user preferences (window, vsync, volumes), created on
  first run and editable by hand
- `save.json` — player progress (best score)
