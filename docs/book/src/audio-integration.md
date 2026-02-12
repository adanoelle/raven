# Audio Integration Guide

Developer reference for integrating sound effects and music into the Raven engine.
Read the [Audio Specification](audio-spec.md) first for asset format requirements.

---

## 1. Pipeline Overview

Data flows from audio files on disk through config and SDL_mixer to the speakers:

```
assets/audio/sfx/*.wav          WAV files for sound effects
assets/audio/music/*.ogg        OGG files for music
        │
        ▼
assets/data/config.json         audio.music_volume, audio.sfx_volume (0–100)
        │
        ▼
Game::init()                    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048)
        │
        ▼
Audio loading                   Mix_LoadWAV() for SFX, Mix_LoadMUS() for music
        │
        ▼
ECS systems / scene logic       Trigger playback in response to game events
        │
        ▼
Mix_PlayChannel() / Mix_PlayMusic()   SDL_mixer mixes and sends to audio device
        │
        ▼
Speakers
```

**Key files:**

| File | Role |
|------|------|
| `src/core/game.cpp:31` | `Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048)` — audio device init |
| `src/core/game.cpp:171` | `Mix_CloseAudio()` — shutdown |
| `src/core/game.cpp:21` | `SDL_INIT_AUDIO` flag in `SDL_Init()` |
| `assets/data/config.json:14–17` | `audio.music_volume` (80), `audio.sfx_volume` (100) |
| `CMakeLists.txt:79` | `SDL2_mixer::SDL2_mixer` linked |

---

## 2. Current Engine State

SDL_mixer is already initialized and torn down. Here is exactly what exists today:

### Initialization (`src/core/game.cpp`)

```cpp
// Line 21: SDL_INIT_AUDIO included in SDL_Init flags
SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER);

// Line 31: Audio device opened
Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
```

Parameters: 44100 Hz sample rate, default format (16-bit signed), 2 channels
(stereo), 2048-sample buffer (~46ms latency at 44100 Hz).

### Shutdown (`src/core/game.cpp`)

```cpp
// Line 171: Audio device closed during Game::shutdown()
Mix_CloseAudio();
```

### Configuration (`assets/data/config.json`)

```json
"audio": {
    "music_volume": 80,
    "sfx_volume": 100
}
```

