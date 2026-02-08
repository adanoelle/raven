#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <entt/entt.hpp>
#include "ecs/components.hpp"
#include "ecs/systems/collision_system.hpp"

using namespace raven;

// Test circle-circle collision math independently of ECS
namespace {

bool circles_overlap(float x1, float y1, float r1,
                     float x2, float y2, float r2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dist_sq = dx * dx + dy * dy;
    float radii = r1 + r2;
    return dist_sq <= radii * radii;
}

} // namespace

TEST_CASE("Circle-circle collision", "[collision]") {
    SECTION("overlapping circles") {
        REQUIRE(circles_overlap(0.f, 0.f, 5.f, 3.f, 0.f, 5.f));
    }

    SECTION("touching circles") {
        REQUIRE(circles_overlap(0.f, 0.f, 5.f, 10.f, 0.f, 5.f));
    }

    SECTION("separated circles") {
        REQUIRE_FALSE(circles_overlap(0.f, 0.f, 5.f, 20.f, 0.f, 5.f));
    }

    SECTION("player hitbox vs projectile") {
        // Player radius 6, projectile radius 3
        REQUIRE(circles_overlap(100.f, 200.f, 6.f, 107.f, 200.f, 3.f));
        REQUIRE_FALSE(circles_overlap(100.f, 200.f, 6.f, 110.f, 200.f, 3.f));
    }
}

TEST_CASE("Collision system integration", "[collision]") {
    entt::registry reg;

    // Create player entity
    auto player = reg.create();
    reg.emplace<Transform2D>(player, 100.f, 100.f);
    reg.emplace<CircleHitbox>(player, 2.f, 0.f, 0.f);
    reg.emplace<Player>(player);
    reg.emplace<Health>(player, 1.f, 1.f);

    SECTION("Enemy bullet hits player") {
        auto bullet = reg.create();
        reg.emplace<Transform2D>(bullet, 101.f, 100.f); // overlaps player
        reg.emplace<CircleHitbox>(bullet, 3.f, 0.f, 0.f);
        reg.emplace<Bullet>(bullet, Bullet::Owner::Enemy);
        reg.emplace<DamageOnContact>(bullet, 1.f);

        systems::update_collision(reg);

        // Bullet should be destroyed
        REQUIRE_FALSE(reg.valid(bullet));

        // Player should have taken damage
        auto& hp = reg.get<Health>(player);
        REQUIRE(hp.current == Catch::Approx(0.f));

        // Player should be invulnerable
        auto* inv = reg.try_get<Invulnerable>(player);
        REQUIRE(inv != nullptr);
        REQUIRE(inv->remaining == Catch::Approx(2.f));
    }

    SECTION("Separated bullet does not hit player") {
        auto bullet = reg.create();
        reg.emplace<Transform2D>(bullet, 200.f, 200.f); // far away
        reg.emplace<CircleHitbox>(bullet, 3.f, 0.f, 0.f);
        reg.emplace<Bullet>(bullet, Bullet::Owner::Enemy);
        reg.emplace<DamageOnContact>(bullet, 1.f);

        systems::update_collision(reg);

        // Bullet should still exist
        REQUIRE(reg.valid(bullet));

        // Player HP unchanged
        auto& hp = reg.get<Health>(player);
        REQUIRE(hp.current == Catch::Approx(1.f));
    }

    SECTION("Invulnerable player is not hit") {
        reg.emplace<Invulnerable>(player, 1.f);

        auto bullet = reg.create();
        reg.emplace<Transform2D>(bullet, 101.f, 100.f); // overlaps player
        reg.emplace<CircleHitbox>(bullet, 3.f, 0.f, 0.f);
        reg.emplace<Bullet>(bullet, Bullet::Owner::Enemy);
        reg.emplace<DamageOnContact>(bullet, 1.f);

        systems::update_collision(reg);

        // Bullet should still exist (player is invulnerable)
        REQUIRE(reg.valid(bullet));

        // Player HP unchanged
        auto& hp = reg.get<Health>(player);
        REQUIRE(hp.current == Catch::Approx(1.f));
    }

    SECTION("Player bullet does not hit player") {
        auto bullet = reg.create();
        reg.emplace<Transform2D>(bullet, 101.f, 100.f);
        reg.emplace<CircleHitbox>(bullet, 3.f, 0.f, 0.f);
        reg.emplace<Bullet>(bullet, Bullet::Owner::Player);
        reg.emplace<DamageOnContact>(bullet, 1.f);

        systems::update_collision(reg);

        // Bullet should still exist (player's own bullet)
        REQUIRE(reg.valid(bullet));

        // Player HP unchanged
        auto& hp = reg.get<Health>(player);
        REQUIRE(hp.current == Catch::Approx(1.f));
    }
}
