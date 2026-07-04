#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace raven {

/// @brief User-configurable settings persisted to the platform pref dir.
///
/// Distinct from assets/data/config.json, which is read-only game data
/// shipped with the build. Settings are written to settings.json inside
/// paths::pref_dir() so they survive updates and reinstalls.
struct Settings {
    int window_scale = 2;    ///< Integer window size multiplier (windowed mode).
    bool fullscreen = false; ///< Start in borderless fullscreen.
    bool vsync = true;       ///< Sync presentation to display refresh.
    int music_volume = 80;   ///< Music volume, 0-100.
    int sfx_volume = 100;    ///< Sound effect volume, 0-100.

    /// @brief Build Settings from JSON, using defaults for missing fields
    /// and clamping out-of-range values.
    /// @param j Parsed settings JSON object.
    /// @return Validated settings.
    [[nodiscard]] static Settings from_json(const nlohmann::json& j);

    /// @brief Serialize to JSON.
    /// @return JSON object with all settings fields.
    [[nodiscard]] nlohmann::json to_json() const;

    /// @brief Load settings from a file, returning defaults if the file is
    /// missing or malformed (first run, corrupted write).
    /// @param file_path Full path to settings.json.
    /// @return Loaded or default settings.
    [[nodiscard]] static Settings load(const std::string& file_path);

    /// @brief Write settings to a file as pretty-printed JSON.
    /// @param file_path Full path to settings.json.
    /// @return True on success.
    bool save(const std::string& file_path) const;
};

} // namespace raven
