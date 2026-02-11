#include "ecs/components.hpp"
#include "ecs/systems/ai_system.hpp"
#include "rendering/tilemap.hpp"

#include <entt/entt.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <random>

using namespace raven;
using Catch::Approx;

namespace {

/// @brief Create a minimal player entity for AI tests.
entt::entity make_player(entt::registry& reg, float x, float y) {
    auto player = reg.create();
    reg.emplace<Transform2D>(player, x, y);
    reg.emplace<PreviousTransform>(player, x, y);
    reg.emplace<Player>(player);
    reg.emplace<CircleHitbox>(player, 6.f);
    reg.emplace<Health>(player, 3.f, 3.f);
    reg.emplace<Velocity>(player);
    return player;
}

/// @brief Create an enemy with an AI behavior.
entt::entity make_enemy(entt::registry& reg, float x, float y, AiBehavior ai) {
    auto enemy = reg.create();
    reg.emplace<Transform2D>(enemy, x, y);
    reg.emplace<PreviousTransform>(enemy, x, y);
    reg.emplace<Velocity>(enemy);
    reg.emplace<Enemy>(enemy);
    reg.emplace<Health>(enemy, 3.f, 3.f);
    reg.emplace<CircleHitbox>(enemy, 7.f);
    reg.emplace<AiBehavior>(enemy, ai);
    return enemy;
}

} // namespace

TEST_CASE("Chaser moves toward player", "[ai]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);
    Tilemap tilemap;

    auto player = make_player(reg, 200.f, 100.f);
    (void)player;

    AiBehavior ai{};
    ai.archetype = AiBehavior::Archetype::Chaser;
    ai.phase = AiBehavior::Phase::Idle;
    ai.move_speed = 70.f;
    ai.activation_range = 200.f;
    auto enemy = make_enemy(reg, 100.f, 100.f, ai);

    // First tick activates and starts moving
    systems::update_ai(reg, tilemap, 1.f / 120.f);

    auto& vel = reg.get<Velocity>(enemy);
    // Should move toward player (positive X direction)
    REQUIRE(vel.dx > 0.f);
    REQUIRE(vel.dx == Approx(70.f));
    REQUIRE(vel.dy == Approx(0.f));
}

TEST_CASE("Idle enemy does not move when out of activation range", "[ai]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);
    Tilemap tilemap;

    make_player(reg, 500.f, 500.f); // Far away

    AiBehavior ai{};
    ai.archetype = AiBehavior::Archetype::Chaser;
    ai.activation_range = 100.f;
    auto enemy = make_enemy(reg, 0.f, 0.f, ai);

    systems::update_ai(reg, tilemap, 1.f / 120.f);

    auto& vel = reg.get<Velocity>(enemy);
    REQUIRE(vel.dx == Approx(0.f));
    REQUIRE(vel.dy == Approx(0.f));

    auto& ai_comp = reg.get<AiBehavior>(enemy);
    REQUIRE(ai_comp.phase == AiBehavior::Phase::Idle);
}

TEST_CASE("Activation triggers when player enters range", "[ai]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);
    Tilemap tilemap;

    make_player(reg, 50.f, 0.f);

    AiBehavior ai{};
    ai.archetype = AiBehavior::Archetype::Chaser;
    ai.activation_range = 100.f;
    ai.move_speed = 70.f;
    auto enemy = make_enemy(reg, 0.f, 0.f, ai);

    systems::update_ai(reg, tilemap, 1.f / 120.f);

    auto& ai_comp = reg.get<AiBehavior>(enemy);
    REQUIRE(ai_comp.phase == AiBehavior::Phase::Advance);

    auto& vel = reg.get<Velocity>(enemy);
    REQUIRE(vel.dx > 0.f);
}

