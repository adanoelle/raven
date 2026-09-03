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

| Option               | Default | Description                           |
| -------------------- | ------- | ------------------------------------- |
| `RAVEN_ENABLE_TESTS` | `ON`    | Build the Catch2 test suite.          |
| `RAVEN_ENABLE_ASAN`  | `OFF`   | Enable AddressSanitizer + UBSan.      |
| `RAVEN_ENABLE_IMGUI` | `ON`\* | Compile the Dear ImGui debug overlay. |
| `RAVEN_PLATFORM_CONSOLE` | `OFF` | Console target: hides desktop-only display options (fullscreen, window scale, vsync). |
| `RAVEN_BUNDLED_DEPS` | `OFF`   | Build SDL3/SDL3_image from source via CPM even if system packages exist. |

\* Defaults to `OFF` for `Release` builds; `just release` and the Nix package
pass it explicitly.

## Offline / Console Builds

CPM downloads EnTT, nlohmann_json, spdlog, LDtkLoader, Dear ImGui, and Catch2
at configure time. For an isolated machine, populate a source cache once while
online and reuse it:

```bash
CPM_SOURCE_CACHE=/path/to/cpm-cache cmake -B build -G Ninja
```

Configure with the same `CPM_SOURCE_CACHE` on the isolated machine and nothing
is fetched. CI does exactly this (see `.github/workflows/ci.yml`).

## Build Outputs

- `build/bin/raven` — the game executable (Debug)
- `build/bin/assets` — symlink to the `assets/` directory
- `build-release/bin/raven` — Release build
- `build-docs/book/` — generated mdBook HTML
- `build-docs/api/` — generated Doxygen HTML
- `build-docs/doxygen-xml/` — generated Doxygen XML
