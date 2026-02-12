# 10. SDL_mixer for Audio

Date: 2026-02-11 Status: Superseded by [ADR-0013](0013-sdl3-migration.md)

> **Note:** SDL_mixer was removed during the SDL3 migration. No audio loading or
> playback code had been implemented — only init/shutdown calls existed. See
> [ADR-0013](0013-sdl3-migration.md) for the migration rationale and
> [Audio Integration Guide](../audio-integration.md) for the current audio path
> forward.

## Context

Raven needs audio for sound effects and music. The game currently initializes
SDL_mixer at startup (`Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048)` in
`src/core/game.cpp:31`) and shuts it down on exit (`Mix_CloseAudio()` in
`src/core/game.cpp:171`). The `config.json` already defines `audio.music_volume`
and `audio.sfx_volume` settings. SDL2_mixer is linked in CMakeLists.txt and
available through the Nix flake.

No audio is actually loaded or played yet. Before implementing playback, the
audio middleware choice needed to be explicit. The alternatives considered:

- **SDL_mixer** — Already linked and initialized. Simple channel-based API for
  loading WAV/OGG and playing SFX + music. Part of the SDL2 ecosystem.
- **FMOD** — Industry-standard middleware with adaptive music, real-time DSP,
  and a visual authoring tool (FMOD Studio). Free for indie projects under $200K
  revenue.
- **Wwise** — Another industry-standard middleware with sophisticated audio
  graphs. Free for indie projects under $150K budget. Steeper learning curve
  than FMOD.
- **Raw SDL_audio** — Low-level callback API. Maximum control but requires
  implementing mixing, decoding, and format handling from scratch.
- **OpenAL / OpenAL Soft** — 3D positional audio API. Overkill for a 2D top-down
  game. Would require an additional dependency.

## Decision

**Use SDL_mixer for the initial audio implementation.** It is already in the
dependency tree, initialized at startup, and provides a sufficient API for
non-adaptive sound effects and looping music. Evaluate FMOD later if adaptive
music (vertical layering, horizontal re-sequencing, dynamic mixing) becomes a
requirement.

## Consequences

**Positive:**

- Zero new dependencies — SDL_mixer is already linked and initialized
- Simple API: `Mix_LoadWAV` / `Mix_PlayChannel` for SFX, `Mix_LoadMUS` /
  `Mix_PlayMusic` for music
- Supports WAV (SFX) and OGG Vorbis (music) out of the box
- Basic crossfading via `Mix_FadeInMusic` / `Mix_FadeOutMusic`
- Well-documented, widely used, stable
- Keeps the project aligned with the "no third-party engine" philosophy
  ([ADR-0006](0006-no-third-party-engine.md)) — SDL_mixer is a natural SDL2
  extension, not an engine

**Negative:**

- No built-in adaptive music system (no vertical layers, no parameter-driven
  transitions, no stinger overlays)
- Limited DSP — no real-time reverb, filtering, or effects without manual
  implementation
- Only one `Mix_Music` stream at a time (cannot layer two music tracks)
- Migration to FMOD or Wwise later would require replacing all audio loading and
  playback calls, though an `AudioManager` wrapper pattern can isolate this
  behind a stable interface
- No visual authoring tool — all audio behavior is defined in code
