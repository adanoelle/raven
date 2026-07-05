#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace raven {

/// @brief Sound effect playback built directly on SDL3 audio.
///
/// No mixer library: SDL3 mixes every audio stream bound to a device, so
/// each play() binds a fresh stream with the sound's samples and update()
/// reaps streams that have finished draining. Suits short WAV effects;
/// streamed music will come later (see ADR-0019).
///
/// All calls are safe before init() or after a failed init — they no-op,
/// matching the sprite/font degradation philosophy (a game without an
/// audio device plays silently rather than crashing).
class AudioEngine {
  public:
    AudioEngine() = default;
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    /// @brief Open the default playback device.
    /// @return True on success; false leaves the engine in silent no-op mode.
    bool init();

    /// @brief Destroy live streams, free sounds, and close the device.
    void shutdown();

    /// @brief Load a WAV file as a named sound.
    /// @param id Name used by play(), e.g. "shoot".
    /// @param path Full path to the WAV file (use paths::asset()).
    /// @return True on success.
    bool load_sound(const std::string& id, const std::string& path);

    /// @brief Play a loaded sound once.
    /// @param id The sound name passed to load_sound().
    /// @param gain Per-play volume multiplier, combined with the master gain.
    void play(const std::string& id, float gain = 1.f);

    /// @brief Reap streams that have finished playing. Call once per frame.
    void update();

    /// @brief Set the master volume applied to all subsequent plays.
    /// @param gain Linear gain, 0.0 (silent) to 1.0 (full).
    void set_master_gain(float gain);

    /// @brief Whether an audio device is open and playback is possible.
    /// @return True after a successful init().
    [[nodiscard]] bool is_ready() const { return device_ != 0; }

    /// @brief Number of currently playing (bound) streams.
    /// @return Live stream count; 0 when idle or not initialised.
    [[nodiscard]] int active_streams() const { return static_cast<int>(streams_.size()); }

  private:
    /// @brief A loaded WAV: samples owned by SDL (freed with SDL_free).
    struct Sound {
        SDL_AudioSpec spec{};
        Uint8* data = nullptr;
        Uint32 len = 0;
    };

    SDL_AudioDeviceID device_ = 0;
    std::unordered_map<std::string, Sound> sounds_;
    std::vector<SDL_AudioStream*> streams_; ///< Streams currently bound and playing.
    float master_gain_ = 1.f;
};

} // namespace raven
