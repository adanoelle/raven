#include "core/string_id.hpp"
#include "patterns/pattern_library.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>

using namespace raven;

namespace {
// M_PI is not portable (MSVC needs _USE_MATH_DEFINES); match the game code
constexpr float PI = 3.14159265358979323846f;
} // namespace

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
        float angle_rad = PI / 2.f;
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

    SECTION("Loading without an interner fails instead of crashing") {
        PatternLibrary no_interner;
        nlohmann::json j = {{"name", "orphan"}, {"emitters", {{{"type", "radial"}}}}};

        REQUIRE_FALSE(no_interner.load_from_json(j));
    }

    SECTION("Unsafe numeric values are clamped") {
        // fire_rate <= 0 would fire a burst every tick; count is capped to
        // prevent thousands of bullets per burst from a data typo.
        nlohmann::json j = {{"name", "hostile"},
                            {"emitters",
                             {{{"type", "radial"},
                               {"count", 100000},
                               {"fire_rate", 0.f},
                               {"lifetime", -1.f},
                               {"hitbox_radius", -3.f}}}}};

        REQUIRE(lib.load_from_json(j));

        const auto* pat = lib.get("hostile");
        REQUIRE(pat != nullptr);
        REQUIRE(pat->emitters[0].count <= 256);
        REQUIRE(pat->emitters[0].fire_rate > 0.f);
        REQUIRE(pat->emitters[0].lifetime > 0.f);
        REQUIRE(pat->emitters[0].hitbox_radius >= 0.f);
    }

    SECTION("Speed, damage and angular velocity are clamped") {
        // Negative speed would move bullets backwards, negative damage would
        // heal targets, and extreme spin rates come from data typos.
        nlohmann::json j = {{"name", "hostile2"},
                            {"emitters",
                             {{{"type", "radial"},
                               {"speed", -50.f},
                               {"damage", 99999.f},
                               {"angular_velocity", 100000.f}}}}};

        REQUIRE(lib.load_from_json(j));

        const auto* pat = lib.get("hostile2");
        REQUIRE(pat != nullptr);
        REQUIRE(pat->emitters[0].speed >= 0.f);
        REQUIRE(pat->emitters[0].damage <= 100.f);
        REQUIRE(pat->emitters[0].angular_velocity <= 1080.f);
        REQUIRE(pat->emitters[0].angular_velocity >= -1080.f);
    }

    SECTION("Pattern with an empty emitters array loads with no emitters") {
        nlohmann::json j = {{"name", "empty"}, {"emitters", nlohmann::json::array()}};

        REQUIRE(lib.load_from_json(j));

        const auto* pat = lib.get("empty");
        REQUIRE(pat != nullptr);
        REQUIRE(pat->emitters.empty());
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

namespace {

/// Write content to a temp file in the working directory (like test_settings).
void write_file(const std::string& path, const std::string& content) {
    std::ofstream f(path);
    f << content;
}

} // namespace

TEST_CASE("PatternLibrary load_file", "[patterns]") {
    StringInterner interner;
    PatternLibrary lib;
    lib.set_interner(interner);

    SECTION("Missing file returns false") {
        REQUIRE_FALSE(lib.load_file("does_not_exist_pattern.json"));
    }

    SECTION("Malformed JSON returns false") {
        const std::string path = "test_pattern_malformed_tmp.json";
        write_file(path, "{ this is not json");

        REQUIRE_FALSE(lib.load_file(path));

        std::remove(path.c_str());
    }

    SECTION("File missing required name field returns false") {
        const std::string path = "test_pattern_noname_tmp.json";
        write_file(path, R"({"emitters": [{"type": "radial"}]})");

        REQUIRE_FALSE(lib.load_file(path));

        std::remove(path.c_str());
    }

    SECTION("Valid file loads and registers the pattern") {
        const std::string path = "test_pattern_valid_tmp.json";
        write_file(path, R"({
            "name": "from_file",
            "emitters": [{"type": "aimed", "count": 2, "speed": 90.0}]
        })");

        REQUIRE(lib.load_file(path));

        const auto* pat = lib.get("from_file");
        REQUIRE(pat != nullptr);
        REQUIRE(pat->emitters.size() == 1);
        REQUIRE(pat->emitters[0].count == 2);
        REQUIRE(pat->emitters[0].speed == Catch::Approx(90.f));

        std::remove(path.c_str());
    }

    SECTION("Loading a file without an interner fails instead of crashing") {
        PatternLibrary no_interner;
        REQUIRE_FALSE(no_interner.load_file("irrelevant.json"));
    }
}

TEST_CASE("PatternLibrary load_manifest", "[patterns]") {
    StringInterner interner;
    PatternLibrary lib;
    lib.set_interner(interner);

    SECTION("Missing manifest returns false") {
        REQUIRE_FALSE(lib.load_manifest("does_not_exist_manifest.json"));
    }

    SECTION("Malformed manifest JSON returns false") {
        const std::string path = "test_manifest_malformed_tmp.json";
        write_file(path, "not json at all");

        REQUIRE_FALSE(lib.load_manifest(path));

        std::remove(path.c_str());
    }

    SECTION("Manifest missing the patterns key returns false") {
        const std::string path = "test_manifest_nokey_tmp.json";
        write_file(path, R"({"wrong_key": []})");

        REQUIRE_FALSE(lib.load_manifest(path));

        std::remove(path.c_str());
    }

    SECTION("Manifest with an empty patterns array returns false") {
        const std::string path = "test_manifest_empty_tmp.json";
        write_file(path, R"({"patterns": []})");

        REQUIRE_FALSE(lib.load_manifest(path));

        std::remove(path.c_str());
    }

    SECTION("Manifest whose entries all fail to load returns false") {
        const std::string path = "test_manifest_badentries_tmp.json";
        write_file(path, R"({"patterns": ["no_such_pattern_file.json"]})");

        REQUIRE_FALSE(lib.load_manifest(path));

        std::remove(path.c_str());
    }
}
