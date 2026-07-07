# 21. Optional Steamworks Integration

Date: 2026-07-06 Status: Accepted

## Context

Shipping on Steam ultimately wants the Steamworks SDK — achievements, rich
presence, Steam Input, and (for some features) the overlay. But the SDK is
distributed under Valve's partner agreement and cannot be committed to the
repository; `vendor/steamworks/` has been gitignored for it since the repo's
early days. The build must therefore work fully without the SDK, and every
contributor (and CI) must be able to compile without a partner account.

## Decision

**Steamworks is an opt-in build flag with a no-op fallback.**

- `-DRAVEN_ENABLE_STEAM=ON` activates `cmake/Steamworks.cmake`, which
  expects the SDK unzipped at `vendor/steamworks/sdk` (a clear fatal error
  explains the layout if it's missing), links the per-platform
  redistributable (`steam_api64` / `libsteam_api`), and copies it next to
  the game binary post-build.
- `src/platform/steam.hpp` exposes a thin `Steam` wrapper — `init()`,
  `run_callbacks()`, `shutdown()`, `is_active()`. The implementation is
  `#ifdef RAVEN_ENABLE_STEAM`; without the flag every method is a no-op and
  `is_active()` stays false, so call sites never carry ifdefs.
- Runtime failure is non-fatal: SDK built in but Steam not running (or no
  `steam_appid.txt` during development) logs a warning and the game runs
  without Steam features — the same degradation philosophy as audio, fonts,
  and sprites.
- `Game` owns the wrapper: init after SDL, `SteamAPI_RunCallbacks` pumped
  once per frame, shutdown before `SDL_Quit`.

Development flow: drop the SDK zip contents into `vendor/steamworks/sdk`,
place a `steam_appid.txt` (the app id, or `480` for Valve's SpaceWar test
app) next to the built binary, configure with the flag. Neither the SDK nor
the appid file is ever committed or shipped in the depot (Steam provides the
appid at launch).

Future Steam features (achievements, rich presence, Steam Input action sets)
extend the `Steam` wrapper behind the same flag; gameplay code observes
`is_active()` rather than the define.

## Consequences

**Positive:**

- The default build has zero Steam coupling; CI and contributors need no SDK
- One wrapper class is the only place `steam_api.h` is included — the API
  surface in use is auditable at a glance
- Graceful runtime fallback means one binary works both under Steam and
  standalone during development

**Negative:**

- The real-SDK build path can't run in CI (SDK can't be distributed); it is
  compile-verified locally against a minimal mock and will get true
  verification once a partner account and app id exist
- Overlay/DRM specifics (launch-under-Steam requirements,
  `SteamAPI_RestartAppIfNecessary`) are deferred until an app id exists
