#include "scenes/pause_scene.hpp"

#include "core/game.hpp"
#include "scenes/title_scene.hpp"

#include <spdlog/spdlog.h>

namespace raven {

void PauseScene::on_enter(Game& /*game*/) {
    spdlog::info("Game paused");
}

void PauseScene::update(Game& game, float /*dt*/) {
    const auto& input = game.input().state();

    // Resume on pause toggle or cancel
    if (input.pause_pressed || input.cancel_pressed) {
        game.scenes().pop(game);
        return;
    }

    // Menu navigation: vertical input edges (movement axes are held state,
    // not edge-detected, so track the previous value ourselves)
    if (input.move_y > 0.5f && prev_move_y_ <= 0.5f) {
        selected_ = (selected_ + 1) % ITEM_COUNT;
    } else if (input.move_y < -0.5f && prev_move_y_ >= -0.5f) {
        selected_ = (selected_ + ITEM_COUNT - 1) % ITEM_COUNT;
    }
    prev_move_y_ = input.move_y;

    if (input.confirm_pressed) {
        if (selected_ == 0) {
            // Resume
            game.scenes().pop(game);
        } else {
            // Quit to title: pop this overlay, then replace the GameScene
            // beneath. Both are queued and applied in order after update().
            game.scenes().pop(game);
            game.scenes().swap(std::make_unique<TitleScene>(), game);
        }
    }
}

void PauseScene::render(Game& game) {
    auto* r = game.renderer().sdl_renderer();
    const auto& font = game.font();
    constexpr float center_x = static_cast<float>(Renderer::VIRTUAL_WIDTH) / 2.f;

    // No clear: the gameplay frame below stays visible. Dim it.
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_FRect overlay{0.f, 0.f, static_cast<float>(Renderer::VIRTUAL_WIDTH),
                      static_cast<float>(Renderer::VIRTUAL_HEIGHT)};
    SDL_SetRenderDrawColor(r, 0, 0, 0, 160);
    SDL_RenderFillRect(r, &overlay);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    font.draw_centered(r, "PAUSED", center_x, 84.f, {255, 255, 255, 255}, 3);

    const SDL_Color active{255, 255, 255, 255};
    const SDL_Color inactive{130, 130, 150, 255};
    font.draw_centered(r, selected_ == 0 ? "> RESUME <" : "RESUME", center_x, 140.f,
                       selected_ == 0 ? active : inactive, 1);
    font.draw_centered(r, selected_ == 1 ? "> QUIT TO TITLE <" : "QUIT TO TITLE", center_x, 156.f,
                       selected_ == 1 ? active : inactive, 1);
}

} // namespace raven
