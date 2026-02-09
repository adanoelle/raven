# 6. No Third-Party Game Engine; Deferred Platform Abstraction

Date: 2026-02-09 Status: Accepted

## Context

Before investing further in Steam shipping and eventual Nintendo Switch porting,
we evaluated whether to adopt a third-party game engine (Unity, Godot, Unreal)
versus continuing with the current custom stack (C++20, EnTT, SDL2).

The codebase already constitutes a small purpose-built engine: 120 Hz
fixed-timestep game loop, EnTT ECS, SDL2 rendering at 480x270 virtual
resolution, sprite sheet management, stack-based scene manager, collision
systems, JSON-driven bullet patterns, and an optional ImGui debug overlay.
Approximately 35 source files, ~1,700 lines total.

SDL2 coupling is moderate. Nine source files include SDL2 headers, with ~145
lines of direct `SDL_*` calls. The ECS, collision, pattern, and scene-management
layers are already platform-independent. `InputState` is a platform-agnostic POD
struct; only the `Input` class that populates it is SDL-specific.

A third-party engine would provide an editor, asset pipeline, scripting layer,
and physics engine. None of these are needed: levels are authored in LDtk,
assets are static sprite sheets, game logic is C++, and physics is circle/AABB
checks.

For Switch, SDL2 is a proven path — Celeste, Stardew Valley, Hollow Knight, and
Shovel Knight all ship SDL2 on Switch. A NintendoSDK backend is an alternative
if the SDL2 Switch port is unavailable.

## Decision

**Do not adopt a third-party game engine.** Continue with the current
C++20/EnTT/SDL2 stack.

**Defer platform abstraction until Switch development begins.** Do not introduce
`IGraphics`, `IAudio`, or `IPlatform` interfaces speculatively. When Switch
porting starts, extract interfaces along these lines:

- `src/platform/` — abstract interfaces (`IGraphics`, `IAudio`, `IPlatform`)
- `src/platform_sdl2/` — current SDL2 code, moved behind the interfaces
- `src/platform_switch/` — NintendoSDK or SDL2-Switch implementation

The refactor scope is bounded: 9 files with SDL2 includes, ~145 lines of direct
SDL calls.

**Ship order:**

1. Ship on Steam with the current SDL2 stack (no abstraction needed)
2. Integrate Steamworks SDK (achievements, cloud saves, overlay)
3. Extract platform interfaces when Switch development begins
4. Implement Switch backend

## Consequences

**Positive:**

- No rewrite: the existing ~1,700 lines of working game code are preserved
- No new dependencies: avoids 100MB+ engine runtimes, editor tooling, and
  scripting layers the game does not need
- SDL2's Switch track record reduces porting risk
- Deferring abstraction avoids speculative interfaces that may not match the
  real Switch SDK constraints
- Steamworks integration is independent of the platform layer and can proceed
  immediately

**Negative:**

- No built-in editor, profiler, or asset pipeline from an engine (mitigated by
  LDtk, external profilers, and simple asset needs)
- The eventual platform abstraction refactor (~9 files) is deferred work that
  will gate Switch development
- `Input` class exposes SDL types (`SDL_Event`, `SDL_GameController*`) in its
  header, requiring cleanup during the abstraction pass
- Scene render methods make raw SDL draw calls that must be ported to the
  abstract interface later
