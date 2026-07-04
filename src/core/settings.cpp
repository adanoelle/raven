#include "core/settings.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>

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
    std::ifstream f(file_path);
    if (!f.is_open()) {
        spdlog::info("No settings file at '{}' — using defaults", file_path);
        return Settings{};
    }

    try {
        return from_json(nlohmann::json::parse(f));
    } catch (const nlohmann::json::exception& e) {
        spdlog::warn("Failed to parse settings '{}': {} — using defaults", file_path, e.what());
        return Settings{};
    }
}

bool Settings::save(const std::string& file_path) const {
    if (file_path.empty()) {
        return false;
    }
    std::ofstream f(file_path);
    if (!f.is_open()) {
        spdlog::warn("Could not write settings to '{}'", file_path);
        return false;
    }
    f << to_json().dump(4) << '\n';
    return f.good();
}

} // namespace raven
