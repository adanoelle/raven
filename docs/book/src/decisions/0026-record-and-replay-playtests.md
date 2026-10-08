# 26. Record and Replay for Playtests

Date: 2026-10-08 Status: Proposed

## Context

The content phase depends on playtests, and playtest reports are the weakest
link in the loop. The [Playtest Playbook](../playtest.md) asks testers to
describe what happened and "attach a screenshot or recording if possible."
A description of a bullet-hell moment is usually wrong in the details that
matter: where the bullet came from, how long the telegraph lasted, whether
the hitbox matched the sprite. A video shows what was on screen, but it
can't be paused on the tick before a hit, can't be queried for state, and
doesn't show inputs.

An exact re-run of the session would let anyone, a developer or an AI
assistant, step through it tick by tick, inspect state, and line it up with
the tester's notes. Claude can't play the game in real time (a look, decide,
act cycle takes on the order of a second, while the game ticks every
8 ms), but it can analyze a recorded run at full resolution. The tester
supplies the experience; the replay makes it inspectable.

The engine is already close to deterministic:

- Gameplay runs in fixed 1/120 s ticks
  ([ADR-0002](0002-fixed-timestep-120hz.md)); `fixed_update` never sees the
  frame delta.
- Gameplay randomness already comes from a single `std::mt19937` in the
  registry context (drifter AI and stabilizer drops).
- The only wall-clock read in gameplay is that RNG's seed
  (`GameScene::on_enter`).
- Systems read input only through `const InputState&`.

So a run is fully determined by its build, its content, its seed, and the
`InputState` each tick saw. Recording those four is enough to reproduce it.

Alternatives considered:

- **Video capture** (OBS, Steam recording) is still useful alongside, but it
  has no inputs or state and can't be stepped or queried.
- **State snapshots** (serializing the registry every tick) need
  serialization for every component, which there is no reflection for. The
  files would also be large and break whenever a component changes.
- **Driving the game from outside** (desktop automation) is too slow for a
  120 Hz action game, and still doesn't capture a human's run.

## Decision

**Playtest builds record each run's seed and per-tick input, and the game
can replay a recording exactly, either in a window for people or headless
for analysis.**

### Recording

- Recording starts in `GameScene::on_enter` and covers *gameplay ticks*
  only: ticks where `GameScene` is on top and updating. Menus, pause and
  options don't affect the simulation and aren't recorded.
- A recording is JSON Lines (`.rvn.jsonl`): readable, diffable and easy to
  load in Python. The first line is a header:
  - format version
  - build id (git describe, stamped at configure time)
  - content hash (map, stages, enemies, patterns, config)
  - class, seed, and the date
- After the header come input lines. Each one holds the tick number and only
  the `InputState` fields that changed since the previous line, including
  the latched `*_pressed` edges exactly as that tick saw them. Recording per
  tick, rather than per frame, makes replays independent of frame rate.
  - Analog values are quantized to 1/256 before comparing, so stick noise
    doesn't produce a line every tick.
  - Mouse position is recorded in virtual pixels.
- Every 120 ticks a checksum line hashes the state that matters: the
  player's position, HP and lives, score, room and wave, enemy count and
  positions, and bullet count. A replay checks these to find the first tick
  where it diverges.
- **Markers:** F2 drops a numbered marker (F1 is taken by the debug overlay),
  and the HUD flashes `MARK 3`. Testers write notes against marker numbers
  ("mark 3: that felt cheap"), and the playbook's reporting steps change to
  ask for the recording plus marker notes.
- Files go to `<pref dir>/recordings/`, named by date, class and seed. The
  newest 20 are kept.
- The file is written in chunks every two seconds and again at run end, so a
  crash leaves a usable recording up to the crash. This adds
  `fs::append_text` to the file seam ([ADR-0005](0005-no-std-filesystem.md)).
- Recordings contain only inputs, the seed and game state, nothing personal.

