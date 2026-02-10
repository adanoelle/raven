#include "ecs/components.hpp"
#include "ecs/systems/collision_system.hpp"
#include "ecs/systems/hitbox_math.hpp"

#include <entt/entt.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace raven;
using systems::circles_overlap;

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

    SECTION("Multiple enemy bullets — only first hits") {
        auto bullet1 = reg.create();
        reg.emplace<Transform2D>(bullet1, 101.f, 100.f); // overlaps player
        reg.emplace<CircleHitbox>(bullet1, 3.f, 0.f, 0.f);
        reg.emplace<Bullet>(bullet1, Bullet::Owner::Enemy);
        reg.emplace<DamageOnContact>(bullet1, 1.f);

        auto bullet2 = reg.create();
        reg.emplace<Transform2D>(bullet2, 99.f, 100.f); // also overlaps player
        reg.emplace<CircleHitbox>(bullet2, 3.f, 0.f, 0.f);
        reg.emplace<Bullet>(bullet2, Bullet::Owner::Enemy);
        reg.emplace<DamageOnContact>(bullet2, 1.f);

        systems::update_collision(reg);

        // Exactly one bullet should be destroyed
        int destroyed = 0;
        if (!reg.valid(bullet1))
            destroyed++;
        if (!reg.valid(bullet2))
            destroyed++;
        REQUIRE(destroyed == 1);

        // Player should have taken exactly one hit of damage
        auto& hp = reg.get<Health>(player);
        REQUIRE(hp.current == Catch::Approx(0.f));

        // Player should be invulnerable
        auto* inv = reg.try_get<Invulnerable>(player);
        REQUIRE(inv != nullptr);
        REQUIRE(inv->remaining == Catch::Approx(2.f));
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

TEST_CASE("Player bullet vs enemy collision", "[collision]") {
    entt::registry reg;

    // Create an enemy
    auto enemy = reg.create();
    reg.emplace<Transform2D>(enemy, 100.f, 100.f);
    reg.emplace<CircleHitbox>(enemy, 6.f, 0.f, 0.f);
    reg.emplace<Enemy>(enemy);
    reg.emplace<Health>(enemy, 3.f, 3.f);

    SECTION("Player bullet hits enemy") {
        auto bullet = reg.create();
        reg.emplace<Transform2D>(bullet, 103.f, 100.f); // overlaps enemy
        reg.emplace<CircleHitbox>(bullet, 3.f, 0.f, 0.f);
        reg.emplace<Bullet>(bullet, Bullet::Owner::Player);
        reg.emplace<DamageOnContact>(bullet, 1.f);

        systems::update_collision(reg);

        // Bullet should be destroyed
        REQUIRE_FALSE(reg.valid(bullet));

        // Enemy should have taken damage
        auto& hp = reg.get<Health>(enemy);
        REQUIRE(hp.current == Catch::Approx(2.f));
    }

    SECTION("Separated player bullet does not hit enemy") {
        auto bullet = reg.create();
        reg.emplace<Transform2D>(bullet, 200.f, 200.f); // far away
        reg.emplace<CircleHitbox>(bullet, 3.f, 0.f, 0.f);
        reg.emplace<Bullet>(bullet, Bullet::Owner::Player);
        reg.emplace<DamageOnContact>(bullet, 1.f);

        systems::update_collision(reg);

        // Bullet should still exist
        REQUIRE(reg.valid(bullet));

        // Enemy HP unchanged
        auto& hp = reg.get<Health>(enemy);
        REQUIRE(hp.current == Catch::Approx(3.f));
    }

    SECTION("Enemy bullet does not damage enemy") {
        auto bullet = reg.create();
        reg.emplace<Transform2D>(bullet, 103.f, 100.f); // overlaps enemy
        reg.emplace<CircleHitbox>(bullet, 3.f, 0.f, 0.f);
        reg.emplace<Bullet>(bullet, Bullet::Owner::Enemy);
        reg.emplace<DamageOnContact>(bullet, 1.f);

        systems::update_collision(reg);

        // Bullet should still exist (enemy bullet doesn't hit enemies)
        REQUIRE(reg.valid(bullet));

        // Enemy HP unchanged
        auto& hp = reg.get<Health>(enemy);
        REQUIRE(hp.current == Catch::Approx(3.f));
    }

    SECTION("Piercing bullet passes through enemy") {
        auto bullet = reg.create();
        reg.emplace<Transform2D>(bullet, 103.f, 100.f); // overlaps enemy
        reg.emplace<CircleHitbox>(bullet, 3.f, 0.f, 0.f);
        reg.emplace<Bullet>(bullet, Bullet::Owner::Player);
        reg.emplace<DamageOnContact>(bullet, 1.f);
        reg.emplace<Piercing>(bullet);

        systems::update_collision(reg);

        // Bullet should still exist (piercing)
        REQUIRE(reg.valid(bullet));

        // Enemy should have taken damage
        auto& hp = reg.get<Health>(enemy);
        REQUIRE(hp.current == Catch::Approx(2.f));
    }

    SECTION("Piercing bullet hits multiple enemies") {
        auto enemy2 = reg.create();
        reg.emplace<Transform2D>(enemy2, 103.f, 100.f); // same spot
        reg.emplace<CircleHitbox>(enemy2, 6.f, 0.f, 0.f);
        reg.emplace<Enemy>(enemy2);
        reg.emplace<Health>(enemy2, 3.f, 3.f);

        auto bullet = reg.create();
        reg.emplace<Transform2D>(bullet, 103.f, 100.f);
        reg.emplace<CircleHitbox>(bullet, 3.f, 0.f, 0.f);
        reg.emplace<Bullet>(bullet, Bullet::Owner::Player);
        reg.emplace<DamageOnContact>(bullet, 1.f);
        reg.emplace<Piercing>(bullet);

        systems::update_collision(reg);

        // Bullet should still exist (piercing)
        REQUIRE(reg.valid(bullet));

        // Both enemies should have taken damage
        auto& hp1 = reg.get<Health>(enemy);
        auto& hp2 = reg.get<Health>(enemy2);
        REQUIRE(hp1.current == Catch::Approx(2.f));
        REQUIRE(hp2.current == Catch::Approx(2.f));
    }
}
