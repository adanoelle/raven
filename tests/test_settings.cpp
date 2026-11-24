#include "core/settings.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <fstream>
#include <string>

using namespace raven;

TEST_CASE("Settings JSON round-trip", "[settings]") {
    SECTION("Defaults survive serialize/deserialize") {
        Settings defaults;
        Settings loaded = Settings::from_json(defaults.to_json());

        REQUIRE(loaded.window_scale == defaults.window_scale);
        REQUIRE(loaded.fullscreen == defaults.fullscreen);
        REQUIRE(loaded.vsync == defaults.vsync);
        REQUIRE(loaded.music_volume == defaults.music_volume);
        REQUIRE(loaded.sfx_volume == defaults.sfx_volume);
    }

    SECTION("Non-default values round-trip") {
        Settings s;
        s.window_scale = 4;
        s.fullscreen = true;
        s.vsync = false;
        s.music_volume = 25;
        s.sfx_volume = 0;

        Settings loaded = Settings::from_json(s.to_json());

        REQUIRE(loaded.window_scale == 4);
        REQUIRE(loaded.fullscreen == true);
        REQUIRE(loaded.vsync == false);
        REQUIRE(loaded.music_volume == 25);
        REQUIRE(loaded.sfx_volume == 0);
    }
}

TEST_CASE("Settings validation", "[settings]") {
    SECTION("Missing fields use defaults") {
        Settings loaded = Settings::from_json(nlohmann::json::object());
        Settings defaults;

        REQUIRE(loaded.window_scale == defaults.window_scale);
        REQUIRE(loaded.vsync == defaults.vsync);
        REQUIRE(loaded.music_volume == defaults.music_volume);
    }

    SECTION("Out-of-range values are clamped") {
        nlohmann::json j = {
            {"window_scale", 100},
            {"music_volume", -5},
            {"sfx_volume", 999},
        };

        Settings loaded = Settings::from_json(j);

        REQUIRE(loaded.window_scale == 8);
        REQUIRE(loaded.music_volume == 0);
        REQUIRE(loaded.sfx_volume == 100);
    }
}

TEST_CASE("Only the settings that changed are re-applied", "[settings]") {
    Settings base;
    base.window_scale = 3;

    SECTION("nothing changed") {
        const auto c = settings_changes(base, base);
        CHECK_FALSE((c.fullscreen || c.window_size || c.vsync || c.sfx_volume));
    }

    SECTION("a volume change leaves the window and vsync alone") {
        Settings to = base;
        to.sfx_volume = 40;
        to.music_volume = 10;
        const auto c = settings_changes(base, to);
        CHECK(c.sfx_volume);
        CHECK_FALSE((c.fullscreen || c.window_size || c.vsync));
    }

    SECTION("window scale") {
        Settings to = base;
        to.window_scale = 4;
        const auto c = settings_changes(base, to);
        CHECK(c.window_size);
        CHECK_FALSE((c.fullscreen || c.vsync || c.sfx_volume));
    }

    SECTION("the scale is not applied while fullscreen") {
        Settings from = base;
        from.fullscreen = true;
        Settings to = from;
        to.window_scale = 5;
        CHECK_FALSE(settings_changes(from, to).window_size);
    }

    SECTION("leaving fullscreen restores the windowed size") {
        Settings from = base;
        from.fullscreen = true;
        const auto c = settings_changes(from, base);
        CHECK(c.fullscreen);
        CHECK(c.window_size);
    }

    SECTION("vsync") {
        Settings to = base;
        to.vsync = !base.vsync;
        const auto c = settings_changes(base, to);
        CHECK(c.vsync);
        CHECK_FALSE((c.fullscreen || c.window_size || c.sfx_volume));
    }
}

TEST_CASE("Settings file persistence", "[settings]") {
    const std::string path = "test_settings_tmp.json";

    SECTION("Missing file yields defaults") {
        Settings loaded = Settings::load("nonexistent_dir/settings.json");
        REQUIRE(loaded.window_scale == Settings{}.window_scale);
    }

    SECTION("Malformed file yields defaults") {
        {
            std::ofstream f(path);
            f << "{not valid json";
        }
        Settings loaded = Settings::load(path);
        REQUIRE(loaded.vsync == Settings{}.vsync);
        std::remove(path.c_str());
    }

    SECTION("Save then load preserves values") {
        Settings s;
        s.window_scale = 3;
        s.fullscreen = true;
        REQUIRE(s.save(path));

        Settings loaded = Settings::load(path);
        REQUIRE(loaded.window_scale == 3);
        REQUIRE(loaded.fullscreen == true);
        std::remove(path.c_str());
    }

    SECTION("Save to empty path fails cleanly") {
        REQUIRE_FALSE(Settings{}.save(""));
    }
}