TEST_CASE("Drifter changes direction periodically", "[ai]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);
    Tilemap tilemap;

    make_player(reg, 50.f, 0.f);

    AiBehavior ai{};
    ai.archetype = AiBehavior::Archetype::Drifter;
    ai.activation_range = 200.f;
    ai.move_speed = 40.f;
    ai.phase_timer = 0.f; // Will trigger direction change on first tick
    auto enemy = make_enemy(reg, 0.f, 0.f, ai);

    systems::update_ai(reg, tilemap, 1.f / 120.f);

    auto& vel1 = reg.get<Velocity>(enemy);
    float dx1 = vel1.dx;
    float dy1 = vel1.dy;

    // Velocity should be set (non-zero magnitude)
    float speed = std::sqrt(dx1 * dx1 + dy1 * dy1);
    REQUIRE(speed == Approx(40.f).margin(0.01f));

    // Advance timer to force direction change
    auto& ai_comp = reg.get<AiBehavior>(enemy);
    ai_comp.phase_timer = 0.f;

    systems::update_ai(reg, tilemap, 0.01f);

    auto& vel2 = reg.get<Velocity>(enemy);
    float speed2 = std::sqrt(vel2.dx * vel2.dx + vel2.dy * vel2.dy);
    REQUIRE(speed2 == Approx(40.f).margin(0.01f));
}

TEST_CASE("Stalker strafes at preferred range", "[ai]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);
    Tilemap tilemap;

    // Player at origin, enemy at preferred range distance to the right
    make_player(reg, 0.f, 0.f);

    AiBehavior ai{};
    ai.archetype = AiBehavior::Archetype::Stalker;
    ai.phase = AiBehavior::Phase::Attack; // Start in attack phase
    ai.activation_range = 200.f;
    ai.move_speed = 90.f;
    ai.preferred_range = 90.f;
    ai.attack_range = 120.f;
    auto enemy = make_enemy(reg, 90.f, 0.f, ai);

    systems::update_ai(reg, tilemap, 1.f / 120.f);

    auto& vel = reg.get<Velocity>(enemy);
    // At preferred range, should strafe (mostly perpendicular to player direction)
    // Direction to player is (-1, 0), so perpendicular is (0, strafe_dir)
    REQUIRE(std::abs(vel.dy) > std::abs(vel.dx));
}

TEST_CASE("Stalker retreats when player closes in", "[ai]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);
    Tilemap tilemap;

    make_player(reg, 0.f, 0.f);

    AiBehavior ai{};
    ai.archetype = AiBehavior::Archetype::Stalker;
    ai.phase = AiBehavior::Phase::Attack;
    ai.activation_range = 200.f;
    ai.move_speed = 90.f;
    ai.preferred_range = 90.f;
    // Enemy at 40px, which is < 50% of preferred (45px) → should retreat
    auto enemy = make_enemy(reg, 40.f, 0.f, ai);

    systems::update_ai(reg, tilemap, 1.f / 120.f);

    auto& ai_comp = reg.get<AiBehavior>(enemy);
    REQUIRE(ai_comp.phase == AiBehavior::Phase::Retreat);

    auto& vel = reg.get<Velocity>(enemy);
    // Should be moving away from player (positive X)
    REQUIRE(vel.dx > 0.f);
}

TEST_CASE("Coward flees from player", "[ai]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);
    Tilemap tilemap;

    make_player(reg, 0.f, 0.f);

    AiBehavior ai{};
    ai.archetype = AiBehavior::Archetype::Coward;
    ai.activation_range = 200.f;
    ai.move_speed = 110.f;
    auto enemy = make_enemy(reg, 50.f, 0.f, ai);

    systems::update_ai(reg, tilemap, 1.f / 120.f);

    auto& vel = reg.get<Velocity>(enemy);
    // Should flee (positive X, away from player at origin)
    REQUIRE(vel.dx > 0.f);
    REQUIRE(vel.dx == Approx(110.f));
}

