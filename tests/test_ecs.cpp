#include "core/string_id.hpp"
#include "ecs/components.hpp"
#include "ecs/systems/animation_system.hpp"

#include <entt/entt.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

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
            reg.emplace<Transform2D>(bullet, static_cast<float>(i), static_cast<float>(i));
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

    SECTION("velocity smoothing converges") {
        // Test the exponential approach used in input_system
        float vel_dx = 0.f;
        float vel_dy = 0.f;
        constexpr float target_dx = 200.f;
        constexpr float target_dy = 0.f;
        constexpr float approach_rate = 60.f;
        constexpr float dt = 1.f / 120.f;

        // Simulate 15 ticks (125ms at 120Hz) of smoothing
        for (int i = 0; i < 15; ++i) {
            float t = 1.f - std::exp(-approach_rate * dt);
            vel_dx += (target_dx - vel_dx) * t;
            vel_dy += (target_dy - vel_dy) * t;
        }

        // After 15 ticks at 120Hz, velocity should be very close to target
        REQUIRE(vel_dx == Catch::Approx(200.f).margin(1.f));
        REQUIRE(vel_dy == Catch::Approx(0.f).margin(0.01f));
    }
}

TEST_CASE("Animation system", "[ecs][animation]") {
    entt::registry reg;
    auto& interner = reg.ctx().emplace<StringInterner>();

    SECTION("looping animation cycles frames") {
        auto entity = reg.create();
        reg.emplace<Sprite>(entity, interner.intern("test"), 0, 0, 16, 16, 0);
        reg.emplace<Animation>(entity, 0, 3, 0.1f, 0.f, 0, true);

        auto& anim = reg.get<Animation>(entity);
        auto& sprite = reg.get<Sprite>(entity);

        // Advance through all 4 frames (0,1,2,3) then wrap
        for (int i = 0; i < 4; ++i) {
            raven::systems::update_animation(reg, 0.1f);
        }
        // After 4 ticks of 0.1s: frame should wrap to start (0)
        REQUIRE(anim.current_frame == 0);
        REQUIRE(sprite.frame_x == 0);

        // One more tick advances to frame 1
        raven::systems::update_animation(reg, 0.1f);
        REQUIRE(anim.current_frame == 1);
        REQUIRE(sprite.frame_x == 1);
    }

    SECTION("one-shot animation stops at end") {
        auto entity = reg.create();
        reg.emplace<Sprite>(entity, interner.intern("test"), 0, 0, 16, 16, 0);
        reg.emplace<Animation>(entity, 0, 2, 0.1f, 0.f, 0, false);

        // Advance well past the end
        for (int i = 0; i < 10; ++i) {
            raven::systems::update_animation(reg, 0.1f);
        }

        auto& anim = reg.get<Animation>(entity);
        auto& sprite = reg.get<Sprite>(entity);
        REQUIRE(anim.current_frame == 2);
        REQUIRE(sprite.frame_x == 2);
    }

    SECTION("frame_x syncs with current_frame") {
        auto entity = reg.create();
        reg.emplace<Sprite>(entity, interner.intern("test"), 0, 0, 16, 16, 0);
        reg.emplace<Animation>(entity, 0, 3, 0.1f, 0.f, 0, true);

        for (int i = 0; i < 3; ++i) {
            raven::systems::update_animation(reg, 0.1f);
            auto& anim = reg.get<Animation>(entity);
            auto& sprite = reg.get<Sprite>(entity);
            REQUIRE(sprite.frame_x == anim.current_frame);
        }
    }

    SECTION("animation does not touch frame_y") {
        auto entity = reg.create();
        reg.emplace<Sprite>(entity, interner.intern("test"), 0, 5, 16, 16, 0);
        reg.emplace<Animation>(entity, 0, 3, 0.1f, 0.f, 0, true);

        for (int i = 0; i < 8; ++i) {
            raven::systems::update_animation(reg, 0.1f);
        }

        auto& sprite = reg.get<Sprite>(entity);
        REQUIRE(sprite.frame_y == 5);
    }
}
