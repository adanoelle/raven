# Raven

Pixel art roguelike. C++20, EnTT ECS, SDL2. Inspired by Blazing Beaks.

## Build (requires nix develop / direnv)

just build    # Debug, Ninja, creates compile_commands.json
just test     # Catch2 via ctest
just run      # Build + launch (build/bin/raven)
just fmt      # clang-format all src/ and tests/
just lint     # clang-tidy

## Layout

src/core/        Game loop (120Hz fixed timestep), input
src/ecs/         components.hpp + systems/ (one file per system)
src/rendering/   SDL_Renderer wrapper, 480x270 virtual res, sprite sheets
src/scenes/      Stack-based scene manager
src/patterns/    Bullet pattern library (JSON emitter defs, used by tests)
src/debug/       ImGui overlay (RAVEN_ENABLE_IMGUI)
tests/           Catch2 (test_collision, test_patterns, test_ecs)
docs/            mdBook, devlog, ADRs

## Conventions

- Namespace: raven (nested: raven::systems)
- Naming: PascalCase types, snake_case functions/variables, trailing _ for members
- Headers: .hpp with #pragma once. Sources: .cpp
- Doxygen comments (///) on all public APIs: @brief, @param, @return
- Include order: project, library (<SDL2/..>, <entt/..>), std
- [[nodiscard]] on const getters
- No std::filesystem (portability). No exceptions in hot paths.
- New .cpp files must be added to CMakeLists.txt add_executable()

## Architecture

- All game state in EnTT registry. Systems are free functions on views.
- Components are small POD structs in components.hpp.
- Scenes own system execution order. Game class owns subsystems + registry.
- Fixed timestep: accumulate delta, step at 1/120s, interpolate for render.

## Commits

- Conventional commits (feat:, fix:, refactor:, test:, docs:, chore:)
- Do not mention Claude or AI in commit messages
- Run clang-format before committing

## Docs

- docs/book/          mdBook source (just book / just book-serve)
- docs/decisions/     ADRs (NNNN-title.md, MADR format)
- docs/devlog/        Date-prefixed entries (YYYY-MM-DD-title.md)
