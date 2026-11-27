#pragma once

#include "ecs/components.hpp"
#include "ecs/systems/wave_system.hpp"
#include "patterns/pattern_library.hpp"
#include "rendering/tilemap.hpp"
#include "scenes/scene.hpp"

#include <SDL3/SDL.h>

#include <string>
#include <vector>

namespace raven {

/// @brief Main gameplay scene. Manages the player, enemies, and all ECS systems.
class GameScene : public Scene {
  public:
    /// @brief Construct with the selected player class.
    /// @param player_class The class to apply on player spawn.
    explicit GameScene(ClassId::Id player_class = ClassId::Id::Brawler);

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

    /// @brief Push the pause menu when the app is suspended mid-run.
    /// @param game The Game instance.
    void on_suspend(Game& game) override;

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

#ifdef RAVEN_ENABLE_SHADER_LIGHTING
    /// @brief Gather this frame's lights and light the world drawn so far.
    /// @param game The game, for the registry and the renderer.
    /// @param alpha Interpolation factor, as passed to the sprite renderer.
    void apply_lighting(Game& game, float alpha);

    /// @brief Draw a stand-in flame at each fire placed in the room.
    /// @param game The game, for the renderer.
    void draw_fires(Game& game);
#endif

    ClassId::Id selected_class_; ///< Player class chosen at character select.
    Tilemap tilemap_;            ///< Tilemap loaded from LDtk for the current room.
    PatternLibrary pattern_lib_; ///< Bullet pattern definitions for enemy emitters.
    EnemyLibrary enemy_lib_;     ///< Enemy definitions that stages place by name.
    StageLoader stage_loader_;   ///< Loaded stage definitions.
    int current_stage_ = 0;      ///< Index of the current stage being played.
#ifdef RAVEN_ENABLE_SHADER_LIGHTING
    /// Seconds of simulated time, for lights that move. It advances in
    /// update(), so lights hold still while the game is paused and repeat
    /// under replay.
    float light_time_ = 0.f;
    std::vector<SDL_FPoint> fires_; ///< Where the room's fires are; set in enter_room().
#endif
};

} // namespace raven
