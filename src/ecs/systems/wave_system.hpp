#pragma once

#include "ecs/components.hpp"
#include "ecs/enemy_library.hpp"
#include "patterns/pattern_library.hpp"
#include "rendering/tilemap.hpp"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace raven {

/// @brief One enemy placed in a wave: which kind, and where.
struct WaveEnemyDef {
    int spawn_index = 0; ///< Index into the level's EnemySpawn list.
    std::string enemy;   ///< Name of an EnemyDef in the EnemyLibrary.
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

/// @brief Loads stage definitions from JSON files following a manifest.
///
/// Wave entries place enemies by name ({"spawn_index": 0, "enemy":
/// "grunt_chaser"}); the enemies themselves are defined in
/// assets/data/enemies.json. An entry without an enemy name is reported
/// and skipped.
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
    [[nodiscard]] std::optional<WaveEnemyDef> parse_enemy(const nlohmann::json& j,
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
/// Each enemy is built from its EnemyDef; an unknown name is reported and
/// skipped. Enemies with contact damage start with SPAWN_CONTACT_GRACE on
/// their contact cooldown.
/// @param reg The ECS registry.
/// @param tilemap Tilemap with spawn point positions.
/// @param stage The current stage definition.
/// @param wave_index Which wave to spawn.
/// @param patterns Bullet pattern library for emitter setup.
/// @param enemies Enemy definitions the stage refers to by name.
void spawn_wave(entt::registry& reg, const Tilemap& tilemap, const StageDef& stage, int wave_index,
                const PatternLibrary& patterns, const EnemyLibrary& enemies);

/// @brief Check if current wave is cleared; advance wave or mark room cleared.
///
/// Clearing the room opens every Exit and switches any exit Sprite to its
/// open frame.
/// @param reg The ECS registry.
/// @param tilemap Tilemap with spawn point positions.
/// @param stage The current stage definition.
/// @param patterns Bullet pattern library for emitter setup.
/// @param enemies Enemy definitions the stage refers to by name.
void update_waves(entt::registry& reg, const Tilemap& tilemap, const StageDef& stage,
                  const PatternLibrary& patterns, const EnemyLibrary& enemies);

/// @brief Check player overlap with open Exit entities.
/// @param reg The ECS registry.
/// @return The open exit the player is touching, or nullptr if none. Valid
///         until the registry is next modified.
[[nodiscard]] const Exit* check_exit_overlap(entt::registry& reg);

} // namespace systems
} // namespace raven