TEST_CASE("Coward wall-slides instead of getting stuck", "[ai]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);

    // 10x10 grid, cell size 16px. Solid wall on right edge (column 9).
    Tilemap tilemap;
    std::vector<bool> grid(100, false);
    for (int row = 0; row < 10; ++row) {
        grid[static_cast<size_t>(row * 10 + 9)] = true; // column 9
    }
    tilemap.init_collision(10, 10, 16, grid);

    // Player to the left, enemy near the right wall
    make_player(reg, 16.f, 80.f);

    AiBehavior ai{};
    ai.archetype = AiBehavior::Archetype::Coward;
    ai.activation_range = 300.f;
    ai.move_speed = 110.f;
    // Enemy at x=128 (cell 8), fleeing right would enter solid cell 9
    auto enemy = make_enemy(reg, 128.f, 80.f, ai);

    systems::update_ai(reg, tilemap, 1.f / 120.f);

    auto& vel = reg.get<Velocity>(enemy);
    // Should not be stuck (velocity magnitude > 0)
    float speed = std::sqrt(vel.dx * vel.dx + vel.dy * vel.dy);
    REQUIRE(speed > 0.f);
    // Should slide along the wall (non-zero perpendicular component)
    REQUIRE(std::abs(vel.dy) > 0.f);
}

TEST_CASE("Coward escapes when fully cornered", "[ai]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);

    // 5x5 grid, cell size 16px. Surround cell (3,2) with walls on 3 sides.
    Tilemap tilemap;
    std::vector<bool> grid(25, false);
    grid[4 + 2 * 5] = true; // (4,2) - right wall
    grid[3 + 1 * 5] = true; // (3,1) - top wall
    grid[3 + 3 * 5] = true; // (3,3) - bottom wall
    tilemap.init_collision(5, 5, 16, grid);

    // Player to the left, enemy in the corner pocket
    make_player(reg, 16.f, 40.f);

    AiBehavior ai{};
    ai.archetype = AiBehavior::Archetype::Coward;
    ai.activation_range = 300.f;
    ai.move_speed = 110.f;
    // Enemy at cell (3,2) center = (56, 40)
    auto enemy = make_enemy(reg, 56.f, 40.f, ai);

    systems::update_ai(reg, tilemap, 1.f / 120.f);

    auto& vel = reg.get<Velocity>(enemy);
    // Should not be stuck — reverses toward player to escape the corner
    float speed = std::sqrt(vel.dx * vel.dx + vel.dy * vel.dy);
    REQUIRE(speed > 0.f);
}

TEST_CASE("Knockback overrides AI velocity", "[ai]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);
    Tilemap tilemap;

    make_player(reg, 200.f, 100.f);

    AiBehavior ai{};
    ai.archetype = AiBehavior::Archetype::Chaser;
    ai.phase = AiBehavior::Phase::Advance;
    ai.activation_range = 300.f;
    ai.move_speed = 70.f;
    auto enemy = make_enemy(reg, 100.f, 100.f, ai);

    // Apply knockback in opposite direction
    reg.emplace<Knockback>(enemy, -150.f, 0.f, 0.1f);

    systems::update_ai(reg, tilemap, 1.f / 120.f);

    auto& vel = reg.get<Velocity>(enemy);
    // Velocity should be knockback, not AI movement
    REQUIRE(vel.dx == Approx(-150.f));
    REQUIRE(vel.dy == Approx(0.f));
}

TEST_CASE("Knockback removed after duration expires", "[ai]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);
    Tilemap tilemap;

    make_player(reg, 200.f, 100.f);

    AiBehavior ai{};
    ai.archetype = AiBehavior::Archetype::Chaser;
    ai.phase = AiBehavior::Phase::Advance;
    ai.activation_range = 300.f;
    ai.move_speed = 70.f;
    auto enemy = make_enemy(reg, 100.f, 100.f, ai);

    reg.emplace<Knockback>(enemy, -150.f, 0.f, 0.005f);

    // dt exceeds knockback remaining → should remove Knockback
    systems::update_ai(reg, tilemap, 1.f / 120.f);

    REQUIRE_FALSE(reg.any_of<Knockback>(enemy));
}

