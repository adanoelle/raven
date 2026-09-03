#pragma once

#include "core/settings.hpp"
#include "scenes/scene.hpp"

#include <string>
#include <vector>

namespace raven {

/// @brief Options menu overlay: display and audio settings.
///
/// Pushed from the title screen or the pause menu; renders over whatever
/// is beneath it (own dim layer, no clear). Up/down selects a setting,
/// left/right adjusts it, and every change is applied live via
/// Game::apply_settings(); settings.json is written once when the scene
/// exits rather than on every adjustment. Cancel or Back pops the scene.
///
/// Items: fullscreen, window scale, vsync, music volume, sfx volume, back.
/// The window rows are omitted on fixed-display platforms
/// (platform::HAS_WINDOW_SETTINGS).
class OptionsScene : public Scene {
  public:
    OptionsScene();

    void on_enter(Game& game) override;
    void on_exit(Game& game) override;
    void update(Game& game, float dt) override;
    void render(Game& game) override;

  private:
    /// @brief All possible menu entries.
    enum Item : int {
        Fullscreen = 0,
        WindowScale,
        Vsync,
        MusicVolume,
        SfxVolume,
        Back,
    };

    std::vector<Item> items_; ///< Entries shown on this platform, in display order.
    int selected_ = 0;        ///< Index into items_ of the highlighted row.
    float prev_move_x_ = 0.f; ///< Previous horizontal input, for adjust edges.
    float prev_move_y_ = 0.f; ///< Previous vertical input, for navigation edges.

    /// @brief Format the value column for a menu row.
    /// @param s The settings to read.
    /// @param item The row.
    /// @return Display text, empty for rows without a value.
    static std::string value_text(const Settings& s, Item item);

    /// @brief The menu entry currently highlighted.
    [[nodiscard]] Item current() const { return items_[static_cast<size_t>(selected_)]; }

    /// @brief Adjust the selected setting by direction and apply.
    /// @param game The Game instance owning the settings.
    /// @param direction -1 to decrease, +1 to increase (or toggle).
    void adjust(Game& game, int direction) const;
};

} // namespace raven
