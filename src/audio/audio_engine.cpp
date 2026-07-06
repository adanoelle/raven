#include "audio/audio_engine.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>

namespace raven {

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::init() {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        spdlog::warn("Audio init failed ({}) — running silent", SDL_GetError());
        return false;
    }

    device_ = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (device_ == 0) {
        spdlog::warn("No audio device ({}) — running silent", SDL_GetError());
        return false;
    }

    spdlog::info("Audio device opened");
    return true;
}

void AudioEngine::shutdown() {
    for (auto* stream : streams_) {
        SDL_DestroyAudioStream(stream); // unbinds automatically
    }
    streams_.clear();

    for (auto& [id, sound] : sounds_) {
        SDL_free(sound.data);
    }
    sounds_.clear();

    if (device_ != 0) {
        SDL_CloseAudioDevice(device_);
        device_ = 0;
    }
}

bool AudioEngine::load_sound(const std::string& id, const std::string& path) {
    if (device_ == 0) {
        return false;
    }

    Sound sound;
    if (!SDL_LoadWAV(path.c_str(), &sound.spec, &sound.data, &sound.len)) {
        spdlog::warn("Failed to load sound '{}' from '{}': {}", id, path, SDL_GetError());
        return false;
    }

    // Replace an existing sound with the same id
    if (auto it = sounds_.find(id); it != sounds_.end()) {
        SDL_free(it->second.data);
        sounds_.erase(it);
    }

    sounds_.emplace(id, sound);
    spdlog::debug("Loaded sound '{}' ({} bytes)", id, sound.len);
    return true;
}

void AudioEngine::play(const std::string& id, float gain) {
    if (device_ == 0) {
        return;
    }

    auto it = sounds_.find(id);
    if (it == sounds_.end()) {
        spdlog::warn("Unknown sound '{}'", id);
        return;
    }
    const Sound& sound = it->second;

    // One stream per playing instance; SDL mixes all streams bound to the
    // device. The stream converts from the WAV format to the device format.
    SDL_AudioStream* stream = SDL_CreateAudioStream(&sound.spec, nullptr);
    if (!stream) {
        spdlog::warn("Failed to create audio stream: {}", SDL_GetError());
        return;
    }

    SDL_SetAudioStreamGain(stream, std::clamp(master_gain_ * gain, 0.f, 1.f));

    // Bind before queueing data: the stream's destination format comes from
    // the device at bind time, and PutData fails without one.
    if (!SDL_BindAudioStream(device_, stream) ||
        !SDL_PutAudioStreamData(stream, sound.data, static_cast<int>(sound.len)) ||
        !SDL_FlushAudioStream(stream)) {
        spdlog::warn("Failed to start sound '{}': {}", id, SDL_GetError());
        SDL_DestroyAudioStream(stream);
        return;
    }

    streams_.push_back(stream);
}

void AudioEngine::update() {
    std::erase_if(streams_, [](SDL_AudioStream* stream) {
        if (SDL_GetAudioStreamAvailable(stream) == 0) {
            SDL_DestroyAudioStream(stream);
            return true;
        }
        return false;
    });
}

void AudioEngine::set_master_gain(float gain) {
    master_gain_ = std::clamp(gain, 0.f, 1.f);
}

} // namespace raven
