# Raven Audio Specification

Reference document for sound effects, music, technical requirements, file
formats, and naming conventions.

---

## 1. Sonic Identity (Undecided)

The game's sonic direction has not been finalized. Three viable approaches are
presented here so the decision can be made later with full context. All three
can work with the current SDL_mixer pipeline (see
[ADR-0010](decisions/0010-sdl2-mixer-audio.md)).

### Option A: Chiptune / Retro

Authentic 8-bit or 16-bit sound using limited waveforms and noise channels.

| Aspect     | Details                                                                           |
| ---------- | --------------------------------------------------------------------------------- |
| SFX        | Generated with sfxr/Bfxr/ChipTone; short, punchy, lo-fi                           |
| Music      | Tracker-composed (FamiTracker, Furnace); 4–8 channel constraint                   |
| Pros       | Perfect match for pixel art; nostalgia factor; tiny file sizes; fast to prototype |
| Cons       | Narrower emotional range; may feel "generic retro" without strong composition     |
| References | Nuclear Throne (Jukio Kallio), Shovel Knight (Jake Kaufman)                       |

### Option B: Modern Electronic

Synthesized and produced in a DAW with full frequency range.

| Aspect     | Details                                                                 |
| ---------- | ----------------------------------------------------------------------- |
| SFX        | Designed in a DAW or generated with synths; can be lo-fi or hi-fi       |
| Music      | Electronic / synth-driven; layered pads, arpeggios, punchy drums        |
| Pros       | Wider dynamic range; can convey tension, dread, triumph more easily     |
| Cons       | Larger file sizes; harder to produce at high quality without experience |
| References | Blazing Beaks (Ivo Sissolak), Hyper Light Drifter (Disasterpeace)       |

### Option C: Hybrid

Chiptune-inspired melodies layered with modern production (reverb, filtering,
sidechaining). This is the approach Celeste uses — piano and synth patches
processed through NI MASSIVE and triggered via FMOD.

| Aspect     | Details                                                                |
| ---------- | ---------------------------------------------------------------------- |
| SFX        | Mix of generated (sfxr) and designed (DAW) depending on context        |
| Music      | Chip leads + modern pads/drums; retro feel with cinematic depth        |
| Pros       | Best of both worlds; distinctive; proven in critically acclaimed games |
| Cons       | Most complex to produce; requires both chip and DAW skills             |
| References | Celeste (Lena Raine), Enter the Gungeon (doseone)                      |

---

## 2. Sound Effects Categories

Exhaustive table of every sound event the game needs. Each entry includes
recommended characteristics to guide creation regardless of which sonic
direction is chosen.

### Player Actions

| Event               | Duration  | Pitch            | Style Notes                                                         |
| ------------------- | --------- | ---------------- | ------------------------------------------------------------------- |
| Shoot (standard)    | 50–100ms  | Mid-high         | Snappy, satisfying pop; vary pitch ±5% per shot to avoid repetition |
| Shoot (charged)     | 100–150ms | Lower, fuller    | Weightier than standard; brief bass thump at start                  |
| Melee swing         | 80–120ms  | Mid, sweeping    | Whoosh with slight metallic edge                                    |
| Melee hit (connect) | 60–100ms  | Mid, punchy      | Crisp impact; distinct from bullet hit                              |
| Dash / dodge        | 60–100ms  | Mid-high, rising | Quick swoosh; conveys speed                                         |
| Jump / land         | 40–80ms   | Low thud         | Subtle; should not compete with combat sounds                       |
| Hurt / take damage  | 100–150ms | Low-mid, harsh   | Unpleasant enough to feel bad, brief enough to not annoy            |
| Death               | 200–400ms | Descending       | Dramatic; can layer with a brief reverb tail                        |
| Pickup weapon       | 80–120ms  | Ascending        | Positive, rewarding click-chime                                     |
| Pickup item         | 60–100ms  | High, bright     | Short cheerful tone                                                 |
| Pickup health       | 80–120ms  | Ascending, warm  | Softer than weapon pickup; restorative feel                         |
| Out of ammo (click) | 30–60ms   | Dry, flat        | Unsatisfying click to signal "empty"                                |
| Reload              | 100–200ms | Mechanical       | Click-slide-click cadence                                           |

### Enemy Actions

| Event              | Duration   | Pitch                   | Style Notes                                           |
| ------------------ | ---------- | ----------------------- | ----------------------------------------------------- |
| Enemy shoot        | 50–100ms   | Distinct from player    | Must be immediately distinguishable from player shots |
| Enemy melee attack | 80–120ms   | Lower than player melee | Heavier, more threatening                             |
| Enemy hurt         | 60–100ms   | Mid                     | Satisfying feedback; brief                            |
| Enemy death        | 100–200ms  | Mid, popping            | Rewarding destruction sound; can vary per enemy type  |
| Boss entrance      | 500–1000ms | Low, ominous            | Rumble or roar; signals danger                        |
| Boss phase change  | 300–600ms  | Rising intensity        | Escalation cue                                        |
| Boss death         | 500–1500ms | Dramatic, multi-layered | Spectacular; should feel like an achievement          |

