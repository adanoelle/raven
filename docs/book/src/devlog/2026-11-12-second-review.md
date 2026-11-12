# Second Code Review: Open PRs and the First Playtest

Date: 2026-11-12 Tags: review, playtest, input, ci, build

## Why

A month after the [first review](2026-10-08-pre-content-review.md), its
blockers are fixed and three pull requests were open: the production plan
(#21), the record and replay ADR (#23) and the controller layout (#24). This
review covered the tree and those three, with one question: what would
distort a first playtest on a controller?

Six reviewers each took one area: the three pull requests, gameplay systems,
the authoring pipeline, the shipping build and CI, and the docs. Findings in
"Fixed" and "Gameplay" were checked against the code by hand. One finding was
dropped after testing: Aseprite 1.3.17 does not renumber frames with
`--split-tags`, with or without untagged frames or overlapping tags.

Baseline: Debug build clean, all tests pass, clang-tidy reports nothing in
project code.

## The Pull Requests

- **#24 controller layout: merge after fixes.** All fixed, see below.
- **#23 record and replay ADR: revise, then merge.** The design holds. The
  ADR was missing things an implementer would hit in the first week:
  - It quantized stick values in the recording while gameplay consumed the
    raw floats, so a replay would not have matched.
  - The determinism test needs the per-tick system list out of
    `GameScene::update`, and a second-run case for the reseed bug.
  - "Damage by source" needs the enemy's definition name on enemies and
    bullets.
  - A controller needs a marker button (Back/Select).
  - The headless clock, the screenshot read-back and the timing of the
    final-death screenshot were unspecified.

  The ADR now covers each of these.
- **#21 production plan: rewrite or close.** It listed the rooms, exits and
  pickups sheet as missing, and its Phase 0 predated the animation and enemy
  ADRs. Its status and Phase 0 were rewritten against the current tree, with
  a first-playtest phase added.

## Fixed

### Controller layout (#24)

- **Sharpshooter tap fired nothing.** A trigger pulled and released between
  two ticks set `shoot_pressed` but never `shoot`, and the charged shot only
  fired on a held-then-released sequence. A tap now fires an uncharged shot.
- **Trigger hysteresis only ran at the poll.** A trigger that crossed the
  press threshold and came back between polls latched a press but left the
  held state stale. Every axis event now updates it.
- **CI format failure.** CI's clang-format and the dev shell's disagree on
  spacing in member-pointer declarations. The one struct is fenced off;
  pinning CI's version is in the build list below.
- **Docs:** `input.md`, `scenes.md`, ADR-0027 and the README controls table.

### Gameplay

- **A disarmed shooter was harmless.** Disarm removes the emitter and makes
  the enemy charge, but only the grunt had contact damage. One melee hit
  left the boss, the stalker and the drifter unable to hurt the player. A
  disarmed enemy with no body damage now gets the default (15, after one
  cooldown). Whether one hit should disarm the boss was a design question:
  see "Decisions" below.
- **Idle enemies never woke when shot.** Enemies spawn 208 px from the
  player start with activation ranges of 160 to 200, so the first room
  could be cleared from the doorway. A hit now wakes an idle enemy.
- **A stalker with `move_speed` 0 went NaN.** Its strafe divided by the
  speed. The enemy became unhittable and the room never cleared. A
  stationary stalker now stays put.
- **Fire cooldown carried across weapons.** After a slow stolen weapon
  decayed, the default gun stayed locked out for up to two seconds. The
  cooldown is now capped at the new weapon's fire rate on pickup and on
  revert.

Each fix has a test.

## Open: Gameplay

The four items that were first in priority are fixed (the same day, in a
follow-up PR):

1. **Dash and melee on the same tick never gave the Knight's spin.** The
   dash system now runs before melee. Melee pressed a tick or two *before*
   the dash is still a plain swing; that would need an input buffer.
2. **Confirm leaked into shoot on keyboard.** A button pressed on the tick
   that hands a menu over to gameplay reads as released until it is let go
   (`Input::block_pressed_until_released`), for class select and for
   resuming from pause.
3. **Spawn grace covered contact damage only.** A spawned enemy now holds
   its fire for 1 s (`SPAWN_FIRE_GRACE`), counted from the spawn even while
   it is dormant.
4. **The exit opened, and was checked, on the tick of the last kill.**
   Exits open 0.75 s after the room is cleared (`EXIT_OPEN_DELAY`).

Still open:

- A drifter keeps its knockback velocity until its next wander timer.
- The boss coward's wall probe is shorter than its hitbox.
- Abilities and melee hit through walls.
- Losing the controller doesn't pause.
- Adjusting any option re-applies window scale and vsync.
- A diagonal stick moves two ways in the options menu.
- Menu direction edges come from the poll, not from events, so a quick
  flick at a low frame rate can be missed.

## Open: Authoring Traps

- Pattern and stage files don't warn on unknown keys, so a typo is ignored.
- A duplicate pattern name overwrites the first without a warning.
- A stage file that fails to parse shifts every later stage down one.
- `spawn_index` follows LDtk's entity list order, which changes when
  entities are edited.
- A one-shot clip doesn't restart when the same clip is requested again
  (two melee swings in a row show one animation).
- `export_art.sh` writes every sheet into one flat directory, overwrites
  without asking and leaves outputs of deleted sources behind.
- The player sprite is fixed at 32x32 in code.
- LDtk layer opacity and non-string entity fields are dropped.

## Shipping Build and CI

The owner wants small binaries that run fast on low-end hardware.

Fixed:

- **Debug logging in Release.** `main.cpp` set the log level to debug in
  every build. Optimised builds now log at info.
- **No LTO, section garbage collection or strip.** Release and MinSizeRel
  now have all three (`cmake/ShippingFlags.cmake`). The Linux Release
  binary went from 2.7 MB to 1.2 MB.
- **`install` shipped dependency files.** EnTT and LDtkLoader headers,
  CMake packages and five `.gitkeep` files went into the package. The
  installed tree went from 5.8 MB to 1.7 MB, and is now the game and its
  assets only. Windows installs its DLLs explicitly.
- **Windows CI built Debug only**, with ImGui on. There is now a Release
  job with the overlay off, which also checks the package layout.
- **Lint ran serially for 20 to 28 minutes.** It now runs one process per
  core, in CI and in `just lint`, which also depends on a build now.
  Headers are no longer linted as files of their own (about 15% of the
  work); they are checked through the sources that include them.
  A pull request lints only the sources it changes, and everything when
  it touches a header or the lint and build setup (`tools/lint_files.sh`,
  `just lint-changed`). Pushes to `main` still lint everything.
- **No concurrency group**, so superseded pushes kept running. Docs-only
  changes ran the full C++ pipeline; they now run a book build instead
  (`docs-check.yml`), which also catches a broken book before merge.
- **clang-format wasn't pinned.** CI now uses the dev shell's version.

Open:

- `spdlog::debug` calls are still compiled into Release. They cost a
  level check each. Compiling them out means moving to the `SPDLOG_*`
  macros.
- clang-tidy isn't pinned. PyPI has no build of the dev shell's version.
- No sanitizer job. Warnings aren't errors in CI (GCC reports a false
  `-Wnull-dereference` inside EnTT).
