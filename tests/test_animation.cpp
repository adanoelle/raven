#include "core/string_id.hpp"
#include "ecs/components.hpp"
#include "ecs/systems/animation_system.hpp"
#include "rendering/animation_library.hpp"

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

using namespace raven;
using Catch::Approx;

namespace {

/// @brief One frame tag in a fake Aseprite export: a row of 16x16 frames.
struct TagSpec {
    std::string name;
    int row = 0;
    int count = 1;
    int ms = 100;
    std::string direction = "forward";
    int repeat = 0;
};

/// @brief Build JSON shaped like `aseprite --sheet-type rows --split-tags
/// --list-tags --format json-array --data`.
nlohmann::json make_export(const std::vector<TagSpec>& tags) {
    nlohmann::json frames = nlohmann::json::array();
    nlohmann::json frame_tags = nlohmann::json::array();
    for (const auto& t : tags) {
        const int from = static_cast<int>(frames.size());
        for (int col = 0; col < t.count; ++col) {
            frames.push_back({{"frame", {{"x", col * 16}, {"y", t.row * 16}, {"w", 16}, {"h", 16}}},
                              {"duration", t.ms}});
        }
        nlohmann::json tag = {{"name", t.name},
                              {"from", from},
                              {"to", static_cast<int>(frames.size()) - 1},
                              {"direction", t.direction}};
        if (t.repeat > 0) {
            tag["repeat"] = std::to_string(t.repeat); // Aseprite writes it as a string
        }
        frame_tags.push_back(tag);
    }
    return {{"frames", frames}, {"meta", {{"frameTags", frame_tags}}}};
}

/// @brief Registry with an interner and an AnimationLibrary holding one sheet's clips.
struct Fixture {
    entt::registry reg;
    StringId sheet;

    explicit Fixture(const std::vector<TagSpec>& tags) {
        reg.ctx().emplace<StringInterner>();
        reg.ctx().emplace<AnimationLibrary>();
        sheet = interner().intern("hero");
        REQUIRE(library().load_json(sheet, make_export(tags), 16, 16, interner()));
    }

    StringInterner& interner() { return reg.ctx().get<StringInterner>(); }
    AnimationLibrary& library() { return reg.ctx().get<AnimationLibrary>(); }
    StringId id(const char* name) { return interner().intern(name); }
    const AnimationClip* clip(const char* name) { return library().get(sheet, id(name)); }

    entt::entity spawn(const char* clip_name) {
        auto e = reg.create();
        reg.emplace<Sprite>(e, sheet, 0, 0, 16, 16, 0);
        reg.emplace<Animation>(e, Animation{id(clip_name)});
        return e;
    }

    entt::entity spawn_player() {
        auto e = spawn(clips::IDLE);
        reg.emplace<Player>(e);
        reg.emplace<Velocity>(e);
        reg.emplace<AimDirection>(e);
        return e;
    }
};

/// @brief Frame columns of a clip in play order.
std::vector<int> columns(const AnimationClip* clip) {
    std::vector<int> cols;
    for (const auto& frame : clip->frames) {
        cols.push_back(frame.frame_x);
    }
    return cols;
}

} // namespace

// ── AnimationLibrary ───────────────────────────────────────────────

TEST_CASE("Aseprite tags load as clips of grid cells", "[animation]") {
    Fixture f({{"idle", 0, 4, 250}, {"walk", 1, 6, 100}, {"attack", 2, 3, 50, "forward", 1}});

    const auto* idle = f.clip("idle");
    REQUIRE(idle != nullptr);
    CHECK(idle->frames.size() == 4);
    CHECK(idle->frames[3].frame_x == 3);
    CHECK(idle->frames[3].frame_y == 0);
    CHECK(idle->frames[0].duration == Approx(0.25f));
    CHECK(idle->repeat == 0);

    const auto* walk = f.clip("walk");
    REQUIRE(walk != nullptr);
    CHECK(walk->frames.size() == 6);
    CHECK(walk->frames[0].frame_y == 1);

    const auto* attack = f.clip("attack");
    REQUIRE(attack != nullptr);
    CHECK(attack->repeat == 1);
    CHECK(attack->frames[0].duration == Approx(0.05f));

    CHECK(f.clip("dash") == nullptr);
    CHECK(f.library().has_sheet(f.sheet));
    CHECK_FALSE(f.library().has_sheet(f.id("other")));
}

