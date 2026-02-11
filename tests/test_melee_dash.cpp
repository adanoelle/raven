#include "core/input.hpp"
#include "core/string_id.hpp"
#include "ecs/components.hpp"
#include "ecs/systems/ai_system.hpp"
#include "ecs/systems/dash_system.hpp"
#include "ecs/systems/hitbox_math.hpp"
#include "ecs/systems/input_system.hpp"
#include "ecs/systems/melee_system.hpp"
#include "patterns/pattern_library.hpp"
#include "rendering/tilemap.hpp"

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <random>

using namespace raven;
using Catch::Approx;

namespace {

/// @brief Create a minimal player entity for melee/dash tests.
entt::entity make_player(entt::registry& reg, float x, float y) {
    auto player = reg.create();
    reg.emplace<Transform2D>(player, x, y);
    reg.emplace<PreviousTransform>(player, x, y);
    reg.emplace<Player>(player);
    reg.emplace<Velocity>(player);
    reg.emplace<CircleHitbox>(player, 6.f);
    reg.emplace<Health>(player, 3.f, 3.f);
    reg.emplace<AimDirection>(player, 1.f, 0.f);
    reg.emplace<MeleeCooldown>(player);
    reg.emplace<DashCooldown>(player);
    return player;
}

/// @brief Create an enemy entity for melee tests.
entt::entity make_enemy(entt::registry& reg, float x, float y) {
    auto enemy = reg.create();
    reg.emplace<Transform2D>(enemy, x, y);
    reg.emplace<PreviousTransform>(enemy, x, y);
    reg.emplace<Velocity>(enemy);
    reg.emplace<Enemy>(enemy);
    reg.emplace<Health>(enemy, 3.f, 3.f);
    reg.emplace<CircleHitbox>(enemy, 7.f);
    return enemy;
}

/// @brief Create a simple empty InputState.
InputState no_input() {
    return InputState{};
}

/// @brief Create an InputState with melee_pressed.
InputState melee_input() {
    InputState input{};
    input.melee = true;
    input.melee_pressed = true;
    return input;
}

/// @brief Create an InputState with dash_pressed.
InputState dash_input(float move_x = 0.f, float move_y = 0.f) {
    InputState input{};
    input.dash = true;
    input.dash_pressed = true;
    input.move_x = move_x;
    input.move_y = move_y;
    return input;
}

} // namespace

// ── point_in_cone tests ─────────────────────────────────────────────

TEST_CASE("point_in_cone: target in front within range", "[melee]") {
    REQUIRE(systems::point_in_cone(0.f, 0.f, 1.f, 0.f, 20.f, 0.f, 30.f, 0.785f));
}

TEST_CASE("point_in_cone: target behind player", "[melee]") {
    REQUIRE_FALSE(systems::point_in_cone(0.f, 0.f, 1.f, 0.f, -20.f, 0.f, 30.f, 0.785f));
}

TEST_CASE("point_in_cone: target outside range", "[melee]") {
    REQUIRE_FALSE(systems::point_in_cone(0.f, 0.f, 1.f, 0.f, 50.f, 0.f, 30.f, 0.785f));
}

TEST_CASE("point_in_cone: target at boundary angle", "[melee]") {
    // 45 degree half-angle. Target at exactly 45 degrees.
    float target_x = 20.f * std::cos(0.785f);
    float target_y = 20.f * std::sin(0.785f);
    // At boundary: cos(angle) == cos(half_angle), should be true (>=)
    REQUIRE(systems::point_in_cone(0.f, 0.f, 1.f, 0.f, target_x, target_y, 30.f, 0.785f));
}

TEST_CASE("point_in_cone: target just outside angle", "[melee]") {
    // Target at 50 degrees (0.873 rad) with 45 deg half-angle (0.785 rad)
    float target_x = 20.f * std::cos(0.873f);
    float target_y = 20.f * std::sin(0.873f);
    REQUIRE_FALSE(systems::point_in_cone(0.f, 0.f, 1.f, 0.f, target_x, target_y, 30.f, 0.785f));
}

TEST_CASE("point_in_cone: target at origin always inside", "[melee]") {
    REQUIRE(systems::point_in_cone(5.f, 5.f, 1.f, 0.f, 5.f, 5.f, 30.f, 0.785f));
}

// ── Melee system tests ──────────────────────────────────────────────

TEST_CASE("Melee arc hits enemy in front of player", "[melee]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    PatternLibrary patterns;

    auto player = make_player(reg, 100.f, 100.f);
    auto enemy = make_enemy(reg, 120.f, 100.f); // 20px in front (aim is +x)

    auto input = melee_input();
    systems::update_melee(reg, input, patterns, 1.f / 120.f);

    auto& e_hp = reg.get<Health>(enemy);
    REQUIRE(e_hp.current < 3.f); // Took damage
    REQUIRE(reg.any_of<Knockback>(enemy));
}

