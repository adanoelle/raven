# Audio Integration Guide

Developer reference for how audio works in the Raven engine and how to add new
sounds. Read the [Audio Specification](audio-spec.md) for asset format
requirements, and the [Music Track List](music-track-list.md) for the
composer-facing track briefs.

---

## 1. Current State

**Sound effects are implemented and shipping** on native SDL3 audio — no mixer
library ([ADR-0019](decisions/0019-sdl3-native-audio.md)). **Music playback is
not yet implemented**; see section 5 for the path forward.

The pieces, all in the tree today:

| Piece                | Location                        | Role                                              |
| -------------------- | ------------------------------- | ------------------------------------------------- |
| `AudioEngine`        | `src/audio/audio_engine.hpp`    | Device, WAV loading, per-play streams, voice cap  |
| `Sfx` / `AudioQueue` | `src/ecs/components.hpp`        | Effect IDs and the per-tick event queue           |
| `push_sfx()`         | `src/ecs/components.hpp`        | How systems request a sound                       |
| Queue drain          | `src/scenes/game_scene.cpp`     | Dedupes and forwards to the engine each tick      |
| Sound manifest       | `assets/data/config.json`       | `"sounds"` map: id → WAV path                     |
| Placeholder WAVs     | `tools/gen_sfx.py`              | Deterministic synthesized effects                 |

Seven effects exist: `shoot`, `player_hit`, `enemy_hit`, `enemy_down`,
`pickup`, `dash`, `melee`. All are synthesized placeholders — **file names are
the contract**, so the real audio pass replaces WAVs in `assets/audio/sfx/`
with no code changes.

The historical decisions: [ADR-0010](decisions/0010-sdl2-mixer-audio.md) chose
SDL_mixer, [ADR-0013](decisions/0013-sdl3-migration.md) removed it during the
SDL3 migration, and ADR-0019 settled on SDL3's own audio API instead.

---

## 2. Architecture

Gameplay systems never touch the audio backend. The flow per fixed tick:

```
system detects event          GameScene::update, after systems run
        │                              │
        ▼                              ▼
push_sfx(reg, Sfx::EnemyHit) ──► AudioQueue (registry ctx) ──► dedupe ──► AudioEngine::play("enemy_hit")
```

- **`Sfx`** is a small enum (`Sfx::Shoot`, `Sfx::Dash`, …) with a `Count`
  sentinel kept last. `sfx_sound_name()` maps each value to its `config.json`
  key.
- **`push_sfx()`** appends to the `AudioQueue` in the registry context. It is
  a no-op when the queue is absent, so unit tests need no audio setup.
- **`GameScene`** drains the queue after the system pipeline. Same-tick
  duplicates are deduped: N sample-aligned copies of one effect sum to N-times
  amplitude (a distorted pop, not a louder hit) — e.g. one shotgun burst
  hitting five enemies at once.
- **`AudioEngine::play()`** binds a fresh SDL audio stream per instance; SDL
  mixes all bound streams. `update()` (called once per frame) reaps drained
  streams. Playback is capped at `MAX_VOICES` (32) — further plays drop.
- **Silent degradation**: a failed `init()` leaves the engine in no-op mode. A
  machine without an audio device plays silently, never crashes — same
  philosophy as missing sprites and fonts.

UI sounds outside the fixed-tick pipeline (e.g. the options menu volume
preview) call `game.audio().play(...)` directly; the queue exists to keep
*gameplay systems* pure and testable, not as the only entry point.

### Volume

User volume lives in `settings.json` under the platform pref path
([ADR-0017](decisions/0017-settings-pref-path.md)) — **not** `config.json`,
which is read-only game data. `sfx_volume` (0–100) maps to linear gain through
a quadratic curve (`volume_to_gain()` in `src/core/game.cpp`) so the slider
feels perceptually even. `music_volume` is stored and shown in the options
menu but drives nothing until music lands.

Note: `set_master_gain()` only affects *subsequent* plays. Fine for sub-second
effects; a future music stream must have its gain updated live.

---

## 3. Adding a New Sound Effect

1. Add the WAV to `assets/audio/sfx/` (16-bit PCM, 44100 Hz — see the
   [Audio Specification](audio-spec.md)). For a placeholder, extend
   `tools/gen_sfx.py`.
2. Register it in the `"sounds"` map in `assets/data/config.json`:
   ```json
   "sounds": {
       "slam": "assets/audio/sfx/slam.wav"
   }
   ```
   `Game::load_assets()` loads every entry at startup via `paths::asset()`.
3. Add an enumerator to `Sfx` in `src/ecs/components.hpp` — **before the
   `Count` sentinel** — and a matching case in `sfx_sound_name()` returning
   the config key.
4. Call `push_sfx(reg, Sfx::YourEffect)` from the system where the event
   happens.
5. Extend the name-mapping test in `tests/test_audio.cpp`.

A `static_assert` in `game_scene.cpp` guards the dedupe bitmask if `Sfx` ever
grows past 32 entries.

---

## 4. Event-Driven Audio Design

The pattern for gameplay sounds: a system detects a state change, then pushes
an event. Real examples from the tree:

```cpp
// collision_system.cpp — bullet hits
push_sfx(reg, Sfx::PlayerHit);
push_sfx(reg, Sfx::EnemyHit);

// shooting_system.cpp — shot fired
push_sfx(reg, Sfx::Shoot);

// pickup_system.cpp — weapon or stabilizer collected
push_sfx(reg, Sfx::Pickup);
```

Systems stay pure functions on the registry: no audio handles, no engine
includes, and tests exercise them without any audio device (`push_sfx` no-ops
when the queue is absent). Swapping the audio backend later touches one drain
site in `GameScene`, not every system.

Known gap: the active abilities (Ground Slam, Concussion Shot) are still
silent — tracked in the
[architecture review devlog](devlog/2026-07-15-architecture-review-fixes.md).

---

## 5. Music: Path Forward

Music needs what the SFX path deliberately lacks: a compressed format (OGG
Vorbis) and a streaming decode path. ADR-0019 explicitly scoped it out.
Options, in rough order of preference:

1. **Decode library feeding an SDL stream** — `stb_vorbis` or `dr_libs`
   decoding OGG frames into a persistent `SDL_AudioStream` that stays bound to
   the same device `AudioEngine` already owns. Zero heavyweight dependencies,
   consistent with ADR-0019's approach.
2. **SDL3_mixer** — the successor to SDL2_mixer. Reevaluate when it has a
   stable release packaged in nixpkgs; it would bring music streaming, fades,
   and loop-point handling for free.
3. **FMOD / Wwise** — only if the game grows into adaptive music (vertical
   layers, re-sequencing). Out of scope for the current design; the queue/drain
   seam would isolate the swap.

Whichever lands must support: live gain changes (the options menu adjusts
`music_volume` in real time), seamless whole-file looping, and eventually
`LOOPSTART`/`LOOPLENGTH` sample tags for intro-then-loop tracks (see the
[Music Track List](music-track-list.md)).

Scene-based track selection follows the same pattern as everything else:
`on_enter()` asks for a track, the engine crossfades. The scene → track
mapping is specified in the track list document so composition can start
before the engine work does.
