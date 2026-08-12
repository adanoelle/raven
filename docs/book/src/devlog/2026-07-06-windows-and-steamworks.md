# Windows Builds and Steamworks Scaffolding

Date: 2026-07-06 Tags: windows, msvc, ci, steamworks, build

## What Changed

The two remaining platform items from the Steam-readiness review: the build
now works on Windows/MSVC, and the Steamworks integration point exists as an
opt-in flag.

## Windows Build

[ADR-0020](../decisions/0020-bundled-dependency-fallback.md): dependency
discovery goes system-packages-first with a CPM source-build fallback. On
Linux (Nix shell, cached CI prefix) nothing changes; on Windows — where no
SDL3 packages or pkg-config exist — SDL 3.4.12 and SDL3_image 3.2.4 build
from source automatically, pinned to the same versions as everywhere else.
`-DRAVEN_BUNDLED_DEPS=ON` forces the source path for testing it on Linux.

Windows-specific fixes that came with it:

- Assets **copy** next to the binary instead of symlinking (symlinks need
  developer mode on Windows); `$<TARGET_RUNTIME_DLLS>` post-build steps put
  SDL3.dll beside both the game and the test runner.
- MSVC gets `/W4 /permissive- /Zc:__cplusplus` — and crucially `/utf-8`,
  because the sources contain UTF-8 punctuation that MSVC misparses under a
  system codepage.
- CI grew a `windows-latest` job: MSVC, bundled deps, full test suite with
  dummy video/audio drivers.

## Steamworks

[ADR-0021](../decisions/0021-optional-steamworks.md): the SDK is
NDA-distributed and cannot live in the repo, so integration is an opt-in
flag with a no-op fallback. `-DRAVEN_ENABLE_STEAM=ON` +
`vendor/steamworks/sdk` links the redistributable and defines
`RAVEN_ENABLE_STEAM`; without it, the `Steam` wrapper
(`src/platform/steam.hpp`) compiles to no-ops and `is_active()` stays
false — no ifdefs at call sites. `Game` inits it after SDL, pumps
`SteamAPI_RunCallbacks` each frame, and shuts down before `SDL_Quit`.
Runtime failure (Steam not running, no dev `steam_appid.txt`) logs a
warning and the game continues standalone.

The real-SDK path was compile-and-link verified against a minimal local
mock of the API surface (the flag, the SDK finder, the redistributable
copy, the graceful init failure). True end-to-end verification waits on a
partner account and app id.

## Verification

- Linux system-SDL build: unchanged, 102/102 tests
- `RAVEN_BUNDLED_DEPS=ON` on Linux: SDL builds from source via CPM,
  102/102 tests — the same resolution path Windows CI takes
- `RAVEN_ENABLE_STEAM=ON` without the SDK: clear fatal error with setup
  instructions; with the mock SDK: builds, links, copies the
  redistributable, and degrades gracefully at runtime
- Windows CI job: validated on this branch before merging
