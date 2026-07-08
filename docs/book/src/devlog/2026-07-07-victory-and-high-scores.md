# Victory Screen and High Scores

Date: 2026-07-07 Tags: scenes, save, progression

## What Changed

Clearing the final stage previously swapped straight back to the title
screen with a `// Victory` comment — the game literally could not
acknowledge being beaten. Now there's a proper gold-on-dark `VictoryScene`
with the run's score, and the best score persists across sessions.

## High Scores

`SaveData` (`src/core/save_data.hpp`) is the first entry in the player
progress file — `save.json` in the pref dir, separate from `settings.json`
(preferences) per the ADR-0017 split. Loading follows the same defensive
pattern: missing or corrupt file yields defaults, so a damaged save never
blocks the game from starting. `Game::record_score()` persists a new best
and reports whether the run beat it.

Both end screens record the score (a personal best on a *losing* run still
counts) and show either a blinking "NEW BEST!" or the standing best; the
title screen shows the best under the menu once one exists. When the
ADR-0014 meta-progression lands, its currency/unlock state joins `SaveData`.

## Verification

- 105/105 tests, including SaveData round-trip, negative-score clamping,
  and missing/corrupt-file fallback
- Victory screen rendering verified through the software-renderer harness
- Headless boot confirms the save loads (and "starting fresh" on first run)