### Determinism rules

These become project rules, enforced by a test:

1. **Gameplay randomness comes only from the registry RNG.** No
   `std::random_device` or `rand()`, and no RNG local to a system.
2. **Gameplay never reads wall-clock time.** The seed is the one exception,
   and it is logged and recorded.
3. **Each run reseeds the RNG.** Today `ctx().emplace<std::mt19937>` keeps the
   previous run's generator (a review finding). A replay supplies its seed
   through the same path.
4. **Replays are exact only for the same build and content.** A different
   build id or content hash produces a warning; the replay still runs and
   reports where it diverges. Matching across compilers or platforms is not
   a goal, since floating point and `std::uniform_real_distribution` differ
   between standard libraries.

A Catch2 determinism test drives a scripted input sequence through the
gameplay systems twice and requires matching checksums at every interval.
The test uses no stored recordings, so balance changes don't break it.

### Replay

- `raven --replay <file>` opens the run in a window at normal speed, skipping
  the menus. Space pauses, `.` steps one tick, `[` and `]` change speed, and
  `M` jumps to the next marker. Anyone can watch exactly what the tester
  saw.
- `raven --replay <file> --headless --out <dir>` runs with the offscreen video
  and dummy audio drivers on a stepped clock, as fast as the CPU allows. It
  writes:
  - `events.jsonl`: room entered or cleared, wave spawned, damage taken
    (amount and the source's enemy name), deaths, pickups, steals, and
    markers
  - screenshots of the 480×270 render target (`SDL_RenderReadPixels` +
    `IMG_SavePNG`) at each marker, death and room clear, plus the 30 ticks
    before each death
  - `summary.json`: time per room, damage by source, cause of each death, and
    the first checksum divergence if there is one
- During a replay, `Input` returns the recorded `InputState` for each tick
  instead of polling SDL. This input source is also the seam a later
  stepped automation API (scripts, or an MCP server) would use; that API is
  out of scope here.

### Build scope

A `RAVEN_ENABLE_REPLAY` CMake option, like `RAVEN_ENABLE_IMGUI`, turns this
on for Debug and playtest builds and off for shipping builds. Without it, the
recorder and replay code isn't compiled in, and `--replay` is rejected.

## Sequencing

1. **Determinism:** reseed per run, log the seed, document the rules, and
   add the determinism test.
2. **Recording:** the input source seam, per-tick recording, markers,
   chunked writes, and playbook updates.
3. **Windowed replay:** watching, pausing, stepping and divergence warnings.
4. **Headless replay:** events, screenshots and summary.

Steps 1 and 2 are enough for a tester's run to be sent and reproduced.
Steps 3 and 4 make it easy to work through.

## Consequences

**Positive:**

- A playtest moment can be reproduced exactly and stepped through tick by
  tick, with full state, instead of reconstructed from memory.
- Bug reports come with a file that reproduces the bug.
- Headless replays turn a session into data: deaths by cause, damage by
  source, and time per room, across many testers.
- An AI assistant can analyze real human runs at frame resolution, which is
  the most useful role it can play in playtesting.
- The input source seam and stepped clock are the foundation for automated
  play later.

**Negative:**

- Determinism becomes a rule every gameplay change must keep. The test
  catches violations in the systems it drives, not in everything.
- A recording is only exact for the build and content it was made with.
  Once tuning moves on, old recordings show where they diverge but are no
  longer exact.
- More moving parts: a file format to version, a build flag, a command-line
  mode, and an append API in the file seam.
- Testers have to send files, and the playbook has to make that easy.
- A replay shows what happened, not how it felt. Human playtests and their
  notes are still the source of feel.

## Open questions

- Should recording also cover menus, for bugs in scene flow, or stay
  gameplay-only?
- Should markers take a short text note in game, or only on the game-over
  screen, or only in the external report?
- How should files get from testers to the team: manual attachments, or an
  upload step for Steam playtest builds?
