# Raven Audio Specification

Reference document for sound effects, music, technical requirements, file
formats, and naming conventions.

---

## 1. Sonic Identity (Undecided)

The game's sonic direction has not been finalized. Three viable approaches are
presented here so the decision can be made later — ideally with a composer —
with full context. The choice is independent of the audio backend: sound
effects already play through native SDL3 audio
([ADR-0019](decisions/0019-sdl3-native-audio.md)), and all three directions
deliver the same WAV/OGG formats.

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

### Working plan: Option C with hardware processing

The direction is still open, but the current lean is the hybrid, with one
specific twist: parts are written "in the box" and then played through a
hardware effects pedal from Hologram Electronics (granular textures,
tape-style wobble, degraded repeats) for a sound that is digital in origin
but worn and organic in character. A friend of the project who worked at
Hologram has the pedals and the experience to run them. Nothing is agreed
yet — this section records the plan so it can be tested.

**Division of work.** Composition stays in the project: parts are written in
a sequencer or DAW (see [Audio Learning Resources](audio-resources.md)) and
exported as separate stems. The collaborator plays those stems through the
pedal and returns the recordings ("re-amping"). The same route works for
sound effects — a clean metallic hit through the pedal is a candidate for the
signature `disarm.wav`.

**Test first.** Before committing, send one short loop (about 16 bars of a
chip-style melody at an Area 1 tempo) for a single pass through the pedal.
That one clip answers both open questions: whether the sound is right for
Raven, and whether the looping approach below holds up.

