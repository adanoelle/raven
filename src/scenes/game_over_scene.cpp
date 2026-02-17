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
    spdlog::info("Game over — final score: {}", final_score_);
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

    if (game.input().state().confirm_pressed) {
        game.scenes().swap(std::make_unique<TitleScene>(), game);
    }
}

void GameOverScene::render(Game& game) {
    auto* r = game.renderer().sdl_renderer();

    // Dark red background
    SDL_SetRenderDrawColor(r, 40, 8, 8, 255);
    SDL_RenderClear(r);

    // "GAME OVER" placeholder rectangle
    SDL_FRect title_rect{140.f, 80.f, 200.f, 40.f};
    SDL_SetRenderDrawColor(r, 200, 40, 40, 255);
    SDL_RenderFillRect(r, &title_rect);

    // Score display — render each digit as a small rectangle
    {
        int score = final_score_;
        // Count digits
        int digits[10] = {};
        int num_digits = 0;
        if (score == 0) {
            digits[0] = 0;
            num_digits = 1;
        } else {
            int temp = score;
            while (temp > 0 && num_digits < 10) {
                digits[num_digits++] = temp % 10;
                temp /= 10;
            }
            // Reverse to get correct order
            for (int i = 0; i < num_digits / 2; ++i) {
                int swap = digits[i];
                digits[i] = digits[num_digits - 1 - i];
                digits[num_digits - 1 - i] = swap;
            }
        }

        // Draw each digit as a small filled rect with width proportional to value
        int start_x = 240 - (num_digits * 8) / 2;
        for (int i = 0; i < num_digits; ++i) {
            SDL_FRect digit_rect{static_cast<float>(start_x + i * 8), 140.f, 6.f, 10.f};
            // Brightness varies with digit value for minimal visual differentiation
            int brightness = 120 + digits[i] * 13;
            SDL_SetRenderDrawColor(r, static_cast<Uint8>(brightness),
                                   static_cast<Uint8>(brightness), static_cast<Uint8>(brightness),
                                   255);
            SDL_RenderFillRect(r, &digit_rect);
        }
    }

    // Blinking "press start" indicator
    if (show_prompt_) {
        SDL_FRect prompt_rect{160.f, 200.f, 160.f, 16.f};
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_RenderFillRect(r, &prompt_rect);
    }
}

} // namespace raven
