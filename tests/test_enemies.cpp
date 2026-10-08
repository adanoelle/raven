#include "core/string_id.hpp"
#include "ecs/components.hpp"
#include "ecs/enemy_library.hpp"
#include "ecs/systems/damage_system.hpp"
#include "patterns/pattern_library.hpp"

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <random>

using namespace raven;
using Catch::Approx;

namespace {

/// @brief Load one enemy definition, returning the library.
EnemyLibrary load_one(const nlohmann::json& def) {
    EnemyLibrary enemies;
    REQUIRE(enemies.load_json({{"enemies", {{"test", def}}}}));
    return enemies;
}

/// @brief The smallest valid definition.
nlohmann::json minimal(const char* tier = "grunt", const char* archetype = "chaser") {
    return {{"tier", tier}, {"sheet", "enemies"}, {"ai", {{"archetype", archetype}}}};
}

} // namespace

TEST_CASE("A minimal enemy definition gets its defaults", "[enemies]") {
    const auto enemies = load_one(minimal("mid", "stalker"));
    const EnemyDef* def = enemies.get("test");
    REQUIRE(def != nullptr);

    CHECK(def->id == "test");
    CHECK(def->tier == Enemy::Type::Mid);
    CHECK(def->sheet == "enemies");
    CHECK(def->width == 0); // drawn at the sheet's frame size
    CHECK(def->height == 0);
    CHECK(def->pattern.empty()); // doesn't shoot
    CHECK(def->contact_damage == 0.f);
    CHECK(def->stabilizer_drop == Approx(default_stabilizer_drop(Enemy::Type::Mid)));

    // AI tuning comes from the archetype
    const AiBehavior stalker = default_ai(AiBehavior::Archetype::Stalker);
    CHECK(def->ai.archetype == AiBehavior::Archetype::Stalker);
    CHECK(def->ai.phase == AiBehavior::Phase::Idle);
    CHECK(def->ai.move_speed == Approx(stalker.move_speed));
    CHECK(def->ai.preferred_range == Approx(stalker.preferred_range));
}

TEST_CASE("Enemy definitions override every default", "[enemies]") {
    const auto enemies = load_one({{"tier", "boss"},
                                   {"sheet", "enemies_boss"},
                                   {"size", {40, 44}},
                                   {"offset_y", -6},
                                   {"hitbox", {{"radius", 18}, {"rect", {32, 36}}}},
                                   {"hp", 10},
                                   {"score", 1000},
                                   {"ai",
                                    {{"archetype", "coward"},
                                     {"move_speed", 120},
                                     {"activation_range", 150},
                                     {"preferred_range", 5},
                                     {"attack_range", 300}}},
                                   {"pattern", "nova_legendary"},
                                   {"contact_damage", 25},
                                   {"stabilizer_drop", 0.5}});
    const EnemyDef* def = enemies.get("test");
    REQUIRE(def != nullptr);

    CHECK(def->width == 40);
    CHECK(def->height == 44);
    CHECK(def->offset_y == Approx(-6.f));
    CHECK(def->radius == Approx(18.f));
    CHECK(def->rect_w == Approx(32.f));
    CHECK(def->rect_h == Approx(36.f));
    CHECK(def->hp == Approx(10.f));
    CHECK(def->score == 1000);
    CHECK(def->ai.move_speed == Approx(120.f));
    CHECK(def->ai.activation_range == Approx(150.f));
    CHECK(def->ai.preferred_range == Approx(5.f));
    CHECK(def->ai.attack_range == Approx(300.f));
    CHECK(def->pattern == "nova_legendary");
    CHECK(def->contact_damage == Approx(25.f));
    CHECK(def->stabilizer_drop == Approx(0.5f));
}

TEST_CASE("Invalid enemy definitions are skipped", "[enemies]") {
    auto with = [](const char* key, const nlohmann::json& value) {
        auto def = minimal();
        def[key] = value;
        return def;
    };
    auto without = [](const char* key) {
        auto def = minimal();
        def.erase(key);
        return def;
    };

    SECTION("missing tier, sheet or ai") {
        CHECK(load_one(without("tier")).count() == 0);
        CHECK(load_one(without("sheet")).count() == 0);
        CHECK(load_one(without("ai")).count() == 0);
    }

    SECTION("unknown tier or archetype") {
        CHECK(load_one(with("tier", "Boss")).count() == 0);
        CHECK(load_one(with("ai", {{"archetype", "stalkr"}})).count() == 0);
    }

    SECTION("values out of range") {
        CHECK(load_one(with("hp", 0)).count() == 0);
        CHECK(load_one(with("score", -5)).count() == 0);
        CHECK(load_one(with("size", {24})).count() == 0);
        CHECK(load_one(with("hitbox", {{"radius", -1}})).count() == 0);
        CHECK(load_one(with("stabilizer_drop", 1.5)).count() == 0);
        CHECK(load_one(with("ai", {{"archetype", "chaser"}, {"move_speed", -10}})).count() == 0);
    }

    SECTION("wrong types") {
        // The old stage format used true/false; the definition wants a damage value
        CHECK(load_one(with("contact_damage", true)).count() == 0);
        CHECK(load_one(with("hp", "lots")).count() == 0);
    }

    SECTION("an unknown key is only a warning") {
        CHECK(load_one(with("hitpoints", 5)).count() == 1);
    }
}

TEST_CASE("One bad enemy definition doesn't stop the others loading", "[enemies]") {
    EnemyLibrary enemies;
    REQUIRE(enemies.load_json({{"enemies", {{"good", minimal()}, {"bad", {{"tier", "huge"}}}}}}));
    CHECK(enemies.count() == 1);
    CHECK(enemies.get("good") != nullptr);
    CHECK(enemies.get("bad") == nullptr);
}

TEST_CASE("Enemy data with the wrong shape is rejected", "[enemies]") {
    EnemyLibrary enemies;
    CHECK_FALSE(enemies.load_json(nlohmann::json::array()));
    CHECK_FALSE(enemies.load_json({{"enemies", nlohmann::json::array()}}));
    CHECK_FALSE(enemies.load_json({{"grunt", minimal()}}));
}

TEST_CASE("A StabilizerDrop chance overrides the tier default", "[enemies][stabilizer]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    reg.ctx().emplace<std::mt19937>(42u);
    PatternLibrary patterns;

    auto kill = [&](Enemy::Type tier, float chance) {
        auto e = reg.create();
        reg.emplace<Transform2D>(e, 100.f, 100.f);
        reg.emplace<Enemy>(e, tier);
        reg.emplace<StabilizerDrop>(e, chance);
        reg.emplace<Health>(e, 0.f, 1.f);
        systems::update_damage(reg, patterns, 0.f);
    };

    // A grunt that is set to always drop one
    kill(Enemy::Type::Grunt, 1.f);
    CHECK(reg.view<StabilizerPickup>().size() == 1);

    // A boss that is set never to drop one
    kill(Enemy::Type::Boss, 0.f);
    CHECK(reg.view<StabilizerPickup>().size() == 1);
}