TEST_CASE("Melee arc misses enemy behind player", "[melee]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    PatternLibrary patterns;

    make_player(reg, 100.f, 100.f);
    auto enemy = make_enemy(reg, 70.f, 100.f); // Behind player (aim is +x)

    auto input = melee_input();
    systems::update_melee(reg, input, patterns, 1.f / 120.f);

    auto& e_hp = reg.get<Health>(enemy);
    REQUIRE(e_hp.current == Approx(3.f)); // No damage
}

TEST_CASE("Melee arc misses enemy outside range", "[melee]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    PatternLibrary patterns;

    make_player(reg, 100.f, 100.f);
    auto enemy = make_enemy(reg, 200.f, 100.f); // 100px away, well beyond range

    auto input = melee_input();
    systems::update_melee(reg, input, patterns, 1.f / 120.f);

    auto& e_hp = reg.get<Health>(enemy);
    REQUIRE(e_hp.current == Approx(3.f)); // No damage
}

TEST_CASE("Melee disarms enemy and spawns weapon pickup", "[melee]") {
    entt::registry reg;
    auto& interner = reg.ctx().emplace<StringInterner>();
    PatternLibrary patterns;
    patterns.set_interner(interner);

    nlohmann::json j = {{"name", "test_pattern"},
                        {"emitters",
                         {{{"type", "radial"},
                           {"count", 3},
                           {"speed", 200.f},
                           {"fire_rate", 0.3f},
                           {"spread_angle", 60.f}}}}};
    patterns.load_from_json(j);

    auto player = make_player(reg, 100.f, 100.f);
    auto enemy = make_enemy(reg, 120.f, 100.f);
    reg.emplace<BulletEmitter>(enemy, BulletEmitter{interner.intern("test_pattern"), {}, {}});

    auto input = melee_input();
    systems::update_melee(reg, input, patterns, 1.f / 120.f);

    // Enemy should be disarmed
    REQUIRE_FALSE(reg.any_of<BulletEmitter>(enemy));
    REQUIRE(reg.any_of<Disarmed>(enemy));

    // A weapon pickup should exist
    auto pickup_view = reg.view<WeaponPickup>();
    REQUIRE(pickup_view.size() >= 1);
}

TEST_CASE("Melee cooldown prevents rapid attacks", "[melee]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    PatternLibrary patterns;

    auto player = make_player(reg, 100.f, 100.f);
    auto enemy = make_enemy(reg, 120.f, 100.f);

    // First attack
    auto input = melee_input();
    systems::update_melee(reg, input, patterns, 1.f / 120.f);
    auto& e_hp = reg.get<Health>(enemy);
    float hp_after_first = e_hp.current;
    REQUIRE(hp_after_first < 3.f);

    // Remove the active MeleeAttack so we're starting fresh
    if (reg.any_of<MeleeAttack>(player)) {
        reg.remove<MeleeAttack>(player);
    }

    // Second attack immediately — should be blocked by cooldown
    systems::update_melee(reg, input, patterns, 1.f / 120.f);
    REQUIRE(e_hp.current == Approx(hp_after_first)); // No additional damage
}

TEST_CASE("Disarmed enemy has no BulletEmitter and has Disarmed tag", "[melee]") {
    entt::registry reg;
    auto& interner = reg.ctx().emplace<StringInterner>();
    PatternLibrary patterns;
    patterns.set_interner(interner);

    nlohmann::json j = {
        {"name", "test_pattern2"},
        {"emitters", {{{"type", "radial"}, {"count", 3}, {"speed", 200.f}, {"fire_rate", 0.3f}}}}};
    patterns.load_from_json(j);

    make_player(reg, 100.f, 100.f);
    auto enemy = make_enemy(reg, 120.f, 100.f);
    reg.emplace<BulletEmitter>(enemy, BulletEmitter{interner.intern("test_pattern2"), {}, {}});

    auto input = melee_input();
    systems::update_melee(reg, input, patterns, 1.f / 120.f);

    REQUIRE_FALSE(reg.any_of<BulletEmitter>(enemy));
    REQUIRE(reg.any_of<Disarmed>(enemy));
}

// ── Dash system tests ───────────────────────────────────────────────

TEST_CASE("Dash activates with movement direction", "[dash]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_player(reg, 100.f, 100.f);

    auto input = dash_input(0.f, -1.f); // Moving up
    systems::update_dash(reg, input, 1.f / 120.f);

    REQUIRE(reg.any_of<Dash>(player));
    auto& dash = reg.get<Dash>(player);
    REQUIRE(dash.dir_y == Approx(-1.f));
}

