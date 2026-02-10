#pragma once

#include "core/clock.hpp"
#include "core/input.hpp"
#include "rendering/renderer.hpp"
#include "rendering/sprite_sheet.hpp"
#include "scenes/scene.hpp"

#ifdef RAVEN_ENABLE_IMGUI
#include "debug/debug_overlay.hpp"
#endif

#include <entt/entt.hpp>

namespace raven {

/// @brief Top-level game state. Owns all subsystems and the ECS registry.
class Game {
  public:
    /// @brief Construct the game with default subsystem state.
    Game();
    ~Game();

    /// @brief Initialise SDL, renderer, input, and load initial assets.
    /// @return True on success, false if any subsystem failed to init.
    bool init();

    /// @brief Enter the main loop. Blocks until the game exits.
    void run();

    /// @brief Tear down all subsystems and release SDL resources.
    void shutdown();

    /// @brief Access the ECS registry shared by all systems.
    /// @return Mutable reference to the EnTT registry.
    entt::registry& registry() { return registry_; }

    /// @brief Access the rendering subsystem.
    /// @return Mutable reference to the Renderer.
    Renderer& renderer() { return renderer_; }

    /// @brief Access the sprite sheet manager.
    /// @return Mutable reference to the SpriteSheetManager.
    SpriteSheetManager& sprites() { return sprites_; }

    /// @brief Access the input subsystem.
    /// @return Mutable reference to the Input handler.
    Input& input() { return input_; }

    /// @brief Access the scene manager.
    /// @return Mutable reference to the SceneManager.
    SceneManager& scenes() { return scenes_; }

    /// @brief Access the game clock (read-only).
    /// @return Const reference to the Clock.
    [[nodiscard]] const Clock& clock() const { return clock_; }

    /// @brief Signal the game loop to stop after the current frame.
    void request_quit() { running_ = false; }

  private:
    bool running_ = false;

    // Subsystems
    Renderer renderer_;
    Input input_;
    Clock clock_;
    SceneManager scenes_;
    SpriteSheetManager sprites_;

#ifdef RAVEN_ENABLE_IMGUI
    DebugOverlay debug_overlay_;
#endif

    // ECS
    entt::registry registry_;

    /// @brief Execute one fixed-timestep tick of game logic.
    /// @param dt Fixed timestep delta in seconds (Clock::TICK_RATE).
    void fixed_update(float dt);

    /// @brief Render the current frame via the active scene and overlay.
    void render();

    /// @brief Load sprite sheets and other initial assets.
    /// @return True if all required assets loaded successfully.
    bool load_assets();
};

} // namespace raven
