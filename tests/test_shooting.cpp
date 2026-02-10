#include "ecs/components.hpp"
#include "ecs/systems/shooting_system.hpp"

#include <entt/entt.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace raven;

namespace {

/// @brief Create a minimal player entity with shooting components.
entt::entity make_player(entt::registry& reg, float x, float y) {
    auto player = reg.create();
    reg.emplace<Transform2D>(player, x, y);
    reg.emplace<PreviousTransform>(player, x, y);
    reg.emplace<Velocity>(player);
    reg.emplace<Player>(player);
    reg.emplace<AimDirection>(player, 1.f, 0.f);
    reg.emplace<ShootCooldown>(player, 0.f, 0.2f);
    return player;
}

/// @brief Count bullet entities in the registry.
int count_bullets(entt::registry& reg) {
    auto view = reg.view<Bullet>();
    int count = 0;
    for ([[maybe_unused]] auto entity : view) {
        ++count;
    }
    return count;
}

} // namespace

TEST_CASE("Shooting cooldown", "[shooting]") {
    entt::registry reg;
    constexpr float dt = 1.f / 120.f;

    SECTION("bullet spawned when shoot held and cooldown expired") {
        make_player(reg, 100.f, 100.f);

        InputState input{};
        input.shoot = true;

        systems::update_shooting(reg, input, dt);

        REQUIRE(count_bullets(reg) == 1);

        // Verify bullet components
        auto bullet_view = reg.view<Bullet, Transform2D, Velocity, Sprite>();
        for (auto [entity, bullet, tf, vel, sprite] : bullet_view.each()) {
            REQUIRE(bullet.owner == Bullet::Owner::Player);
            REQUIRE(tf.x == Catch::Approx(100.f));
            REQUIRE(tf.y == Catch::Approx(100.f));
            REQUIRE(sprite.sheet_id == "projectiles");
            REQUIRE(sprite.frame_x == 1);
            REQUIRE(sprite.frame_y == 0);
            REQUIRE(sprite.width == 8);
            REQUIRE(sprite.height == 8);
        }
    }

    SECTION("no bullet when cooldown still active") {
        auto player = make_player(reg, 100.f, 100.f);
        auto& cd = reg.get<ShootCooldown>(player);
        cd.remaining = 0.1f; // Still on cooldown

        InputState input{};
        input.shoot = true;

        systems::update_shooting(reg, input, dt);

        REQUIRE(count_bullets(reg) == 0);
    }

    SECTION("cooldown resets after firing") {
        auto player = make_player(reg, 100.f, 100.f);

        InputState input{};
        input.shoot = true;

        systems::update_shooting(reg, input, dt);
        REQUIRE(count_bullets(reg) == 1);

        auto& cd = reg.get<ShootCooldown>(player);
        // Cooldown is decremented by dt first, then reset to rate on fire
        REQUIRE(cd.remaining == Catch::Approx(0.2f).margin(0.001f));
    }

    SECTION("no bullet when shoot not held") {
        make_player(reg, 100.f, 100.f);

        InputState input{};
        input.shoot = false;

        systems::update_shooting(reg, input, dt);

        REQUIRE(count_bullets(reg) == 0);
    }
}

