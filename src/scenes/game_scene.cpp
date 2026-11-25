#include "scenes/game_scene.hpp"

#include "core/game.hpp"
#include "core/paths.hpp"
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
#include "rendering/animation_library.hpp"
#include "rendering/sheet_ids.hpp"
#include "scenes/game_over_scene.hpp"
#include "scenes/pause_scene.hpp"
#include "scenes/title_scene.hpp"
#include "scenes/victory_scene.hpp"

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <cmath>
#include <random>

namespace raven {

GameScene::GameScene(ClassId::Id player_class) : selected_class_(player_class) {}

void GameScene::on_enter(Game& game) {
    spdlog::info("Entered game scene");

    game.input().set_renderer(game.renderer().sdl_renderer());
    game.input().set_window(game.renderer().sdl_window());

    auto& interner = game.registry().ctx().get<StringInterner>();
    pattern_lib_.set_interner(interner);
    pattern_lib_.load_manifest(paths::asset("assets/data/patterns/manifest.json"));

    // Rebuilt every run, so edits to enemies.json apply from the next run
    enemy_lib_ = EnemyLibrary{};
    enemy_lib_.load_file(paths::asset("assets/data/enemies.json"));

    // Seed from SDL's clocks rather than std::random_device: some console
    // standard libraries implement the latter as a constant or throw.
    const auto seed = static_cast<std::uint32_t>(SDL_GetPerformanceCounter() ^ SDL_GetTicksNS());
    game.registry().ctx().emplace<std::mt19937>(seed);
    game.registry().ctx().emplace<AudioQueue>();

    // Erase any stale GameState first: ctx().emplace is a no-op when the
    // value already exists, and the victory path (swap to TitleScene) does
    // not go through GameOverScene::on_exit, which normally erases it.
    game.registry().ctx().erase<GameState>();
    auto& game_state = game.registry().ctx().emplace<GameState>();
    game_state.player_class = selected_class_;

    // Load stage manifest
    stage_loader_.load_manifest(paths::asset("assets/data/stages/stage_manifest.json"));
    current_stage_ = 0;

    spawn_player(game);

    // Enter first room
    const auto* stage = stage_loader_.get(current_stage_);
    if (stage) {
        enter_room(game, stage->level);
    } else {
        // Fallback: load the test room if no stages available
        tilemap_.load(game.renderer().sdl_renderer(), paths::asset("assets/maps/raven.ldtk"),
                      "Test_Room");
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
    reg.emplace<CircleHitbox>(player, 6.f, 0.f, 2.f);
    reg.emplace<RectHitbox>(player, 12.f, 14.f, 0.f, 2.f);
    // A size of 0 draws the sheet's frame size (config.json), so a class
    // sheet with larger frames needs no change here
    reg.emplace<Sprite>(player, interner.intern(sheets::PLAYER), 0, 0, 0, 0, 10, false, 0.f, -5.f);
    reg.emplace<Animation>(player, Animation{interner.intern(clips::IDLE)});
    reg.emplace<AimDirection>(player, 1.f, 0.f);
    reg.emplace<ShootCooldown>(player, 0.f, 0.2f);
    reg.emplace<MeleeCooldown>(player);
    reg.emplace<DashCooldown>(player);
    auto& weapon = reg.emplace<Weapon>(player);
    weapon.bullet_sheet = interner.intern(sheets::PROJECTILES);

    // Apply class recipe
    switch (selected_class_) {
    case ClassId::Id::Brawler:
        apply_brawler(reg, player);
        break;
    case ClassId::Id::Sharpshooter:
        apply_sharpshooter(reg, player);
        break;
    case ClassId::Id::Knight:
        apply_knight(reg, player);
        break;
    }

    spdlog::debug("Player spawned at ({}, {})", spawn_x, spawn_y);
}

void GameScene::enter_room(Game& game, const std::string& level) {
    clear_room_entities(game);

    // Reload tilemap
    tilemap_ = Tilemap{};
    if (!tilemap_.load(game.renderer().sdl_renderer(), paths::asset("assets/maps/raven.ldtk"),
                       level)) {
        spdlog::error("Room '{}' failed to load; check that the level exists in raven.ldtk", level);
    }

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
    auto& interner = reg.ctx().get<StringInterner>();
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
        reg.emplace<Sprite>(exit_entity, interner.intern(sheets::PROPS),
                            sheets::PROP_FRAME_EXIT_CLOSED, 0, 16, 16, 1);
        reg.emplace<Exit>(exit_entity, Exit{std::move(target), false});
    }
    if (exit_spawns.empty()) {
        spdlog::error("Room '{}' has no Exit entity, so the run can't continue past it", level);
    }

    // Reset wave state
    const auto* stage = stage_loader_.get(current_stage_);
    auto& state = reg.ctx().get<GameState>();
    state.current_wave = 0;
    state.total_waves = stage ? static_cast<int>(stage->waves.size()) : 0;
    state.room_cleared = false;

    // Spawn wave 0
    if (stage && !stage->waves.empty()) {
        systems::spawn_wave(reg, tilemap_, *stage, 0, pattern_lib_, enemy_lib_);
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
    const auto& input = game.input().state();

    // Run ECS systems in order
    systems::update_charged_shot(reg, input, dt);
    systems::update_input(reg, input, dt);
    // Dash before melee: pressing both on one tick starts the dash first,
    // so the melee is the Knight's spin.
    systems::update_dash(reg, input, dt);
    systems::update_melee(reg, input, tilemap_, pattern_lib_, dt);
    systems::update_ground_slam(reg, input, tilemap_, dt);
    systems::update_concussion_shot(reg, input, tilemap_, dt);
    systems::update_shooting(reg, input, dt);
    systems::update_emitters(reg, pattern_lib_, dt);
    systems::update_ai(reg, tilemap_, dt);

    systems::update_player_animation(reg);
    systems::update_animation(reg, dt);
    systems::update_movement(reg, dt);
    systems::update_tile_collision(reg, tilemap_);
    systems::update_bullet_walls(reg, tilemap_);
    systems::update_collision(reg);
    systems::update_pickups(reg);
    systems::update_weapon_decay(reg, dt);
    systems::update_damage(reg, pattern_lib_, dt);
    systems::update_cleanup(reg, dt, Renderer::VIRTUAL_WIDTH, Renderer::VIRTUAL_HEIGHT);

    // Wave clear check + next wave spawn
    const auto* stage = stage_loader_.get(current_stage_);
    if (stage) {
        systems::update_waves(reg, tilemap_, *stage, pattern_lib_, enemy_lib_, dt);
    }

    // Forward sound requests pushed by the systems above to the engine.
    // Dedupe per tick: N same-tick copies of one effect play sample-aligned
    // and sum to N-times amplitude (a distorted pop, not a louder hit) —
    // e.g. one shotgun burst hitting five enemies at once.
    if (auto* audio_queue = reg.ctx().find<AudioQueue>()) {
        static_assert(static_cast<uint32_t>(Sfx::Count) <= 32,
                      "played_mask is 32 bits; widen it before growing Sfx");
        uint32_t played_mask = 0;
        for (Sfx sfx : audio_queue->events) {
            const uint32_t bit = 1u << static_cast<uint32_t>(sfx);
            if (played_mask & bit) {
                continue;
            }
            played_mask |= bit;
            game.audio().play(sfx_sound_name(sfx));
        }
        audio_queue->events.clear();
    }

    // Game over check. Runs before the exit check: the player entity
    // outlives its final death, and touching an exit on that same tick must
    // not count as clearing the stage.
    auto* game_state = reg.ctx().find<GameState>();
    if (game_state && game_state->game_over) {
        game.scenes().swap(std::make_unique<GameOverScene>(), game);
        return;
    }

    // Exit overlap check — room transition. Rooms follow the stage list;
    // an exit's target_level is only checked against it.
    if (const auto* exit = systems::check_exit_overlap(reg)) {
        const auto* next = stage_loader_.get(current_stage_ + 1);
        const std::string expected = next ? next->level : std::string{};
        if (!exit->target_level.empty() && exit->target_level != expected) {
            spdlog::warn("Exit targets level '{}' but the next stage uses '{}'; following the "
                         "stage list",
                         exit->target_level, next ? expected : "(none: final stage)");
        }

        current_stage_++;
        if (next) {
            enter_room(game, next->level);
        } else {
            // Final stage cleared
            game.scenes().swap(std::make_unique<VictoryScene>(), game);
        }
        return;
    }

    // Pause: push the overlay; this scene stops updating but keeps rendering.
    // Losing the controller pauses too, so a flat battery or a pulled cable
    // doesn't leave the player standing in the bullets.
    if (input.pause_pressed || input.gamepad_lost) {
        game.scenes().push(std::make_unique<PauseScene>(), game);
        return;
    }
}

void GameScene::on_suspend(Game& game) {
    // Only when this scene is live: an overlay (pause/options) on top
    // already means gameplay is frozen.
    if (game.scenes().is_top(this)) {
        game.scenes().push(std::make_unique<PauseScene>(), game);
    }
}

void GameScene::render(Game& game) {
    auto* r = game.renderer().sdl_renderer();

    // Dark background
    SDL_SetRenderDrawColor(r, 8, 8, 24, 255);
    SDL_RenderClear(r);

    // Render tilemap as background layer
    systems::render_tilemap(tilemap_, r);

    // Render all sprites via ECS (interpolated). While an overlay (pause)
    // is on top, this scene no longer ticks, so snap to current positions —
    // a varying alpha would make sprites shimmer between prev and current.
    float alpha = game.scenes().is_top(this) ? game.clock().interpolation_alpha : 1.f;
#ifdef RAVEN_ENABLE_SHADER_LIGHTING
    // By default only the tiles are lit: sprites are drawn onto the lit
    // room, so players, enemies and bullets keep the colours they were
    // drawn in whatever the light is doing.
    const bool light_sprites = game.registry().ctx().emplace<LightingSettings>().light_sprites;
    if (!light_sprites) {
        apply_lighting(game, alpha);
    }
    systems::render_sprites(game.registry(), r, game.sprites(), alpha);
    if (light_sprites) {
        apply_lighting(game, alpha);
    }
    draw_fires(game);
#else
    systems::render_sprites(game.registry(), r, game.sprites(), alpha);
#endif

    // HUD overlay
    render_hud(game);
}

#ifdef RAVEN_ENABLE_SHADER_LIGHTING
void GameScene::apply_lighting(Game& game, float alpha) {
    auto& reg = game.registry();
    const auto& settings = reg.ctx().emplace<LightingSettings>();
    if (!settings.enabled) {
        return;
    }

    // Persistent scratch buffer, as the sprite renderer keeps.
    auto& lights = reg.ctx().emplace<std::vector<Light>>();
    lights.clear();
    auto light_at = [&](entt::entity entity, const Transform2D& tf, int radius, int level) {
        float x = tf.x;
        float y = tf.y;
        if (const auto* prev = reg.try_get<PreviousTransform>(entity)) {
            x = prev->x + (tf.x - prev->x) * alpha;
            y = prev->y + (tf.y - prev->y) * alpha;
        }
        return Light{x, y, static_cast<float>(radius), level, false};
    };

    // Fires placed in the room ("Fire" entities in LDtk). The light swells
    // and sinks slowly and sways a little, as two slow waves of different
    // length so it does not visibly repeat; each fire is out of step with
    // the others.
    const auto fires = tilemap_.find_all_spawns("Fire");
    const float seconds = static_cast<float>(SDL_GetTicks() % 3'600'000) / 1000.f;
    for (size_t i = 0; i < fires.size(); ++i) {
        if (settings.fire_level <= 0) {
            break;
        }
        const float phase = static_cast<float>(i) * 2.4f;
        const float breath = 0.6f * std::sin(seconds * 1.9f + phase) +
                             0.4f * std::sin(seconds * 3.1f + phase * 1.7f);
        const float sway = std::sin(seconds * 1.3f + phase * 0.6f);
        const auto flicker = static_cast<float>(settings.fire_flicker);
        lights.push_back({fires[i]->x + 1.5f * sway, fires[i]->y + 0.8f * breath,
                          static_cast<float>(settings.fire_radius) + 0.5f * flicker * breath,
                          settings.fire_level + static_cast<int>(std::lround(flicker * breath)),
                          false});
    }

    // Then players, then bullets: when the loop is in use and there are
    // more lights than it takes, it is bullets that go without.
    for (auto [entity, player, tf] : reg.view<Player, Transform2D>().each()) {
        if (settings.player_zones > 0) {
            lights.push_back(
                light_at(entity, tf, settings.player_radius, settings.player_zones * 16));
        }
    }
    const Uint64 crackle_beat = SDL_GetTicks() / 83;
    for (auto [entity, bullet, tf] : reg.view<Bullet, Transform2D>().each()) {
        if (settings.use_loop &&
            lights.size() >= static_cast<size_t>(LightingPass::MAX_LOOP_LIGHTS)) {
            break;
        }
        if (settings.bullet_level <= 0) {
            break;
        }
        // Each bullet's light jumps between strengths about twelve times a
        // second, on its own rhythm: a crackle, like a spark
        const Uint64 h = (crackle_beat + static_cast<Uint64>(entt::to_integral(entity)) * 7919u) *
                         0x9E3779B97F4A7C15ull;
        const int jump = static_cast<int>((h >> 58) % 5) - 2; // -2..2
        Light light = light_at(entity, tf, settings.bullet_radius,
                               settings.bullet_level + jump * settings.bullet_crackle / 2);
        // Enemy fire is electric: its light is the cool one
        light.cool = bullet.owner == Bullet::Owner::Enemy;
        lights.push_back(light);
    }
    game.renderer().apply_lighting(settings, lights);
}

void GameScene::draw_fires(Game& game) {
    // A stand-in flame until there is art for one. Drawn after the
    // lighting pass, so it is not lit itself.
    auto* r = game.renderer().sdl_renderer();
    for (const auto* fire : tilemap_.find_all_spawns("Fire")) {
        const float x = std::floor(fire->x);
        const float y = std::floor(fire->y);
        SDL_SetRenderDrawColor(r, 58, 40, 44, 255);
        const SDL_FRect bowl{x - 3.f, y + 1.f, 6.f, 3.f};
        SDL_RenderFillRect(r, &bowl);
        SDL_SetRenderDrawColor(r, 214, 120, 54, 255);
        const SDL_FRect flame{x - 2.f, y - 3.f, 4.f, 4.f};
        SDL_RenderFillRect(r, &flame);
        SDL_SetRenderDrawColor(r, 246, 205, 110, 255);
        const SDL_FRect core{x - 1.f, y - 1.f, 2.f, 2.f};
        SDL_RenderFillRect(r, &core);
    }
}
#endif

void GameScene::render_hud(Game& game) {
    systems::render_hud(game.registry(), game.renderer().sdl_renderer(), game.font());
}

} // namespace raven
