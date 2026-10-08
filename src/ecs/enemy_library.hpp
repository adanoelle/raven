#pragma once

#include "ecs/components.hpp"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace raven {

/// @brief Map an enemy tier string to its tier.
/// @param str Tier string ("grunt", "mid" or "boss").
/// @return The tier, or std::nullopt if the string is not a known tier.
[[nodiscard]] std::optional<Enemy::Type> parse_enemy_type(std::string_view str);

/// @brief Map an AI archetype string to its archetype.
/// @param str Archetype string ("chaser", "drifter", "stalker" or "coward").
/// @return The archetype, or std::nullopt if the string is not a known archetype.
[[nodiscard]] std::optional<AiBehavior::Archetype> parse_ai_archetype(std::string_view str);

/// @brief Default movement tuning for an archetype. Enemy definitions can
/// override each value.
/// @param archetype The AI archetype.
/// @return An AiBehavior in its Idle phase with the archetype's defaults.
[[nodiscard]] AiBehavior default_ai(AiBehavior::Archetype archetype);

/// @brief One kind of enemy, defined once in assets/data/enemies.json and
/// placed by name in stage files.
struct EnemyDef {
    std::string id;                        ///< Name that stage files use.
    Enemy::Type tier = Enemy::Type::Grunt; ///< Tier; sets the default stabilizer drop.
    std::string sheet;                     ///< Sprite sheet id.
    int width = 0;               ///< Drawn width in pixels; 0 draws the sheet's frame size.
    int height = 0;              ///< Drawn height in pixels; 0 draws the sheet's frame size.
    float offset_y = 0.f;        ///< Sprite offset in pixels (negative moves it up).
    float radius = 7.f;          ///< Circle hitbox radius (bullets, melee, contact).
    float rect_w = 12.f;         ///< Rect hitbox width (walls).
    float rect_h = 14.f;         ///< Rect hitbox height (walls).
    float hp = 1.f;              ///< Starting hit points.
    int score = 100;             ///< Points awarded on kill.
    AiBehavior ai;               ///< Archetype and movement tuning.
    std::string pattern;         ///< Bullet pattern; empty for an enemy that doesn't shoot.
    float contact_damage = 0.f;  ///< Body damage per hit; 0 for none.
    float stabilizer_drop = 0.f; ///< Chance (0-1) of dropping a weapon stabilizer on death.
};

/// @brief Enemy definitions loaded from JSON, looked up by name.
///
/// The file holds an "enemies" object keyed by enemy name. Only "tier",
/// "sheet" and "ai.archetype" are required; see EnemyDef for the rest and
/// docs/book/src/decisions/0025-named-enemy-definitions.md for the format.
/// A definition with an unknown tier or archetype, or a value out of range,
/// is reported and skipped; unknown keys are reported as likely typos.
class EnemyLibrary {
  public:
    /// @brief Load enemy definitions from a JSON file.
    /// @param path Path to the file.
    /// @return True if the file was read and parsed. Bad definitions are
    ///         reported and skipped.
    bool load_file(const std::string& path);

    /// @brief Load enemy definitions from parsed JSON.
    /// @param j JSON object with an "enemies" object.
    /// @param source Name used in log messages (usually the file path).
    /// @return True if the JSON had the expected shape.
    bool load_json(const nlohmann::json& j, const std::string& source = "enemy data");

    /// @brief Look up an enemy definition.
    /// @param id Enemy name.
    /// @return The definition, or nullptr if there is none by that name.
    [[nodiscard]] const EnemyDef* get(const std::string& id) const;

    /// @brief Number of loaded definitions.
    /// @return Definition count.
    [[nodiscard]] int count() const { return static_cast<int>(defs_.size()); }

    /// @brief Names of all loaded definitions.
    /// @return Enemy names, in no particular order.
    [[nodiscard]] std::vector<std::string> names() const;

  private:
    std::unordered_map<std::string, EnemyDef> defs_;
};

} // namespace raven
