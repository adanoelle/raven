#pragma once

#include "scenes/scene.hpp"

namespace raven {

/// @brief Title screen with a Start / Options menu.
///
/// Confirm on START swaps to CharacterSelectScene; OPTIONS pushes
/// OptionsScene as an overlay.
class TitleScene : public Scene {
  public:
    /// @brief Set up title screen assets and state.
    /// @param game The Game instance providing access to subsystems.
    void on_enter(Game& game) override;

    /// @brief Handle menu navigation and confirm input.
    /// @param game The Game instance.
    /// @param dt Fixed timestep delta in seconds.
    void update(Game& game, float dt) override;

    /// @brief Draw the title logo and menu.
    /// @param game The Game instance.
    void render(Game& game) override;

  private:
    static constexpr int ITEM_COUNT = 2; ///< Start, Options.

    int selected_ = 0;        ///< Highlighted menu item index.
    float prev_move_y_ = 0.f; ///< Previous vertical input, for menu edge detection.
};

} // namespace raven
