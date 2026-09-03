# Switch Port Readiness Pass

Date: 2026-09-02 Tags: review, portability, saves, input, art-pipeline

## The Question

Before any console toolchain work starts, is the source actually ready
for it? A full code review of the tree plus a portability sweep answered
that. The short version: the architecture is right (no threads, no
`std::filesystem`, no platform `#ifdef`s outside the two optional
subsystems, all timing through SDL, exceptions confined to load-time
JSON), but the rules we wrote for ourselves were not all being followed,
and a handful of desktop assumptions had leaked into core code.

## What the Review Found

Ordered by how much a port would have to chase; all fixed in this change
unless noted:

1. **The file I/O rule was honoured in letter, not spirit.**
   [ADR-0005](../decisions/0005-no-std-filesystem.md) says all file
   access goes through SDL, but settings, save data, `config.json`, stage
   and pattern manifests, and the LDtk loader all used `std::ifstream` /
   `std::ofstream` directly. That is six call sites and one library a
   port would have to rewrite. There is now one seam, `raven::fs`
   (`src/core/fs.hpp`): `read_text` and `write_text` on `SDL_LoadFile` /
   `SDL_SaveFile`. Every load goes through it, and LDtk projects load
   via the library's `FileLoader` hook so they do too. ADR-0005 is
   amended to make direct `<fstream>` use in `src/` off-limits.
2. **Saves were not atomic.** Both save functions truncated the file in
   place; a crash mid-write left a corrupt file that loaded as a fresh
   profile. `fs::write_text` now writes a temporary sibling and
   `SDL_RenamePath`s it over the target.
3. **The save path fell back to the working directory.** When
   `SDL_GetPrefPath` failed, the settings and save paths became bare
   filenames and the game wrote them wherever it was launched from,
   while logging that settings would not persist. The paths now stay
   empty on failure and the save functions take their existing early
   return.
4. **No suspend handling.** Only `SDL_EVENT_QUIT` was handled. The home
   button and sleep deliver `SDL_EVENT_WILL_ENTER_BACKGROUND`; gameplay
   now auto-pauses on it and the audio device pauses, with
   `SDL_EVENT_DID_ENTER_FOREGROUND` resuming audio. The fixed-step clamp
   already protected against the resume time jump.
5. **RNG seeded from `std::random_device`.** Console standard libraries
   sometimes implement it as a constant or throw. The seed now comes
   from `SDL_GetPerformanceCounter` mixed with `SDL_GetTicksNS`.
6. **Input discovered devices before `SDL_Init`.** `Input` is a member
   of `Game`, so its constructor ran gamepad and keyboard queries before
   SDL was up. It worked because SDL3 replays gamepad-added events, but
   it was order-dependent. Device discovery moved to an explicit
   `init()` called after `SDL_Init`.
7. **A per-frame mouse debug log** in the input path, at debug level,
   which `main.cpp` enables. Gone; console log sinks are slow enough for
   that to show up in frame time.
8. **Release builds shipped the ImGui overlay.** `RAVEN_ENABLE_IMGUI`
   now defaults `OFF` for `Release`; `just release` and the Nix package
   pass it explicitly.
9. **The options menu was desktop-shaped.** Fullscreen, window scale,
   and vsync mean nothing on a console. A `RAVEN_PLATFORM_CONSOLE`
   CMake option feeds a capability constant in
   `src/platform/platform.hpp`, and the options scene hides those rows
   when it is set.
10. **Absolute-path detection missed URI schemes.** A colon at index one
    was treated as a Windows drive letter; `romfs:/` or `sdmc:/` would
    have been prefixed with the base directory. `paths::is_absolute`
    now recognises scheme-style prefixes.
11. **Sprite frames were not bounds-checked.** `SpriteSheet::draw` now
    rejects a frame outside the sheet instead of sampling past it.

Still open, deliberately: dependencies arrive via CPM at configure
time. The README and building guide now document `CPM_SOURCE_CACHE` for
isolated machines (CI already uses it); vendoring is a later call.

## The Art Pipeline Half

The same review looked at the knight sprite work on this branch and
found the pipeline drifting from its own rules:

- **One sheet id for every class.** The global `player` sheet had been
  re-pointed at `knight.png`, so Brawler and Sharpshooter wore the
  knight art too. Per [ADR-0011](../decisions/0011-class-as-component-recipe.md),
  the knight now has its own sheet id (`knight`) and `apply_knight`
  sets it; `player.png` stays as the placeholder for classes without
  final art. Every doc that said "the knight exports to `player.png`"
  now says `knight.png`.
- **The working file was a stale template copy.** `knight.aseprite` was
  byte-identical to `medium_32x32.aseprite` with an old single frame and
  no tags, while the shipped PNG actually derived from the sketch.
  It has been rebuilt from the sketch: art centered in the frame (it
  was 2 px left, so it shifted on every facing flip), 10 frames tagged
  `idle` (4 @ 250 ms) and `walk` (6 @ 100 ms), and `knight.png`
  re-exported from it with the documented command. The frames are a
  mechanical blocking pass — the pose plus the body bob — so the sheet
  animates and is reproducible, but the real poses are still to draw.
- **The walk-cycle page contradicted itself.** It named two different
  frames as the lowest point and gave two totals for the bob. It now has
  one offset table (Contact 0, Down +1, Passing −1) that every note
  refers to, explains that the body block intentionally overlaps the
  legs, and points at the Aseprite guide and the spec as the owners of
  brush sizes, guide rows, and timing instead of restating them.
- **Flake cleanup.** The commented-out overlay lines and the now-unused
  `allowUnfree` are deleted rather than left as fossils; git history
  has them.

## What This Buys

The port surface is now one file (`fs.cpp`), one CMake option, and one
capability header. Everything else in the tree is already platform
neutral. That is the state we wanted before anyone opens a console SDK.
