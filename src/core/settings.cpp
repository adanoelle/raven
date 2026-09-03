#include "core/settings.hpp"

#include "core/fs.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>

namespace raven {

Settings Settings::from_json(const nlohmann::json& j) {
    Settings s;
    s.window_scale = std::clamp(j.value("window_scale", s.window_scale), 1, 8);
    s.fullscreen = j.value("fullscreen", s.fullscreen);
    s.vsync = j.value("vsync", s.vsync);
    s.music_volume = std::clamp(j.value("music_volume", s.music_volume), 0, 100);
    s.sfx_volume = std::clamp(j.value("sfx_volume", s.sfx_volume), 0, 100);
    return s;
}

nlohmann::json Settings::to_json() const {
    return {
        {"window_scale", window_scale}, {"fullscreen", fullscreen}, {"vsync", vsync},
        {"music_volume", music_volume}, {"sfx_volume", sfx_volume},
    };
}

Settings Settings::load(const std::string& file_path) {
    const auto text = fs::read_text(file_path);
    if (!text) {
        spdlog::info("No settings file at '{}' — using defaults", file_path);
        return Settings{};
    }

    try {
        return from_json(nlohmann::json::parse(*text));
    } catch (const nlohmann::json::exception& e) {
        spdlog::warn("Failed to parse settings '{}': {} — using defaults", file_path, e.what());
        return Settings{};
    }
}

bool Settings::save(const std::string& file_path) const {
    if (!fs::write_text(file_path, to_json().dump(4) + '\n')) {
        spdlog::warn("Could not write settings to '{}'", file_path);
        return false;
    }
    return true;
}

} // namespace raven
