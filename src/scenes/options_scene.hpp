#pragma once

#include "scenes/scene.hpp"

namespace raven {

/// @brief Options menu overlay: display and audio settings.
///
/// Pushed from the title screen or the pause menu; renders over whatever
/// is beneath it (own dim layer, no clear). Up/down selects a setting,
/// left/right adjusts it, and every change is applied live and persisted
/// via Game::apply_settings(). Cancel or Back pops the scene.
///
/// Items: fullscreen, window scale, vsync, music volume, sfx volume, back.
class OptionsScene : public Scene {
  public:
    void on_enter(Game& game) override;
    void update(Game& game, float dt) override;
    void render(Game& game) override;

  private:
    /// @brief Menu entries, in display order.
    enum Item : int {
        Fullscreen = 0,
        WindowScale,
        Vsync,
        MusicVolume,
        SfxVolume,
        Back,
        COUNT,
    };

    int selected_ = 0;        ///< Highlighted menu item index.
    float prev_move_x_ = 0.f; ///< Previous horizontal input, for adjust edges.
    float prev_move_y_ = 0.f; ///< Previous vertical input, for navigation edges.

    /// @brief Adjust the selected setting by direction and apply.
    /// @param game The Game instance owning the settings.
    /// @param direction -1 to decrease, +1 to increase (or toggle).
    void adjust(Game& game, int direction);
};

} // namespace raven
