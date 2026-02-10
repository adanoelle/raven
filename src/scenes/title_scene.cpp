#include "scenes/title_scene.hpp"

#include "core/game.hpp"
#include "scenes/game_scene.hpp"

#include <spdlog/spdlog.h>

namespace raven {

void TitleScene::on_enter(Game& /*game*/) {
    spdlog::info("Entered title scene");
}

void TitleScene::update(Game& game, float dt) {
    blink_timer_ += dt;
    if (blink_timer_ >= 0.5f) {
        blink_timer_ -= 0.5f;
        show_prompt_ = !show_prompt_;
    }

    if (game.input().state().confirm_pressed) {
        game.scenes().swap(std::make_unique<GameScene>(), game);
    }
}

void TitleScene::render(Game& game) {
    auto* r = game.renderer().sdl_renderer();

    // Draw title text (placeholder — replace with sprite-based text)
    // For now, just draw a colored rectangle as a placeholder
    SDL_SetRenderDrawColor(r, 20, 10, 40, 255);
    SDL_RenderClear(r);

    // Title area
    SDL_Rect title_rect{92, 100, 200, 40};
    SDL_SetRenderDrawColor(r, 180, 60, 120, 255);
    SDL_RenderFillRect(r, &title_rect);

    // Blinking "press start" indicator
    if (show_prompt_) {
        SDL_Rect prompt_rect{132, 300, 120, 16};
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_RenderFillRect(r, &prompt_rect);
    }
}

} // namespace raven
