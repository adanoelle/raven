#include "scenes/title_scene.hpp"

#include "core/game.hpp"
#include "scenes/char_select_scene.hpp"

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
        game.scenes().swap(std::make_unique<CharacterSelectScene>(), game);
    }
}

void TitleScene::render(Game& game) {
    auto* r = game.renderer().sdl_renderer();
    const auto& font = game.font();
    constexpr float center_x = static_cast<float>(Renderer::VIRTUAL_WIDTH) / 2.f;

    SDL_SetRenderDrawColor(r, 20, 10, 40, 255);
    SDL_RenderClear(r);

    font.draw_centered(r, "RAVEN", center_x, 88.f, {230, 90, 140, 255}, 5);

    if (show_prompt_) {
        font.draw_centered(r, "PRESS START", center_x, 200.f, {255, 255, 255, 255}, 1);
    }
}

} // namespace raven