**Looping.** These effects produce long tails and partly random textures, so
the end of a pass does not match its start. Record every looping part
**played twice in a row and keep the second pass**: it already contains the
tail of the first, so the end-to-start joint is continuous. Cut to a
sample-exact length computed from BPM and bar count. This satisfies the
"bake the tail into the start of the loop" rule in the
[Music Track List](music-track-list.md#3-looping).

**Deliverables to ask for.**

- Processed (wet) stems, one file per part, plus the untouched (dry) stems —
  wet/dry pairs allow rebalancing later and keep adaptive layering possible
- WAV or FLAC at the recording's native rate and bit depth; conversion to the
  shipping formats in section 4 happens in the project, not at the source
- A note of the pedal and settings per take, so a part can be redone

**Terms.** Agree in writing before any recording is used, even informally:
who owns the recordings, how the collaborator is credited, and whether there
is payment or a revenue share. The game is headed for Steam and possibly
console, and this is far easier to settle before release than after.

---

## 2. Sound Effects Catalog

Every entry below corresponds to a mechanic that exists in the game today.
The catalog is split into **shipping** effects (synthesized placeholders in
the tree — replacing the file replaces the sound) and **needed** effects
(the mechanic fires, but no sound plays yet). For *when* each sound triggers
and what it must communicate, read the
[Game Feel — Feedback Anchors](game-feel.md) page; this table is the
spec sheet.

### Shipping today (replace the placeholder)

These seven effects play in the game now. A WAV saved under the **exact
file name** supersedes the placeholder with zero code changes.

| File               | Event                                | Duration  | Pitch            | Style Notes                                                         |
| ------------------ | ------------------------------------ | --------- | ---------------- | ------------------------------------------------------------------- |
| `shoot.wav`        | Player fires (also charged release)  | 50–100ms  | Mid-high         | Snappy, satisfying pop; must not fatigue at ~5 shots/second         |
| `melee.wav`        | Melee swing (on button press)        | 80–120ms  | Mid, sweeping    | Whoosh with slight metallic edge; the *swing*, not the connect      |
| `dash.wav`         | Dash start                           | 60–100ms  | Mid-high, rising | Quick swoosh; conveys speed                                         |
| `pickup.wav`       | Weapon **or** stabilizer collected   | 80–120ms  | Ascending        | Positive, rewarding click-chime (split into two sounds — see below) |
| `player_hit.wav`   | Player takes bullet/contact damage   | 100–150ms | Low-mid, harsh   | Unpleasant enough to feel bad, brief enough to not annoy            |
| `enemy_hit.wav`    | Enemy takes bullet damage            | 60–100ms  | Mid              | Satisfying feedback; plays constantly, so keep it light             |
| `enemy_down.wav`   | Enemy dies                           | 100–200ms | Mid, popping     | Rewarding destruction sound                                         |

### Needed — mechanic exists, currently silent

These events happen in the game but have no sound. Each needs a one-line
registration by a developer (see the
[Audio Integration Guide](audio-integration.md#3-adding-a-new-sound-effect)),
so agree on the file name up front. Suggested names below.

| Suggested file          | Event                                          | Duration   | Pitch                  | Style Notes                                                                 |
| ----------------------- | ---------------------------------------------- | ---------- | ---------------------- | --------------------------------------------------------------------------- |
| `disarm.wav`            | Melee knocks a weapon out of an enemy          | 80–150ms   | Metallic clang         | **The game's signature reward moment** — must cut through everything        |
| `melee_connect.wav`     | Melee swing actually hits flesh                | 60–100ms   | Mid, punchy            | Crisp impact; distinct from bullet `enemy_hit`                              |
| `ground_slam.wav`       | Brawler's AoE slam (50px radius)               | 150–300ms  | Low, booming           | Heaviest player sound; pairs with knockback shove                           |
| `concussion_shot.wav`   | Sharpshooter's AoE blast (45px radius)         | 100–200ms  | Mid whump + airy ring  | More "air pressure" than explosion — it pushes, barely damages              |
| `charge_ready.wav`      | Charge crosses full threshold (shot will pierce)| 60–100ms  | High, clear ping       | Tells the player "release now"; matches the HUD bar turning yellow          |
| `decay_warning.wav`     | Stolen weapon's 10s timer near expiry          | 30–60ms    | Dry tick/beep          | Repeating countdown feel over the final ~3 seconds                          |
| `decay_explosion.wav`   | Stolen weapon explodes on the player           | 200–400ms  | Low, booming           | The loop's punishment beat; biggest explosion in the game                   |
| `stabilizer.wav`        | Stabilizer collected — weapon now permanent    | 100–200ms  | Ascending, resolving   | "Locked in" — warmer and more final than `pickup.wav`                       |
| `player_death.wav`      | Player loses a life                            | 200–400ms  | Descending             | Dramatic; can layer a brief reverb tail                                     |
| `wave_clear.wav`        | Last enemy of final wave dies, exits open      | 200–400ms  | Ascending, triumphant  | Brief fanfare; also marks the exit becoming usable                          |
| `enemy_shoot.wav`       | Enemy bullet pattern fires                     | 50–100ms   | Distinct from player   | Must be immediately distinguishable from player shots; fires *very* often   |
| `boss_entrance.wav`     | Boss wave spawns (stage 3, wave 3)             | 500–1000ms | Low, ominous           | Rumble or roar; signals danger                                              |
| `boss_death.wav`        | Boss dies                                      | 500–1500ms | Dramatic, layered      | Spectacular; should feel like an achievement                                |

### UI

| Suggested file      | Event              | Duration   | Pitch              | Style Notes                            |
| ------------------- | ------------------ | ---------- | ------------------ | -------------------------------------- |
| `ui_navigate.wav`   | Menu navigate      | 20–40ms    | High, soft         | Subtle tick or blip                    |
| `ui_confirm.wav`    | Menu confirm       | 40–80ms    | Rising, bright     | Decisive positive tone                 |
| `ui_back.wav`       | Menu cancel / pause| 40–80ms    | Descending, soft   | Gentle negative tone                   |

The game-over moment is covered by the `mus_game_over.ogg` sting in the
[Music Track List](music-track-list.md), not an SFX.

### A note on weapon variety

There is no fixed arsenal of pistols and shotguns. Stolen weapons are
**enemy bullet patterns** (`spiral_3way`, `aimed_burst`, `nova_legendary`,
and future patterns defined in JSON), all currently firing with `shoot.wav`.
Per-pattern or per-tier fire sounds (a Legendary weapon *sounding* dangerous)
are a desirable future refinement — raise it when the core set above is done.

---

## 3. Music Structure

### Per-Area Themes

Each area of the game should have its own musical identity. Themes should loop
seamlessly and feel appropriate for extended play sessions (players may be in an
area for many minutes).

The concrete per-track brief — file names, moods, BPM and key targets, and the
music pre-flight checklist — is in the [Music Track List](music-track-list.md).

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

All music tracks (except the game-over sting) must loop seamlessly. The default
is a whole-file loop — the engine repeats from the beginning when the file
ends. For OGG files, use the `LOOPSTART` and `LOOPLENGTH` tags (sample offsets)
if a non-zero loop point is needed; see the
[Music Track List](music-track-list.md#3-looping) for details.

### Layering and Adaptive Music (Future)

For the initial implementation, simple crossfades between tracks on scene
transitions are sufficient (see the
[Audio Integration Guide](audio-integration.md) for the planned music backend).

More sophisticated approaches (vertical layering, horizontal re-sequencing,
stinger overlays) would require FMOD or Wwise. These are explicitly out of scope
for the initial implementation but should be considered if the game grows (see
[ADR-0019](decisions/0019-sdl3-native-audio.md)).

---

## 4. Technical Requirements

Sound effects play on **native SDL3 audio** — no mixer library
([ADR-0019](decisions/0019-sdl3-native-audio.md)). The engine
(`src/audio/audio_engine.hpp`) binds one SDL audio stream per playing
instance and lets SDL mix them.

| Property         | Value            | Rationale                                                              |
| ---------------- | ---------------- | ---------------------------------------------------------------------- |
| Sample rate      | 44100 Hz         | SDL converts on load, but authoring at 44100 avoids resampling artifacts |
| Format (SFX)     | WAV, 16-bit PCM  | Zero decode latency; loaded whole at startup                           |
| Format (music)   | OGG Vorbis       | For the planned streaming path — see the [integration guide](audio-integration.md#5-music-path-forward) |
| Channels         | Stereo (2ch)     | Mono is accepted and converted, but stereo keeps the set consistent    |
| Simultaneous voices | 32 (`MAX_VOICES`) | Further plays in the same moment are dropped, not queued            |
| Same-tick dedupe | Yes              | N identical effects in one tick play **once** (prevents amplitude stacking) |
| SFX loudness     | -16 to -12 LUFS  | Consistent perceived volume across all effects                         |
| Music loudness   | -18 to -14 LUFS  | Sits behind SFX in the mix                                             |
| Peak ceiling     | -1 dBFS          | No clipping; leave headroom for mixing                                 |

### Volume

User volume settings live in `settings.json` under the platform preference
path ([ADR-0017](decisions/0017-settings-pref-path.md)) — **not** in
`config.json`, which is read-only game data. `sfx_volume` and `music_volume`
are 0–100 sliders in the options menu; the engine maps them through a
quadratic gain curve (`volume_to_gain()` in `src/core/game.cpp`) so the
slider feels perceptually even. `music_volume` is stored and displayed but
drives nothing until music playback lands.

### Loudness of short effects

The LUFS targets above cannot be applied literally to most of the catalog.
Integrated loudness (EBU R 128) is measured over 400 ms blocks, so a sound
shorter than that has **no integrated reading** — tools report -70 LUFS or
"-inf", and single-pass normalizers such as ffmpeg's `loudnorm` leave the
file effectively unchanged. Even measured generously, a ~100 ms burst peaking
at the -1 dBFS ceiling reads only about -16 LUFS, so the upper part of the
SFX range is unreachable for 50–100 ms effects without heavy limiting.

Until the targets are restated, treat them this way:

| Effect length   | How to level it                                                                 |
| --------------- | ------------------------------------------------------------------------------- |
| 400 ms or more  | Integrated LUFS, as in the table above                                          |
| Under 400 ms    | Match **peak and RMS within a family** (shots, hits, UI, pickups), then confirm by ear in-game against a reference effect |

For a single comparable number on a short file, pad it with 0.5 s of silence
and read the **maximum momentary** loudness:

```bash
ffmpeg -nostats -i in.wav \
  -af "apad=pad_dur=0.5,ebur128=metadata=1,ametadata=mode=print:key=lavfi.r128.M:file=-" \
  -f null - 2>/dev/null | grep -oE "lavfi.r128.M=-?[0-9.]+" | cut -d= -f2 | sort -n | tail -1
```

Open item: restate the SFX target in terms of maximum momentary loudness (or
per-family peak/RMS) once a handful of final effects exist to calibrate
against.

Practical consequence for sound design: because the same master gain applies
to every effect, **relative loudness between effects must be baked into the
files** — hit the LUFS targets rather than relying on per-sound mixing.

---

## 5. Naming Conventions

All audio files use **lowercase with underscores**. Sound effects and music
live in separate directories, so no `sfx_` prefix is used on effect files —
the directory conveys the type.

### SFX

Effect file names are keys in the game's sound manifest
(`assets/data/config.json`, `"sounds"` map). Two cases:

- **Replacing a shipping placeholder** — save under the *exact* manifest
  name (`shoot.wav`, `melee.wav`, `dash.wav`, `pickup.wav`,
  `player_hit.wav`, `enemy_hit.wav`, `enemy_down.wav`). The file drops into
  `assets/audio/sfx/` and supersedes the placeholder with zero code changes.
- **A new effect** — the name must also be registered in the manifest and
  the `Sfx` enum (a one-line change each — see the
  [Audio Integration Guide](audio-integration.md#3-adding-a-new-sound-effect)).
  Use the suggested names from the catalog in section 2, or pick a name
  together first.

Numbered variants (`_01`, `_02`) for randomized playback are a planned
engine feature — name variants with numbered suffixes and the random
selection gets wired when the feature lands.

### Music

Pattern: `mus_<context>.ogg`

```
mus_title.ogg
mus_area_01.ogg
mus_area_02.ogg
mus_area_03.ogg
mus_boss.ogg
mus_shop.ogg
mus_game_over.ogg
mus_victory.ogg
```

---

## 6. Asset Directory Structure

```
assets/
└── audio/
    ├── sfx/
    │   ├── shoot.wav
    │   ├── dash.wav
    │   ├── enemy_down.wav
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

| Property                 | Value                                    |
| ------------------------ | ---------------------------------------- |
| Sample rate              | 44100 Hz                                 |
| SFX format               | WAV, 16-bit PCM, stereo                  |
| Music format             | OGG Vorbis, stereo                       |
| SFX naming               | Bare manifest key (`shoot.wav`)          |
| Music prefix             | `mus_`                                   |
| SFX directory            | `assets/audio/sfx/`                      |
| Music directory          | `assets/audio/music/`                    |
| Volume settings          | `settings.json` (pref path), 0–100       |
| Loudness target (SFX)    | -16 to -12 LUFS                          |
| Loudness target (music)  | -18 to -14 LUFS                          |
| Peak ceiling             | -1 dBFS                                  |
| Simultaneous voices      | 32                                       |

---

## 8. Pre-flight Checklist

A quick self-check before a sound goes into the game:

- [ ] Sample rate is 44100 Hz
- [ ] SFX are WAV, 16-bit PCM, stereo
- [ ] Music tracks are OGG Vorbis, stereo
- [ ] SFX filename matches the sound manifest exactly (replacement), or the
      name is agreed and registered (new effect); music uses the `mus_` prefix
- [ ] File is in the correct directory (`assets/audio/sfx/` or
      `assets/audio/music/`)
- [ ] SFX duration is appropriate for the event (see catalog above)
- [ ] Music loops seamlessly across the end-to-start joint
- [ ] Peak level does not exceed -1 dBFS
- [ ] Perceived loudness is within the target LUFS range (effects under
      400 ms: matched to their family by peak/RMS and by ear — see
      [Loudness of short effects](#loudness-of-short-effects))
- [ ] No audible clicks, pops, or DC offset at loop points or file boundaries
- [ ] Variants (if any) are numbered sequentially (`_01`, `_02`, etc.)
- [ ] Audio plays correctly in-game at both low and high volume settings
