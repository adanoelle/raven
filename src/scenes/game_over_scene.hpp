#pragma once

#include "scenes/scene.hpp"

namespace raven {

/// @brief Game over screen showing final score with a blinking restart prompt.
///
/// Transitions to TitleScene when the confirm button is pressed.
class GameOverScene : public Scene {
  public:
    /// @brief Read final score from GameState context.
    /// @param game The Game instance providing access to subsystems.
    void on_enter(Game& game) override;

    /// @brief Clear GameState from registry context for a fresh start.
    /// @param game The Game instance.
    void on_exit(Game& game) override;

    /// @brief Advance the blink timer and check for confirm input.
    /// @param game The Game instance.
    /// @param dt Fixed timestep delta in seconds.
    void update(Game& game, float dt) override;

    /// @brief Draw the game over screen with score and blinking prompt.
    /// @param game The Game instance.
    void render(Game& game) override;

  private:
    int final_score_ = 0;     ///< Score captured from GameState on enter.
    float blink_timer_ = 0.f; ///< Timer controlling prompt visibility toggle.
    bool show_prompt_ = true; ///< Whether the prompt text is currently visible.
};

} // namespace raven
