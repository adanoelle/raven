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

/// @brief Enemy definitions for the wave tests: a grunt with and without
/// contact damage, a grunt that doesn't shoot, and a mid.
EnemyLibrary make_enemies() {
    const nlohmann::json j = {
        {"enemies",
         {{"grunt",
           {{"tier", "grunt"},
            {"sheet", "enemies"},
            {"ai", {{"archetype", "chaser"}}},
            {"pattern", "spiral_3way"}}},
          {"grunt_contact",
           {{"tier", "grunt"},
            {"sheet", "enemies"},
            {"ai", {{"archetype", "chaser"}}},
            {"pattern", "spiral_3way"},
            {"contact_damage", 15}}},
          {"silent", {{"tier", "grunt"}, {"sheet", "enemies"}, {"ai", {{"archetype", "drifter"}}}}},
          {"mid",
           {{"tier", "mid"},
            {"sheet", "enemies_mid"},
            {"hp", 3},
            {"score", 300},
            {"ai", {{"archetype", "stalker"}}},
            {"pattern", "spiral_3way"}}}}}};
    EnemyLibrary enemies;
    REQUIRE(enemies.load_json(j));
    REQUIRE(enemies.count() == 4);
    return enemies;
}

/// @brief Build a test StageDef with one wave; only the first enemy has
/// contact damage. With no LDtk level loaded, enemies spawn at the fallback
/// position.
StageDef make_test_stage(int num_enemies) {
    StageDef stage;
    stage.name = "test_stage";
    stage.level = "Test_Room";

    WaveDef wave;
    for (int i = 0; i < num_enemies; ++i) {
        wave.enemies.push_back({i, i == 0 ? "grunt_contact" : "grunt"});
    }
    stage.waves.push_back(wave);
    return stage;
}

/// @brief Build a two-wave test stage: a grunt, then a mid.
StageDef make_two_wave_stage() {
    StageDef stage;
    stage.name = "two_wave_stage";
    stage.level = "Test_Room";
    stage.waves.push_back(WaveDef{{{0, "grunt"}}});
    stage.waves.push_back(WaveDef{{{0, "mid"}}});
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
    const auto enemies = make_enemies();
    // No LDtk loaded — enemies will use fallback center position

    auto stage = make_test_stage(3);
    reg.ctx().emplace<GameState>();

    systems::spawn_wave(reg, tilemap, stage, 0, patterns, enemies);

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
    const auto enemies = make_enemies();
    auto stage = make_test_stage(2);
    reg.ctx().emplace<GameState>();

    systems::spawn_wave(reg, tilemap, stage, 0, patterns, enemies);

    auto cd_view = reg.view<ContactDamage>();
    REQUIRE(cd_view.size() == 1); // Only first enemy has contact_damage=true
}

TEST_CASE("spawn_wave starts contact damage on a grace cooldown", "[waves]") {
    entt::registry reg;
    auto& interner = reg.ctx().emplace<StringInterner>();
    PatternLibrary patterns;
    patterns.set_interner(interner);
    patterns.load_from_json(
        {{"name", "spiral_3way"},
         {"emitters",
          {{{"type", "radial"}, {"count", 3}, {"speed", 100.f}, {"fire_rate", 0.5f}}}}});

    Tilemap tilemap;
    const auto enemies = make_enemies();
    StageDef stage{"grace", "Test_Room", {WaveDef{{{0, "grunt_contact"}, {1, "silent"}}}}};
    reg.ctx().emplace<GameState>();

    systems::spawn_wave(reg, tilemap, stage, 0, patterns, enemies);

    // An enemy spawned on top of the player can't hit them straight away
    auto cd_view = reg.view<ContactDamage>();
    REQUIRE(cd_view.size() == 1);
    for (auto [entity, contact] : cd_view.each()) {
        CHECK(contact.timer == Approx(systems::SPAWN_CONTACT_GRACE));
        CHECK(contact.damage == Approx(15.f));
    }

    // An enemy defined without a pattern doesn't shoot
    CHECK(reg.view<BulletEmitter>().size() == 1);
}

