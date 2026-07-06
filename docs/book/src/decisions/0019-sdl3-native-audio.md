# 19. Sound Effects on Native SDL3 Audio

Date: 2026-07-05 Status: Accepted (supersedes [ADR-0010](0010-sdl2-mixer-audio.md))

## Context

[ADR-0010](0010-sdl2-mixer-audio.md) chose SDL_mixer for audio, but that
decision predates the SDL3 migration ([ADR-0013](0013-sdl3-migration.md)) and
no playback code was ever written. SDL_mixer's SDL3 port had no stable release
to pin, and adding a prerelease dependency to the freshly built CI pipeline
(which compiles SDL from source) doubles the dependency surface for a game
that currently needs exactly one audio feature: short overlapping sound
effects.

SDL3's own audio API changed the calculus: audio streams bound to a device
are **mixed by SDL itself**, each with its own format conversion and gain.
That is the core of what a mixer library provides for SFX playback.

## Decision

**Sound effects play through native SDL3 audio** via `AudioEngine`
(`src/audio/audio_engine.hpp/.cpp`), with no mixer dependency:

- `init()` opens the default playback device; failure leaves the engine in
  silent no-op mode (a machine without audio plays silently, never crashes).
- Sounds are WAVs loaded with `SDL_LoadWAV`, kept in memory (all effects are
  well under a second).
- `play()` creates a stream per instance, sets its gain (master volume from
  user settings x per-play gain), **binds it to the device first** (the
  stream's destination format comes from the device at bind time — queueing
  data before binding fails), then queues and flushes the samples. SDL mixes
  all bound streams.
- `update()` runs once per frame and reaps streams that have drained.

Gameplay systems never touch the engine. They push `Sfx` values into an
`AudioQueue` in the registry context (`push_sfx()`, a no-op when the queue is
absent, so unit tests need no audio setup); `GameScene` drains the queue after
the system pipeline and forwards to the engine using the `sfx_sound_name()`
mapping, which matches the `sounds` section of `config.json`.

Placeholder effects are synthesized by `tools/gen_sfx.py` (square waves,
noise bursts; deterministic output) — file names are the contract, so the
audio pass replaces WAVs without code changes.

**Music is out of scope for this decision.** Streaming long tracks wants a
compressed format and a decode path; when that lands, SDL3_mixer (if stable
by then) or a decode library (stb_vorbis, dr_mp3) feeding an SDL stream are
both compatible with this architecture — `music_volume` in settings is
already reserved.

## Consequences

**Positive:**

- Zero new dependencies; CI and the Nix flake are untouched
- Overlapping instances, per-play gain, and format conversion all come from
  SDL directly
- The event-queue seam keeps systems pure and testable, and decouples
  gameplay code from the audio backend — swapping the engine later touches
  one drain site
- Silent-degradation matches the sprite/font philosophy

**Negative:**

- WAV-only, fully in memory: fine for effects, unsuitable for music
- One `SDL_CreateAudioStream` + destroy per play; trivial at current rates
  (a pool would be the fix if profiling ever disagrees)
- No pitch variation, panning, or priority channels — add to `AudioEngine`
  when a real need appears
