#include "scenes/title_scene.hpp"

#include "core/game.hpp"
#include "scenes/char_select_scene.hpp"
#include "scenes/options_scene.hpp"

#include <spdlog/spdlog.h>

namespace raven {

void TitleScene::on_enter(Game& /*game*/) {
    spdlog::info("Entered title scene");
}

void TitleScene::update(Game& game, float /*dt*/) {
    const auto& input = game.input().state();

    // Menu navigation: vertical input edges
    if (input.move_y > 0.5f && prev_move_y_ <= 0.5f) {
        selected_ = (selected_ + 1) % ITEM_COUNT;
    } else if (input.move_y < -0.5f && prev_move_y_ >= -0.5f) {
        selected_ = (selected_ + ITEM_COUNT - 1) % ITEM_COUNT;
    }
    prev_move_y_ = input.move_y;

    if (input.confirm_pressed) {
        if (selected_ == 0) {
            game.scenes().swap(std::make_unique<CharacterSelectScene>(), game);
        } else {
            game.scenes().push(std::make_unique<OptionsScene>(), game);
        }
    }
}

void TitleScene::render(Game& game) {
    auto* r = game.renderer().sdl_renderer();
    const auto& font = game.font();
    constexpr float center_x = static_cast<float>(Renderer::VIRTUAL_WIDTH) / 2.f;

    SDL_SetRenderDrawColor(r, 20, 10, 40, 255);
    SDL_RenderClear(r);

    font.draw_centered(r, "RAVEN", center_x, 88.f, {230, 90, 140, 255}, 5);

    const SDL_Color active{255, 255, 255, 255};
    const SDL_Color inactive{130, 130, 150, 255};
    font.draw_centered(r, selected_ == 0 ? "> START <" : "START", center_x, 184.f,
                       selected_ == 0 ? active : inactive, 1);
    font.draw_centered(r, selected_ == 1 ? "> OPTIONS <" : "OPTIONS", center_x, 200.f,
                       selected_ == 1 ? active : inactive, 1);
}

} // namespace raven
