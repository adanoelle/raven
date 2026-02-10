#include "scenes/game_scene.hpp"

#include "core/game.hpp"
#include "ecs/components.hpp"
#include "ecs/systems/animation_system.hpp"
#include "ecs/systems/cleanup_system.hpp"
#include "ecs/systems/collision_system.hpp"
#include "ecs/systems/damage_system.hpp"
#include "ecs/systems/input_system.hpp"
#include "ecs/systems/movement_system.hpp"
#include "ecs/systems/projectile_system.hpp"
#include "ecs/systems/render_system.hpp"
#include "ecs/systems/shooting_system.hpp"
#include "ecs/systems/tile_collision_system.hpp"
#include "ecs/systems/tilemap_render_system.hpp"

#include <spdlog/spdlog.h>

namespace raven {

void GameScene::on_enter(Game& game) {
    spdlog::info("Entered game scene");

    game.input().set_renderer(game.renderer().sdl_renderer());
    game.input().set_window(game.renderer().sdl_window());
    tilemap_.load(game.renderer().sdl_renderer(), "assets/maps/raven.ldtk", "Test_Room");

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

    float spawn_x = static_cast<float>(Renderer::VIRTUAL_WIDTH) / 2.f;
    float spawn_y = static_cast<float>(Renderer::VIRTUAL_HEIGHT) / 2.f;
    if (tilemap_.is_loaded()) {
        if (const auto* start = tilemap_.find_spawn("PlayerStart")) {
            spawn_x = start->x;
            spawn_y = start->y;
        }
    }

    reg.emplace<Transform2D>(player, spawn_x, spawn_y);
    reg.emplace<PreviousTransform>(player, spawn_x, spawn_y);
    reg.emplace<Velocity>(player);
    reg.emplace<Player>(player);
    reg.emplace<Health>(player, 1.f, 1.f);
    reg.emplace<CircleHitbox>(player, 6.f);
    reg.emplace<RectHitbox>(player, 12.f, 14.f, 0.f, 2.f);
    reg.emplace<Sprite>(player, std::string{"player"}, 0, 0, 16, 16, 10);
    reg.emplace<Animation>(player, 0, 3, 0.25f, 0.f, 0, true);
    reg.emplace<AnimationState>(player);
    reg.emplace<AimDirection>(player, 1.f, 0.f);
    reg.emplace<ShootCooldown>(player, 0.f, 0.2f);

    spdlog::debug("Player spawned at ({}, {})", spawn_x, spawn_y);
}

void GameScene::update(Game& game, float dt) {
    auto& reg = game.registry();
    auto& input = game.input().state();

    // Run ECS systems in order
    systems::update_input(reg, input, dt);
    systems::update_shooting(reg, input, dt);

    // Animation state switching (velocity → idle/walk)
    auto anim_view = reg.view<Player, Velocity, Animation, Sprite, AnimationState>();
    for (auto [entity, player, vel, anim, sprite, state] : anim_view.each()) {
        bool moving = (vel.dx * vel.dx + vel.dy * vel.dy) > 1.f;

        auto desired = moving ? AnimationState::State::Walk : AnimationState::State::Idle;
        if (state.current != desired) {
            state.current = desired;
            if (desired == AnimationState::State::Walk) {
                sprite.frame_y = 1;
                anim.start_frame = 0;
                anim.end_frame = 5;
                anim.frame_duration = 0.1f;
                anim.looping = true;
            } else {
                sprite.frame_y = 0;
                anim.start_frame = 0;
                anim.end_frame = 3;
                anim.frame_duration = 0.25f;
                anim.looping = true;
            }
            anim.current_frame = anim.start_frame;
            anim.elapsed = 0.f;
        }

        // Flip sprite based on horizontal velocity
        if (vel.dx > 1.f)
            sprite.flip_x = false;
        else if (vel.dx < -1.f)
            sprite.flip_x = true;
    }

    systems::update_animation(reg, dt);
    systems::update_movement(reg, dt);
    systems::update_tile_collision(reg, tilemap_);
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

    // Render tilemap as background layer
    systems::render_tilemap(tilemap_, r);

    // Render all sprites via ECS (interpolated)
    float alpha = game.clock().interpolation_alpha;
    systems::render_sprites(game.registry(), r, game.sprites(), alpha);

    // // Debug: draw player hitbox (interpolated to match sprite)
    // auto view = game.registry().view<Transform2D, CircleHitbox, Player>();
    // for (auto [entity, tf, hb, _] : view.each()) {
    //     float hb_x = tf.x;
    //     float hb_y = tf.y;
    //     if (auto* prev = game.registry().try_get<PreviousTransform>(entity)) {
    //         hb_x = prev->x + (tf.x - prev->x) * alpha;
    //         hb_y = prev->y + (tf.y - prev->y) * alpha;
    //     }
    //     SDL_SetRenderDrawColor(r, 255, 255, 255, 100);
    //     // Approximate circle with small rect for debug
    //     SDL_Rect debug_rect{static_cast<int>(hb_x - hb.radius), static_cast<int>(hb_y -
    //     hb.radius),
    //                         static_cast<int>(hb.radius * 2.f), static_cast<int>(hb.radius
    //                         * 2.f)};
    //     SDL_RenderDrawRect(r, &debug_rect);
    // }
}

} // namespace raven
