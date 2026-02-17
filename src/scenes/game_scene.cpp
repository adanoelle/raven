#include "scenes/game_scene.hpp"

#include "core/game.hpp"
#include "core/string_id.hpp"
#include "ecs/components.hpp"
#include "ecs/player_class.hpp"
#include "ecs/systems/ai_system.hpp"
#include "ecs/systems/animation_system.hpp"
#include "ecs/systems/charged_shot_system.hpp"
#include "ecs/systems/cleanup_system.hpp"
#include "ecs/systems/collision_system.hpp"
#include "ecs/systems/concussion_shot_system.hpp"
#include "ecs/systems/damage_system.hpp"
#include "ecs/systems/dash_system.hpp"
#include "ecs/systems/emitter_system.hpp"
#include "ecs/systems/ground_slam_system.hpp"
#include "ecs/systems/hud_system.hpp"
#include "ecs/systems/input_system.hpp"
#include "ecs/systems/melee_system.hpp"
#include "ecs/systems/movement_system.hpp"
#include "ecs/systems/pickup_system.hpp"
#include "ecs/systems/render_system.hpp"
#include "ecs/systems/shooting_system.hpp"
#include "ecs/systems/tile_collision_system.hpp"
#include "ecs/systems/tilemap_render_system.hpp"
#include "ecs/systems/wave_system.hpp"
#include "scenes/game_over_scene.hpp"
#include "scenes/title_scene.hpp"

#include <spdlog/spdlog.h>

#include <random>

