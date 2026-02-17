# Audio Integration Guide

Developer reference for integrating sound effects and music into the Raven
engine. Read the [Audio Specification](audio-spec.md) first for asset format
requirements.

---

## 1. Current State

**Audio playback is not currently implemented.** The SDL3 migration
([ADR-0013](decisions/0013-sdl3-migration.md)) removed SDL_mixer because a
stable SDL3 port (sdl3-mixer) is not yet available in nixpkgs. No audio loading
or playback code existed prior to the migration — only `Mix_OpenAudio` /
`Mix_CloseAudio` init/shutdown calls — so nothing functional was lost.

`SDL_INIT_AUDIO` has been removed from the `SDL_Init` call. The `config.json`
audio volume fields (`audio.music_volume`, `audio.sfx_volume`) remain and are
forward-compatible with any future audio backend.

The previous audio middleware decision
([ADR-0010](decisions/0010-sdl2-mixer-audio.md)) is superseded by ADR-0013.

---

## 2. Path Forward

There are three options for re-adding audio, which can be combined:

### Option A: sdl3-mixer (when stable)

SDL3_mixer (sdl3-mixer) is the direct successor to SDL2_mixer. When it reaches a
stable release and is packaged in nixpkgs, it can be added back as a dependency.
The API will be similar to SDL2_mixer:

- `Mix_OpenAudio` → device init
- `Mix_LoadWAV` → SFX loading (WAV chunks, fully in memory)
- `Mix_PlayChannel` → SFX playback on mixing channels
- `Mix_LoadMUS` / `Mix_PlayMusic` → music streaming (OGG Vorbis)
- `Mix_FadeInMusic` / `Mix_FadeOutMusic` → crossfading

This is the simplest path if SDL3_mixer stabilises before audio integration
begins. Re-adding it requires:

1. Add `sdl3-mixer` to `flake.nix` build inputs
2. Add `pkg_check_modules(sdl3-mixer ...)` to `Dependencies.cmake`
3. Link `SDL3_mixer::SDL3_mixer` in `CMakeLists.txt`
4. Add `SDL_INIT_AUDIO` back to the `SDL_Init` call in `game.cpp`
5. Implement the `AudioManager` wrapper (see section 4)

### Option B: SDL3 AudioStream (no mixer dependency)

SDL3's `SDL_AudioStream` provides a push-model audio API with built-in
resampling. This is viable for simple SFX playback without any external mixer
dependency.

The model:

1. Open an audio device with `SDL_OpenAudioDevice`
2. Create an `SDL_AudioStream` per sound effect or category
3. Push PCM data into the stream with `SDL_PutAudioStreamData`
4. SDL handles resampling and mixing to the output device

Advantages over SDL_mixer:

- No additional dependency — `SDL_AudioStream` is part of core SDL3
- Push model is simpler than SDL2's callback-based `SDL_AudioCallback`
- Built-in format conversion and resampling
- Multiple streams can feed the same device

Disadvantages:

- No built-in music streaming (must decode OGG frames manually or use a decoder
  library)
- No built-in channel management or spatial mixing
- More manual work for features SDL_mixer provides out of the box

This is a good fit if the game only needs simple SFX (shoot, hit, pickup) and
doesn't need streamed music yet.

### Option C: FMOD or Wwise (adaptive music)

If the game needs adaptive music — vertical layers, horizontal re-sequencing,
dynamic mixing based on gameplay state — the path is a professional middleware:

- **FMOD** — Industry-standard with a visual authoring tool (FMOD Studio). Free
  for indie projects under $200K revenue.
- **Wwise** — Sophisticated audio graphs and spatial audio. Free for indie
  projects under $150K budget. Steeper learning curve.

Both integrate as C/C++ libraries and are independent of SDL's audio system. The
`AudioManager` wrapper pattern (section 4) isolates the middleware choice from
game systems.

When to consider this path:

- Music needs to react to gameplay in real time (e.g., adding instrument layers
  as intensity increases)
- Complex DSP effects are needed (reverb zones, occlusion, real-time pitch
  shifting)
- A dedicated sound designer joins the project and needs a visual authoring tool

See the [Audio Resources](audio-resources.md) page for FMOD and Wwise details.

---

## 3. Asset Pipeline (unchanged)

The asset directory structure and format requirements are the same regardless of
which audio backend is chosen:

```
assets/audio/sfx/*.wav          WAV files for sound effects
assets/audio/music/*.ogg        OGG files for music
```

- **SFX** — 16-bit 44100 Hz mono WAV. Loaded fully into memory.
- **Music** — OGG Vorbis, stereo, 44100 Hz. Streamed from disk.

The `config.json` volume settings apply to any backend:

```json
"audio": {
    "music_volume": 80,
    "sfx_volume": 100
}
```

Volume values are 0–100 and should be mapped to the backend's native range
during initialisation.

---

## 4. AudioManager Wrapper Pattern

Regardless of which backend is chosen, audio calls should go through a thin
wrapper rather than being scattered through ECS systems. This isolates the
middleware choice and centralises volume mapping, caching, and error logging.

```cpp
class AudioManager {
public:
    void play_sfx(const std::string& id);
    void play_music(const std::string& id, int fade_ms = 0);
    void stop_music(int fade_ms = 0);
    void set_sfx_volume(int volume_0_100);
    void set_music_volume(int volume_0_100);
    void unload_all();
};
```

The `AudioManager` can be stored in the EnTT registry context alongside the
existing `SpriteSheetManager`:

```cpp
auto& audio = registry_.ctx().emplace<AudioManager>();
```

Systems call `audio.play_sfx("sfx_player_shoot")` rather than touching the
backend directly. When the backend changes (e.g., from SDL3_mixer to FMOD), only
the `AudioManager` implementation changes — not the call sites.

---

## 5. Event-Driven Audio Design

Audio playback should be triggered from ECS systems in response to game events.
The pattern: a system detects a state change, then calls the audio manager.

### Collision system → hit sounds

```cpp
void resolve_collisions(entt::registry& reg) {
    auto& audio = reg.ctx().get<AudioManager>();

    // When a bullet hits an enemy:
    audio.play_sfx("sfx_enemy_hurt");

    // When the player takes damage:
    audio.play_sfx("sfx_player_hurt");
}
```

### Weapon system → shot sounds

```cpp
void update_shooting(entt::registry& reg) {
    auto& audio = reg.ctx().get<AudioManager>();

    // When a shot is fired:
    audio.play_sfx("sfx_player_shoot");
}
```

### Scene transitions → music changes

```cpp
void GameScene::on_enter() {
    auto& audio = registry_.ctx().get<AudioManager>();
    audio.play_music("mus_area_01", 500);  // 500ms fade-in
}

void TitleScene::on_enter() {
    auto& audio = registry_.ctx().get<AudioManager>();
    audio.play_music("mus_title", 1000);
}
```

These patterns are backend-agnostic. The system code stays the same whether
`AudioManager` uses SDL3_mixer, `SDL_AudioStream`, or FMOD internally.

---

## 6. Implementation Priority

The recommended order when audio work begins:

1. **Choose a backend** — Check if sdl3-mixer is stable. If yes, use it (option
   A). If not, evaluate whether simple SFX via `SDL_AudioStream` (option B) is
   sufficient for the current milestone.
2. **Implement `AudioManager`** with SFX loading and playback.
3. **Add SFX triggers** to the collision, shooting, and pickup systems.
4. **Add music playback** with scene-based track changes and crossfading.
5. **Evaluate adaptive music** needs (option C) once the base gameplay loop is
   polished.
