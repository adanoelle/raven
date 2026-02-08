#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <entt/entt.hpp>
#include "ecs/components.hpp"

using namespace raven;

TEST_CASE("Entity creation with components", "[ecs]") {
    entt::registry reg;

    SECTION("create player entity") {
        auto player = reg.create();
        reg.emplace<Transform2D>(player, 100.f, 200.f);
        reg.emplace<Player>(player);
        reg.emplace<Health>(player, 1.f, 1.f);

        auto& tf = reg.get<Transform2D>(player);
        REQUIRE(tf.x == Catch::Approx(100.f));
        REQUIRE(tf.y == Catch::Approx(200.f));

        auto& hp = reg.get<Health>(player);
        REQUIRE(hp.current == Catch::Approx(1.f));
    }

    SECTION("create many bullets") {
        constexpr int NUM_BULLETS = 2000;

        for (int i = 0; i < NUM_BULLETS; ++i) {
            auto bullet = reg.create();
            reg.emplace<Transform2D>(bullet,
                static_cast<float>(i), static_cast<float>(i));
            reg.emplace<Velocity>(bullet, 0.f, 100.f);
            reg.emplace<Bullet>(bullet, Bullet::Owner::Enemy);
            reg.emplace<Lifetime>(bullet, 5.f);
        }

        auto view = reg.view<Transform2D, Velocity, Bullet>();
        REQUIRE(view.size_hint() == NUM_BULLETS);
    }

    SECTION("movement integration") {
        auto entity = reg.create();
        reg.emplace<Transform2D>(entity, 0.f, 0.f);
        reg.emplace<Velocity>(entity, 100.f, 50.f);

        float dt = 1.f / 120.f;
        auto view = reg.view<Transform2D, Velocity>();
        for (auto [e, tf, vel] : view.each()) {
            tf.x += vel.dx * dt;
            tf.y += vel.dy * dt;
        }

        auto& tf = reg.get<Transform2D>(entity);
        REQUIRE(tf.x == Catch::Approx(100.f / 120.f));
        REQUIRE(tf.y == Catch::Approx(50.f / 120.f));
    }
}