- `test_content.cpp` uses a shared temp file name.
  `catch_discover_tests` runs at build time.
- Dependencies are pinned by tag, not by commit hash.
- The tilemap is redrawn tile by tile every frame. Baking it to a texture
  once per room is the largest cheap rendering win.

## Out-of-Date Docs

Refreshed against the code:

- `playtest.md` and `weapons.md`: disarms drop weapons, kills don't; 0.5 s of
  invulnerability after a hit; the checks for this review's fixes.
- Render layer tables (characters 10, bullets and pickups 5, exits 1).
- `art-spec.md` (grunts drawn 1:1, 16x16 pickups, the pickups sheet exists)
  and `art-integration.md` (frame addressing instead of `sprite_defs`).
- System-order listings on eight pages, all matching `GameScene::update`.
- `player-classes.md`: the Knight and the dash chain.
- `melee-and-dash.md`: disarm hits and the spin.
- Examples in ADR-0004 and ADR-0009 (the latter as a dated note).
- SDL2 names in the README, ARCHITECTURE, and the environment, tilemap,
  smooth-movement, shooting and audio-resources pages.
- `CLAUDE.md`: the test list, `just` recipes and `tools/`.

Still stale: `guide/project-tour.md` (test list, docs tree, system and
component lists) needs a pass of its own. The doc pass also turned up things
in the code:

- The decay explosion takes 1 HP from a player with 60 to 150
  (`pickup_system.cpp`). It reads as a leftover from when the player had a
  few hit points; the penalty needs a real number.
- `weapon_from_emitter` never sets `piercing`.
- The `sprite_defs` block in `assets/data/config.json` is read by nothing.
- `ClassId` and `GameState::player_class` are written and never read.
- The header comment in `damage_system.hpp` still says deaths drop weapons,
  and `input.cpp` names SDL2 functions in a comment.

## Decisions

Both questions the review raised were settled the same day.

- **Bosses take more than one melee hit to disarm.** `disarm_hits` in an
  enemy's definition sets the count (default 1, boss 3). Boss HP went from
  10 to 40: melee does 2 damage, so at 10 HP the boss was half dead by the
  third swing and the count meant little. 40 is a first guess for
  playtests to correct.
- **Body damage of a disarmed enemy** is per enemy
  (`disarmed_contact_damage`), defaulting by tier: grunt 15, mid 20,
  boss 30.
- **Idea, not built:** bosses re-arm by pulling their weapon back
  magnetically, and the player can counter the pull. Noted on the
  [enemy archetypes](../enemy-archetypes.md) page.

## Status

| Item | Status |
|------|--------|
| #24 controller layout fixes | Fixed |
| Gameplay: disarm, idle wake, stalker NaN, cooldown carry-over | Fixed |
| #23 ADR-0026 revisions | Done |
| #21 production plan | Rewritten |
| Boss disarm and disarmed body damage | Decided and built |
| Gameplay items 1-4 | Done |
| Smaller gameplay items | Open |
| Authoring traps | Open |
| Shipping build and CI | Log level, LTO and strip, package contents, Windows Release job, parallel lint, docs check and the clang-format pin done; the rest open |
| Out-of-date docs | Done, except `project-tour.md` |
