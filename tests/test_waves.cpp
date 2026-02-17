#include "core/string_id.hpp"
#include "ecs/components.hpp"
#include "ecs/systems/damage_system.hpp"
#include "ecs/systems/wave_system.hpp"
#include "patterns/pattern_library.hpp"
#include "rendering/tilemap.hpp"

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <random>

using namespace raven;
using Catch::Approx;

namespace {

/// @brief Create a minimal player entity for wave tests.
entt::entity make_player(entt::registry& reg, float x, float y) {
    auto player = reg.create();
    reg.emplace<Transform2D>(player, x, y);
    reg.emplace<PreviousTransform>(player, x, y);
    reg.emplace<Player>(player);
    reg.emplace<Velocity>(player);
    reg.emplace<CircleHitbox>(player, 6.f);
    reg.emplace<Health>(player, 1.f, 1.f);
    return player;
}

/// @brief Create a tilemap with test spawn points via init_collision + manual spawns.
/// Note: We can't call find_all_spawns on a tilemap without loading from LDtk,
/// so wave tests construct spawn points differently — by using a StageDef with
/// spawn_index = 0 and providing EnemySpawn positions via a loaded tilemap.
/// For unit tests, we'll test the StageLoader parsing directly, and test
/// wave/room logic with a minimal tilemap.

/// @brief Build a test StageDef with one wave containing specified enemy count.
StageDef make_test_stage(int num_enemies, const std::string& pattern = "spiral_3way") {
    StageDef stage;
    stage.name = "test_stage";
    stage.level = "Test_Room";

    WaveDef wave;
    for (int i = 0; i < num_enemies; ++i) {
        WaveEnemyDef def;
        def.spawn_index = i;
        def.type = Enemy::Type::Grunt;
        def.pattern = pattern;
        def.hp = 1.f;
        def.score = 100;
        def.ai = AiBehavior::Archetype::Chaser;
        def.contact_damage = (i == 0); // First enemy gets contact damage
        wave.enemies.push_back(def);
    }
    stage.waves.push_back(wave);
    return stage;
}

/// @brief Build a two-wave test stage.
StageDef make_two_wave_stage() {
    StageDef stage;
    stage.name = "two_wave_stage";
    stage.level = "Test_Room";

    WaveDef wave1;
    {
        WaveEnemyDef def;
        def.spawn_index = 0;
        def.type = Enemy::Type::Grunt;
        def.pattern = "spiral_3way";
        def.hp = 1.f;
        def.score = 100;
        def.ai = AiBehavior::Archetype::Chaser;
        wave1.enemies.push_back(def);
    }
    stage.waves.push_back(wave1);

    WaveDef wave2;
    {
        WaveEnemyDef def;
        def.spawn_index = 0;
        def.type = Enemy::Type::Mid;
        def.pattern = "spiral_3way";
        def.hp = 3.f;
        def.score = 300;
        def.ai = AiBehavior::Archetype::Stalker;
        wave2.enemies.push_back(def);
    }
    stage.waves.push_back(wave2);

    return stage;
}

} // namespace

// ── Wave spawning tests ────────────────────────────────────────────

TEST_CASE("spawn_wave creates correct enemy count", "[waves]") {
    entt::registry reg;
    auto& interner = reg.ctx().emplace<StringInterner>();
    PatternLibrary patterns;
    patterns.set_interner(interner);

    nlohmann::json pj = {
        {"name", "spiral_3way"},
        {"emitters", {{{"type", "radial"}, {"count", 3}, {"speed", 100.f}, {"fire_rate", 0.5f}}}}};
    patterns.load_from_json(pj);

    Tilemap tilemap;
    // No LDtk loaded — enemies will use fallback center position

    auto stage = make_test_stage(3);
    reg.ctx().emplace<GameState>();

    systems::spawn_wave(reg, tilemap, stage, 0, patterns);

    auto enemy_view = reg.view<Enemy>();
    REQUIRE(enemy_view.size() == 3);
}

TEST_CASE("spawn_wave assigns contact damage to first enemy only", "[waves]") {
    entt::registry reg;
    auto& interner = reg.ctx().emplace<StringInterner>();
    PatternLibrary patterns;
    patterns.set_interner(interner);

    nlohmann::json pj = {
        {"name", "spiral_3way"},
        {"emitters", {{{"type", "radial"}, {"count", 3}, {"speed", 100.f}, {"fire_rate", 0.5f}}}}};
    patterns.load_from_json(pj);

    Tilemap tilemap;
    auto stage = make_test_stage(2);
    reg.ctx().emplace<GameState>();

    systems::spawn_wave(reg, tilemap, stage, 0, patterns);

    auto cd_view = reg.view<ContactDamage>();
    REQUIRE(cd_view.size() == 1); // Only first enemy has contact_damage=true
}

// ── Wave progression tests ──────────────────────────────────────────