TEST_CASE("Tag directions set the play order", "[animation]") {
    Fixture f({{"rev", 0, 3, 100, "reverse"},
               {"pingpong", 1, 3, 100, "pingpong"},
               {"pingpong_once", 2, 3, 100, "pingpong", 1},
               {"pingpong_rev", 3, 3, 100, "pingpong_reverse"}});

    CHECK(columns(f.clip("rev")) == std::vector<int>{2, 1, 0});
    // Looping: back again, but not onto the frame the loop returns to
    CHECK(columns(f.clip("pingpong")) == std::vector<int>{0, 1, 2, 1});
    // One-shot: ends where it started
    CHECK(columns(f.clip("pingpong_once")) == std::vector<int>{0, 1, 2, 1, 0});
    CHECK(columns(f.clip("pingpong_rev")) == std::vector<int>{2, 1, 0, 1});
}

TEST_CASE("Malformed animation data is rejected or skipped", "[animation]") {
    StringInterner interner;
    AnimationLibrary library;
    const StringId sheet = interner.intern("hero");

    SECTION("json-hash exports are rejected") {
        nlohmann::json j = {{"frames", {{"hero 0", {{"frame", {{"x", 0}}}}}}}, {"meta", {}}};
        CHECK_FALSE(library.load_json(sheet, j, 16, 16, interner));
    }

    SECTION("tags using frames off the grid are skipped") {
        auto j = make_export({{"good", 0, 2}, {"padded", 1, 2}});
        j["frames"][2]["frame"]["x"] = 1; // a border/padding offset
        REQUIRE(library.load_json(sheet, j, 16, 16, interner));
        CHECK(library.get(sheet, interner.intern("good")) != nullptr);
        CHECK(library.get(sheet, interner.intern("padded")) == nullptr);
    }

    SECTION("zero-length frames, bad ranges and unknown directions are skipped") {
        auto j = make_export({{"zero", 0, 2, 0}, {"range", 1, 2}, {"sideways", 2, 2, 100, "up"}});
        j["meta"]["frameTags"][1]["to"] = 99;
        REQUIRE(library.load_json(sheet, j, 16, 16, interner));
        CHECK(library.get(sheet, interner.intern("zero")) == nullptr);
        CHECK(library.get(sheet, interner.intern("range")) == nullptr);
        CHECK(library.get(sheet, interner.intern("sideways")) == nullptr);
        CHECK_FALSE(library.has_sheet(sheet));
    }
}

// ── update_animation ───────────────────────────────────────────────

TEST_CASE("update_animation loops a clip and writes its cells into the sprite", "[animation]") {
    Fixture f({{"walk", 1, 3, 100}});
    auto e = f.spawn("walk");
    const auto& sprite = f.reg.get<Sprite>(e);

    systems::update_animation(f.reg, 0.f);
    CHECK(sprite.frame_x == 0);
    CHECK(sprite.frame_y == 1);

    systems::update_animation(f.reg, 0.1f);
    CHECK(sprite.frame_x == 1);

    systems::update_animation(f.reg, 0.2f); // two frames in one step: wraps to 0
    CHECK(sprite.frame_x == 0);
    CHECK_FALSE(f.reg.get<Animation>(e).finished);
}

TEST_CASE("A one-shot clip shows its last frame in full, then holds it", "[animation]") {
    Fixture f({{"attack", 2, 3, 100, "forward", 1}});
    auto e = f.spawn("attack");
    const auto& anim = f.reg.get<Animation>(e);
    const auto& sprite = f.reg.get<Sprite>(e);

    systems::update_animation(f.reg, 0.1f);
    systems::update_animation(f.reg, 0.1f);
    CHECK(anim.frame == 2); // last frame just appeared
    CHECK_FALSE(anim.finished);

    systems::update_animation(f.reg, 0.05f);
    CHECK_FALSE(anim.finished); // still on screen for the rest of its duration

    systems::update_animation(f.reg, 0.05f);
    CHECK(anim.finished);

    systems::update_animation(f.reg, 1.f);
    CHECK(anim.frame == 2);
    CHECK(sprite.frame_x == 2);
    CHECK(sprite.frame_y == 2);
}

TEST_CASE("A repeat count plays the clip that many times", "[animation]") {
    Fixture f({{"blink", 0, 2, 100, "forward", 2}});
    auto e = f.spawn("blink");
    const auto& anim = f.reg.get<Animation>(e);

    for (int i = 0; i < 3; ++i) {
        systems::update_animation(f.reg, 0.1f);
        CHECK_FALSE(anim.finished);
    }
    systems::update_animation(f.reg, 0.1f);
    CHECK(anim.finished);
    CHECK(anim.frame == 1);
}

