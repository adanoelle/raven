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

    // Left/right toggles between classes
    if (input.move_x < -0.5f && selected_index_ > 0) {
        selected_index_ = 0;
    } else if (input.move_x > 0.5f && selected_index_ < 1) {
        selected_index_ = 1;
    }

    if (input.confirm_pressed) {
        auto selected = (selected_index_ == 0) ? ClassId::Id::Brawler : ClassId::Id::Sharpshooter;
        game.scenes().swap(std::make_unique<GameScene>(selected), game);
    }
}

void CharacterSelectScene::render(Game& game) {
    auto* r = game.renderer().sdl_renderer();
    const auto& font = game.font();

    // Dark background
    SDL_SetRenderDrawColor(r, 15, 10, 30, 255);
    SDL_RenderClear(r);

    // Two class boxes
    constexpr int box_w = 80;
    constexpr int box_h = 100;
    constexpr int gap = 40;
    constexpr int total_w = box_w * 2 + gap;
    constexpr int start_x = (480 - total_w) / 2;
    constexpr int box_y = 80;

    font.draw_centered(r, "SELECT CHARACTER", 240.f, 40.f, {255, 255, 255, 255}, 2);

    // Brawler box
    SDL_FRect brawler_rect{static_cast<float>(start_x), static_cast<float>(box_y),
                           static_cast<float>(box_w), static_cast<float>(box_h)};
    if (selected_index_ == 0) {
        SDL_SetRenderDrawColor(r, 200, 80, 60, 255);
    } else {
        SDL_SetRenderDrawColor(r, 80, 40, 30, 255);
    }
    SDL_RenderFillRect(r, &brawler_rect);

    // Sharpshooter box
    SDL_FRect sharp_rect{static_cast<float>(start_x + box_w + gap), static_cast<float>(box_y),
                         static_cast<float>(box_w), static_cast<float>(box_h)};
    if (selected_index_ == 1) {
        SDL_SetRenderDrawColor(r, 60, 120, 200, 255);
    } else {
        SDL_SetRenderDrawColor(r, 30, 50, 80, 255);
    }
    SDL_RenderFillRect(r, &sharp_rect);

    // Class names under each box
    constexpr float box1_center = static_cast<float>(start_x) + static_cast<float>(box_w) / 2.f;
    constexpr float box2_center =
        static_cast<float>(start_x + box_w + gap) + static_cast<float>(box_w) / 2.f;
    constexpr float name_y = static_cast<float>(box_y + box_h) + 12.f;
    font.draw_centered(
        r, "BRAWLER", box1_center, name_y,
        selected_index_ == 0 ? SDL_Color{255, 200, 180, 255} : SDL_Color{140, 110, 100, 255}, 1);
    font.draw_centered(
        r, "SHARPSHOOTER", box2_center, name_y,
        selected_index_ == 1 ? SDL_Color{180, 210, 255, 255} : SDL_Color{100, 120, 150, 255}, 1);

    // Selection indicator (blinking underline)
    if (show_indicator_) {
        int indicator_x = (selected_index_ == 0) ? start_x : start_x + box_w + gap;
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
