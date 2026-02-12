# SDL3 Migration

Date: 2026-02-12 Tags: sdl3, rendering, migration, audio

## What Changed

Migrated the entire engine from SDL2 to SDL3 + SDL3_image. Dropped SDL_mixer (no
stable SDL3 port yet; zero playback code existed). Every source file that
touched SDL was updated. The build system, Nix flake, renderer, input handler,
sprite sheet loader, tilemap renderer, HUD, debug overlay, scene code, and test
harness all moved to SDL3 APIs.

**Build system** — `CMakeLists.txt` now links `SDL3::SDL3` and
`SDL3_image::SDL3_image` instead of SDL2 targets. `Dependencies.cmake` uses
`find_package(SDL3)` and pkg-config for `sdl3-image`. The SDL_mixer dependency
was removed entirely. The Nix flake was updated to provide SDL3 and SDL3_image
packages.

**Renderer** — `Renderer::init()` now calls `SDL_CreateWindow` with the SDL3
three-argument form (title, width, height, flags). VSync is configured via
`SDL_SetRenderVSync` instead of a renderer creation flag. The virtual resolution
render target uses `SDL_SetTextureScaleMode(target, SDL_SCALEMODE_PIXELART)` and
`SDL_SetRenderLogicalPresentation` with `SDL_LOGICAL_PRESENTATION_LETTERBOX`.
Frame presentation uses `SDL_RenderTexture` (replacing `SDL_RenderCopy`).

**Sprite sheets** — `SpriteSheet::load()` sets
`SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_PIXELART)` on every loaded
texture. `SpriteSheet::draw()` uses `SDL_FRect` for both source and destination
rectangles (float precision) and `SDL_RenderTextureRotated` with the
`SDL_FlipMode` enum.

**Input** — `SDL_INIT_GAMECONTROLLER` became `SDL_INIT_GAMEPAD`. The gamepad API
now uses `SDL_GetGamepads` (returns an array + count) to enumerate devices on
startup, and `SDL_OpenGamepad` / `SDL_CloseGamepad` for device lifecycle.
Hotplug events moved to `SDL_EVENT_GAMEPAD_ADDED` / `SDL_EVENT_GAMEPAD_REMOVED`
with `SDL_GetGamepadID` for device matching. Button and axis queries use the
same enum names (`SDL_GAMEPAD_BUTTON_SOUTH`, `SDL_GAMEPAD_AXIS_LEFTX`, etc.).
`SDL_GetKeyboardState` still works the same way. Mouse state comes from
`SDL_GetMouseState` with float output coordinates.

**Events** — All event type constants moved to `SDL_EVENT_*` naming: `SDL_QUIT`
→ `SDL_EVENT_QUIT`, `SDL_KEYDOWN` → `SDL_EVENT_KEY_DOWN`, `SDL_WINDOWEVENT` →
individual `SDL_EVENT_WINDOW_*` events. Window resize is now
`SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED` (no sub-event union).

**ImGui** — The debug overlay backend switched from `imgui_impl_sdl2` +
`imgui_impl_sdlrenderer2` to `imgui_impl_sdl3` + `imgui_impl_sdlrenderer3`. Both
are available upstream in imgui 1.91.x. No changes to the overlay logic.

**HUD and scenes** — Mechanical SDL_Rect → SDL_FRect conversions throughout.
`SDL_RenderFillRect`, `SDL_SetRenderDrawColor`, and `SDL_RenderClear` keep the
same signatures. Scene code updated for new event names.

**Audio removal** — `SDL_INIT_AUDIO` removed from the `SDL_Init` call.
`Mix_OpenAudio` / `Mix_CloseAudio` calls deleted. `SDL2_mixer` link target
removed from CMakeLists.txt. The `config.json` audio volume fields remain (they
are forward-compatible with any future audio backend).

## The Interesting Problems

**Render target recreation** — SDL3 fires `SDL_EVENT_RENDER_TARGETS_RESET` and
`SDL_EVENT_RENDER_DEVICE_RESET` when the GPU context is lost (e.g. display mode
change, window moved between monitors). The renderer now handles these events by
destroying and recreating the virtual resolution render target, re-applying
`SDL_SCALEMODE_PIXELART`.

**Mouse coordinate mapping** — `SDL_RenderWindowToLogical` (the SDL3 equivalent
of the old coordinate transform) gave incorrect results when combined with a
render-target texture and `SDL_SetRenderLogicalPresentation`. The input handler
now computes the window-to-virtual-resolution mapping manually: it reads the
window size, calculates the letterbox scale and offset, and transforms mouse
coordinates directly. This is more reliable and avoids coupling to SDL's
internal logical-presentation state.

**SDL_Init return type** — SDL3 changed `SDL_Init` to return `bool` (true on
success) instead of `int` (0 on success). A subtle inversion that would compile
silently with the old check pattern.

## New Capabilities

The migration was primarily about future-proofing, but several improvements are
immediately useful:

**Pixel-art scale mode** — `SDL_SCALEMODE_PIXELART` is applied per-texture
rather than as a global hint. Every sprite sheet texture and the virtual
resolution render target get this mode on creation. The scaling is purpose-built
for pixel art — it ensures clean integer-multiple upscaling without blurring.

**Float-precision rendering** — All draw calls now use `SDL_FRect` (float
rectangles). The render system's interpolation between previous and current tick
positions produces sub-pixel coordinates. With SDL2's integer `SDL_Rect`, these
were truncated, causing occasional 1px jitter on slow-moving entities. Float
rects eliminate this.

**Logical presentation** — `SDL_SetRenderLogicalPresentation` with
`SDL_LOGICAL_PRESENTATION_LETTERBOX` provides deterministic scaling behaviour.
The engine renders at 480x270 and SDL handles the letterboxing on any window
size.

**GPU shader path** — SDL3's `SDL_GPU` API is available (though not yet used).
This opens the door to fragment shaders for bloom, glow, CRT scanline filters,
and screen-space effects. The concussion shot VFX
([ADR-0012](../decisions/0012-concussion-shot-vfx.md)) currently uses additive
blending; real bloom could be added via shaders in the future.

**Audio stream API** — SDL3's `SDL_AudioStream` provides a push-model audio API
with built-in resampling. When audio is re-added, this is a viable option for
simple SFX playback without depending on an external mixer library.

## Test Results

92/92 tests pass. The test harness was updated to use SDL3 headers and function
signatures. No test logic changed — all tests exercise game systems and
components, not SDL API calls directly.

## Next

- Re-add audio playback (evaluate sdl3-mixer stability, or use `SDL_AudioStream`
  directly for simple SFX)
- Explore `SDL_GPU` for a post-processing shader pass (bloom on additive-blend
  VFX, optional CRT filter)
- Begin sprite sheet art integration now that the pixel-art scaling pipeline is
  confirmed working