TEST_CASE("spawn_wave builds enemies from their definitions", "[waves]") {
    entt::registry reg;
    auto& interner = reg.ctx().emplace<StringInterner>();
    PatternLibrary patterns;
    patterns.set_interner(interner);

    Tilemap tilemap;
    const auto enemies = make_enemies();
    StageDef stage{"defs", "Test_Room", {WaveDef{{{0, "mid"}}}}};
    reg.ctx().emplace<GameState>();

    systems::spawn_wave(reg, tilemap, stage, 0, patterns, enemies);

    auto view = reg.view<Enemy>();
    REQUIRE(view.size() == 1);
    const auto e = view.front();
    const EnemyDef& def = *enemies.get("mid");

    CHECK(reg.get<Enemy>(e).type == Enemy::Type::Mid);
    CHECK(reg.get<Health>(e).max == Approx(3.f));
    CHECK(reg.get<ScoreValue>(e).points == 300);
    CHECK(reg.get<CircleHitbox>(e).radius == Approx(def.radius));
    CHECK(reg.get<RectHitbox>(e).width == Approx(def.rect_w));
    CHECK(reg.get<AiBehavior>(e).archetype == AiBehavior::Archetype::Stalker);
    CHECK(reg.get<AiBehavior>(e).move_speed == Approx(def.ai.move_speed));
    CHECK(reg.get<StabilizerDrop>(e).chance == Approx(default_stabilizer_drop(Enemy::Type::Mid)));

    // No size in the definition: drawn at the sheet's frame size
    const auto& sprite = reg.get<Sprite>(e);
    CHECK(sprite.sheet_id == interner.intern("enemies_mid"));
    CHECK(sprite.width == 0);
    CHECK(sprite.height == 0);
    CHECK(reg.all_of<Animation>(e));
}

TEST_CASE("spawn_wave skips enemies with no definition", "[waves]") {
    entt::registry reg;
    auto& interner = reg.ctx().emplace<StringInterner>();
    PatternLibrary patterns;
    patterns.set_interner(interner);

    Tilemap tilemap;
    const auto enemies = make_enemies();
    StageDef stage{"typo", "Test_Room", {WaveDef{{{0, "grunt"}, {1, "gruntt"}}}}};
    reg.ctx().emplace<GameState>();

    systems::spawn_wave(reg, tilemap, stage, 0, patterns, enemies);
    CHECK(reg.view<Enemy>().size() == 1);
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
    const auto enemies = make_enemies();
    auto stage = make_two_wave_stage();

    auto& state = reg.ctx().emplace<GameState>();
    state.current_wave = 0;
    state.total_waves = 2;

    // Spawn wave 0
    systems::spawn_wave(reg, tilemap, stage, 0, patterns, enemies);
    REQUIRE(reg.view<Enemy>().size() == 1);

    // Kill all enemies
    auto enemy_view = reg.view<Enemy>();
    for (auto [entity, enemy] : enemy_view.each()) {
        reg.destroy(entity);
    }
    REQUIRE(reg.view<Enemy>().size() == 0);

    // update_waves should advance to wave 1
    systems::update_waves(reg, tilemap, stage, patterns, enemies);
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
    const auto enemies = make_enemies();
    auto stage = make_test_stage(1); // Single wave with 1 enemy

    auto& state = reg.ctx().emplace<GameState>();
    state.current_wave = 0;
    state.total_waves = 1;

    systems::spawn_wave(reg, tilemap, stage, 0, patterns, enemies);

    // Kill the enemy
    auto enemy_view = reg.view<Enemy>();
    for (auto [entity, enemy] : enemy_view.each()) {
        reg.destroy(entity);
    }

    // update_waves should mark room cleared
    systems::update_waves(reg, tilemap, stage, patterns, enemies);
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
    const auto enemies = make_enemies();
    auto stage = make_test_stage(1);

    auto& state = reg.ctx().emplace<GameState>();
    state.current_wave = 0;
    state.total_waves = 1;

    // Create an exit entity
    auto exit_ent = reg.create();
    reg.emplace<Transform2D>(exit_ent, 400.f, 200.f);
    reg.emplace<Exit>(exit_ent, Exit{"Room_02", false});

    // Spawn and kill enemies
    systems::spawn_wave(reg, tilemap, stage, 0, patterns, enemies);
    auto enemy_view = reg.view<Enemy>();
    for (auto [entity, enemy] : enemy_view.each()) {
        reg.destroy(entity);
    }

    systems::update_waves(reg, tilemap, stage, patterns, enemies);
    REQUIRE(state.room_cleared);

    auto& exit = reg.get<Exit>(exit_ent);
    REQUIRE(exit.open);
}

// ── Exit overlap tests ─────────────────────────────────────────────

