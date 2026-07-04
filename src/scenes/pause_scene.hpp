#pragma once

#include "scenes/scene.hpp"

namespace raven {

/// @brief Pause overlay pushed on top of GameScene.
///
/// While this scene is on top, GameScene stops updating (only the top
/// scene receives update()) but keeps rendering underneath — render()
/// here deliberately does not clear, drawing a translucent dim layer and
/// the menu over the frozen gameplay frame.
///
/// Menu: Resume (pop) or Quit to Title (pop + swap the scene below).
/// Pause or cancel also resumes, so Esc/Start toggles cleanly.
class PauseScene : public Scene {
  public:
    void on_enter(Game& game) override;
    void update(Game& game, float dt) override;
    void render(Game& game) override;

  private:
    static constexpr int ITEM_COUNT = 2; ///< Resume, Quit to Title.

    int selected_ = 0;        ///< Highlighted menu item index.
    float prev_move_y_ = 0.f; ///< Previous vertical input, for menu edge detection.
};

} // namespace raven
