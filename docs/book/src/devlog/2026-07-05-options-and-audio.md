# Options Menu and Sound Effects

Date: 2026-07-05 Tags: audio, options, settings, scenes

## What Changed

The last two engine-side blockers from the Steam-readiness review: the game
now has an options menu wired to the persisted settings, and it makes sound.

## Options Menu

`OptionsScene` (fullscreen, window scale, vsync, music volume, sfx volume) is
reachable from both the title screen — which grew a proper Start/Options menu —
and the pause menu. Every change applies **live** through the new
`Game::apply_settings()`: the renderer gained `set_fullscreen` /
`set_window_scale` / `set_vsync`, the audio engine picks up the SFX volume as
its master gain, and `settings.json` is written on each change. If the driver
refuses vsync, the menu shows "VSYNC UNAVAILABLE — USING FRAME LIMITER"
instead of lying about the state. Adjusting SFX volume plays a blip for
audible feedback.

The scene renders as an overlay (own dim layer, no clear), so it works
identically over the title screen and over paused gameplay.

## Sound Effects

[ADR-0019](../decisions/0019-sdl3-native-audio.md): effects play through
**native SDL3 audio** — no mixer dependency. SDL mixes every audio stream
bound to a device, so `AudioEngine` binds one stream per playing instance
with per-play gain and reaps drained streams each frame. One subtlety worth
recording: the stream must be **bound before queueing samples** — its
destination format comes from the device at bind time, and `PutData` fails
without one. (Caught by the headless verification harness, not by eye.)

Gameplay systems push `Sfx` events into an `AudioQueue` in the registry
context — shoot, player hit, enemy hit, enemy down, pickup, dash, melee —
and `GameScene` drains the queue after the system pipeline. `push_sfx()` is a
no-op when the queue is absent, so unit tests need no audio setup.

Placeholder sounds are synthesized square waves and noise bursts from
`tools/gen_sfx.py` (deterministic output; file names are the contract with
config.json). The audio pass replaces the WAVs without touching code.

Music remains future work — `music_volume` is wired through settings and the
options menu, waiting for a streaming decode path.

## Verification

- 102/102 tests pass, including new coverage: engine safety when
  uninitialised, dummy-driver init/shutdown, sound-name contract, and
  AudioQueue semantics
- Headless end-to-end check: two overlapping plays of a generated WAV
  through the dummy device, streams drained and reaped
- CI now exports `SDL_AUDIODRIVER=dummy` for the test step