TEST_CASE("update_waves advances to next wave when all enemies dead", "[waves]") {
    entt::registry reg;
    auto& interner = reg.ctx().emplace<StringInterner>();
    PatternLibrary patterns;
    patterns.set_interner(interner);

    nlohmann::json pj = {
        {"name", "spiral_3way"},
        {"emitters", {{{"type", "radial"}, {"count", 3}, {"speed", 100.f}, {"fire_rate", 0.5f}}}}};
    patterns.load_from_json(pj);

    Tilemap tilemap;
    auto stage = make_two_wave_stage();

    auto& state = reg.ctx().emplace<GameState>();
    state.current_wave = 0;
    state.total_waves = 2;

    // Spawn wave 0
    systems::spawn_wave(reg, tilemap, stage, 0, patterns);
    REQUIRE(reg.view<Enemy>().size() == 1);

    // Kill all enemies
    auto enemy_view = reg.view<Enemy>();
    for (auto [entity, enemy] : enemy_view.each()) {
        reg.destroy(entity);
    }
    REQUIRE(reg.view<Enemy>().size() == 0);

    // update_waves should advance to wave 1
    systems::update_waves(reg, tilemap, stage, patterns);
    REQUIRE(state.current_wave == 1);
    REQUIRE(reg.view<Enemy>().size() == 1); // Wave 2 has 1 enemy
}

TEST_CASE("update_waves sets room_cleared when all waves exhausted", "[waves]") {
    entt::registry reg;
    auto& interner = reg.ctx().emplace<StringInterner>();
    PatternLibrary patterns;
    patterns.set_interner(interner);

    nlohmann::json pj = {
        {"name", "spiral_3way"},
        {"emitters", {{{"type", "radial"}, {"count", 3}, {"speed", 100.f}, {"fire_rate", 0.5f}}}}};
    patterns.load_from_json(pj);

    Tilemap tilemap;
    auto stage = make_test_stage(1); // Single wave with 1 enemy

    auto& state = reg.ctx().emplace<GameState>();
    state.current_wave = 0;
    state.total_waves = 1;

    systems::spawn_wave(reg, tilemap, stage, 0, patterns);

    // Kill the enemy
    auto enemy_view = reg.view<Enemy>();
    for (auto [entity, enemy] : enemy_view.each()) {
        reg.destroy(entity);
    }

    // update_waves should mark room cleared
    systems::update_waves(reg, tilemap, stage, patterns);
    REQUIRE(state.room_cleared);
}

TEST_CASE("Exit entities marked open when room cleared", "[waves]") {
    entt::registry reg;
    auto& interner = reg.ctx().emplace<StringInterner>();
    PatternLibrary patterns;
    patterns.set_interner(interner);

    nlohmann::json pj = {
        {"name", "spiral_3way"},
        {"emitters", {{{"type", "radial"}, {"count", 3}, {"speed", 100.f}, {"fire_rate", 0.5f}}}}};
    patterns.load_from_json(pj);

    Tilemap tilemap;
    auto stage = make_test_stage(1);

    auto& state = reg.ctx().emplace<GameState>();
    state.current_wave = 0;
    state.total_waves = 1;

    // Create an exit entity
    auto exit_ent = reg.create();
    reg.emplace<Transform2D>(exit_ent, 400.f, 200.f);
    reg.emplace<Exit>(exit_ent, Exit{"Room_02", false});

    // Spawn and kill enemies
    systems::spawn_wave(reg, tilemap, stage, 0, patterns);
    auto enemy_view = reg.view<Enemy>();
    for (auto [entity, enemy] : enemy_view.each()) {
        reg.destroy(entity);
    }

    systems::update_waves(reg, tilemap, stage, patterns);
    REQUIRE(state.room_cleared);

    auto& exit = reg.get<Exit>(exit_ent);
    REQUIRE(exit.open);
}

// ── Exit overlap tests ─────────────────────────────────────────────

TEST_CASE("check_exit_overlap returns empty when exit closed", "[waves]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    make_player(reg, 100.f, 100.f);

    auto exit_ent = reg.create();
    reg.emplace<Transform2D>(exit_ent, 100.f, 100.f);    // Same position as player
    reg.emplace<Exit>(exit_ent, Exit{"Room_02", false}); // Closed

    auto result = systems::check_exit_overlap(reg);
    REQUIRE(result.empty());
}

TEST_CASE("check_exit_overlap returns target_level when open and overlapping", "[waves]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    make_player(reg, 100.f, 100.f);

    auto exit_ent = reg.create();
    reg.emplace<Transform2D>(exit_ent, 105.f, 100.f);   // Close to player
    reg.emplace<Exit>(exit_ent, Exit{"Room_02", true}); // Open

    auto result = systems::check_exit_overlap(reg);
    REQUIRE(result == "Room_02");
}

TEST_CASE("check_exit_overlap returns empty when player far from exit", "[waves]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    make_player(reg, 100.f, 100.f);

    auto exit_ent = reg.create();
    reg.emplace<Transform2D>(exit_ent, 400.f, 400.f);   // Far away
    reg.emplace<Exit>(exit_ent, Exit{"Room_02", true}); // Open but distant

    auto result = systems::check_exit_overlap(reg);
    REQUIRE(result.empty());
}