TEST_CASE("Aim direction resolution", "[shooting]") {
    entt::registry reg;
    constexpr float dt = 1.f / 120.f;

    SECTION("right stick sets aim direction") {
        auto player = make_player(reg, 100.f, 100.f);

        InputState input{};
        input.aim_x = 0.f;
        input.aim_y = 1.f;

        systems::update_shooting(reg, input, dt);

        auto& aim = reg.get<AimDirection>(player);
        REQUIRE(aim.x == Catch::Approx(0.f).margin(0.01f));
        REQUIRE(aim.y == Catch::Approx(1.f).margin(0.01f));
    }

    SECTION("mouse position sets aim when stick inactive") {
        auto player = make_player(reg, 100.f, 100.f);

        InputState input{};
        input.aim_x = 0.f;
        input.aim_y = 0.f;
        input.mouse_active = true;
        input.mouse_x = 200.f;
        input.mouse_y = 100.f; // Directly to the right

        systems::update_shooting(reg, input, dt);

        auto& aim = reg.get<AimDirection>(player);
        REQUIRE(aim.x == Catch::Approx(1.f).margin(0.01f));
        REQUIRE(aim.y == Catch::Approx(0.f).margin(0.01f));
    }

    SECTION("aim direction retained when no input") {
        auto player = make_player(reg, 100.f, 100.f);

        // Set initial aim to downward
        auto& aim = reg.get<AimDirection>(player);
        aim.x = 0.f;
        aim.y = 1.f;

        InputState input{};
        // No stick, no mouse
        input.aim_x = 0.f;
        input.aim_y = 0.f;
        input.mouse_active = false;

        systems::update_shooting(reg, input, dt);

        REQUIRE(aim.x == Catch::Approx(0.f).margin(0.01f));
        REQUIRE(aim.y == Catch::Approx(1.f).margin(0.01f));
    }

    SECTION("aim direction is normalized") {
        auto player = make_player(reg, 100.f, 100.f);

        InputState input{};
        input.aim_x = 0.8f;
        input.aim_y = 0.8f; // Not normalized

        systems::update_shooting(reg, input, dt);

        auto& aim = reg.get<AimDirection>(player);
        float len = std::sqrt(aim.x * aim.x + aim.y * aim.y);
        REQUIRE(len == Catch::Approx(1.f).margin(0.01f));
    }

    SECTION("mouse aim right") {
        auto player = make_player(reg, 100.f, 100.f);

        InputState input{};
        input.mouse_active = true;
        input.mouse_x = 200.f;
        input.mouse_y = 100.f;

        systems::update_shooting(reg, input, dt);

        auto& aim = reg.get<AimDirection>(player);
        REQUIRE(aim.x == Catch::Approx(1.f).margin(0.01f));
        REQUIRE(aim.y == Catch::Approx(0.f).margin(0.01f));
    }

    SECTION("mouse aim left") {
        auto player = make_player(reg, 100.f, 100.f);

        InputState input{};
        input.mouse_active = true;
        input.mouse_x = 0.f;
        input.mouse_y = 100.f;

        systems::update_shooting(reg, input, dt);

        auto& aim = reg.get<AimDirection>(player);
        REQUIRE(aim.x == Catch::Approx(-1.f).margin(0.01f));
        REQUIRE(aim.y == Catch::Approx(0.f).margin(0.01f));
    }

    SECTION("mouse aim up") {
        auto player = make_player(reg, 100.f, 100.f);

        InputState input{};
        input.mouse_active = true;
        input.mouse_x = 100.f;
        input.mouse_y = 0.f;

        systems::update_shooting(reg, input, dt);

        auto& aim = reg.get<AimDirection>(player);
        REQUIRE(aim.x == Catch::Approx(0.f).margin(0.01f));
        REQUIRE(aim.y == Catch::Approx(-1.f).margin(0.01f));
    }

    SECTION("mouse aim down") {
        auto player = make_player(reg, 100.f, 100.f);

        InputState input{};
        input.mouse_active = true;
        input.mouse_x = 100.f;
        input.mouse_y = 200.f;

        systems::update_shooting(reg, input, dt);

        auto& aim = reg.get<AimDirection>(player);
        REQUIRE(aim.x == Catch::Approx(0.f).margin(0.01f));
        REQUIRE(aim.y == Catch::Approx(1.f).margin(0.01f));
    }

    SECTION("mouse aim diagonal is normalized") {
        auto player = make_player(reg, 100.f, 100.f);

        InputState input{};
        input.mouse_active = true;
        input.mouse_x = 200.f;
        input.mouse_y = 200.f;

        systems::update_shooting(reg, input, dt);

        auto& aim = reg.get<AimDirection>(player);
        float len = std::sqrt(aim.x * aim.x + aim.y * aim.y);
        REQUIRE(len == Catch::Approx(1.f).margin(0.01f));
        REQUIRE(aim.x == Catch::Approx(aim.y).margin(0.01f));
    }

    SECTION("mouse on player retains previous aim") {
        auto player = make_player(reg, 100.f, 100.f);

        auto& aim = reg.get<AimDirection>(player);
        aim.x = 0.f;
        aim.y = -1.f;

        InputState input{};
        input.mouse_active = true;
        input.mouse_x = 100.f; // Same as player position
        input.mouse_y = 100.f;

        systems::update_shooting(reg, input, dt);

        REQUIRE(aim.x == Catch::Approx(0.f).margin(0.01f));
        REQUIRE(aim.y == Catch::Approx(-1.f).margin(0.01f));
    }

    SECTION("shoot fires bullet toward mouse") {
        auto player = make_player(reg, 100.f, 100.f);

        InputState input{};
        input.shoot = true;
        input.mouse_active = true;
        input.mouse_x = 100.f;
        input.mouse_y = 0.f; // Directly above

        systems::update_shooting(reg, input, dt);

        REQUIRE(count_bullets(reg) == 1);
        auto bullet_view = reg.view<Bullet, Velocity>();
        for (auto [entity, bullet, vel] : bullet_view.each()) {
            REQUIRE(vel.dx == Catch::Approx(0.f).margin(0.1f));
            REQUIRE(vel.dy == Catch::Approx(-300.f).margin(0.1f));
        }
    }

    SECTION("right stick overrides mouse when both active") {
        auto player = make_player(reg, 100.f, 100.f);

        InputState input{};
        input.mouse_active = true;
        input.mouse_x = 200.f;
        input.mouse_y = 100.f; // Mouse aims right
        input.aim_x = 0.f;
        input.aim_y = 1.f; // Stick aims down

        systems::update_shooting(reg, input, dt);

        auto& aim = reg.get<AimDirection>(player);
        REQUIRE(aim.x == Catch::Approx(0.f).margin(0.01f));
        REQUIRE(aim.y == Catch::Approx(1.f).margin(0.01f));
    }

    SECTION("bullet velocity matches aim direction") {
        auto player = make_player(reg, 100.f, 100.f);

        // Aim to the right
        auto& aim = reg.get<AimDirection>(player);
        aim.x = 1.f;
        aim.y = 0.f;

        InputState input{};
        input.shoot = true;

        systems::update_shooting(reg, input, dt);

        auto bullet_view = reg.view<Bullet, Velocity>();
        for (auto [entity, bullet, vel] : bullet_view.each()) {
            REQUIRE(vel.dx == Catch::Approx(300.f).margin(0.1f));
            REQUIRE(vel.dy == Catch::Approx(0.f).margin(0.1f));
        }
    }
}