These values are read but not yet applied to SDL_mixer. The volume mapping (0–100
to SDL_mixer's 0–128) is described in the audio spec.

### Build system (`CMakeLists.txt`)

SDL2_mixer is already a linked dependency. No CMake changes are needed to start
using the audio API.

### Dev shell (`flake.nix`)

Audacity and ffmpeg are available in the Nix dev shell for audio editing and
format conversion.

---

## 3. SFX Playback Pattern

SDL_mixer uses `Mix_Chunk` for short sound effects. These are loaded entirely into
memory and played on one of the available mixing channels.

### Loading a sound effect

```cpp
#include <SDL_mixer.h>

// Load a WAV file into memory
Mix_Chunk* chunk = Mix_LoadWAV("assets/audio/sfx/sfx_player_shoot.wav");
if (!chunk) {
    spdlog::error("Failed to load SFX: {}", Mix_GetError());
}
```

`Mix_LoadWAV` accepts WAV, OGG, and other formats despite the name. For SFX, WAV
is preferred for zero decode latency.

### Playing a sound effect

```cpp
// Play on the first available channel, no looping
int channel = Mix_PlayChannel(-1, chunk, 0);
if (channel == -1) {
    spdlog::warn("No free channel for SFX: {}", Mix_GetError());
}
```

| Parameter | Value | Meaning |
|-----------|-------|---------|
| channel | `-1` | Let SDL_mixer pick the first free channel |
| chunk | pointer | The loaded `Mix_Chunk` |
| loops | `0` | Play once (use `-1` for infinite loop) |

### Setting volume

```cpp
// Set volume on a specific chunk (0–128)
Mix_VolumeChunk(chunk, 100);

// Or set volume on a specific channel
Mix_Volume(channel, 100);
```

### Freeing a sound effect

```cpp
Mix_FreeChunk(chunk);
chunk = nullptr;
```

All chunks must be freed before `Mix_CloseAudio()` is called during shutdown.

### Lifetime management

Sound effects should be loaded once (at startup or scene entry) and reused. A
simple approach:

```cpp
// In a header or manager class
std::unordered_map<std::string, Mix_Chunk*> sfx_cache;

Mix_Chunk* load_sfx(const std::string& path) {
    auto it = sfx_cache.find(path);
    if (it != sfx_cache.end()) {
        return it->second;
    }
    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (chunk) {
        sfx_cache[path] = chunk;
    }
    return chunk;
}

void unload_all_sfx() {
    for (auto& [path, chunk] : sfx_cache) {
        Mix_FreeChunk(chunk);
    }
    sfx_cache.clear();
}
```

---

## 4. Music Playback Pattern

SDL_mixer uses `Mix_Music` for streamed audio. Only one music track plays at a
time — this is a limitation (and simplification) of SDL_mixer.

### Loading music

```cpp
Mix_Music* music = Mix_LoadMUS("assets/audio/music/mus_area_01.ogg");
if (!music) {
    spdlog::error("Failed to load music: {}", Mix_GetError());
}
```

### Playing music

```cpp
// Play with infinite looping
Mix_PlayMusic(music, -1);

// Or fade in over 1 second
Mix_FadeInMusic(music, -1, 1000);
```

| Parameter | Value | Meaning |
|-----------|-------|---------|
| music | pointer | The loaded `Mix_Music` |
| loops | `-1` | Loop forever (`0` = play once, `n` = play n times) |
| ms | `1000` | Fade-in duration in milliseconds |

### Crossfading between tracks

SDL_mixer does not have a built-in crossfade. Simulate it by fading out the
current track, then fading in the new one:

```cpp
void crossfade_music(Mix_Music* next, int fade_ms) {
    Mix_FadeOutMusic(fade_ms);
    // After fade completes, start the new track
    // Option 1: Poll Mix_FadingMusic() in the game loop
    // Option 2: Use Mix_HookMusicFinished() callback
    // For simplicity, queue the next track after fade_ms
}
```

A practical approach is to track the pending music in a variable and check
`Mix_PlayingMusic()` each frame:

```cpp
// In the game loop or an audio system
if (pending_music_ && !Mix_PlayingMusic()) {
    Mix_FadeInMusic(pending_music_, -1, 500);
    pending_music_ = nullptr;
}
```

### Volume control

```cpp
// Set music volume (0–128)
Mix_VolumeMusic(volume);

// Map from config (0–100) to SDL_mixer (0–128)
int sdl_vol = static_cast<int>(config_volume / 100.0 * MIX_MAX_VOLUME);
Mix_VolumeMusic(sdl_vol);
```

### Freeing music

```cpp
Mix_FreeMusic(music);
music = nullptr;
```

The current track must be stopped before freeing: `Mix_HaltMusic()` then
`Mix_FreeMusic()`.

---

## 5. Event-Driven Audio Design

Audio playback should be triggered from ECS systems in response to game events.
The pattern: a system detects a state change, then calls the audio API.

### Collision system → hit sounds

```cpp
void resolve_collisions(entt::registry& reg) {
    // ... existing collision detection ...

    // When a bullet hits an enemy:
    play_sfx("assets/audio/sfx/sfx_enemy_hurt.wav");

    // When the player takes damage:
    play_sfx("assets/audio/sfx/sfx_player_hurt.wav");
}
```

### Weapon system → shot sounds

```cpp
void update_shooting(entt::registry& reg) {
    // ... existing weapon fire logic ...

    // When a shot is fired:
    play_sfx("assets/audio/sfx/sfx_player_shoot.wav");
}
```

### Scene transitions → music changes

```cpp
void GameScene::on_enter() {
    // Start area music when entering a game scene
    crossfade_music(load_music("assets/audio/music/mus_area_01.ogg"), 500);
}

void TitleScene::on_enter() {
    // Title music
    crossfade_music(load_music("assets/audio/music/mus_title.ogg"), 1000);
}
```

### Audio manager pattern

Rather than scattering SDL_mixer calls through systems, consider a thin wrapper:

```cpp
class AudioManager {
public:
    void play_sfx(const std::string& path);
    void play_music(const std::string& path, int fade_ms = 0);
    void stop_music(int fade_ms = 0);
    void set_sfx_volume(int volume_0_100);
    void set_music_volume(int volume_0_100);
    void unload_all();

private:
    std::unordered_map<std::string, Mix_Chunk*> sfx_cache_;
    Mix_Music* current_music_ = nullptr;
};
```

This centralizes volume mapping, caching, and error logging. Systems call
`audio.play_sfx("sfx_player_shoot")` rather than touching SDL_mixer directly. The
`AudioManager` can be stored in the EnTT registry context alongside the existing
`SpriteSheetManager`.

---

## 6. Future Middleware Path

If the game outgrows SDL_mixer's capabilities — particularly around adaptive music
(vertical layers, horizontal re-sequencing, dynamic mixing based on gameplay
state) — the natural upgrade path is **FMOD** or **Wwise**.

### When to consider migrating

- Music needs to react to gameplay in real time (e.g., adding instrument layers as
  intensity increases)
- Complex DSP effects are needed (reverb zones, occlusion, real-time pitch
  shifting)
- The number of simultaneous sounds exceeds SDL_mixer's comfortable range
- The project needs a visual audio authoring tool for a dedicated sound designer

### What migration looks like

1. Replace `Mix_Chunk` / `Mix_Music` loading with FMOD's `System::createSound()`
2. Replace `Mix_PlayChannel()` / `Mix_PlayMusic()` with FMOD channel playback
3. Move crossfade and adaptive logic into FMOD Studio events (authored in the
   FMOD Studio GUI rather than code)
4. The `AudioManager` wrapper pattern makes this easier — only the internal
   implementation changes, not the call sites in systems

Both FMOD and Wwise offer free licenses for indie projects under revenue
thresholds. See the [Audio Resources](audio-resources.md) page for details.

For the initial implementation, SDL_mixer is sufficient and already integrated. See
[ADR-0010](decisions/0010-sdl2-mixer-audio.md) for the full rationale.
