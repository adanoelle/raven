# 1. C++20 with EnTT and SDL2

Date: 2026-02-08
Status: Accepted

## Context

We needed to choose a language, ECS framework, and windowing/rendering library for a pixel art roguelike targeting Steam (primary) and Nintendo Switch (future).

Key requirements:
- High performance at a 120 Hz tick rate with many active entities
- Cross-platform: Linux, Windows, macOS, and eventually Switch
- Good tooling (debuggers, profilers, static analysers)
- Mature ecosystem for 2D game development

## Decision

- **C++20** for the language. It provides structured bindings, concepts, ranges, and designated initialisers that improve code clarity. EnTT requires modern C++ features. The performance characteristics are well-suited for a high-tick-rate game loop.

- **EnTT** for the ECS. It is header-only, cache-friendly (archetype-based storage), and battle-tested in indie games. Alternatives considered: flecs (C API felt cumbersome from C++), custom ECS (unnecessary complexity for a small team).

- **SDL2** for windowing, input, and audio. Nintendo has official SDL2 support on Switch, making future porting straightforward. SDL2_Renderer provides hardware-accelerated 2D rendering — ideal for pixel art. Alternatives considered: SFML (no Switch support), raylib (less control over the rendering pipeline).

## Consequences

**Positive:**
- Deterministic, low-latency game loop suitable for action gameplay
- EnTT's cache-friendly iteration keeps systems fast even with many entities
- SDL2's broad platform support simplifies future console porting
- Extensive C++ tooling (clangd, sanitisers, Valgrind) catches bugs early

**Negative:**
- C++ compile times are longer than C or Rust
- Manual memory management requires discipline (mitigated by RAII and sanitisers)
- EnTT's template-heavy API produces verbose compiler errors
