#pragma once

#include "ecs/components.hpp"
#include "scenes/scene.hpp"

namespace raven {

/// @brief Character selection scene. Left/right toggles class, confirm starts game.
class CharacterSelectScene : public Scene {
  public:
    /// @brief Set up character select state.
    /// @param game The Game instance providing access to subsystems.
    void on_enter(Game& game) override;

    /// @brief Handle input for class selection and confirmation.
    /// @param game The Game instance.
    /// @param dt Fixed timestep delta in seconds.
    void update(Game& game, float dt) override;

    /// @brief Draw the class selection UI.
    /// @param game The Game instance.
    void render(Game& game) override;

  private:
    int selected_index_ = 0;     ///< 0 = Brawler, 1 = Sharpshooter.
    float blink_timer_ = 0.f;    ///< Timer controlling selection indicator blink.
    bool show_indicator_ = true; ///< Whether the selection indicator is visible.
    bool first_frame_ = true;    ///< Skip input on first frame to consume stale confirm.
};

} // namespace raven