### Environment

| Event                       | Duration  | Pitch             | Style Notes                                   |
| --------------------------- | --------- | ----------------- | --------------------------------------------- |
| Door open / room transition | 200–400ms | Mechanical        | Thud, grind, or whoosh depending on aesthetic |
| Door locked (attempt)       | 100–200ms | Dull buzz/clunk   | Signals "not yet"                             |
| Chest open                  | 150–300ms | Ascending, bright | Reward sound with anticipation                |
| Destructible break          | 100–200ms | Mid, crunchy      | Satisfying crumble                            |
| Hazard (spikes, fire)       | 100–200ms | Sharp, alarming   | Should read as "danger" instantly             |
| Ambient loop (per area)     | Looping   | Low, atmospheric  | Subtle background texture; area-specific      |

### UI

| Event              | Duration         | Pitch                     | Style Notes                            |
| ------------------ | ---------------- | ------------------------- | -------------------------------------- |
| Menu navigate      | 20–40ms          | High, soft                | Subtle tick or blip                    |
| Menu confirm       | 40–80ms          | Rising, bright            | Decisive positive tone                 |
| Menu cancel / back | 40–80ms          | Descending, soft          | Gentle negative tone                   |
| Pause              | 50–100ms         | Mid, neutral              | Brief snap or tone that says "stopped" |
| Unpause            | 50–100ms         | Reverse of pause          | Continuation cue                       |
| Game over sting    | 500–1500ms       | Descending, somber        | Musical phrase; not just an effect     |
| Wave clear         | 200–400ms        | Ascending, triumphant     | Brief fanfare or chime                 |
| Score tick-up      | 20–40ms per tick | Ascending pitch per digit | Rapid-fire ticks as score increments   |

### Weapons (per weapon type — expand as arsenal grows)

| Event             | Duration  | Pitch           | Style Notes                                 |
| ----------------- | --------- | --------------- | ------------------------------------------- |
| Pistol fire       | 50–80ms   | Mid, sharp      | Baseline weapon sound                       |
| Shotgun fire      | 80–120ms  | Low, wide       | Bassy with spread feel                      |
| Machine gun fire  | 30–60ms   | Mid, staccato   | Tight, rapid; must not fatigue ears on loop |
| Laser fire        | 100–200ms | High, sustained | Beam-like with slight warble                |
| Grenade launch    | 80–120ms  | Low thump       | Hollow, projectile-in-flight feel           |
| Grenade explosion | 200–400ms | Low, booming    | Biggest explosion sound; bass-heavy         |

---

## 3. Music Structure

### Per-Area Themes

Each area of the game should have its own musical identity. Themes should loop
seamlessly and feel appropriate for extended play sessions (players may be in an
area for many minutes).

| Context           | Characteristics                                        | Loop Length     |
| ----------------- | ------------------------------------------------------ | --------------- |
| Title / main menu | Sets the mood; establishes the game's tone             | 60–120s         |
| Area 1 (starting) | Accessible, mid-tempo, builds confidence               | 90–180s         |
| Area 2            | Increased tension, slightly faster or darker           | 90–180s         |
| Area 3            | High intensity, urgency                                | 90–180s         |
| Shop / safe room  | Calm, restful contrast to combat areas                 | 30–60s          |
| Boss fight        | High energy, driving rhythm, distinct from area themes | 60–120s         |
| Game over         | Brief, somber; transitions to title or retry           | 5–15s (no loop) |
| Victory / credits | Triumphant, reflective                                 | 60–120s         |

### Looping

All music tracks (except game-over and victory stings) must loop seamlessly. The
loop point should be at the end of the file — SDL_mixer's `Mix_PlayMusic(-1)`
loops from the beginning. For OGG files, use the `LOOPSTART` and `LOOPLENGTH`
tags if a non-zero loop point is needed.

### Layering and Adaptive Music (Future)

SDL_mixer supports basic crossfading between tracks via `Mix_FadeInMusic` and
`Mix_FadeOutMusic`. For initial implementation, this is sufficient for scene
transitions.

More sophisticated approaches (vertical layering, horizontal re-sequencing,
stinger overlays) would require FMOD or Wwise. These are explicitly out of scope
for the initial implementation but should be considered if the game grows (see
[ADR-0010](decisions/0010-sdl2-mixer-audio.md)).

---

## 4. Technical Requirements

