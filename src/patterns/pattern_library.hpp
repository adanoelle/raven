#pragma once

#include "ecs/components.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace raven {

/// @brief Describes one emitter within a bullet pattern.
///
/// Each emitter fires bursts of bullets in a configurable arc.
/// Multiple emitters combine to form a complete PatternDef.
struct EmitterDef {
    /// @brief Shape of the bullet emission.
    enum class Type {
        Radial, ///< Bullets spread evenly across spread_angle.
        Aimed,  ///< Bullets aimed at the player position.
        Linear  ///< Bullets fired in a straight line.
    };

    Type type = Type::Radial;     ///< Emission shape.
    int count = 1;                ///< Bullets per burst.
    float speed = 100.f;          ///< Bullet speed in pixels/sec.
    float angular_velocity = 0.f; ///< Emitter rotation in degrees/sec.
    float fire_rate = 0.1f;       ///< Seconds between bursts.
    float spread_angle = 360.f;   ///< Arc width in degrees.
    float start_angle = 0.f;      ///< Initial angle offset in degrees.
    StringId bullet_sheet;        ///< Interned sprite sheet ID for emitted bullets.
    int bullet_frame_x = 0;       ///< Frame column in the sheet.
    int bullet_frame_y = 0;       ///< Frame row in the sheet.
    int bullet_width = 8;         ///< Pixel width of bullet frame.
    int bullet_height = 8;        ///< Pixel height of bullet frame.
    float lifetime = 5.f;         ///< Bullet lifetime in seconds.
    float damage = 1.f;           ///< Damage dealt per bullet on contact.
    float hitbox_radius = 3.f;    ///< Bullet collision radius in pixels.
};

/// @brief A complete bullet pattern composed of one or more emitters.
struct PatternDef {
    std::string name;                         ///< Unique pattern identifier.
    Weapon::Tier tier = Weapon::Tier::Common; ///< Weapon rarity tier for drops.
    std::vector<EmitterDef> emitters;         ///< Emitters that fire together.
};

/// @brief Loads and stores bullet pattern definitions from JSON files.
class PatternLibrary {
  public:
    /// @brief Load a manifest JSON that lists pattern files to load.
    /// @param manifest_path Path to the manifest.json file.
    /// @return True if all listed patterns loaded successfully.
    bool load_manifest(const std::string& manifest_path);

    /// @brief Load a single pattern definition from a JSON file.
    /// @param file_path Path to the pattern JSON file.
    /// @return True on success, false if the file could not be parsed.
    bool load_file(const std::string& file_path);

    /// @brief Load a pattern definition from an already-parsed JSON object.
    /// @param j JSON object containing the pattern definition.
    /// @return True on success, false if the JSON structure is invalid.
    bool load_from_json(const nlohmann::json& j);

    /// @brief Retrieve a pattern by name.
    /// @param name The pattern identifier.
    /// @return Pointer to the PatternDef, or nullptr if not found.
    [[nodiscard]] const PatternDef* get(const std::string& name) const;

    /// @brief Get the names of all loaded patterns.
    /// @return Vector of pattern name strings.
    [[nodiscard]] std::vector<std::string> names() const;

    /// @brief Set the string interner used for bullet_sheet fields during parsing.
    /// @param interner The StringInterner to use. Must outlive the library.
    void set_interner(StringInterner& interner) { interner_ = &interner; }

  private:
    std::unordered_map<std::string, PatternDef> patterns_;
    StringInterner* interner_ = nullptr;

    PatternDef parse_pattern(const nlohmann::json& j) const;
    EmitterDef parse_emitter(const nlohmann::json& j) const;
};

} // namespace raven
