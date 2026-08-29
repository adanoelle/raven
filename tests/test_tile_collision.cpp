#include "ecs/components.hpp"
#include "ecs/systems/tile_collision_system.hpp"
#include "rendering/tilemap.hpp"

#include <entt/entt.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace raven;

namespace {

constexpr int GRID_W = 10;
constexpr int GRID_H = 10;
constexpr int CELL = 16;

/// Build a collision grid where cells matching the predicate are solid.
template <typename Pred>
std::vector<bool> make_grid(Pred pred) {
    std::vector<bool> grid(static_cast<size_t>(GRID_W) * GRID_H, false);
    for (int gy = 0; gy < GRID_H; ++gy) {
        for (int gx = 0; gx < GRID_W; ++gx) {
            grid[static_cast<size_t>(gy) * GRID_W + gx] = pred(gx, gy);
        }
    }
    return grid;
}

/// Spawn an entity with an 8x8 centred hitbox at (x, y), previously at (px, py).
entt::entity spawn_mover(entt::registry& reg, float x, float y, float px, float py, float dx,
                         float dy) {
    auto ent = reg.create();
    reg.emplace<Transform2D>(ent, x, y);
    reg.emplace<PreviousTransform>(ent, px, py);
    reg.emplace<Velocity>(ent, dx, dy);
    reg.emplace<RectHitbox>(ent, 8.f, 8.f);
    return ent;
}

} // namespace

TEST_CASE("Tile collision axis-separated resolution", "[tile_collision]") {
    entt::registry reg;
    Tilemap tilemap;

    SECTION("No overlap leaves entity untouched") {
        tilemap.init_collision(GRID_W, GRID_H, CELL,
                               make_grid([](int gx, int) { return gx == 5; }));

        auto ent = spawn_mover(reg, 40.f, 40.f, 38.f, 40.f, 100.f, 0.f);
        systems::update_tile_collision(reg, tilemap);

        auto& tf = reg.get<Transform2D>(ent);
        auto& vel = reg.get<Velocity>(ent);
        REQUIRE(tf.x == Catch::Approx(40.f));
        REQUIRE(tf.y == Catch::Approx(40.f));
        REQUIRE(vel.dx == Catch::Approx(100.f));
    }

    SECTION("Moving into a wall reverts the blocked axis only") {
        // Solid column at gx == 5 (world x in [80, 96))
        tilemap.init_collision(GRID_W, GRID_H, CELL,
                               make_grid([](int gx, int) { return gx == 5; }));

        // Diagonal move right+down into the wall: X is blocked, Y is free.
        auto ent = spawn_mover(reg, 78.f, 44.f, 70.f, 40.f, 100.f, 50.f);
        systems::update_tile_collision(reg, tilemap);

        auto& tf = reg.get<Transform2D>(ent);
        auto& vel = reg.get<Velocity>(ent);
        REQUIRE(tf.x == Catch::Approx(70.f)); // X reverted
        REQUIRE(tf.y == Catch::Approx(44.f)); // Y movement kept (slide along wall)
        REQUIRE(vel.dx == Catch::Approx(0.f));
        REQUIRE(vel.dy == Catch::Approx(50.f));
    }

    SECTION("Moving straight down onto a floor reverts Y and keeps X") {
        // Solid row at gy == 5 (world y in [80, 96))
        tilemap.init_collision(GRID_W, GRID_H, CELL,
                               make_grid([](int, int gy) { return gy == 5; }));

        auto ent = spawn_mover(reg, 40.f, 78.f, 40.f, 70.f, 0.f, 100.f);
        systems::update_tile_collision(reg, tilemap);

        auto& tf = reg.get<Transform2D>(ent);
        auto& vel = reg.get<Velocity>(ent);
        REQUIRE(tf.x == Catch::Approx(40.f));
        REQUIRE(tf.y == Catch::Approx(70.f)); // Y reverted
        REQUIRE(vel.dy == Catch::Approx(0.f));
    }

    SECTION("Corner with both axes blocked fully reverts") {
        // Solid column at gx == 5 and solid row at gy == 5
        tilemap.init_collision(GRID_W, GRID_H, CELL,
                               make_grid([](int gx, int gy) { return gx == 5 || gy == 5; }));

        auto ent = spawn_mover(reg, 78.f, 78.f, 70.f, 70.f, 100.f, 100.f);
        systems::update_tile_collision(reg, tilemap);

        auto& tf = reg.get<Transform2D>(ent);
        auto& vel = reg.get<Velocity>(ent);
        REQUIRE(tf.x == Catch::Approx(70.f));
        REQUIRE(tf.y == Catch::Approx(70.f));
        REQUIRE(vel.dx == Catch::Approx(0.f));
        REQUIRE(vel.dy == Catch::Approx(0.f));
    }

    SECTION("Unloaded tilemap is a no-op") {
        Tilemap unloaded;
        auto ent = spawn_mover(reg, 88.f, 88.f, 40.f, 40.f, 100.f, 100.f);
        systems::update_tile_collision(reg, unloaded);

        auto& tf = reg.get<Transform2D>(ent);
        REQUIRE(tf.x == Catch::Approx(88.f));
        REQUIRE(tf.y == Catch::Approx(88.f));
    }
}
