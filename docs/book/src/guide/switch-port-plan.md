# Switch Port Plan

Where the codebase stands after the 2026-09-02 readiness pass
([devlog](../devlog/2026-09-02-switch-port-readiness.md)), and the
ordered steps from here to a build running on hardware. Each step
lists what "done" looks like so progress is checkable, not vibes.

---

## Starting Point

The engine was designed with a console port in mind
([ADR-0005](../decisions/0005-no-std-filesystem.md)), and the readiness
pass closed the gaps the review found:

- All file access goes through `raven::fs` (`src/core/fs.cpp`), backed
  by SDL and written atomically. Nothing else in `src/` touches disk.
- App suspend pauses gameplay and the audio device; resume restarts it.
- No `std::random_device`, no threads, no `std::filesystem`, no
  platform `#ifdef`s outside the optional Steam and ImGui subsystems.
- Release builds drop the ImGui overlay. `RAVEN_PLATFORM_CONSOLE` hides
  the window rows of the options menu.
- Asset paths accept `romfs:/`-style mounts as absolute.

What is deliberately *not* done yet: any Nintendo SDK code, a
cross-compilation toolchain, and the knight's final animation frames.

---

## Step 1 — Land the readiness branch

Merge `docs/action-animation-guide` through a pull request. Run the
deep review (`/code-review ultra <PR#>`) on the PR rather than on the
pre-fix branch, so findings land on the code that will actually ship.

**Done when:** the PR is merged to `main` and CI (Linux Debug/Release,
Windows, clang-tidy, clang-format) is green.

## Step 2 — Draw the knight's idle and walk poses

`assets/sprites/knight.png` is now reproducible from
`art/characters/knight/knight.aseprite` (tags `idle` 1–4, `walk` 5–10)
but the frames are a mechanical blocking pass: the sketch pose with a
1 px body bob and the feet locked to FT. The leg split, counter-swing,
and hair lag from the
[blocking page](../art/walk-cycle-blocking.md) still need drawing.

Re-export with the command in the
[Aseprite guide](../art/art-aseprite-guide.md#73-exporting-the-sprite-sheet);
no code changes are involved.

**Done when:** the walk reads at 1x in a real room and the knight page
records the final palette. This step runs in parallel with 3–6.

## Step 3 — Toolchain and offline dependencies

1. Register for Nintendo developer access and obtain the NX SDK and
   its SDL3 port. (SDL3 itself has no public Switch backend; the
   authorised port ships through the developer portal.)
2. Add `cmake/toolchains/switch.cmake` (compiler, sysroot, `find_package`
   hints for SDL3/SDL3_image) and a `just switch` recipe that configures
   with `-DRAVEN_PLATFORM_CONSOLE=ON -DRAVEN_ENABLE_IMGUI=OFF
   -DRAVEN_ENABLE_TESTS=OFF -DRAVEN_ENABLE_STEAM=OFF`.
3. Populate `CPM_SOURCE_CACHE` once on a networked machine and point
   the console configure at it
   ([Building and Running](building.md#offline--console-builds)).
   EnTT, nlohmann_json, spdlog, and LDtkLoader are header-only or
   plain C++ and should cross-compile unchanged; spdlog's default
   stdout sink is the one to watch — swap it for an `SDL_Log` sink if
   console output is not captured.

**Done when:** `raven` links for the target with every dependency
resolved from the cache, no network, on a clean checkout.

## Step 4 — Port `fs.cpp`

The only source file that needs platform code. Two functions:

| Function     | Desktop today                     | Switch                                      |
| ------------ | --------------------------------- | ------------------------------------------- |
| `read_text`  | `SDL_LoadFile`                    | ROM filesystem mount (`romfs:/`) via the SDK's file API, or SDL if the port maps it |
| `write_text` | `SDL_SaveFile` + `SDL_RenamePath` | Save-data mount: write, then commit the journal |

`paths::pref_dir()` is the other seam: on Switch it should return the
save-data mount prefix rather than `SDL_GetPrefPath`. Keep the desktop
implementation as the default and select the console one with
`RAVEN_PLATFORM_CONSOLE`, so the file stays a single translation unit
with two `#ifdef` branches rather than a second file.

**Done when:** settings round-trip through the save-data mount across a
power cycle, and a deliberately interrupted save leaves the previous
file intact.

## Step 5 — Gamepad-only pass

Unplug the keyboard and mouse on desktop and play from title to game
over using only a controller. Things to confirm:

- Every menu (title, character select, options, pause, game over,
  victory) can be navigated and exited.
- Aim falls back to the right stick when no mouse has moved
  (`InputState::mouse_active` stays false).
- Nothing in gameplay is reachable only by a key (F1 is debug-only and
  compiled out of release builds).
- Controller hot-plug mid-run keeps working: `Input` reopens on
  `SDL_EVENT_GAMEPAD_ADDED`.

Then on hardware: handheld ↔ docked switching fires
`SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED`, which recreates the 480x270
render target — verify there is no visible hitch and the letterbox
scale is right at both 720p and 1080p.

**Done when:** a full run completes with no keyboard present and the
docked/handheld transition is clean.

## Step 6 — Profile on target

The 120 Hz fixed step and `MAX_STEPS_PER_FRAME = 4` were chosen for
desktop. On the Switch CPU the budget per tick is ~8.3 ms, and a
bullet-hell wave is the worst case. Measure:

- Tick time under the densest stage wave (ImGui overlay is off in
  release; use the SDK profiler or a temporary `spdlog` timing line).
- Draw call count per frame: one `SDL_RenderTexture` per sprite plus
  the tilemap. At 480x270 the GPU side should be far under budget.
- Audio: `MAX_VOICES` streams mixing in software.

If the tick overruns, the levers in order are: bullet counts per wave
(data), the collision broad phase (`collision_system.cpp`), and only
then the tick rate ([ADR-0002](../decisions/0002-fixed-timestep-120hz.md)).

**Done when:** the densest wave holds 60 fps docked and handheld with
the step cap never engaging.

---

## After These Steps

Not blocking a first hardware build, but on the list for a shippable
one: save-data versioning (`save.json` has one field today), a
platform-specific achievements/leaderboard shim beside the Steam
wrapper, and localisation of the bitmap-font strings. Each deserves its
own ADR when it comes up.