TEST_CASE("Sprites without a matching clip keep their frame", "[animation]") {
    Fixture f({{"idle", 0, 4, 100}});

    SECTION("sheet without animation data") {
        auto e = f.reg.create();
        f.reg.emplace<Sprite>(e, f.id("plain"), 2, 3, 16, 16, 0);
        f.reg.emplace<Animation>(e, Animation{f.id("idle")});
        systems::update_animation(f.reg, 1.f);
        CHECK(f.reg.get<Sprite>(e).frame_x == 2);
        CHECK(f.reg.get<Sprite>(e).frame_y == 3);
    }

    SECTION("clip the sheet doesn't have") {
        auto e = f.spawn("attack");
        f.reg.get<Sprite>(e).frame_x = 5;
        systems::update_animation(f.reg, 1.f);
        CHECK(f.reg.get<Sprite>(e).frame_x == 5);
    }

    SECTION("no animation library") {
        entt::registry reg;
        auto e = reg.create();
        reg.emplace<Sprite>(e, StringId{1}, 2, 0, 16, 16, 0);
        reg.emplace<Animation>(e);
        systems::update_animation(reg, 1.f);
        CHECK(reg.get<Sprite>(e).frame_x == 2);
    }
}

TEST_CASE("play_clip restarts only when the clip changes", "[animation]") {
    Fixture f({{"idle", 0, 4, 100}, {"walk", 1, 4, 100}});
    auto e = f.spawn("idle");
    auto& anim = f.reg.get<Animation>(e);

    systems::update_animation(f.reg, 0.25f);
    REQUIRE(anim.frame == 2);

    systems::play_clip(anim, f.id("idle"));
    CHECK(anim.frame == 2);

    systems::play_clip(anim, f.id("walk"));
    CHECK(anim.frame == 0);
    CHECK(anim.elapsed == 0.f);
    CHECK(anim.clip == f.id("walk"));
}

// ── update_player_animation ────────────────────────────────────────

TEST_CASE("Player animation follows what the player is doing", "[animation]") {
    Fixture f({{"idle", 0, 4, 250},
               {"walk", 1, 6, 100},
               {"attack", 2, 3, 50, "forward", 1},
               {"dash", 3, 3, 40, "forward", 1}});
    auto e = f.spawn_player();
    auto& anim = f.reg.get<Animation>(e);
    auto& vel = f.reg.get<Velocity>(e);

    systems::update_player_animation(f.reg);
    CHECK(anim.clip == f.id("idle"));

    vel.dx = 50.f;
    systems::update_player_animation(f.reg);
    CHECK(anim.clip == f.id("walk"));

    f.reg.emplace<MeleeAttack>(e);
    systems::update_player_animation(f.reg);
    CHECK(anim.clip == f.id("attack"));

    SECTION("the attack clip plays to its end after the attack itself ends") {
        f.reg.remove<MeleeAttack>(e);
        vel.dx = 0.f;
        systems::update_animation(f.reg, 0.05f);
        systems::update_player_animation(f.reg);
        CHECK(anim.clip == f.id("attack"));

        // Remaining two frames, then idle takes over
        systems::update_animation(f.reg, 0.1f);
        REQUIRE(anim.finished);
        systems::update_player_animation(f.reg);
        CHECK(anim.clip == f.id("idle"));
    }

    SECTION("a new action interrupts a playing one") {
        f.reg.remove<MeleeAttack>(e);
        f.reg.emplace<Dash>(e);
        systems::update_player_animation(f.reg);
        CHECK(anim.clip == f.id("dash"));
    }

    SECTION("ground slam uses the attack clip") {
        f.reg.remove<MeleeAttack>(e);
        systems::play_clip(anim, f.id("idle"));
        f.reg.emplace<GroundSlam>(e);
        systems::update_player_animation(f.reg);
        CHECK(anim.clip == f.id("attack"));
    }
}

TEST_CASE("Player animation falls back when the sheet lacks a clip", "[animation]") {
    Fixture f({{"idle", 0, 4, 250}, {"walk", 1, 6, 100}}); // no action tags yet
    auto e = f.spawn_player();
    auto& anim = f.reg.get<Animation>(e);

    f.reg.emplace<Dash>(e);
    systems::update_player_animation(f.reg);
    CHECK(anim.clip == f.id("walk"));

    f.reg.remove<Dash>(e);
    f.reg.emplace<MeleeAttack>(e);
    systems::update_player_animation(f.reg);
    CHECK(anim.clip == f.id("walk"));
}

TEST_CASE("Player sprite faces the aim direction", "[animation]") {
    Fixture f({{"idle", 0, 4, 250}});
    auto e = f.spawn_player();
    auto& aim = f.reg.get<AimDirection>(e);
    const auto& sprite = f.reg.get<Sprite>(e);

    aim.x = -1.f;
    systems::update_player_animation(f.reg);
    CHECK(sprite.flip_x);

    aim.x = 0.f; // straight up or down keeps the last facing
    systems::update_player_animation(f.reg);
    CHECK(sprite.flip_x);

    aim.x = 1.f;
    systems::update_player_animation(f.reg);
    CHECK_FALSE(sprite.flip_x);
}
