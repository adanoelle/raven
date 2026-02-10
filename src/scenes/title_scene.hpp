#pragma once

#include "scenes/scene.hpp"

namespace raven {

/// @brief Title screen scene with a blinking "Press Start" prompt.
///
/// Transitions to GameScene when the confirm button is pressed.
class TitleScene : public Scene {
  public:
    /// @brief Set up title screen assets and state.
    /// @param game The Game instance providing access to subsystems.
    void on_enter(Game& game) override;

    /// @brief Advance the blink timer and check for confirm input.
    /// @param game The Game instance.
    /// @param dt Fixed timestep delta in seconds.
    void update(Game& game, float dt) override;

    /// @brief Draw the title logo and blinking prompt.
    /// @param game The Game instance.
    void render(Game& game) override;

  private:
    float blink_timer_ = 0.f; ///< Timer controlling prompt visibility toggle.
    bool show_prompt_ = true; ///< Whether the prompt text is currently visible.
};

} // namespace raven
