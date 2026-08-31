#include "scenes/char_select_scene.hpp"

#include "core/game.hpp"
#include "scenes/game_scene.hpp"

#include <spdlog/spdlog.h>

namespace raven {

void CharacterSelectScene::on_enter(Game& /*game*/) {
    spdlog::info("Entered character select scene");
}

void CharacterSelectScene::update(Game& game, float dt) {
    blink_timer_ += dt;
    if (blink_timer_ >= 0.3f) {
        blink_timer_ -= 0.3f;
        show_indicator_ = !show_indicator_;
    }

    // Skip first frame to consume stale confirm_pressed from previous scene
    if (first_frame_) {
        first_frame_ = false;
        return;
    }

    const auto& input = game.input().state();

    // Left/right steps between classes; latched so a held direction
    // moves one box per press instead of one per frame.
    if (input.move_x > -0.5f && input.move_x < 0.5f) {
        move_latched_ = false;
    } else if (!move_latched_) {
        move_latched_ = true;
        if (input.move_x < 0.f && selected_index_ > 0) {
            --selected_index_;
        } else if (input.move_x > 0.f && selected_index_ < 2) {
            ++selected_index_;
        }
    }

    if (input.confirm_pressed) {
        constexpr ClassId::Id classes[] = {ClassId::Id::Brawler, ClassId::Id::Knight,
                                           ClassId::Id::Sharpshooter};
        game.scenes().swap(std::make_unique<GameScene>(classes[selected_index_]), game);
    }
}

void CharacterSelectScene::render(Game& game) {
    auto* r = game.renderer().sdl_renderer();
    const auto& font = game.font();

    // Dark background
    SDL_SetRenderDrawColor(r, 15, 10, 30, 255);
    SDL_RenderClear(r);

    // Class boxes
    constexpr int box_count = 3;
    constexpr int box_w = 80;
    constexpr int box_h = 100;
    constexpr int gap = 40;
    constexpr int total_w = box_w * box_count + gap * (box_count - 1);
    constexpr int start_x = (480 - total_w) / 2;
    constexpr int box_y = 80;

    font.draw_centered(r, "SELECT CHARACTER", 240.f, 40.f, {255, 255, 255, 255}, 2);

    struct ClassBox {
        const char* name;
        SDL_Color box_selected;
        SDL_Color box_dim;
        SDL_Color name_selected;
        SDL_Color name_dim;
    };
    constexpr ClassBox boxes[box_count] = {
        {"BRAWLER",
         {200, 80, 60, 255},
         {80, 40, 30, 255},
         {255, 200, 180, 255},
         {140, 110, 100, 255}},
        {"KNIGHT",
         {150, 150, 180, 255},
         {60, 60, 75, 255},
         {225, 225, 245, 255},
         {115, 115, 135, 255}},
        {"SHARPSHOOTER",
         {60, 120, 200, 255},
         {30, 50, 80, 255},
         {180, 210, 255, 255},
         {100, 120, 150, 255}},
    };

    constexpr float name_y = static_cast<float>(box_y + box_h) + 12.f;
    for (int i = 0; i < box_count; ++i) {
        const int x = start_x + i * (box_w + gap);
        const bool selected = (selected_index_ == i);

        SDL_FRect rect{static_cast<float>(x), static_cast<float>(box_y), static_cast<float>(box_w),
                       static_cast<float>(box_h)};
        const SDL_Color& bc = selected ? boxes[i].box_selected : boxes[i].box_dim;
        SDL_SetRenderDrawColor(r, bc.r, bc.g, bc.b, bc.a);
        SDL_RenderFillRect(r, &rect);

        font.draw_centered(r, boxes[i].name,
                           static_cast<float>(x) + static_cast<float>(box_w) / 2.f, name_y,
                           selected ? boxes[i].name_selected : boxes[i].name_dim, 1);
    }

    // Selection indicator (blinking underline)
    if (show_indicator_) {
        const int indicator_x = start_x + selected_index_ * (box_w + gap);
        SDL_FRect indicator{static_cast<float>(indicator_x), static_cast<float>(box_y + box_h + 4),
                            static_cast<float>(box_w), 3.f};
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_RenderFillRect(r, &indicator);
    }

    // Blinking "press confirm" prompt
    if (show_indicator_) {
        font.draw_centered(r, "PRESS CONFIRM", 240.f, 224.f, {200, 200, 200, 255}, 1);
    }
}

} // namespace raven
