#include "scenes/game_scene.hpp"
#include "core/game.hpp"
#include "ecs/components.hpp"
#include "ecs/systems/input_system.hpp"
#include "ecs/systems/movement_system.hpp"
#include "ecs/systems/projectile_system.hpp"
#include "ecs/systems/collision_system.hpp"
#include "ecs/systems/damage_system.hpp"
#include "ecs/systems/cleanup_system.hpp"
#include "ecs/systems/render_system.hpp"

#include <spdlog/spdlog.h>

namespace raven {

void GameScene::on_enter(Game& game) {
    spdlog::info("Entered game scene");
    spawn_player(game);
}

void GameScene::on_exit(Game& game) {
    // Clear all entities
    game.registry().clear();
    spdlog::info("Exited game scene");
}

void GameScene::spawn_player(Game& game) {
    auto& reg = game.registry();
    auto player = reg.create();

    reg.emplace<Transform2D>(player,
        static_cast<float>(Renderer::VIRTUAL_WIDTH) / 2.f,
        static_cast<float>(Renderer::VIRTUAL_HEIGHT) / 2.f
    );
    reg.emplace<Velocity>(player);
    reg.emplace<Player>(player);
    reg.emplace<Health>(player, 1.f, 1.f);
    reg.emplace<CircleHitbox>(player, 6.f);
    reg.emplace<Sprite>(player, std::string{"player"}, 0, 0, 16, 16, 10);

    spdlog::debug("Player spawned at ({}, {})",
                  Renderer::VIRTUAL_WIDTH / 2.f,
                  Renderer::VIRTUAL_HEIGHT / 2.f);
}

void GameScene::update(Game& game, float dt) {
    auto& reg = game.registry();
    auto& input = game.input().state();

    // Run ECS systems in order
    systems::update_input(reg, input);
    systems::update_movement(reg, dt);
    systems::update_projectiles(reg, dt);
    systems::update_collision(reg);
    systems::update_damage(reg);
    systems::update_cleanup(reg, Renderer::VIRTUAL_WIDTH, Renderer::VIRTUAL_HEIGHT);

    // Check for pause
    if (input.pause_pressed) {
        // TODO: push pause scene
    }
}

void GameScene::render(Game& game) {
    auto* r = game.renderer().sdl_renderer();

    // Dark background
    SDL_SetRenderDrawColor(r, 8, 8, 24, 255);
    SDL_RenderClear(r);

    // Render all sprites via ECS
    systems::render_sprites(game.registry(), r, game.sprites());

    // Debug: draw player hitbox
    auto view = game.registry().view<Transform2D, CircleHitbox, Player>();
    for (auto [entity, tf, hb, _] : view.each()) {
        SDL_SetRenderDrawColor(r, 255, 255, 255, 100);
        // Approximate circle with small rect for debug
        SDL_Rect debug_rect{
            static_cast<int>(tf.x - hb.radius),
            static_cast<int>(tf.y - hb.radius),
            static_cast<int>(hb.radius * 2.f),
            static_cast<int>(hb.radius * 2.f)
        };
        SDL_RenderDrawRect(r, &debug_rect);
    }
}

} // namespace raven