| Property        | Value           | Rationale                                                           |
| --------------- | --------------- | ------------------------------------------------------------------- |
| Sample rate     | 44100 Hz        | Matches `Mix_OpenAudio(44100, ...)` in `game.cpp:31`                |
| Format (SFX)    | WAV, 16-bit PCM | Zero decode latency; SDL_mixer loads as `Mix_Chunk`                 |
| Format (music)  | OGG Vorbis      | Streaming playback; small file size; SDL_mixer `Mix_Music`          |
| Channels        | Stereo (2ch)    | Matches `Mix_OpenAudio(..., 2, ...)` init                           |
| Audio buffer    | 2048 samples    | Matches `Mix_OpenAudio(..., 2048)` init; ~46ms latency              |
| Mixing channels | 16 (default)    | SDL_mixer default; increase with `Mix_AllocateChannels()` if needed |
| SFX loudness    | -16 to -12 LUFS | Consistent perceived volume across all effects                      |
| Music loudness  | -18 to -14 LUFS | Sits behind SFX in the mix                                          |
| Volume config   | `config.json`   | `audio.music_volume` (0–100), `audio.sfx_volume` (0–100)            |
| Peak ceiling    | -1 dBFS         | No clipping; leave headroom for mixing                              |

### Volume Mapping

The `config.json` volume values (0–100) map to SDL_mixer's 0–128 range:

```
sdl_volume = static_cast<int>(config_volume / 100.0 * MIX_MAX_VOLUME);
```

`Mix_VolumeMusic(sdl_volume)` for music, `Mix_VolumeChunk(chunk, sdl_volume)`
for individual SFX, or `Mix_Volume(channel, sdl_volume)` for a specific channel.

---

## 5. Naming Conventions

All audio files use **lowercase with underscores**. A category prefix identifies
the type at a glance.

### SFX

Pattern: `sfx_<category>_<action>[_<variant>].wav`

```
sfx_player_shoot.wav
sfx_player_shoot_charged.wav
sfx_player_dash.wav
sfx_player_hurt.wav
sfx_player_death.wav
sfx_enemy_shoot.wav
sfx_enemy_death_01.wav
sfx_enemy_death_02.wav
sfx_boss_entrance.wav
sfx_boss_phase_change.wav
sfx_ui_navigate.wav
sfx_ui_confirm.wav
sfx_env_door_open.wav
sfx_env_chest_open.wav
sfx_weapon_shotgun_fire.wav
sfx_weapon_laser_fire.wav
sfx_pickup_health.wav
sfx_pickup_weapon.wav
```

Numbered variants (`_01`, `_02`) are for randomized playback to reduce
repetition.

### Music

Pattern: `mus_<context>.ogg`

```
mus_title.ogg
mus_area_01.ogg
mus_area_02.ogg
mus_area_03.ogg
mus_boss.ogg
mus_shop.ogg
mus_gameover.ogg
mus_victory.ogg
```

---

## 6. Asset Directory Structure

```
assets/
└── audio/
    ├── sfx/
    │   ├── sfx_player_shoot.wav
    │   ├── sfx_player_dash.wav
    │   ├── sfx_enemy_death_01.wav
    │   └── ...
    └── music/
        ├── mus_title.ogg
        ├── mus_area_01.ogg
        └── ...
```

All audio assets live under `assets/audio/`, separated into `sfx/` and `music/`
subdirectories. This parallels the existing `assets/sprites/` directory for art
assets.

---

## 7. Quick Reference Card

| Property                 | Value                   |
| ------------------------ | ----------------------- |
| Sample rate              | 44100 Hz                |
| SFX format               | WAV, 16-bit PCM, stereo |
| Music format             | OGG Vorbis, stereo      |
| SFX prefix               | `sfx_`                  |
| Music prefix             | `mus_`                  |
| SFX directory            | `assets/audio/sfx/`     |
| Music directory          | `assets/audio/music/`   |
| Volume range (config)    | 0–100                   |
| Volume range (SDL_mixer) | 0–128 (MIX_MAX_VOLUME)  |
| Loudness target (SFX)    | -16 to -12 LUFS         |
| Loudness target (music)  | -18 to -14 LUFS         |
| Peak ceiling             | -1 dBFS                 |
| Mixing channels          | 16 (default)            |

---

## 8. Delivery Checklist

Run through this list before every audio asset handoff:

- [ ] Sample rate is 44100 Hz
- [ ] SFX are WAV, 16-bit PCM, stereo
- [ ] Music tracks are OGG Vorbis, stereo
- [ ] Filename follows naming convention (`sfx_` or `mus_` prefix, lowercase,
      underscores)
- [ ] File is in the correct directory (`assets/audio/sfx/` or
      `assets/audio/music/`)
- [ ] SFX duration is appropriate for the event (see category tables above)
- [ ] Music loops seamlessly (test with `Mix_PlayMusic(-1)`)
- [ ] Peak level does not exceed -1 dBFS
- [ ] Perceived loudness is within the target LUFS range
- [ ] No audible clicks, pops, or DC offset at loop points or file boundaries
- [ ] Variants (if any) are numbered sequentially (`_01`, `_02`, etc.)
- [ ] Audio plays correctly in-game at both low and high volume settings
