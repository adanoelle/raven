#include "ecs/components.hpp"
#include "ecs/systems/tile_collision_system.hpp"
#include "rendering/tilemap.hpp"

#include <entt/entt.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace raven;

namespace {

/// @brief Create a 4x4 collision grid with a solid border and open interior.
///   1 1 1 1
///   1 0 0 1
///   1 0 0 1
///   1 1 1 1
Tilemap make_bordered_tilemap() {
    // clang-format off
    std::vector<bool> grid = {
        true,  true,  true,  true,
        true,  false, false, true,
        true,  false, false, true,
        true,  true,  true,  true,
    };
    // clang-format on
    Tilemap tm;
    tm.init_collision(4, 4, 16, std::move(grid));
    return tm;
}

} // namespace

TEST_CASE("Tilemap collision grid", "[tilemap]") {
    auto tm = make_bordered_tilemap();

    SECTION("is_solid detects solid cells") {
        // Top-left corner cell is solid
        REQUIRE(tm.is_solid(0.f, 0.f, 8.f, 8.f));
        // Bottom-right corner cell is solid
        REQUIRE(tm.is_solid(48.f, 48.f, 8.f, 8.f));
    }

    SECTION("is_solid returns false for empty cells") {
        // Interior cell (1,1) is open
        REQUIRE_FALSE(tm.is_solid(20.f, 20.f, 8.f, 8.f));
        // Interior cell (2,2) is open
        REQUIRE_FALSE(tm.is_solid(36.f, 36.f, 8.f, 8.f));
    }

    SECTION("out of bounds treated as non-solid") {
        REQUIRE_FALSE(tm.is_solid(-10.f, -10.f, 4.f, 4.f));
        REQUIRE_FALSE(tm.is_solid(100.f, 100.f, 4.f, 4.f));
    }

    SECTION("partial overlap with solid cell detected") {
        // AABB overlaps both cell (0,1) solid border and cell (1,1) open interior
        REQUIRE(tm.is_solid(12.f, 16.f, 8.f, 8.f));
    }
}

TEST_CASE("Tilemap cell queries", "[tilemap]") {
    auto tm = make_bordered_tilemap();

    SECTION("is_cell_solid returns correct values") {
        REQUIRE(tm.is_cell_solid(0, 0));
        REQUIRE(tm.is_cell_solid(3, 3));
        REQUIRE_FALSE(tm.is_cell_solid(1, 1));
        REQUIRE_FALSE(tm.is_cell_solid(2, 2));
    }

    SECTION("out of bounds cells are non-solid") {
        REQUIRE_FALSE(tm.is_cell_solid(-1, 0));
        REQUIRE_FALSE(tm.is_cell_solid(0, -1));
        REQUIRE_FALSE(tm.is_cell_solid(4, 0));
        REQUIRE_FALSE(tm.is_cell_solid(0, 4));
    }
}

TEST_CASE("Tilemap spawn points", "[tilemap]") {
    Tilemap tm;
    // init_collision sets loaded_ = true but spawns are empty
    tm.init_collision(2, 2, 16, {false, false, false, false});

    SECTION("find_spawn returns nullptr for unknown name") {
        REQUIRE(tm.find_spawn("PlayerStart") == nullptr);
    }
}

TEST_CASE("Tilemap properties", "[tilemap]") {
    auto tm = make_bordered_tilemap();

    REQUIRE(tm.width_px() == 64);
    REQUIRE(tm.height_px() == 64);
    REQUIRE(tm.cell_size() == 16);
    REQUIRE(tm.is_loaded());
    REQUIRE(tm.texture() == nullptr); // no SDL texture in test
}

TEST_CASE("Tile collision resolution", "[tilemap][ecs]") {
    auto tm = make_bordered_tilemap();
    entt::registry reg;

    // Create an entity in the open interior
    auto entity = reg.create();
    float start_x = 32.f; // center of cell (2,1)
    float start_y = 32.f; // center of cell (1,2)
    reg.emplace<Transform2D>(entity, start_x, start_y);
    reg.emplace<PreviousTransform>(entity, start_x, start_y);
    reg.emplace<Velocity>(entity, 0.f, 0.f);
    reg.emplace<RectHitbox>(entity, 8.f, 8.f, 0.f, 0.f);

    SECTION("entity moves freely in open space") {
        // Move within open area
        auto& tf = reg.get<Transform2D>(entity);
        tf.x = 36.f;
        tf.y = 36.f;
        reg.get<Velocity>(entity).dx = 100.f;

        systems::update_tile_collision(reg, tm);

        // Should not be pushed back
        REQUIRE(tf.x == Catch::Approx(36.f));
        REQUIRE(tf.y == Catch::Approx(36.f));
    }

    SECTION("entity pushed out of solid tile") {
        // Move into right wall (cell 3,1 is solid)
        auto& tf = reg.get<Transform2D>(entity);
        tf.x = 50.f; // hitbox right edge = 50 + 4 = 54, into cell 3
        reg.get<Velocity>(entity).dx = 200.f;

        systems::update_tile_collision(reg, tm);

        // Should be pushed back to previous X
        REQUIRE(tf.x == Catch::Approx(start_x));
        REQUIRE(reg.get<Velocity>(entity).dx == Catch::Approx(0.f));
    }

    SECTION("entity velocity zeroed on collision axis") {
        // Move into bottom wall (cell 1,3 is solid)
        auto& tf = reg.get<Transform2D>(entity);
        tf.y = 50.f; // hitbox bottom edge = 50 + 4 = 54, into cell 3
        auto& vel = reg.get<Velocity>(entity);
        vel.dx = 50.f;
        vel.dy = 200.f;

        systems::update_tile_collision(reg, tm);

        // Y should be reverted, dy zeroed. X velocity should be preserved.
        REQUIRE(tf.y == Catch::Approx(start_y));
        REQUIRE(vel.dy == Catch::Approx(0.f));
        REQUIRE(vel.dx == Catch::Approx(50.f));
    }
}
