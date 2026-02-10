#include "scenes/game_scene.hpp"

#include "core/game.hpp"
#include "core/string_id.hpp"
#include "ecs/components.hpp"
#include "ecs/systems/animation_system.hpp"
#include "ecs/systems/cleanup_system.hpp"
#include "ecs/systems/collision_system.hpp"
#include "ecs/systems/damage_system.hpp"
#include "ecs/systems/emitter_system.hpp"
#include "ecs/systems/input_system.hpp"
#include "ecs/systems/movement_system.hpp"
#include "ecs/systems/pickup_system.hpp"
#include "ecs/systems/render_system.hpp"
#include "ecs/systems/shooting_system.hpp"
#include "ecs/systems/tile_collision_system.hpp"
#include "ecs/systems/tilemap_render_system.hpp"

#include <spdlog/spdlog.h>

#include <random>

namespace raven {

void GameScene::on_enter(Game& game) {
    spdlog::info("Entered game scene");

    game.input().set_renderer(game.renderer().sdl_renderer());
    game.input().set_window(game.renderer().sdl_window());
    tilemap_.load(game.renderer().sdl_renderer(), "assets/maps/raven.ldtk", "Test_Room");

    auto& interner = game.registry().ctx().get<StringInterner>();
    pattern_lib_.set_interner(interner);
    pattern_lib_.load_manifest("assets/data/patterns/manifest.json");

    game.registry().ctx().emplace<std::mt19937>(std::random_device{}());

    spawn_player(game);
    spawn_enemies(game);
}

void GameScene::on_exit(Game& game) {
    // Clear all entities
    game.registry().clear();
    spdlog::info("Exited game scene");
}

void GameScene::spawn_player(Game& game) {
    auto& reg = game.registry();
    auto& interner = reg.ctx().get<StringInterner>();
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
    reg.emplace<Sprite>(player, interner.intern("player"), 0, 0, 16, 16, 10);
    reg.emplace<Animation>(player, 0, 3, 0.25f, 0.f, 0, true);
    reg.emplace<AnimationState>(player);
    reg.emplace<AimDirection>(player, 1.f, 0.f);
    reg.emplace<ShootCooldown>(player, 0.f, 0.2f);
    auto& weapon = reg.emplace<Weapon>(player);
    weapon.bullet_sheet = interner.intern("projectiles");

    spdlog::debug("Player spawned at ({}, {})", spawn_x, spawn_y);
}

void GameScene::spawn_enemies(Game& game) {
    auto& reg = game.registry();
    auto& interner = reg.ctx().get<StringInterner>();

    struct EnemyDef {
        float x;
        float y;
        Enemy::Type type;
        float hp;
        std::string pattern;
        int score;
        int frame;
    };

    const EnemyDef defs[] = {
        {100.f, 60.f, Enemy::Type::Grunt, 1.f, "spiral_3way", 100, 0},
        {240.f, 50.f, Enemy::Type::Grunt, 1.f, "spiral_3way", 100, 0},
        {240.f, 200.f, Enemy::Type::Grunt, 1.f, "spiral_3way", 100, 0},
        {380.f, 60.f, Enemy::Type::Mid, 3.f, "aimed_burst", 300, 1},
        {140.f, 140.f, Enemy::Type::Mid, 3.f, "aimed_burst", 300, 1},
        {340.f, 140.f, Enemy::Type::Boss, 10.f, "nova_legendary", 1000, 2},
    };

    for (const auto& def : defs) {
        auto enemy = reg.create();
        reg.emplace<Transform2D>(enemy, def.x, def.y);
        reg.emplace<PreviousTransform>(enemy, def.x, def.y);
        reg.emplace<Enemy>(enemy, def.type);
        reg.emplace<Health>(enemy, def.hp, def.hp);
        reg.emplace<CircleHitbox>(enemy, 7.f);
        reg.emplace<Sprite>(enemy, interner.intern("enemies"), def.frame, 0, 16, 16, 10);
        reg.emplace<ScoreValue>(enemy, def.score);
        reg.emplace<BulletEmitter>(enemy, BulletEmitter{interner.intern(def.pattern), {}, {}});
    }

    spdlog::debug("Spawned {} playtest enemies", std::size(defs));
}

void GameScene::update(Game& game, float dt) {
    auto& reg = game.registry();
    auto& input = game.input().state();

    // Run ECS systems in order
    systems::update_input(reg, input, dt);
    systems::update_shooting(reg, input, dt);
    systems::update_emitters(reg, pattern_lib_, dt);

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

        // Flip sprite to face aim direction
        if (auto* aim = reg.try_get<AimDirection>(entity)) {
            if (aim->x > 0.f)
                sprite.flip_x = false;
            else if (aim->x < 0.f)
                sprite.flip_x = true;
        }
    }

    systems::update_animation(reg, dt);
    systems::update_movement(reg, dt);
    systems::update_tile_collision(reg, tilemap_);
    systems::update_collision(reg);
    systems::update_pickups(reg);
    systems::update_weapon_decay(reg, dt);
    systems::update_damage(reg, pattern_lib_, dt);
    systems::update_cleanup(reg, dt, Renderer::VIRTUAL_WIDTH, Renderer::VIRTUAL_HEIGHT);

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