TEST_CASE("Contact damage applies with cooldown", "[ai]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);
    Tilemap tilemap;

    auto player = make_player(reg, 100.f, 100.f);

    AiBehavior ai{};
    ai.archetype = AiBehavior::Archetype::Chaser;
    ai.phase = AiBehavior::Phase::Advance;
    ai.activation_range = 300.f;
    auto enemy = make_enemy(reg, 100.f, 100.f, ai); // Overlapping player
    reg.emplace<ContactDamage>(enemy, ContactDamage{1.f, 0.5f, 0.f});

    systems::update_ai(reg, tilemap, 1.f / 120.f);

    auto& p_hp = reg.get<Health>(player);
    REQUIRE(p_hp.current == Approx(2.f));

    // Cooldown should be set
    auto& contact = reg.get<ContactDamage>(enemy);
    REQUIRE(contact.timer == Approx(0.5f));

    // Second tick: cooldown active, no additional damage
    systems::update_ai(reg, tilemap, 1.f / 120.f);
    REQUIRE(p_hp.current == Approx(2.f));
}

TEST_CASE("Contact damage respects invulnerability", "[ai]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);
    Tilemap tilemap;

    auto player = make_player(reg, 100.f, 100.f);
    reg.emplace<Invulnerable>(player, 1.f);

    AiBehavior ai{};
    ai.archetype = AiBehavior::Archetype::Chaser;
    ai.phase = AiBehavior::Phase::Advance;
    ai.activation_range = 300.f;
    auto enemy = make_enemy(reg, 100.f, 100.f, ai);
    reg.emplace<ContactDamage>(enemy, ContactDamage{1.f, 0.5f, 0.f});

    systems::update_ai(reg, tilemap, 1.f / 120.f);

    auto& p_hp = reg.get<Health>(player);
    REQUIRE(p_hp.current == Approx(3.f)); // No damage, player is invulnerable
}

TEST_CASE("Emitter deactivated when player outside attack range", "[ai]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);
    Tilemap tilemap;

    make_player(reg, 200.f, 0.f);

    AiBehavior ai{};
    ai.archetype = AiBehavior::Archetype::Chaser;
    ai.activation_range = 300.f;
    ai.attack_range = 80.f;
    ai.move_speed = 70.f;
    auto enemy = make_enemy(reg, 0.f, 0.f, ai);
    reg.emplace<BulletEmitter>(enemy, BulletEmitter{StringId{}, {}, {}, true});

    systems::update_ai(reg, tilemap, 1.f / 120.f);

    auto& emitter = reg.get<BulletEmitter>(enemy);
    // Player is 200px away, attack range is 80 → emitter should be inactive
    REQUIRE_FALSE(emitter.active);
}

TEST_CASE("Line-of-sight blocks activation through solid tiles", "[ai]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);

    // Create a tilemap with a wall between enemy and player
    Tilemap tilemap;
    // 10x1 grid, cell size 16px. Cell 5 (80-96px) is solid.
    std::vector<bool> grid(10, false);
    grid[5] = true;
    tilemap.init_collision(10, 1, 16, grid);

    // Player at x=140 (cell 8), enemy at x=20 (cell 1)
    make_player(reg, 140.f, 8.f);

    AiBehavior ai{};
    ai.archetype = AiBehavior::Archetype::Chaser;
    ai.activation_range = 300.f;
    ai.move_speed = 70.f;
    auto enemy = make_enemy(reg, 20.f, 8.f, ai);

    systems::update_ai(reg, tilemap, 1.f / 120.f);

    auto& ai_comp = reg.get<AiBehavior>(enemy);
    REQUIRE(ai_comp.phase == AiBehavior::Phase::Idle);

    auto& vel = reg.get<Velocity>(enemy);
    REQUIRE(vel.dx == Approx(0.f));
    REQUIRE(vel.dy == Approx(0.f));
}
