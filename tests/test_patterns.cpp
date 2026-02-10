#include "core/string_id.hpp"
#include "patterns/pattern_library.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace raven;

// Test bullet pattern math
TEST_CASE("Radial pattern angle distribution", "[patterns]") {
    SECTION("3-way spread in 360 degrees") {
        int count = 3;
        float spread = 360.f;
        float start = 0.f;

        for (int i = 0; i < count; ++i) {
            float angle = start + (spread / static_cast<float>(count)) * static_cast<float>(i);
            float expected = static_cast<float>(i) * 120.f;
            REQUIRE(angle == Catch::Approx(expected));
        }
    }

    SECTION("5-way aimed forward") {
        int count = 5;
        float spread = 30.f;                // narrow cone
        float start = 270.f - spread / 2.f; // centered upward

        float first_angle = start;
        float last_angle =
            start + spread * (static_cast<float>(count - 1) / static_cast<float>(count));

        REQUIRE(first_angle == Catch::Approx(255.f));
        REQUIRE(last_angle < 270.f + spread / 2.f);
    }
}

TEST_CASE("Bullet velocity from angle and speed", "[patterns]") {
    float speed = 100.f;

    SECTION("0 degrees = right") {
        float angle_rad = 0.f;
        float vx = std::cos(angle_rad) * speed;
        float vy = std::sin(angle_rad) * speed;
        REQUIRE(vx == Catch::Approx(100.f));
        REQUIRE(vy == Catch::Approx(0.f).margin(0.001f));
    }

    SECTION("90 degrees = down") {
        float angle_rad = static_cast<float>(M_PI) / 2.f;
        float vx = std::cos(angle_rad) * speed;
        float vy = std::sin(angle_rad) * speed;
        REQUIRE(vx == Catch::Approx(0.f).margin(0.001f));
        REQUIRE(vy == Catch::Approx(100.f));
    }
}

TEST_CASE("PatternLibrary load_from_json", "[patterns]") {
    StringInterner interner;
    PatternLibrary lib;
    lib.set_interner(interner);

    SECTION("Valid JSON parses correctly") {
        nlohmann::json j = {{"name", "test_spiral"},
                            {"emitters",
                             {{{"type", "radial"},
                               {"count", 8},
                               {"speed", 150.f},
                               {"spread_angle", 360.f},
                               {"fire_rate", 0.2f}}}}};

        REQUIRE(lib.load_from_json(j));

        const auto* pat = lib.get("test_spiral");
        REQUIRE(pat != nullptr);
        REQUIRE(pat->name == "test_spiral");
        REQUIRE(pat->emitters.size() == 1);
        REQUIRE(pat->emitters[0].count == 8);
        REQUIRE(pat->emitters[0].speed == Catch::Approx(150.f));
        REQUIRE(pat->emitters[0].spread_angle == Catch::Approx(360.f));
        REQUIRE(pat->emitters[0].fire_rate == Catch::Approx(0.2f));
    }

    SECTION("Missing optional fields use defaults") {
        nlohmann::json j = {{"name", "minimal"}, {"emitters", {{{"type", "aimed"}}}}};

        REQUIRE(lib.load_from_json(j));

        const auto* pat = lib.get("minimal");
        REQUIRE(pat != nullptr);
        REQUIRE(pat->emitters.size() == 1);
        REQUIRE(pat->emitters[0].type == EmitterDef::Type::Aimed);
        REQUIRE(pat->emitters[0].count == 1);
        REQUIRE(pat->emitters[0].speed == Catch::Approx(100.f));
        REQUIRE(pat->emitters[0].lifetime == Catch::Approx(5.f));
        REQUIRE(pat->emitters[0].damage == Catch::Approx(1.f));
        REQUIRE(pat->emitters[0].hitbox_radius == Catch::Approx(3.f));
    }

    SECTION("Multiple emitters") {
        nlohmann::json j = {{"name", "multi"},
                            {"emitters",
                             {{{"type", "radial"}, {"count", 3}},
                              {{"type", "aimed"}, {"count", 1}},
                              {{"type", "linear"}, {"count", 5}}}}};

        REQUIRE(lib.load_from_json(j));

        const auto* pat = lib.get("multi");
        REQUIRE(pat != nullptr);
        REQUIRE(pat->emitters.size() == 3);
        REQUIRE(pat->emitters[0].type == EmitterDef::Type::Radial);
        REQUIRE(pat->emitters[1].type == EmitterDef::Type::Aimed);
        REQUIRE(pat->emitters[2].type == EmitterDef::Type::Linear);
    }

    SECTION("Invalid JSON returns false") {
        nlohmann::json j = {{"wrong_key", "no_name_field"}};

        REQUIRE_FALSE(lib.load_from_json(j));
    }

    SECTION("names() returns loaded pattern names") {
        nlohmann::json j1 = {{"name", "alpha"}, {"emitters", {{{"type", "radial"}}}}};
        nlohmann::json j2 = {{"name", "beta"}, {"emitters", {{{"type", "aimed"}}}}};

        REQUIRE(lib.load_from_json(j1));
        REQUIRE(lib.load_from_json(j2));

        auto names = lib.names();
        REQUIRE(names.size() == 2);
        // Names may be in any order (unordered_map)
        bool has_alpha = false;
        bool has_beta = false;
        for (const auto& n : names) {
            if (n == "alpha")
                has_alpha = true;
            if (n == "beta")
                has_beta = true;
        }
        REQUIRE(has_alpha);
        REQUIRE(has_beta);
    }
}