namespace raven {

GameScene::GameScene(ClassId::Id player_class) : selected_class_(player_class) {}

void GameScene::on_enter(Game& game) {
    spdlog::info("Entered game scene");

    game.input().set_renderer(game.renderer().sdl_renderer());
    game.input().set_window(game.renderer().sdl_window());

    auto& interner = game.registry().ctx().get<StringInterner>();
    pattern_lib_.set_interner(interner);
    pattern_lib_.load_manifest("assets/data/patterns/manifest.json");

    game.registry().ctx().emplace<std::mt19937>(std::random_device{}());
    auto& game_state = game.registry().ctx().emplace<GameState>();
    game_state.player_class = selected_class_;

    // Load stage manifest
    stage_loader_.load_manifest("assets/data/stages/stage_manifest.json");
    current_stage_ = 0;

    spawn_player(game);

    // Enter first room
    const auto* stage = stage_loader_.get(current_stage_);
    if (stage) {
        enter_room(game, stage->level);
    } else {
        // Fallback: load the test room if no stages available
        tilemap_.load(game.renderer().sdl_renderer(), "assets/maps/raven.ldtk", "Test_Room");
    }
}

void GameScene::on_exit(Game& game) {
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
    reg.emplace<MeleeCooldown>(player);
    reg.emplace<DashCooldown>(player);
    auto& weapon = reg.emplace<Weapon>(player);
    weapon.bullet_sheet = interner.intern("projectiles");

    // Apply class recipe
    switch (selected_class_) {
    case ClassId::Id::Brawler:
        apply_brawler(reg, player);
        break;
    case ClassId::Id::Sharpshooter:
        apply_sharpshooter(reg, player);
        break;
    }

    spdlog::debug("Player spawned at ({}, {})", spawn_x, spawn_y);
}

void GameScene::enter_room(Game& game, const std::string& level) {
    clear_room_entities(game);

    // Reload tilemap
    tilemap_ = Tilemap{};
    tilemap_.load(game.renderer().sdl_renderer(), "assets/maps/raven.ldtk", level);

    // Reposition player to PlayerStart
    auto& reg = game.registry();
    auto player_view = reg.view<Player, Transform2D, PreviousTransform>();
    for (auto [entity, player, tf, prev] : player_view.each()) {
        float spawn_x = static_cast<float>(Renderer::VIRTUAL_WIDTH) / 2.f;
        float spawn_y = static_cast<float>(Renderer::VIRTUAL_HEIGHT) / 2.f;
        if (const auto* start = tilemap_.find_spawn("PlayerStart")) {
            spawn_x = start->x;
            spawn_y = start->y;
        }
        tf.x = spawn_x;
        tf.y = spawn_y;
        prev.x = spawn_x;
        prev.y = spawn_y;
    }

    // Spawn Exit entities from tilemap
    auto exit_spawns = tilemap_.find_all_spawns("Exit");
    for (const auto* sp : exit_spawns) {
        std::string target;
        auto it = sp->fields.find("target_level");
        if (it != sp->fields.end()) {
            target = it->second;
        }

        auto exit_entity = reg.create();
        reg.emplace<Transform2D>(exit_entity, sp->x, sp->y);
        reg.emplace<CircleHitbox>(exit_entity, 12.f);
        reg.emplace<Exit>(exit_entity, Exit{std::move(target), false});
    }

    // Reset wave state
    const auto* stage = stage_loader_.get(current_stage_);
    auto& state = reg.ctx().get<GameState>();
    state.current_wave = 0;
    state.total_waves = stage ? static_cast<int>(stage->waves.size()) : 0;
    state.room_cleared = false;

    // Spawn wave 0
    if (stage && !stage->waves.empty()) {
        systems::spawn_wave(reg, tilemap_, *stage, 0, pattern_lib_);
    }

    spdlog::info("Entered room '{}'", level);
}

void GameScene::clear_room_entities(Game& game) {
    auto& reg = game.registry();

    // Destroy all entities except the player
    std::vector<entt::entity> to_destroy;
    auto& entities = reg.storage<entt::entity>();
    for (auto entity : entities) {
        if (reg.valid(entity) && !reg.any_of<Player>(entity)) {
            to_destroy.push_back(entity);
        }
    }
    for (auto entity : to_destroy) {
        if (reg.valid(entity)) {
            reg.destroy(entity);
        }
    }
}

void GameScene::update(Game& game, float dt) {
    auto& reg = game.registry();
    auto& input = game.input().state();

    // Run ECS systems in order
    systems::update_charged_shot(reg, input, dt);
    systems::update_input(reg, input, dt);
    systems::update_melee(reg, input, pattern_lib_, dt);
    systems::update_dash(reg, input, dt);
    systems::update_ground_slam(reg, input, dt);
    systems::update_concussion_shot(reg, input, dt);
    systems::update_shooting(reg, input, dt);
    systems::update_emitters(reg, pattern_lib_, dt);
    systems::update_ai(reg, tilemap_, dt);

    // Animation state switching (priority: Melee > Dash > Walk > Idle)
    auto anim_view = reg.view<Player, Velocity, Animation, Sprite, AnimationState>();
    for (auto [entity, player, vel, anim, sprite, state] : anim_view.each()) {
        AnimationState::State desired;
        if (reg.any_of<MeleeAttack>(entity) || reg.any_of<GroundSlam>(entity)) {
            desired = AnimationState::State::Melee;
        } else if (reg.any_of<Dash>(entity)) {
            desired = AnimationState::State::Dash;
        } else if ((vel.dx * vel.dx + vel.dy * vel.dy) > 1.f) {
            desired = AnimationState::State::Walk;
        } else {
            desired = AnimationState::State::Idle;
        }

        if (state.current != desired) {
            state.current = desired;
            switch (desired) {
            case AnimationState::State::Melee:
                sprite.frame_y = 1;
                anim.start_frame = 0;
                anim.end_frame = 2;
                anim.frame_duration = 0.05f;
                anim.looping = false;
                break;
            case AnimationState::State::Dash:
                sprite.frame_y = 1;
                anim.start_frame = 0;
                anim.end_frame = 2;
                anim.frame_duration = 0.04f;
                anim.looping = false;
                break;
            case AnimationState::State::Walk:
                sprite.frame_y = 1;
                anim.start_frame = 0;
                anim.end_frame = 5;
                anim.frame_duration = 0.1f;
                anim.looping = true;
                break;
            case AnimationState::State::Idle:
                sprite.frame_y = 0;
                anim.start_frame = 0;
                anim.end_frame = 3;
                anim.frame_duration = 0.25f;
                anim.looping = true;
                break;
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

    // Wave clear check + next wave spawn
    const auto* stage = stage_loader_.get(current_stage_);
    if (stage) {
        systems::update_waves(reg, tilemap_, *stage, pattern_lib_);
    }

    // Exit overlap check — room transition
    auto target = systems::check_exit_overlap(reg);
    if (!target.empty()) {
        current_stage_++;
        const auto* next = stage_loader_.get(current_stage_);
        if (next) {
            enter_room(game, next->level);
        } else {
            // Victory — return to title
            game.scenes().swap(std::make_unique<TitleScene>(), game);
        }
        return;
    }

    // Game over check
    auto* game_state = reg.ctx().find<GameState>();
    if (game_state && game_state->game_over) {
        game.scenes().swap(std::make_unique<GameOverScene>(), game);
        return;
    }

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

    // HUD overlay
    render_hud(game);
}

void GameScene::render_hud(Game& game) {
    systems::render_hud(game.registry(), game.renderer().sdl_renderer());
}

} // namespace raven
