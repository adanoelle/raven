# 13. Migrate from SDL2 to SDL3

Date: 2026-02-12 Status: Accepted

## Context

SDL3 reached its first stable release (3.2.0 in February 2025, now at 3.4.x)
while Raven's art and audio integration had not yet begun. The original tech
stack decision ([ADR-0001](0001-cpp20-entt-sdl2.md)) chose SDL2 for its broad
platform support and hardware-accelerated 2D renderer. That rationale still
holds — SDL3 is its direct successor — but SDL3 adds several features that
directly benefit a pixel-art roguelike:

**Rendering improvements:**

- `SDL_SCALEMODE_PIXELART` — a per-texture scale mode purpose-built for pixel
  art. Replaces the global `SDL_HINT_RENDER_SCALE_QUALITY "0"` hint, giving
  explicit control over which textures use pixel-art scaling.
- `SDL_FRect` — float-precision rectangles for source and destination rects. The
  render system can now position sprites at sub-pixel coordinates, producing
  smoother interpolated movement between fixed-timestep ticks.
- `SDL_SetRenderLogicalPresentation` — replaces `SDL_RenderSetLogicalSize` with
  an explicit presentation policy (letterbox, stretch, overscan). Letterbox mode
  matches Raven's virtual resolution design.
- `SDL_GPU` API — exposes a cross-platform GPU abstraction with shader support.
  Fragment shaders enable bloom, glow, CRT filters, and screen-space effects
  without leaving the SDL ecosystem. Not needed today, but opens a path for
  post-processing VFX like the concussion shot bloom described in
  [ADR-0012](0012-concussion-shot-vfx.md).

**API modernisation:**

- `SDL_INIT_GAMEPAD` replaces `SDL_INIT_GAMECONTROLLER`. The gamepad API uses
  `SDL_OpenGamepad` / `SDL_GetGamepads` with clearer ownership semantics.
- Event types moved to `SDL_EVENT_*` naming (e.g. `SDL_EVENT_QUIT`,
  `SDL_EVENT_KEY_DOWN`).
- `SDL_Init` returns `bool` instead of `int`, simplifying error checks.
- `SDL_RenderTexture` replaces `SDL_RenderCopy`; `SDL_RenderTextureRotated`
  replaces `SDL_RenderCopyEx`.
- ImGui has native SDL3 + SDL_Renderer3 backends, so the debug overlay continues
  to work.

**Audio trade-off:**

SDL_mixer has not yet been ported to a stable SDL3 release (sdl3-mixer exists
but is not packaged in nixpkgs). The audio decision in
[ADR-0010](0010-sdl2-mixer-audio.md) chose SDL_mixer for playback, but no audio
loading or playback code was ever written — only `Mix_OpenAudio` and
`Mix_CloseAudio` calls existed. Dropping SDL_mixer has zero functional impact.
SDL3's `SDL_AudioStream` API (push-model with built-in resampling) provides a
viable path for simple SFX playback without a mixer dependency.

**Alternatives considered:**

1. **Stay on SDL2** — No migration effort, but forfeits `SCALEMODE_PIXELART`,
   float rects, and the GPU shader path. SDL2 is in maintenance mode; new
   features land only in SDL3.
2. **Wait for sdl3-mixer** — Delays the migration to an unknown date. Since no
   audio code exists, there is nothing to preserve.
3. **Migrate now, drop SDL_mixer temporarily** — Captures all rendering benefits
   immediately. Audio can be re-added via sdl3-mixer, raw `SDL_AudioStream`, or
   FMOD/Wwise when needed.

## Decision

**Migrate to SDL3 + SDL3_image. Drop SDL_mixer temporarily.** Re-evaluate audio
middleware when playback is needed (sdl3-mixer, `SDL_AudioStream`, or
FMOD/Wwise).

This supersedes the SDL2 portion of [ADR-0001](0001-cpp20-entt-sdl2.md) (C++20
and EnTT choices remain unchanged) and supersedes
[ADR-0010](0010-sdl2-mixer-audio.md) (SDL_mixer for audio).

## Consequences

**Positive:**

- Per-texture `SDL_SCALEMODE_PIXELART` gives precise control over pixel-art
  scaling, applied automatically when sprite sheets are loaded
- `SDL_FRect` float-precision rendering enables smoother sub-pixel interpolation
  between fixed-timestep ticks
- `SDL_SetRenderLogicalPresentation` with explicit letterbox policy replaces the
  less predictable `SDL_RenderSetLogicalSize`
- `SDL_GPU` shader API opens a future path for bloom, glow, and CRT filter
  effects — directly relevant to VFX like the concussion shot
  ([ADR-0012](0012-concussion-shot-vfx.md))
- Modernised gamepad API with clearer device enumeration and ownership
- ImGui backends (`imgui_impl_sdl3`, `imgui_impl_sdlrenderer3`) are available
  upstream, so the debug overlay required only a backend swap
- SDL3 is actively developed; SDL2 is in maintenance mode

**Negative:**

- Audio pipeline is temporarily broken — `SDL_INIT_AUDIO` removed, no playback
  capability until a mixer solution is re-added (acceptable: zero playback code
  existed)
- All SDL2 API calls required mechanical translation (event names, function
  signatures, type changes). This was a one-time cost with no ongoing burden.
- SDL3 is newer; some ecosystem tools and tutorials still reference SDL2.
  However, the API is well-documented and the migration guide is thorough.
- Nintendo Switch SDL3 support status is unconfirmed (SDL2 had official Nintendo
  support). This is a future concern — Switch porting is not imminent.
