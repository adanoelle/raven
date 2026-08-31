# Music Track List

The brief for Raven's soundtrack: one row per track with mood, tempo,
key, and loop targets. Read alongside the
[Audio Specification](audio-spec.md) (formats, loudness, naming) — this page
is *what to write*, that page is *the technical format it lives in*.

**Status:** music playback is not yet implemented in the engine
([ADR-0019](decisions/0019-sdl3-native-audio.md) scoped it to sound effects;
the [Audio Integration Guide](audio-integration.md) describes the plan).
Composition can start now — **file names do the wiring**. Tracks dropped
into `assets/audio/music/` under the names below will be picked up when the
streaming work lands, with no renaming.

---

## 1. Track List

BPM and key values are *targets*, not laws: they exist so the soundtrack hangs
together across scene transitions and so intensity ramps stage over stage. If
a track works better a few BPM off or in a related key, follow the music.

| #   | File              | Where it plays                | Mood                                                    | BPM     | Key      | Loop length |
| --- | ----------------- | ----------------------------- | ------------------------------------------------------- | ------- | -------- | ----------- |
| 1   | `mus_title.ogg`   | Title screen, character select | Brooding, spacious; establishes the game's tone. Restraint over energy — the run hasn't started yet. | 90–100  | A minor  | 60–120s     |
| 2   | `mus_area_01.ogg` | Stage 1 rooms                 | Accessible, mid-tempo, confident. The player is learning; the music should invite, not pressure.     | 118–124 | A minor  | 90–180s     |
| 3   | `mus_area_02.ogg` | Stage 2 rooms                 | Darker and tenser than Area 1. Same sonic family, tighter pulse.                                      | 126–132 | D minor  | 90–180s     |
| 4   | `mus_area_03.ogg` | Stage 3 rooms                 | Urgent, high intensity. The run's climax short of a boss.                                             | 138–144 | E minor  | 90–180s     |
| 5   | `mus_boss.ogg`    | Boss encounters *(planned)*   | Driving, relentless; clearly distinct from area themes.                                               | 150–160 | E minor  | 60–120s     |
| 6   | `mus_shop.ogg`    | Shop / safe rooms *(planned — [ADR-0014](decisions/0014-npc-shop-progression.md))* | Calm respite; warm contrast to combat. | 70–84   | C major  | 30–60s      |
| 7   | `mus_game_over.ogg` | Game over screen            | Somber, descending sting. A musical phrase, not just a sound effect.                                  | free    | A minor  | 5–15s, **no loop** |
| 8   | `mus_victory.ogg` | Victory screen                | Triumphant, then reflective — the player just cleared a full run.                                     | 105–115 | A major  | 60–120s     |

Notes on the set:

- **Key plan.** Everything orbits A minor and its neighbors (D minor, E minor,
  C major, A major) so hard cuts between scenes never feel like a wrong-note
  splice. The victory theme's shift to A major is the deliberate payoff.
- **Intensity ramp.** Areas 1 → 3 step up in tempo and density. A player should
  be able to feel which stage they're in with their eyes closed.
- **Extended listening.** Area themes play for many minutes at a time. Aim for
  loops that breathe — a texture that thrills for 30 seconds can exhaust in
  five minutes.
- **Planned tracks** (#5, #6): the boss encounter and shop rooms are designed
  but not yet in the game. They're safe to write — the specs won't shift under
  you — or to defer until last.

## 2. Priority Order

Writing one track at a time, the useful order is: **2, 1, 7, 8, 3, 4, 5,
6.** Area 1 and the title
theme define the game's sound; the game-over and victory pieces are short;
areas 2–3 extend the established palette; boss and shop land with their
features.

---

## 3. Looping

- **Default: seamless whole-file loop.** The engine will initially loop from
  the end of the file straight back to the start. The joint must be
  click-free and musically continuous — no reverb/delay tails cut off at the
  boundary (bake the tail into the start of the loop if needed).
- **Intro-then-loop (optional).** If a track wants a one-time intro before the
  repeating section, use `LOOPSTART` and `LOOPLENGTH` Vorbis comment tags with
  **sample offsets**, and note the values alongside the file. The streaming
  implementation will honor them; until then whole-file looping is the
  fallback, so the track should still survive an end-to-start jump.
- **Stings don't loop.** `mus_game_over.ogg` plays once and ends in silence.

## 4. File Format

Formats and levels are specified in the
[Audio Specification](audio-spec.md#4-technical-requirements); the short
version:

- OGG Vorbis, stereo, 44100 Hz (quality ~q5–q7 is plenty at this fidelity)
- Loudness −18 to −14 LUFS integrated; peaks ≤ −1 dBFS
- Files in `assets/audio/music/`, named exactly as in the table above
- No leading silence; no trailing silence on looping tracks

### Music pre-flight checklist

- [ ] File is OGG Vorbis, stereo, 44100 Hz
- [ ] Named exactly per the track list (`mus_<context>.ogg`)
- [ ] Integrated loudness −18 to −14 LUFS; true peak ≤ −1 dBFS
- [ ] Loop point is click-free (listen across the joint at least 5 times)
- [ ] Track survives 10+ minutes of continuous looping without fatiguing
- [ ] Reverb/delay tails don't get cut at the loop boundary
- [ ] `LOOPSTART` / `LOOPLENGTH` tags present *and documented* if an intro is
      used
- [ ] Game-over sting ends cleanly in silence (no loop)
- [ ] No leading silence on any track

---

## 5. Scene → Track Mapping (engine reference)

For the developer wiring music playback later:

| Scene                        | Track               | Transition               |
| ---------------------------- | ------------------- | ------------------------ |
| `TitleScene`, `CharSelectScene` | `mus_title.ogg`  | Fade in ~1000ms          |
| `GameScene` (stage 1/2/3)    | `mus_area_0N.ogg`   | Crossfade ~500ms on stage change; keep playing across rooms within a stage |
| Boss room *(planned)*        | `mus_boss.ogg`      | Crossfade ~500ms         |
| Shop room *(planned)*        | `mus_shop.ogg`      | Crossfade ~500ms         |
| `GameOverScene`              | `mus_game_over.ogg` | Hard cut from area theme |
| `VictoryScene`               | `mus_victory.ogg`   | Short fade, ~300ms       |
| `PauseScene`, `OptionsScene` | *(no change)*       | Overlays keep the current track; ducking is a possible later polish |