// ── Score tracking tests ───────────────────────────────────────────

TEST_CASE("Score accumulates on enemy death via update_damage", "[waves]") {
    entt::registry reg;
    auto& interner = reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);
    PatternLibrary patterns;

    auto& state = reg.ctx().emplace<GameState>();

    // Create player
    auto player = make_player(reg, 100.f, 100.f);
    reg.emplace<AimDirection>(player, 1.f, 0.f);

    // Create enemy with 0 HP (already dead)
    auto enemy = reg.create();
    reg.emplace<Transform2D>(enemy, 200.f, 200.f);
    reg.emplace<Enemy>(enemy, Enemy::Type::Grunt);
    reg.emplace<Health>(enemy, 0.f, 1.f);
    reg.emplace<ScoreValue>(enemy, 100);

    systems::update_damage(reg, patterns, 1.f / 120.f);

    REQUIRE(state.score == 100);
}

TEST_CASE("Game over flag set when player loses all lives", "[waves]") {
    entt::registry reg;
    auto& interner = reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);
    PatternLibrary patterns;

    auto& state = reg.ctx().emplace<GameState>();

    // Create player with 1 life and 0 HP
    auto player = make_player(reg, 100.f, 100.f);
    reg.get<Player>(player).lives = 1;
    reg.get<Health>(player).current = 0.f;

    systems::update_damage(reg, patterns, 1.f / 120.f);

    REQUIRE(state.game_over);
}

// ── StageLoader parsing tests ──────────────────────────────────────

TEST_CASE("StageLoader parses stage JSON correctly", "[waves]") {
    StageLoader loader;

    nlohmann::json j = {{"name", "test_stage"},
                        {"level", "Test_Room"},
                        {"waves",
                         {{{"enemies",
                            {{{"spawn_index", 0},
                              {"type", "grunt"},
                              {"pattern", "spiral_3way"},
                              {"hp", 1.0},
                              {"score", 100},
                              {"ai", "chaser"},
                              {"contact_damage", true}},
                             {{"spawn_index", 1},
                              {"type", "mid"},
                              {"pattern", "aimed_burst"},
                              {"hp", 3.0},
                              {"score", 300},
                              {"ai", "stalker"}}}}}}}};

    REQUIRE(loader.load_from_json(j));
    REQUIRE(loader.count() == 1);

    const auto* stage = loader.get(0);
    REQUIRE(stage != nullptr);
    REQUIRE(stage->name == "test_stage");
    REQUIRE(stage->level == "Test_Room");
    REQUIRE(stage->waves.size() == 1);
    REQUIRE(stage->waves[0].enemies.size() == 2);

    const auto& e0 = stage->waves[0].enemies[0];
    REQUIRE(e0.type == Enemy::Type::Grunt);
    REQUIRE(e0.hp == Approx(1.f));
    REQUIRE(e0.score == 100);
    REQUIRE(e0.ai == AiBehavior::Archetype::Chaser);
    REQUIRE(e0.contact_damage);

    const auto& e1 = stage->waves[0].enemies[1];
    REQUIRE(e1.type == Enemy::Type::Mid);
    REQUIRE(e1.hp == Approx(3.f));
    REQUIRE(e1.score == 300);
    REQUIRE(e1.ai == AiBehavior::Archetype::Stalker);
    REQUIRE_FALSE(e1.contact_damage);
}

TEST_CASE("Enemy type strings map to correct enums", "[waves]") {
    StageLoader loader;

    auto make_stage = [](const std::string& type, const std::string& ai) {
        return nlohmann::json{
            {"name", "test"},
            {"level", "Room"},
            {"waves", {{{"enemies", {{{"type", type}, {"ai", ai}, {"pattern", "spiral_3way"}}}}}}}};
    };

    // Test all enemy types
    REQUIRE(loader.load_from_json(make_stage("grunt", "chaser")));
    REQUIRE(loader.get(0)->waves[0].enemies[0].type == Enemy::Type::Grunt);
    REQUIRE(loader.get(0)->waves[0].enemies[0].ai == AiBehavior::Archetype::Chaser);

    REQUIRE(loader.load_from_json(make_stage("mid", "drifter")));
    REQUIRE(loader.get(1)->waves[0].enemies[0].type == Enemy::Type::Mid);
    REQUIRE(loader.get(1)->waves[0].enemies[0].ai == AiBehavior::Archetype::Drifter);

    REQUIRE(loader.load_from_json(make_stage("boss", "stalker")));
    REQUIRE(loader.get(2)->waves[0].enemies[0].type == Enemy::Type::Boss);
    REQUIRE(loader.get(2)->waves[0].enemies[0].ai == AiBehavior::Archetype::Stalker);

    REQUIRE(loader.load_from_json(make_stage("grunt", "coward")));
    REQUIRE(loader.get(3)->waves[0].enemies[0].ai == AiBehavior::Archetype::Coward);
}
