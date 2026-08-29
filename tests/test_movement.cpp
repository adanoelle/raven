#include "ecs/components.hpp"
#include "ecs/systems/movement_system.hpp"
#include "rendering/renderer.hpp"

#include <entt/entt.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace raven;

namespace {
constexpr float DT = 1.f / 120.f;
} // namespace

TEST_CASE("Movement system integration", "[movement]") {
    entt::registry reg;

    SECTION("Velocity integrates into position") {
        auto ent = reg.create();
        reg.emplace<Transform2D>(ent, 100.f, 100.f);
        reg.emplace<Velocity>(ent, 120.f, -60.f);

        systems::update_movement(reg, DT);

        auto& tf = reg.get<Transform2D>(ent);
        REQUIRE(tf.x == Catch::Approx(101.f));
        REQUIRE(tf.y == Catch::Approx(99.5f));
    }

    SECTION("PreviousTransform snapshots the pre-move position") {
        auto ent = reg.create();
        reg.emplace<Transform2D>(ent, 50.f, 60.f);
        reg.emplace<PreviousTransform>(ent, 0.f, 0.f);
        reg.emplace<Velocity>(ent, 120.f, 120.f);

        systems::update_movement(reg, DT);

        auto& prev = reg.get<PreviousTransform>(ent);
        REQUIRE(prev.x == Catch::Approx(50.f));
        REQUIRE(prev.y == Catch::Approx(60.f));

        auto& tf = reg.get<Transform2D>(ent);
        REQUIRE(tf.x > prev.x);
        REQUIRE(tf.y > prev.y);
    }
}

TEST_CASE("Movement system screen bounds", "[movement]") {
    entt::registry reg;

    auto spawn_player = [&](float x, float y) {
        auto ent = reg.create();
        reg.emplace<Transform2D>(ent, x, y);
        reg.emplace<Velocity>(ent);
        reg.emplace<Player>(ent);
        Sprite spr;
        spr.width = 16;
        spr.height = 16;
        reg.emplace<Sprite>(ent, spr);
        return ent;
    };

    SECTION("Player is clamped to the left and top edges") {
        auto ent = spawn_player(2.f, -5.f);
        systems::update_movement(reg, DT);

        auto& tf = reg.get<Transform2D>(ent);
        REQUIRE(tf.x == Catch::Approx(8.f)); // half sprite width
        REQUIRE(tf.y == Catch::Approx(8.f));
    }

    SECTION("Player is clamped to the right and bottom edges") {
        auto ent = spawn_player(static_cast<float>(Renderer::VIRTUAL_WIDTH) + 20.f,
                                static_cast<float>(Renderer::VIRTUAL_HEIGHT) + 20.f);
        systems::update_movement(reg, DT);

        auto& tf = reg.get<Transform2D>(ent);
        REQUIRE(tf.x == Catch::Approx(static_cast<float>(Renderer::VIRTUAL_WIDTH) - 8.f));
        REQUIRE(tf.y == Catch::Approx(static_cast<float>(Renderer::VIRTUAL_HEIGHT) - 8.f));
    }

    SECTION("Player in bounds is not moved by clamping") {
        auto ent = spawn_player(240.f, 135.f);
        systems::update_movement(reg, DT);

        auto& tf = reg.get<Transform2D>(ent);
        REQUIRE(tf.x == Catch::Approx(240.f));
        REQUIRE(tf.y == Catch::Approx(135.f));
    }

    SECTION("Non-player entities are not clamped") {
        auto ent = reg.create();
        reg.emplace<Transform2D>(ent, -50.f, -50.f);
        reg.emplace<Velocity>(ent);

        systems::update_movement(reg, DT);

        auto& tf = reg.get<Transform2D>(ent);
        REQUIRE(tf.x == Catch::Approx(-50.f)); // bullets may fly off-screen
        REQUIRE(tf.y == Catch::Approx(-50.f));
    }
}
