#include "ecs/components.hpp"
#include "ecs/systems/emitter_system.hpp"
#include "patterns/pattern_library.hpp"

#include <entt/entt.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace raven;

namespace {

/// @brief Count bullet entities in the registry.
int count_bullets(entt::registry& reg) {
    auto view = reg.view<Bullet>();
    int count = 0;
    for ([[maybe_unused]] auto entity : view) {
        ++count;
    }
    return count;
}

/// @brief Load a simple radial pattern into the library.
void load_radial_pattern(PatternLibrary& lib, const std::string& name, int count, float speed,
                         float fire_rate, float spread_angle) {
    nlohmann::json j = {{"name", name},
                        {"emitters",
                         {{{"type", "radial"},
                           {"count", count},
                           {"speed", speed},
                           {"fire_rate", fire_rate},
                           {"spread_angle", spread_angle}}}}};
    lib.load_from_json(j);
}

} // namespace

TEST_CASE("Emitter system", "[emitters]") {
    entt::registry reg;
    PatternLibrary patterns;
    constexpr float dt = 1.f / 120.f;

    SECTION("Radial emitter fires on first tick") {
        load_radial_pattern(patterns, "test_radial", 3, 100.f, 0.1f, 360.f);

        auto enemy = reg.create();
        reg.emplace<Transform2D>(enemy, 50.f, 50.f);
        BulletEmitter emitter;
        emitter.pattern_name = "test_radial";
        reg.emplace<BulletEmitter>(enemy, std::move(emitter));

        systems::update_emitters(reg, patterns, dt);

        // Should fire 3 bullets (radial, 360 degrees)
        REQUIRE(count_bullets(reg) == 3);

        // All bullets should be enemy-owned
        auto bullet_view = reg.view<Bullet>();
        for (auto [entity, bullet] : bullet_view.each()) {
            REQUIRE(bullet.owner == Bullet::Owner::Enemy);
        }
    }

    SECTION("Emitter respects cooldown") {
        load_radial_pattern(patterns, "cooldown_test", 1, 100.f, 1.0f, 0.f);

        auto enemy = reg.create();
        reg.emplace<Transform2D>(enemy, 50.f, 50.f);
        BulletEmitter emitter;
        emitter.pattern_name = "cooldown_test";
        reg.emplace<BulletEmitter>(enemy, std::move(emitter));

        // First tick fires
        systems::update_emitters(reg, patterns, dt);
        REQUIRE(count_bullets(reg) == 1);

        // Second tick should not fire (cooldown = 1.0s, dt ~= 0.008s)
        systems::update_emitters(reg, patterns, dt);
        REQUIRE(count_bullets(reg) == 1);
    }

    SECTION("Inactive emitter does not fire") {
        load_radial_pattern(patterns, "inactive_test", 3, 100.f, 0.1f, 360.f);

        auto enemy = reg.create();
        reg.emplace<Transform2D>(enemy, 50.f, 50.f);
        BulletEmitter emitter;
        emitter.pattern_name = "inactive_test";
        emitter.active = false;
        reg.emplace<BulletEmitter>(enemy, std::move(emitter));

        systems::update_emitters(reg, patterns, dt);

        REQUIRE(count_bullets(reg) == 0);
    }

    SECTION("Missing pattern name does not fire") {
        auto enemy = reg.create();
        reg.emplace<Transform2D>(enemy, 50.f, 50.f);
        BulletEmitter emitter;
        emitter.pattern_name = "nonexistent";
        reg.emplace<BulletEmitter>(enemy, std::move(emitter));

        systems::update_emitters(reg, patterns, dt);

        REQUIRE(count_bullets(reg) == 0);
    }

    SECTION("Aimed emitter fires toward player") {
        // Create an aimed pattern
        nlohmann::json j = {{"name", "aimed_test"},
                            {"emitters",
                             {{{"type", "aimed"},
                               {"count", 1},
                               {"speed", 100.f},
                               {"fire_rate", 0.1f},
                               {"spread_angle", 0.f}}}}};
        patterns.load_from_json(j);

        // Player directly to the right of enemy
        auto player = reg.create();
        reg.emplace<Transform2D>(player, 200.f, 50.f);
        reg.emplace<Player>(player);

        auto enemy = reg.create();
        reg.emplace<Transform2D>(enemy, 50.f, 50.f);
        BulletEmitter emitter;
        emitter.pattern_name = "aimed_test";
        reg.emplace<BulletEmitter>(enemy, std::move(emitter));

        systems::update_emitters(reg, patterns, dt);

        REQUIRE(count_bullets(reg) == 1);

        // Bullet should be heading to the right (toward player)
        auto bullet_view = reg.view<Bullet, Velocity>();
        for (auto [entity, bullet, vel] : bullet_view.each()) {
            REQUIRE(vel.dx == Catch::Approx(100.f).margin(1.f));
            REQUIRE(vel.dy == Catch::Approx(0.f).margin(1.f));
        }
    }

    SECTION("Bullets spawn at emitter position") {
        load_radial_pattern(patterns, "pos_test", 1, 100.f, 0.1f, 0.f);

        auto enemy = reg.create();
        reg.emplace<Transform2D>(enemy, 123.f, 456.f);
        BulletEmitter emitter;
        emitter.pattern_name = "pos_test";
        reg.emplace<BulletEmitter>(enemy, std::move(emitter));

        systems::update_emitters(reg, patterns, dt);

        auto bullet_view = reg.view<Bullet, Transform2D>();
        for (auto [entity, bullet, tf] : bullet_view.each()) {
            REQUIRE(tf.x == Catch::Approx(123.f));
            REQUIRE(tf.y == Catch::Approx(456.f));
        }
    }
}
