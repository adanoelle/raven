#pragma once

#include "scenes/scene.hpp"

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
};

} // namespace raven
