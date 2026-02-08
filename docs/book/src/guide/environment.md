# Development Environment

Raven uses a [Nix flake](https://nixos.wiki/wiki/Flakes) to define a fully reproducible development environment. Every tool, library, and compiler version is pinned — no "works on my machine" problems.

## Prerequisites

1. Install [Nix](https://nixos.org/download/) with flakes enabled.
2. Install [direnv](https://direnv.net/) (optional but recommended).

## Entering the Dev Shell

```bash
# Option A: manual
nix develop

# Option B: automatic with direnv
echo "use flake" > .envrc
direnv allow
```

## What's in the Shell

### Build Toolchain
- **GCC 14** — C++20 compiler
- **CMake** + **Ninja** — build system
- **pkg-config** — finds system libraries

### C++ Tooling
- **clang-tools** — clangd (LSP), clang-format, clang-tidy
- **cppcheck** — additional static analysis
- **include-what-you-use** — header hygiene

### Debugging & Profiling
- **GDB** — debugger
- **Valgrind** — memory error detection
- **RenderDoc** — GPU frame capture and debugging
- **Tracy** — real-time frame profiler (great for games)

### Asset Tools
- **LibreSprite** — open-source pixel art editor (Aseprite fork)
- **LDtk** — level editor with auto-tiling
- **Audacity** — audio editing
- **FFmpeg** — audio/video conversion

### Documentation
- **mdbook** — builds this book from Markdown sources

### Build Dependencies (linked at compile time)
- SDL2, SDL2_image, SDL2_mixer, SDL2_ttf
- OpenGL libraries (libGL, libGLU)
- X11 libraries (input, cursor, display)
- Audio backends (PulseAudio, ALSA)

## CPM Dependencies (fetched by CMake)

These are pulled automatically at configure time by [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake):

- **EnTT 3.13.2** — Entity Component System
- **nlohmann/json 3.11.3** — JSON parsing
- **spdlog 1.14.1** — logging
- **Catch2 3.7.0** — testing framework
- **Dear ImGui 1.91.0** — debug UI (optional, SDL2 backend)
