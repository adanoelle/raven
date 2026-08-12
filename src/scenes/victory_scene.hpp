#pragma once

#include "scenes/scene.hpp"

namespace raven {

/// @brief Victory screen shown after clearing the final stage.
///
/// Records the run's score against the persisted best (celebrating a new
/// record), then returns to TitleScene on confirm. Mirrors GameOverScene's
/// lifecycle: on_exit clears the registry and erases GameState so the next
/// run starts fresh.
class VictoryScene : public Scene {
  public:
    /// @brief Capture the final score and record it against the best.
    /// @param game The Game instance providing access to subsystems.
    void on_enter(Game& game) override;

    /// @brief Clear GameState from registry context for a fresh start.
    /// @param game The Game instance.
    void on_exit(Game& game) override;

    /// @brief Advance the blink timer and check for confirm input.
    /// @param game The Game instance.
    /// @param dt Fixed timestep delta in seconds.
    void update(Game& game, float dt) override;

    /// @brief Draw the victory screen with score, best, and prompt.
    /// @param game The Game instance.
    void render(Game& game) override;

  private:
    int final_score_ = 0;     ///< Score captured from GameState on enter.
    bool new_best_ = false;   ///< Whether this run set a new best score.
    float blink_timer_ = 0.f; ///< Timer controlling prompt visibility toggle.
    bool show_prompt_ = true; ///< Whether the prompt text is currently visible.
};

} // namespace raven
