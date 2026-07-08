#include "core/save_data.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <fstream>
#include <string>

using namespace raven;

TEST_CASE("SaveData JSON round-trip", "[save]") {
    SECTION("Defaults survive serialize/deserialize") {
        SaveData d;
        SaveData loaded = SaveData::from_json(d.to_json());
        REQUIRE(loaded.best_score == 0);
    }

    SECTION("Best score round-trips") {
        SaveData d;
        d.best_score = 123450;
        SaveData loaded = SaveData::from_json(d.to_json());
        REQUIRE(loaded.best_score == 123450);
    }
}

TEST_CASE("SaveData validation", "[save]") {
    SECTION("Missing fields use defaults") {
        SaveData loaded = SaveData::from_json(nlohmann::json::object());
        REQUIRE(loaded.best_score == 0);
    }

    SECTION("Negative scores are clamped to zero") {
        nlohmann::json j = {{"best_score", -500}};
        REQUIRE(SaveData::from_json(j).best_score == 0);
    }
}

TEST_CASE("SaveData file persistence", "[save]") {
    const std::string path = "test_save_tmp.json";

    SECTION("Missing file yields defaults") {
        SaveData loaded = SaveData::load("nonexistent_dir/save.json");
        REQUIRE(loaded.best_score == 0);
    }

    SECTION("Malformed file yields defaults") {
        {
            std::ofstream f(path);
            f << "not json at all";
        }
        REQUIRE(SaveData::load(path).best_score == 0);
        std::remove(path.c_str());
    }

    SECTION("Save then load preserves the best score") {
        SaveData d;
        d.best_score = 9001;
        REQUIRE(d.save(path));

        REQUIRE(SaveData::load(path).best_score == 9001);
        std::remove(path.c_str());
    }

    SECTION("Save to empty path fails cleanly") {
        REQUIRE_FALSE(SaveData{}.save(""));
    }
}
