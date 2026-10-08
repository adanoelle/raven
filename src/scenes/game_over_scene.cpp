#include "scenes/game_over_scene.hpp"

#include "core/game.hpp"
#include "ecs/components.hpp"
#include "scenes/title_scene.hpp"

#include <spdlog/spdlog.h>

namespace raven {

void GameOverScene::on_enter(Game& game) {
    auto* state = game.registry().ctx().find<GameState>();
    if (state) {
        final_score_ = state->score;
    }
    new_best_ = game.record_score(final_score_);
    spdlog::info("Game over — final score: {}{}", final_score_, new_best_ ? " (new best)" : "");
}

void GameOverScene::on_exit(Game& game) {
    game.registry().clear();
    game.registry().ctx().erase<GameState>();
    spdlog::info("Exited game over scene");
}

void GameOverScene::update(Game& game, float dt) {
    blink_timer_ += dt;
    if (blink_timer_ >= 0.5f) {
        blink_timer_ -= 0.5f;
        show_prompt_ = !show_prompt_;
    }

    // Ignore presses for a moment, so a dash or confirm from the last
    // instant of play doesn't skip the screen. Start works, as the prompt says.
    elapsed_ += dt;
    const auto& input = game.input().state();
    if (elapsed_ >= INPUT_DELAY && (input.confirm_pressed || input.pause_pressed)) {
        game.scenes().swap(std::make_unique<TitleScene>(), game);
    }
}

void GameOverScene::render(Game& game) {
    auto* r = game.renderer().sdl_renderer();
    const auto& font = game.font();
    constexpr float center_x = static_cast<float>(Renderer::VIRTUAL_WIDTH) / 2.f;

    // Dark red background
    SDL_SetRenderDrawColor(r, 40, 8, 8, 255);
    SDL_RenderClear(r);

    font.draw_centered(r, "GAME OVER", center_x, 84.f, {220, 60, 60, 255}, 3);
    font.draw_centered(r, "SCORE " + std::to_string(final_score_), center_x, 136.f,
                       {255, 255, 255, 255}, 2);

    if (new_best_) {
        if (show_prompt_) {
            font.draw_centered(r, "NEW BEST!", center_x, 160.f, {255, 210, 90, 255}, 1);
        }
    } else {
        font.draw_centered(r, "BEST " + std::to_string(game.save_data().best_score), center_x,
                           160.f, {180, 180, 200, 255}, 1);
    }

    if (show_prompt_) {
        font.draw_centered(r, "PRESS START", center_x, 210.f, {255, 255, 255, 255}, 1);
    }
}

} // namespace raven
