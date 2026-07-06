#include "scenes/options_scene.hpp"

#include "core/game.hpp"
#include "ecs/components.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <string>

namespace raven {

namespace {

/// @brief Format a value column string for a menu row.
std::string value_text(const Settings& s, int item) {
    switch (item) {
    case 0:
        return s.fullscreen ? "ON" : "OFF";
    case 1:
        return std::to_string(s.window_scale) + "X";
    case 2:
        return s.vsync ? "ON" : "OFF";
    case 3:
        return std::to_string(s.music_volume);
    case 4:
        return std::to_string(s.sfx_volume);
    default:
        return "";
    }
}

constexpr const char* LABELS[] = {"FULLSCREEN",   "WINDOW SCALE", "VSYNC",
                                  "MUSIC VOLUME", "SFX VOLUME",   "BACK"};

} // anonymous namespace

void OptionsScene::on_enter(Game& /*game*/) {
    spdlog::info("Entered options scene");
}

void OptionsScene::adjust(Game& game, int direction) {
    auto& s = game.settings_mut();

    switch (selected_) {
    case Fullscreen:
        s.fullscreen = !s.fullscreen;
        break;
    case WindowScale:
        s.window_scale = std::clamp(s.window_scale + direction, 1, 8);
        break;
    case Vsync:
        s.vsync = !s.vsync;
        break;
    case MusicVolume:
        s.music_volume = std::clamp(s.music_volume + direction * 10, 0, 100);
        break;
    case SfxVolume:
        s.sfx_volume = std::clamp(s.sfx_volume + direction * 10, 0, 100);
        break;
    default:
        return;
    }

    game.apply_settings();

    // Audible feedback while tuning the SFX volume
    if (selected_ == SfxVolume) {
        game.audio().play(sfx_sound_name(Sfx::Pickup));
    }
}

void OptionsScene::update(Game& game, float /*dt*/) {
    const auto& input = game.input().state();

    if (input.cancel_pressed || input.pause_pressed) {
        game.scenes().pop(game);
        return;
    }

    // Vertical navigation on input edges
    if (input.move_y > 0.5f && prev_move_y_ <= 0.5f) {
        selected_ = (selected_ + 1) % COUNT;
    } else if (input.move_y < -0.5f && prev_move_y_ >= -0.5f) {
        selected_ = (selected_ + COUNT - 1) % COUNT;
    }
    prev_move_y_ = input.move_y;

    // Horizontal adjustment on input edges
    if (input.move_x > 0.5f && prev_move_x_ <= 0.5f) {
        adjust(game, +1);
    } else if (input.move_x < -0.5f && prev_move_x_ >= -0.5f) {
        adjust(game, -1);
    }
    prev_move_x_ = input.move_x;

    if (input.confirm_pressed) {
        if (selected_ == Back) {
            game.scenes().pop(game);
        } else {
            adjust(game, +1); // confirm toggles/increments the selected item
        }
    }
}

void OptionsScene::render(Game& game) {
    auto* r = game.renderer().sdl_renderer();
    const auto& font = game.font();
    constexpr float center_x = static_cast<float>(Renderer::VIRTUAL_WIDTH) / 2.f;

    // Dim whatever is beneath (title screen or paused gameplay)
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_FRect overlay{0.f, 0.f, static_cast<float>(Renderer::VIRTUAL_WIDTH),
                      static_cast<float>(Renderer::VIRTUAL_HEIGHT)};
    SDL_SetRenderDrawColor(r, 0, 0, 0, 200);
    SDL_RenderFillRect(r, &overlay);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    font.draw_centered(r, "OPTIONS", center_x, 48.f, {255, 255, 255, 255}, 3);

    const auto& s = game.settings();
    const SDL_Color active{255, 255, 255, 255};
    const SDL_Color inactive{130, 130, 150, 255};
    const SDL_Color value_color{180, 210, 255, 255};

    constexpr float label_x = 140.f;
    constexpr float value_x = 310.f;
    constexpr float row_h = 16.f;
    constexpr float first_y = 100.f;

    for (int i = 0; i < COUNT; ++i) {
        float y = first_y + static_cast<float>(i) * row_h;
        bool is_selected = (i == selected_);

        if (is_selected) {
            font.draw(r, ">", label_x - 12.f, y, active, 1);
        }
        font.draw(r, LABELS[i], label_x, y, is_selected ? active : inactive, 1);

        std::string value = value_text(s, i);
        if (!value.empty()) {
            std::string display = is_selected ? "< " + value + " >" : value;
            float x = is_selected ? value_x - 12.f : value_x;
            font.draw(r, display, x, y, is_selected ? value_color : inactive, 1);
        }
    }

    // VSync may be refused by the driver; surface the real state
    if (s.vsync && !game.renderer().vsync_enabled()) {
        font.draw_centered(r, "VSYNC UNAVAILABLE - USING FRAME LIMITER", center_x, 220.f,
                           {200, 160, 90, 255}, 1);
    }
}

} // namespace raven