TEST_CASE("check_exit_overlap returns nullptr when exit closed", "[waves]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    make_player(reg, 100.f, 100.f);

    auto exit_ent = reg.create();
    reg.emplace<Transform2D>(exit_ent, 100.f, 100.f);    // Same position as player
    reg.emplace<Exit>(exit_ent, Exit{"Room_02", false}); // Closed

    const auto* result = systems::check_exit_overlap(reg);
    REQUIRE(result == nullptr);
}

TEST_CASE("check_exit_overlap returns the exit when open and overlapping", "[waves]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    make_player(reg, 100.f, 100.f);

    auto exit_ent = reg.create();
    reg.emplace<Transform2D>(exit_ent, 105.f, 100.f);   // Close to player
    reg.emplace<Exit>(exit_ent, Exit{"Room_02", true}); // Open

    const auto* result = systems::check_exit_overlap(reg);
    REQUIRE(result != nullptr);
    REQUIRE(result->target_level == "Room_02");
}

TEST_CASE("check_exit_overlap triggers for an exit with no target_level", "[waves]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    make_player(reg, 100.f, 100.f);

    // The final room's exit has no target; touching it must still count
    auto exit_ent = reg.create();
    reg.emplace<Transform2D>(exit_ent, 100.f, 100.f);
    reg.emplace<Exit>(exit_ent, Exit{"", true});

    REQUIRE(systems::check_exit_overlap(reg) != nullptr);
}

TEST_CASE("check_exit_overlap returns nullptr when player far from exit", "[waves]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    make_player(reg, 100.f, 100.f);

    auto exit_ent = reg.create();
    reg.emplace<Transform2D>(exit_ent, 400.f, 400.f);   // Far away
    reg.emplace<Exit>(exit_ent, Exit{"Room_02", true}); // Open but distant

    const auto* result = systems::check_exit_overlap(reg);
    REQUIRE(result == nullptr);
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
                            {{{"spawn_index", 0}, {"enemy", "grunt_chaser"}},
                             {{"spawn_index", 1}, {"enemy", "mid_stalker"}}}}},
                          {{"enemies", {{{"spawn_index", 2}, {"enemy", "boss_coward"}}}}}}}};

    REQUIRE(loader.load_from_json(j));
    REQUIRE(loader.count() == 1);

    const auto* stage = loader.get(0);
    REQUIRE(stage != nullptr);
    REQUIRE(stage->name == "test_stage");
    REQUIRE(stage->level == "Test_Room");
    REQUIRE(stage->waves.size() == 2);
    REQUIRE(stage->waves[0].enemies.size() == 2);

    CHECK(stage->waves[0].enemies[0].spawn_index == 0);
    CHECK(stage->waves[0].enemies[0].enemy == "grunt_chaser");
    CHECK(stage->waves[0].enemies[1].enemy == "mid_stalker");
    CHECK(stage->waves[1].enemies[0].spawn_index == 2);
    CHECK(stage->waves[1].enemies[0].enemy == "boss_coward");
}

TEST_CASE("StageLoader skips wave entries that name no enemy", "[waves]") {
    StageLoader loader;

    // The old inline format: type, ai and stats in the stage file
    nlohmann::json j = {{"name", "old_format"},
                        {"level", "Room"},
                        {"waves",
                         {{{"enemies",
                            {{{"spawn_index", 0}, {"type", "grunt"}, {"ai", "chaser"}},
                             {{"spawn_index", 1}, {"enemy", "grunt_chaser"}}}}}}}};

    REQUIRE(loader.load_from_json(j));
    const auto& wave = loader.get(0)->waves[0];
    REQUIRE(wave.enemies.size() == 1);
    CHECK(wave.enemies[0].enemy == "grunt_chaser");
}

TEST_CASE("Enemy type and AI parsers reject unknown strings", "[waves]") {
    CHECK(parse_enemy_type("grunt") == Enemy::Type::Grunt);
    CHECK(parse_enemy_type("mid") == Enemy::Type::Mid);
    CHECK(parse_enemy_type("boss") == Enemy::Type::Boss);
    CHECK_FALSE(parse_enemy_type("Boss").has_value());
    CHECK_FALSE(parse_enemy_type("").has_value());

    CHECK(parse_ai_archetype("chaser") == AiBehavior::Archetype::Chaser);
    CHECK(parse_ai_archetype("drifter") == AiBehavior::Archetype::Drifter);
    CHECK(parse_ai_archetype("stalker") == AiBehavior::Archetype::Stalker);
    CHECK(parse_ai_archetype("coward") == AiBehavior::Archetype::Coward);
    CHECK_FALSE(parse_ai_archetype("stalkr").has_value());
}
