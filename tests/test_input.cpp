#include "core/input.hpp"
#include "ecs/components.hpp"
#include "ecs/systems/input_system.hpp"

#include <entt/entt.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace raven;

namespace {

constexpr float DT = 1.f / 120.f;

entt::entity spawn_player(entt::registry& reg, float speed = 200.f) {
    auto ent = reg.create();
    reg.emplace<Player>(ent, speed, 3);
    reg.emplace<Velocity>(ent);
    return ent;
}

/// Run enough ticks for the exponential approach to fully converge.
void settle(entt::registry& reg, const InputState& input) {
    for (int i = 0; i < 200; ++i) {
        systems::update_input(reg, input, DT);
    }
}

} // namespace

TEST_CASE("Input system movement", "[input]") {
    entt::registry reg;

    SECTION("Diagonal input is normalised to unit length") {
        auto ent = spawn_player(reg);
        InputState input;
        input.move_x = 1.f;
        input.move_y = 1.f;

        settle(reg, input);

        auto& vel = reg.get<Velocity>(ent);
        float mag = std::sqrt(vel.dx * vel.dx + vel.dy * vel.dy);
        REQUIRE(mag == Catch::Approx(200.f).margin(0.1f));
        REQUIRE(vel.dx == Catch::Approx(200.f / std::sqrt(2.f)).margin(0.1f));
        REQUIRE(vel.dy == Catch::Approx(200.f / std::sqrt(2.f)).margin(0.1f));
    }

    SECTION("Cardinal input reaches full speed") {
        auto ent = spawn_player(reg);
        InputState input;
        input.move_x = 1.f;

        settle(reg, input);

        auto& vel = reg.get<Velocity>(ent);
        REQUIRE(vel.dx == Catch::Approx(200.f).margin(0.1f));
        REQUIRE(vel.dy == Catch::Approx(0.f).margin(0.001f));
    }

    SECTION("Sub-unit stick input is not boosted to full speed") {
        auto ent = spawn_player(reg);
        InputState input;
        input.move_x = 0.5f;

        settle(reg, input);

        auto& vel = reg.get<Velocity>(ent);
        REQUIRE(vel.dx == Catch::Approx(100.f).margin(0.1f));
    }

    SECTION("Single tick applies the exponential approach factor") {
        auto ent = spawn_player(reg);
        InputState input;
        input.move_x = 1.f;

        systems::update_input(reg, input, DT);

        float expected = 200.f * (1.f - std::exp(-60.f * DT));
        auto& vel = reg.get<Velocity>(ent);
        REQUIRE(vel.dx == Catch::Approx(expected).margin(0.001f));
    }

    SECTION("Releasing input decays velocity back to zero") {
        auto ent = spawn_player(reg);
        InputState moving;
        moving.move_x = 1.f;
        settle(reg, moving);

        InputState idle;
        settle(reg, idle);

        auto& vel = reg.get<Velocity>(ent);
        REQUIRE(vel.dx == Catch::Approx(0.f).margin(0.1f));
    }

    SECTION("Dash overrides input-driven movement") {
        auto ent = spawn_player(reg);
        reg.get<Velocity>(ent) = {400.f, 0.f}; // dash burst velocity
        reg.emplace<Dash>(ent);

        InputState input;
        input.move_x = -1.f;
        systems::update_input(reg, input, DT);

        auto& vel = reg.get<Velocity>(ent);
        REQUIRE(vel.dx == Catch::Approx(400.f)); // untouched
    }

    SECTION("Charging a shot applies the move penalty") {
        auto ent = spawn_player(reg);
        ChargedShot cs;
        cs.charging = true;
        cs.move_penalty = 0.5f;
        reg.emplace<ChargedShot>(ent, cs);

        InputState input;
        input.move_x = 1.f;
        settle(reg, input);

        auto& vel = reg.get<Velocity>(ent);
        REQUIRE(vel.dx == Catch::Approx(100.f).margin(0.1f));
    }
}
