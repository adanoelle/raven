# 26. Record and Replay for Playtests

Date: 2026-10-08 Status: Proposed (revised 2026-11-12 after review)

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
- Systems read input only through `const InputState&`. Everything that
  happens before that (press edges latched from SDL events, trigger
  hysteresis, deadzones) is already folded into the `InputState` a tick
  receives, so none of it needs recording separately.

So a run is fully determined by its build, its content, its seed, and the
`InputState` each tick saw. Recording those four is enough to reproduce it.

Three things in today's code stand in the way, and the sequencing below
starts with them:

- The per-tick system list lives inline in `GameScene::update`, and the
  room index is a private member of the scene. A test can't run "the
  gameplay tick" without a window.
- `ctx().emplace<std::mt19937>` doesn't replace an existing generator, so a
  second run in the same session continues the first run's sequence instead
  of using its own seed.
- A bullet carries only which side fired it, and an enemy doesn't carry the
  name of its definition, so "what hit the player" can't be answered.

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
  - content hash (map, stages, enemies, patterns, config), taken from the
    data as loaded when the run starts, not from the files at build time,
    since testers and developers edit content between runs
  - class, seed, and the date
- After the header come input lines. Each one holds the tick number and only
  the `InputState` fields that changed since the previous line, including
  the latched `*_pressed` edges exactly as that tick saw them. Recording per
  tick, rather than per frame, makes replays independent of frame rate.
  - The recorded value is the value gameplay consumed, bit for bit. Stick
    axes are quantized to 1/256 inside `Input`, in every build, before
    gameplay sees them. That keeps stick noise from producing a line every
    tick without the recording and the simulation ever disagreeing. (A
    recorder that rounded on its own would replay slightly different
    aim and movement than the tester had.)
  - Mouse position is in virtual pixels and is fractional at most window
    sizes. It is written with enough digits to read back as the same float.
- Every 120 ticks a checksum line hashes the state that matters: the
  player's position, HP and lives, score, room and wave, enemy count and
  positions, and bullet count. A replay checks these to find the first tick
  where it diverges.
- **Markers:** F2 on the keyboard, or Back/Select on a controller, drops a
  numbered marker, and the HUD flashes `MARK 3`. (F1 is taken by the debug
  overlay; the gamepad layout in [ADR-0027](0027-gamepad-layout.md) keeps Back/Select free for this.)
  The marker is not part of `InputState` and doesn't affect the simulation. Testers write notes against marker numbers
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
gameplay tick twice and requires matching checksums at every interval. The
test uses no stored recordings, so balance changes don't break it.

For the test to cover the real game rather than a copy of it:

- The system list moves out of `GameScene::update` into one function that
  the scene, the test and the replay all call. The room index and anything
  else the tick reads move from scene members into registry state.
- The script plays through at least one room change, so wave spawning and
  room transitions are covered.
- One case starts a second run in the same registry with the same seed and
  requires the same checksums as the first. That is the test for rule 3.

### Replay

- `raven --replay <file>` opens the run in a window at normal speed, skipping
  the menus. Space pauses, `.` steps one tick, `[` and `]` change speed, and
  `M` jumps to the next marker. Anyone can watch exactly what the tester
  saw.
- `raven --replay <file> --headless --out <dir>` runs with the offscreen video
  and dummy audio drivers, as fast as the CPU allows. It writes:
  - `events.jsonl`: room entered or cleared, wave spawned, damage taken
    (amount and the source's enemy name), deaths, pickups, steals, and
    markers
  - screenshots of the 480×270 render target at each marker, death and room
    clear, plus the 30 ticks before each death
  - `summary.json`: time per room, damage by source, cause of each death, and
    the first checksum divergence if there is one

  Three details the headless mode depends on:
  - **Stepped clock.** The main loop today measures real time and sleeps to
    cap the frame rate. Headless replay doesn't use that loop: it runs one
    tick per recorded tick with no timing and no sleep, and renders only the
    ticks it needs a screenshot of.
  - **Screenshots.** The scene is drawn to the 480×270 target texture and
    read back (`SDL_RenderReadPixels`) while that texture is still the
    render target, before it is scaled to the window. A screenshot is taken
    after the tick's gameplay render and before any scene change that tick
    asked for, so the last death shows the room and the bullet, not the
    game-over screen.
  - **Damage sources.** Each enemy carries the name of its definition
    ([ADR-0025](0025-named-enemy-definitions.md)), and each enemy bullet
    carries the name of the enemy that fired it, as interned strings. Damage
    events read the name from whatever dealt the hit. This is ordinary game
    state, compiled in regardless of the replay flag, since the HUD and
    death screen can use it too.
- During a replay, `Input` returns the recorded `InputState` for each tick
  instead of polling SDL. This input source is also the seam a later
  stepped automation API (scripts, or an MCP server) would use; that API is
  out of scope here.

### Build scope

A `RAVEN_ENABLE_REPLAY` CMake option, like `RAVEN_ENABLE_IMGUI`, turns this
on for Debug and playtest builds and off for shipping builds. Without it, the
recorder and replay code isn't compiled in, and `--replay` is rejected.

## Sequencing

1. **Determinism:** move the tick into one shared function, reseed per run,
   log the seed, quantize stick axes in `Input`, document the rules, and add
   the determinism test.
2. **Recording:** the input source seam, per-tick recording, markers,
   chunked writes, and playbook updates.
3. **Windowed replay:** watching, pausing, stepping and divergence warnings.
4. **Headless replay:** damage source names, events, screenshots and
   summary.

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
- Some of the groundwork lands in every build, not only playtest builds:
  the shared tick function, stick quantization (1/256 is finer than any
  stick's noise, so it shouldn't be felt) and a name on enemies and their
  bullets (four bytes each).
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
