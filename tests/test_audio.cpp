#include "audio/audio_engine.hpp"
#include "ecs/components.hpp"

#include <SDL3/SDL.h>
#include <entt/entt.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace raven;

TEST_CASE("AudioEngine is safe when uninitialised", "[audio]") {
    AudioEngine engine;

    REQUIRE_FALSE(engine.is_ready());
    REQUIRE(engine.active_streams() == 0);

    // All calls must no-op, not crash
    REQUIRE_FALSE(engine.load_sound("shoot", "nonexistent.wav"));
    engine.play("shoot");
    engine.update();
    engine.set_master_gain(0.5f);
    engine.shutdown();
}

TEST_CASE("AudioEngine with dummy driver", "[audio]") {
    // Force SDL's dummy audio backend so this runs on CI without a device
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");

    AudioEngine engine;
    if (!engine.init()) {
        // Environment without even a dummy backend: nothing to test
        SUCCEED("audio unavailable in this environment");
        return;
    }

    REQUIRE(engine.is_ready());

    SECTION("Playing an unknown sound is a warning, not a crash") {
        engine.play("does_not_exist");
        REQUIRE(engine.active_streams() == 0);
    }

    SECTION("Missing WAV file fails cleanly") {
        REQUIRE_FALSE(engine.load_sound("ghost", "no/such/file.wav"));
    }

    engine.shutdown();
    REQUIRE_FALSE(engine.is_ready());
}

TEST_CASE("Sfx sound names are stable and unique", "[audio]") {
    // These strings are the contract with config.json's "sounds" section
    REQUIRE(std::string(sfx_sound_name(Sfx::Shoot)) == "shoot");
    REQUIRE(std::string(sfx_sound_name(Sfx::PlayerHit)) == "player_hit");
    REQUIRE(std::string(sfx_sound_name(Sfx::EnemyHit)) == "enemy_hit");
    REQUIRE(std::string(sfx_sound_name(Sfx::EnemyDown)) == "enemy_down");
    REQUIRE(std::string(sfx_sound_name(Sfx::Pickup)) == "pickup");
    REQUIRE(std::string(sfx_sound_name(Sfx::Dash)) == "dash");
    REQUIRE(std::string(sfx_sound_name(Sfx::Melee)) == "melee");
}

TEST_CASE("push_sfx queues events only when an AudioQueue exists", "[audio]") {
    entt::registry reg;

    // No queue: silently ignored (unit tests and headless paths)
    push_sfx(reg, Sfx::Shoot);

    auto& queue = reg.ctx().emplace<AudioQueue>();
    push_sfx(reg, Sfx::Shoot);
    push_sfx(reg, Sfx::EnemyDown);

    REQUIRE(queue.events.size() == 2);
    REQUIRE(queue.events[0] == Sfx::Shoot);
    REQUIRE(queue.events[1] == Sfx::EnemyDown);
}
