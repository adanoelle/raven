#pragma once

#include "ecs/systems/wave_system.hpp"
#include "patterns/pattern_library.hpp"
#include "rendering/tilemap.hpp"
#include "scenes/scene.hpp"

#include <string>

namespace raven {

/// @brief Main gameplay scene. Manages the player, enemies, and all ECS systems.
class GameScene : public Scene {
  public:
    /// @brief Spawn the player and initialise gameplay state.
    /// @param game The Game instance providing access to subsystems.
    void on_enter(Game& game) override;

    /// @brief Clean up gameplay entities from the registry.
    /// @param game The Game instance.
    void on_exit(Game& game) override;

    /// @brief Run all gameplay systems for one fixed-timestep tick.
    /// @param game The Game instance.
    /// @param dt Fixed timestep delta in seconds.
    void update(Game& game, float dt) override;

    /// @brief Render all gameplay entities via the render system.
    /// @param game The Game instance.
    void render(Game& game) override;

  private:
    /// @brief Create the player entity with all required components.
    /// @param game The Game instance providing registry and sprite access.
    void spawn_player(Game& game);

    /// @brief Enter a new room: clear non-player entities, reload tilemap, spawn exits and wave 0.
    /// @param game The Game instance.
    /// @param level LDtk level name to load.
    void enter_room(Game& game, const std::string& level);

    /// @brief Destroy all entities except the player.
    /// @param game The Game instance.
    void clear_room_entities(Game& game);

    /// @brief Render the HUD overlay (health, lives, score, decay timer, wave indicator).
    /// @param game The Game instance.
    void render_hud(Game& game);

    Tilemap tilemap_;            ///< Tilemap loaded from LDtk for the current room.
    PatternLibrary pattern_lib_; ///< Bullet pattern definitions for enemy emitters.
    StageLoader stage_loader_;   ///< Loaded stage definitions.
    int current_stage_ = 0;      ///< Index of the current stage being played.
};

} // namespace raven
