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

    // Brawler box
    SDL_Rect brawler_rect{start_x, box_y, box_w, box_h};
    if (selected_index_ == 0) {
        SDL_SetRenderDrawColor(r, 200, 80, 60, 255);
    } else {
        SDL_SetRenderDrawColor(r, 80, 40, 30, 255);
    }
    SDL_RenderFillRect(r, &brawler_rect);

    // Sharpshooter box
    SDL_Rect sharp_rect{start_x + box_w + gap, box_y, box_w, box_h};
    if (selected_index_ == 1) {
        SDL_SetRenderDrawColor(r, 60, 120, 200, 255);
    } else {
        SDL_SetRenderDrawColor(r, 30, 50, 80, 255);
    }
    SDL_RenderFillRect(r, &sharp_rect);

    // Selection indicator (blinking underline)
    if (show_indicator_) {
        int indicator_x = (selected_index_ == 0) ? start_x : start_x + box_w + gap;
        SDL_Rect indicator{indicator_x, box_y + box_h + 4, box_w, 3};
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_RenderFillRect(r, &indicator);
    }

    // Blinking "press confirm" prompt
    if (show_indicator_) {
        SDL_Rect prompt_rect{180, 220, 120, 12};
        SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
        SDL_RenderFillRect(r, &prompt_rect);
    }
}

} // namespace raven
