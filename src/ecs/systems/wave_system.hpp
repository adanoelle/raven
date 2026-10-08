#pragma once

#include "ecs/components.hpp"
#include "patterns/pattern_library.hpp"
#include "rendering/tilemap.hpp"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace raven {

/// @brief Definition of a single enemy within a wave.
struct WaveEnemyDef {
    int spawn_index = 0;                                      ///< Index into the EnemySpawn list.
    Enemy::Type type = Enemy::Type::Grunt;                    ///< Enemy tier.
    std::string pattern;                                      ///< Bullet pattern name.
    float hp = 1.f;                                           ///< Starting hit points.
    int score = 100;                                          ///< Points awarded on kill.
    AiBehavior::Archetype ai = AiBehavior::Archetype::Chaser; ///< AI movement archetype.
    bool contact_damage = false; ///< Whether this enemy deals body damage.
};

/// @brief A single wave of enemies to spawn simultaneously.
struct WaveDef {
    std::vector<WaveEnemyDef> enemies; ///< Enemies in this wave.
};

/// @brief A complete stage definition with level reference and waves.
struct StageDef {
    std::string name;           ///< Stage identifier.
    std::string level;          ///< LDtk level name to load.
    std::vector<WaveDef> waves; ///< Ordered list of waves.
};

/// @brief Map a stage-file enemy type string to its tier.
/// @param str Type string from stage JSON ("grunt", "mid" or "boss").
/// @return The tier, or std::nullopt if the string is not a known type.
[[nodiscard]] std::optional<Enemy::Type> parse_enemy_type(std::string_view str);

/// @brief Map a stage-file AI string to its archetype.
/// @param str AI string from stage JSON ("chaser", "drifter", "stalker" or "coward").
/// @return The archetype, or std::nullopt if the string is not a known archetype.
[[nodiscard]] std::optional<AiBehavior::Archetype> parse_ai_archetype(std::string_view str);

/// @brief Loads stage definitions from JSON files following a manifest.
///
/// Unknown enemy type or AI strings log a warning and fall back to grunt
/// and chaser, so a typo in stage data is visible in the log.
class StageLoader {
  public:
    /// @brief Load a manifest JSON listing stage files.
    /// @param manifest_path Path to the stage_manifest.json file.
    /// @return True if at least one stage loaded successfully.
    bool load_manifest(const std::string& manifest_path);

    /// @brief Load a single stage definition from a JSON file.
    /// @param file_path Path to the stage JSON file.
    /// @return True on success.
    bool load_file(const std::string& file_path);

    /// @brief Load a stage definition from an already-parsed JSON object.
    /// @param j JSON object containing the stage definition.
    /// @return True on success.
    bool load_from_json(const nlohmann::json& j);

    /// @brief Retrieve a stage by index.
    /// @param index Zero-based stage index.
    /// @return Pointer to the StageDef, or nullptr if out of range.
    [[nodiscard]] const StageDef* get(int index) const;

    /// @brief Get the number of loaded stages.
    /// @return Stage count.
    [[nodiscard]] int count() const;

  private:
    std::vector<StageDef> stages_;

    [[nodiscard]] StageDef parse_stage(const nlohmann::json& j) const;
    [[nodiscard]] WaveDef parse_wave(const nlohmann::json& j, const std::string& stage_name) const;
    [[nodiscard]] WaveEnemyDef parse_enemy(const nlohmann::json& j,
                                           const std::string& stage_name) const;
};

namespace systems {

/// @brief Seconds after spawning before an enemy's contact damage can hit.
///
/// Keeps an enemy that appears on top of the player from hurting them
/// before they can react.
inline constexpr float SPAWN_CONTACT_GRACE = 1.f;

/// @brief Spawn enemies for a wave at EnemySpawn positions from the tilemap.
///
/// Enemies with contact damage start with SPAWN_CONTACT_GRACE on their
/// contact cooldown.
/// @param reg The ECS registry.
/// @param tilemap Tilemap with spawn point positions.
/// @param stage The current stage definition.
/// @param wave_index Which wave to spawn.
/// @param patterns Bullet pattern library for emitter setup.
void spawn_wave(entt::registry& reg, const Tilemap& tilemap, const StageDef& stage, int wave_index,
                const PatternLibrary& patterns);

/// @brief Check if current wave is cleared; advance wave or mark room cleared.
///
/// Clearing the room opens every Exit and switches any exit Sprite to its
/// open frame.
/// @param reg The ECS registry.
/// @param tilemap Tilemap with spawn point positions.
/// @param stage The current stage definition.
/// @param patterns Bullet pattern library for emitter setup.
void update_waves(entt::registry& reg, const Tilemap& tilemap, const StageDef& stage,
                  const PatternLibrary& patterns);

/// @brief Check player overlap with open Exit entities.
/// @param reg The ECS registry.
/// @return The open exit the player is touching, or nullptr if none. Valid
///         until the registry is next modified.
[[nodiscard]] const Exit* check_exit_overlap(entt::registry& reg);

} // namespace systems
} // namespace raven
