#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace raven {

/// @brief Persistent player progress, stored in the platform pref dir.
///
/// Separate from Settings (user preferences) and from assets/data/
/// (read-only game data): this file records what the player has achieved.
/// Written to save.json inside paths::pref_dir(). Loading is defensive —
/// a missing or corrupt file yields defaults rather than an error, so a
/// damaged save never blocks the game from starting.
struct SaveData {
    int best_score = 0; ///< Highest score across all runs.

    /// @brief Build SaveData from JSON, using defaults for missing fields
    /// and clamping out-of-range values.
    /// @param j Parsed save JSON object.
    /// @return Validated save data.
    [[nodiscard]] static SaveData from_json(const nlohmann::json& j);

    /// @brief Serialize to JSON.
    /// @return JSON object with all save fields.
    [[nodiscard]] nlohmann::json to_json() const;

    /// @brief Load save data from a file, returning defaults if the file
    /// is missing or malformed.
    /// @param file_path Full path to save.json.
    /// @return Loaded or default save data.
    [[nodiscard]] static SaveData load(const std::string& file_path);

    /// @brief Write save data to a file as pretty-printed JSON.
    /// @param file_path Full path to save.json.
    /// @return True on success.
    bool save(const std::string& file_path) const;
};

} // namespace raven
