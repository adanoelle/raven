#include "scenes/victory_scene.hpp"

#include "core/game.hpp"
#include "ecs/components.hpp"
#include "scenes/title_scene.hpp"

#include <spdlog/spdlog.h>

#include <string>

namespace raven {

void VictoryScene::on_enter(Game& game) {
    if (auto* state = game.registry().ctx().find<GameState>()) {
        final_score_ = state->score;
    }
    new_best_ = game.record_score(final_score_);
    spdlog::info("Victory! Final score: {}{}", final_score_, new_best_ ? " (new best)" : "");
}

void VictoryScene::on_exit(Game& game) {
    game.registry().clear();
    game.registry().ctx().erase<GameState>();
    spdlog::info("Exited victory scene");
}

void VictoryScene::update(Game& game, float dt) {
    blink_timer_ += dt;
    if (blink_timer_ >= 0.5f) {
        blink_timer_ -= 0.5f;
        show_prompt_ = !show_prompt_;
    }

    if (game.input().state().confirm_pressed) {
        game.scenes().swap(std::make_unique<TitleScene>(), game);
    }
}

void VictoryScene::render(Game& game) {
    auto* r = game.renderer().sdl_renderer();
    const auto& font = game.font();
    constexpr float center_x = static_cast<float>(Renderer::VIRTUAL_WIDTH) / 2.f;

    // Deep gold-tinted background
    SDL_SetRenderDrawColor(r, 30, 24, 8, 255);
    SDL_RenderClear(r);

    font.draw_centered(r, "VICTORY", center_x, 72.f, {255, 210, 90, 255}, 4);

    font.draw_centered(r, "SCORE " + std::to_string(final_score_), center_x, 132.f,
                       {255, 255, 255, 255}, 2);

    if (new_best_) {
        // Blink the celebration alongside the prompt for a little sparkle
        if (show_prompt_) {
            font.draw_centered(r, "NEW BEST!", center_x, 156.f, {255, 210, 90, 255}, 1);
        }
    } else {
        font.draw_centered(r, "BEST " + std::to_string(game.save_data().best_score), center_x,
                           156.f, {180, 180, 200, 255}, 1);
    }

    if (show_prompt_) {
        font.draw_centered(r, "PRESS START", center_x, 210.f, {255, 255, 255, 255}, 1);
    }
}

} // namespace raven
