# 17. User Settings in the Platform Pref Path

Date: 2026-07-03 Status: Accepted

## Context

`assets/data/config.json` mixed two kinds of data: read-only game data (sprite
sheet definitions, tilemap references) and what should have been user
preferences (`window.scale`, `window.fullscreen`, `window.vsync`, audio
volumes). The preference half was dead: nothing ever read it — the renderer
hardcoded scale 2 and vsync on — and nothing could ever *write* it, because
the assets directory ships with the game and must be treated as read-only
(Steam installs may not be writable, and updates overwrite it).

Nothing in the game persisted anything: zero writes to disk anywhere in src/.
An options menu, key rebinding, or high-score table had nowhere to live.

## Decision

**User settings live in `settings.json` inside `SDL_GetPrefPath("adanoelle",
"raven")`** — the platform-appropriate per-user writable directory
(`~/.local/share/adanoelle/raven/` on Linux, `%APPDATA%` on Windows, Application
Support on macOS). `assets/data/config.json` is read-only game data from here
on; its `window` and `audio` sections were removed.

The `Settings` struct (`src/core/settings.hpp`) carries window scale,
fullscreen, vsync, and music/sfx volumes. Loading is defensive: missing file →
defaults (first run), malformed file → defaults with a warning, out-of-range
values → clamped. `Game::init()` loads settings, immediately writes them back
(creating the file on first run so players can find and edit it, and
normalising in any fields added by newer versions), and passes them to
`Renderer::init()` — which now actually honors scale, fullscreen, and vsync.

If the driver refuses vsync, the main loop falls back to a 240 fps frame
limiter rather than busy-spinning.

## Consequences

**Positive:**

- Settings survive game updates and reinstalls, and never require write access
  to the install directory
- The renderer's behaviour is finally user-controllable (file-editable today,
  options-menu-driven later — the struct and persistence are ready)
- `paths::pref_dir()` gives future systems (saves, high scores, key bindings)
  a home
- Serialisation is unit-testable without SDL (pure JSON round-trip)

**Negative:**

- Two config files to reason about — the split (game data vs user prefs) must
  stay disciplined
- Settings written at init are not re-read until next launch; an in-game
  options menu will need an explicit save-on-change