TEST_CASE("Dash uses aim direction when stationary", "[dash]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_player(reg, 100.f, 100.f);
    // Aim direction is default (1, 0)

    auto input = dash_input(); // No movement input
    systems::update_dash(reg, input, 1.f / 120.f);

    REQUIRE(reg.any_of<Dash>(player));
    auto& dash = reg.get<Dash>(player);
    REQUIRE(dash.dir_x == Approx(1.f));
    REQUIRE(dash.dir_y == Approx(0.f));
}

TEST_CASE("Dash overrides velocity for duration", "[dash]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_player(reg, 100.f, 100.f);

    auto input = dash_input(1.f, 0.f);
    systems::update_dash(reg, input, 1.f / 120.f);

    auto& vel = reg.get<Velocity>(player);
    REQUIRE(vel.dx == Approx(400.f));
    REQUIRE(vel.dy == Approx(0.f));
}

TEST_CASE("Dash expires and is removed", "[dash]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_player(reg, 100.f, 100.f);

    auto input = dash_input(1.f, 0.f);
    systems::update_dash(reg, input, 1.f / 120.f);
    REQUIRE(reg.any_of<Dash>(player));

    // Tick past the entire dash duration
    auto empty = no_input();
    for (int i = 0; i < 20; ++i) {
        systems::update_dash(reg, empty, 1.f / 120.f);
    }

    REQUIRE_FALSE(reg.any_of<Dash>(player));
}

TEST_CASE("Dash cooldown prevents rapid dashing", "[dash]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_player(reg, 100.f, 100.f);

    // First dash
    auto input = dash_input(1.f, 0.f);
    systems::update_dash(reg, input, 1.f / 120.f);
    REQUIRE(reg.any_of<Dash>(player));

    // Expire the dash
    auto empty = no_input();
    for (int i = 0; i < 20; ++i) {
        systems::update_dash(reg, empty, 1.f / 120.f);
    }
    REQUIRE_FALSE(reg.any_of<Dash>(player));

    // Try to dash again immediately — cooldown should prevent it
    systems::update_dash(reg, input, 1.f / 120.f);
    REQUIRE_FALSE(reg.any_of<Dash>(player));
}

TEST_CASE("Input system skips player during dash", "[dash]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_player(reg, 100.f, 100.f);

    // Start a dash
    auto dash_in = dash_input(1.f, 0.f);
    systems::update_dash(reg, dash_in, 1.f / 120.f);
    REQUIRE(reg.any_of<Dash>(player));

    // Dash sets velocity to 400
    auto& vel = reg.get<Velocity>(player);
    REQUIRE(vel.dx == Approx(400.f));

    // Input system with opposite direction should NOT override velocity
    InputState move_input{};
    move_input.move_x = -1.f;
    systems::update_input(reg, move_input, 1.f / 120.f);

    // Velocity should still be dash velocity (input was skipped)
    REQUIRE(vel.dx == Approx(400.f));
}

TEST_CASE("Invulnerability granted during dash", "[dash]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_player(reg, 100.f, 100.f);

    auto input = dash_input(1.f, 0.f);
    systems::update_dash(reg, input, 1.f / 120.f);

    REQUIRE(reg.any_of<Invulnerable>(player));
    auto& inv = reg.get<Invulnerable>(player);
    REQUIRE(inv.remaining == Approx(0.18f));
}

// ── Contact damage cooldown fix tests ───────────────────────────────

TEST_CASE("Multiple ContactDamage entities tick cooldowns independently", "[ai]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);
    Tilemap tilemap;

    auto player = make_player(reg, 100.f, 100.f);

    // Create two enemies with different cooldown states
    auto enemy1 = make_enemy(reg, 200.f, 200.f); // Far from player, no overlap
    reg.emplace<AiBehavior>(
        enemy1, AiBehavior{AiBehavior::Archetype::Chaser, AiBehavior::Phase::Advance, 70.f, 300.f});
    reg.emplace<ContactDamage>(enemy1, ContactDamage{1.f, 0.5f, 0.3f}); // 0.3s remaining

    auto enemy2 = make_enemy(reg, 300.f, 300.f); // Far from player, no overlap
    reg.emplace<AiBehavior>(
        enemy2, AiBehavior{AiBehavior::Archetype::Chaser, AiBehavior::Phase::Advance, 70.f, 400.f});
    reg.emplace<ContactDamage>(enemy2, ContactDamage{1.f, 0.5f, 0.1f}); // 0.1s remaining

    float dt = 1.f / 120.f; // ~0.00833s
    systems::update_ai(reg, tilemap, dt);

    auto& cd1 = reg.get<ContactDamage>(enemy1);
    auto& cd2 = reg.get<ContactDamage>(enemy2);

    // Both should have ticked by dt independently
    REQUIRE(cd1.timer == Approx(0.3f - dt));
    REQUIRE(cd2.timer == Approx(0.1f - dt));
}
