#include "ecs/components.hpp"
#include "ecs/systems/damage_system.hpp"
#include "ecs/systems/pickup_system.hpp"
#include "patterns/pattern_library.hpp"

#include <entt/entt.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <random>

using namespace raven;

namespace {

/// @brief Create a player entity with weapon and hitbox for pickup testing.
entt::entity make_player(entt::registry& reg, float x, float y) {
    auto player = reg.create();
    reg.emplace<Transform2D>(player, x, y);
    reg.emplace<CircleHitbox>(player, 6.f, 0.f, 0.f);
    reg.emplace<Player>(player);
    reg.emplace<Weapon>(player);
    reg.emplace<Health>(player, 1.f, 1.f);
    return player;
}

/// @brief Create a weapon pickup entity at a given position.
entt::entity make_pickup(entt::registry& reg, float x, float y, Weapon weapon) {
    auto pickup = reg.create();
    reg.emplace<Transform2D>(pickup, x, y);
    reg.emplace<CircleHitbox>(pickup, 8.f, 0.f, 0.f);
    reg.emplace<WeaponPickup>(pickup, WeaponPickup{std::move(weapon)});
    reg.emplace<Lifetime>(pickup, 5.f);
    return pickup;
}

} // namespace

TEST_CASE("Weapon pickup collection", "[pickups]") {
    entt::registry reg;

    SECTION("Player collects weapon pickup") {
        auto player = make_player(reg, 100.f, 100.f);
        Weapon stolen;
        stolen.bullet_speed = 500.f;
        stolen.bullet_damage = 5.f;
        auto pickup = make_pickup(reg, 103.f, 100.f, stolen); // overlaps player

        systems::update_pickups(reg);

        // Pickup should be destroyed
        REQUIRE_FALSE(reg.valid(pickup));

        // Player weapon should match the pickup's weapon
        auto& weapon = reg.get<Weapon>(player);
        REQUIRE(weapon.bullet_speed == Catch::Approx(500.f));
        REQUIRE(weapon.bullet_damage == Catch::Approx(5.f));

        // Player should have WeaponDecay
        auto* decay = reg.try_get<WeaponDecay>(player);
        REQUIRE(decay != nullptr);
        REQUIRE(decay->remaining == Catch::Approx(10.f));

        // Player should have DefaultWeapon (original)
        auto* def = reg.try_get<DefaultWeapon>(player);
        REQUIRE(def != nullptr);
        REQUIRE(def->weapon.bullet_speed == Catch::Approx(300.f)); // default
    }

    SECTION("Separated pickup is not collected") {
        auto player = make_player(reg, 100.f, 100.f);
        Weapon stolen;
        stolen.bullet_speed = 500.f;
        auto pickup = make_pickup(reg, 200.f, 200.f, stolen); // far away

        systems::update_pickups(reg);

        // Pickup should still exist
        REQUIRE(reg.valid(pickup));

        // Player weapon unchanged
        auto& weapon = reg.get<Weapon>(player);
        REQUIRE(weapon.bullet_speed == Catch::Approx(300.f));
    }
}

TEST_CASE("Weapon decay and revert", "[pickups]") {
    entt::registry reg;
    constexpr float dt = 1.f / 120.f;

    SECTION("Weapon reverts after decay expires") {
        auto player = make_player(reg, 100.f, 100.f);
        Weapon stolen;
        stolen.bullet_speed = 500.f;
        auto pickup = make_pickup(reg, 103.f, 100.f, stolen);

        systems::update_pickups(reg);
        REQUIRE_FALSE(reg.valid(pickup));

        // Simulate decay expiring
        auto& decay = reg.get<WeaponDecay>(player);
        decay.remaining = dt; // will expire on next tick

        systems::update_weapon_decay(reg, dt);

        // Weapon should be reverted to default
        auto& weapon = reg.get<Weapon>(player);
        REQUIRE(weapon.bullet_speed == Catch::Approx(300.f));

        // WeaponDecay and DefaultWeapon should be removed
        REQUIRE_FALSE(reg.any_of<WeaponDecay>(player));
        REQUIRE_FALSE(reg.any_of<DefaultWeapon>(player));
    }

    SECTION("Decay timer ticks down") {
        auto player = make_player(reg, 100.f, 100.f);
        reg.emplace<WeaponDecay>(player, 10.f);

        systems::update_weapon_decay(reg, dt);

        auto& decay = reg.get<WeaponDecay>(player);
        REQUIRE(decay.remaining == Catch::Approx(10.f - dt));
    }

    SECTION("Collecting second pickup while decaying does not overwrite DefaultWeapon") {
        auto player = make_player(reg, 100.f, 100.f);

        // First pickup
        Weapon stolen1;
        stolen1.bullet_speed = 500.f;
        auto p1 = make_pickup(reg, 103.f, 100.f, stolen1);
        systems::update_pickups(reg);
        REQUIRE_FALSE(reg.valid(p1));

        auto* def = reg.try_get<DefaultWeapon>(player);
        REQUIRE(def != nullptr);
        float original_speed = def->weapon.bullet_speed;

        // Second pickup
        Weapon stolen2;
        stolen2.bullet_speed = 800.f;
        auto p2 = make_pickup(reg, 103.f, 100.f, stolen2);
        systems::update_pickups(reg);
        REQUIRE_FALSE(reg.valid(p2));

        // Player should have the second stolen weapon
        auto& weapon = reg.get<Weapon>(player);
        REQUIRE(weapon.bullet_speed == Catch::Approx(800.f));

        // DefaultWeapon should still be the original
        def = reg.try_get<DefaultWeapon>(player);
        REQUIRE(def != nullptr);
        REQUIRE(def->weapon.bullet_speed == Catch::Approx(original_speed));
    }
}

TEST_CASE("weapon_from_emitter conversion", "[pickups]") {
    EmitterDef emitter;
    emitter.speed = 200.f;
    emitter.damage = 2.f;
    emitter.lifetime = 4.f;
    emitter.hitbox_radius = 5.f;
    emitter.fire_rate = 0.5f;
    emitter.count = 3;
    emitter.spread_angle = 45.f;
    emitter.bullet_sheet = "projectiles";
    emitter.bullet_frame_x = 1;
    emitter.bullet_frame_y = 1;

    auto weapon = systems::weapon_from_emitter(emitter);

    REQUIRE(weapon.bullet_speed == Catch::Approx(200.f));
    REQUIRE(weapon.bullet_damage == Catch::Approx(2.f));
    REQUIRE(weapon.bullet_lifetime == Catch::Approx(4.f));
    REQUIRE(weapon.bullet_hitbox == Catch::Approx(5.f));
    REQUIRE(weapon.fire_rate == Catch::Approx(0.5f));
    REQUIRE(weapon.bullet_count == 3);
    REQUIRE(weapon.spread_angle == Catch::Approx(45.f));
    REQUIRE(weapon.bullet_sheet == "projectiles");
    REQUIRE(weapon.bullet_frame_x == 1);
    REQUIRE(weapon.bullet_frame_y == 1);
}

TEST_CASE("Enemy death spawns weapon pickup", "[pickups]") {
    entt::registry reg;
    PatternLibrary patterns;

    // Load a test pattern
    nlohmann::json j = {{"name", "test_pattern"},
                        {"emitters",
                         {{{"type", "radial"},
                           {"count", 5},
                           {"speed", 200.f},
                           {"fire_rate", 0.3f},
                           {"spread_angle", 60.f}}}}};
    patterns.load_from_json(j);

    // Create an enemy with a bullet emitter that's already dead
    auto enemy = reg.create();
    reg.emplace<Transform2D>(enemy, 50.f, 75.f);
    reg.emplace<CircleHitbox>(enemy, 6.f);
    reg.emplace<Enemy>(enemy);
    reg.emplace<Health>(enemy, 0.f, 3.f); // already dead
    BulletEmitter emitter;
    emitter.pattern_name = "test_pattern";
    reg.emplace<BulletEmitter>(enemy, std::move(emitter));

    systems::update_damage(reg, patterns);

    // Enemy should be destroyed
    REQUIRE_FALSE(reg.valid(enemy));

    // A WeaponPickup entity should exist at the enemy's death position
    auto pickup_view = reg.view<WeaponPickup, Transform2D, Lifetime>();
    int pickup_count = 0;
    for (auto [ent, wp, tf, life] : pickup_view.each()) {
        ++pickup_count;
        REQUIRE(tf.x == Catch::Approx(50.f));
        REQUIRE(tf.y == Catch::Approx(75.f));
        REQUIRE(wp.weapon.bullet_speed == Catch::Approx(200.f));
        REQUIRE(wp.weapon.bullet_count == 5);
        REQUIRE(wp.weapon.spread_angle == Catch::Approx(60.f));
        REQUIRE(life.remaining == Catch::Approx(5.f));
    }
    REQUIRE(pickup_count == 1);
}

// ── Explosion on decay ─────────────────────────────────────────

TEST_CASE("Decay explosion damages player", "[pickups][explosion]") {
    entt::registry reg;
    constexpr float dt = 1.f / 120.f;

    SECTION("Decay expires — player takes 1 damage, gets invulnerability, weapon reverts") {
        auto player = make_player(reg, 100.f, 100.f);
        Weapon stolen;
        stolen.bullet_speed = 500.f;
        auto pickup = make_pickup(reg, 103.f, 100.f, stolen);
        systems::update_pickups(reg);
        REQUIRE_FALSE(reg.valid(pickup));

        auto& decay = reg.get<WeaponDecay>(player);
        decay.remaining = dt; // will expire next tick

        systems::update_weapon_decay(reg, dt);

        // Player should have taken damage
        auto& hp = reg.get<Health>(player);
        REQUIRE(hp.current == Catch::Approx(0.f));

        // Player should be invulnerable for 2s
        auto* inv = reg.try_get<Invulnerable>(player);
        REQUIRE(inv != nullptr);
        REQUIRE(inv->remaining == Catch::Approx(2.f));

        // Weapon should revert to default
        auto& weapon = reg.get<Weapon>(player);
        REQUIRE(weapon.bullet_speed == Catch::Approx(300.f));

        // Decay and default weapon removed
        REQUIRE_FALSE(reg.any_of<WeaponDecay>(player));
        REQUIRE_FALSE(reg.any_of<DefaultWeapon>(player));
    }

    SECTION("Decay expires while player invulnerable — no damage, weapon still reverts") {
        auto player = make_player(reg, 100.f, 100.f);
        Weapon stolen;
        stolen.bullet_speed = 500.f;
        auto pickup = make_pickup(reg, 103.f, 100.f, stolen);
        systems::update_pickups(reg);
        REQUIRE_FALSE(reg.valid(pickup));

        // Make player invulnerable before decay expires
        reg.emplace<Invulnerable>(player, 1.f);

        auto& decay = reg.get<WeaponDecay>(player);
        decay.remaining = dt;

        systems::update_weapon_decay(reg, dt);

        // Player should NOT have taken damage
        auto& hp = reg.get<Health>(player);
        REQUIRE(hp.current == Catch::Approx(1.f));

        // Weapon should still revert
        auto& weapon = reg.get<Weapon>(player);
        REQUIRE(weapon.bullet_speed == Catch::Approx(300.f));
        REQUIRE_FALSE(reg.any_of<WeaponDecay>(player));
    }

    SECTION("Explosion spawns ExplosionVfx entity at player position") {
        auto player = make_player(reg, 50.f, 75.f);
        Weapon stolen;
        stolen.bullet_speed = 500.f;
        auto pickup = make_pickup(reg, 53.f, 75.f, stolen);
        systems::update_pickups(reg);

        auto& decay = reg.get<WeaponDecay>(player);
        decay.remaining = dt;

        systems::update_weapon_decay(reg, dt);

        auto vfx_view = reg.view<ExplosionVfx, Transform2D, Lifetime>();
        int vfx_count = 0;
        for (auto [ent, tf, life] : vfx_view.each()) {
            ++vfx_count;
            REQUIRE(tf.x == Catch::Approx(50.f));
            REQUIRE(tf.y == Catch::Approx(75.f));
            REQUIRE(life.remaining == Catch::Approx(0.5f));
        }
        REQUIRE(vfx_count == 1);
    }
}

// ── Stabilizer collection ──────────────────────────────────────

TEST_CASE("Stabilizer pickup collection", "[pickups][stabilizer]") {
    entt::registry reg;

    SECTION("Stabilizer collected removes WeaponDecay and DefaultWeapon") {
        auto player = make_player(reg, 100.f, 100.f);
        Weapon stolen;
        stolen.bullet_speed = 500.f;
        auto pickup = make_pickup(reg, 103.f, 100.f, stolen);
        systems::update_pickups(reg);
        REQUIRE_FALSE(reg.valid(pickup));
        REQUIRE(reg.any_of<WeaponDecay>(player));

        // Spawn a stabilizer overlapping the player
        auto stab = reg.create();
        reg.emplace<Transform2D>(stab, 103.f, 100.f);
        reg.emplace<CircleHitbox>(stab, 8.f);
        reg.emplace<StabilizerPickup>(stab);

        systems::update_pickups(reg);

        // Stabilizer should be destroyed
        REQUIRE_FALSE(reg.valid(stab));

        // WeaponDecay and DefaultWeapon should be removed
        REQUIRE_FALSE(reg.any_of<WeaponDecay>(player));
        REQUIRE_FALSE(reg.any_of<DefaultWeapon>(player));

        // Weapon should remain the stolen one (now permanent)
        auto& weapon = reg.get<Weapon>(player);
        REQUIRE(weapon.bullet_speed == Catch::Approx(500.f));
    }

    SECTION("Stabilizer ignored when weapon tier is Legendary") {
        auto player = make_player(reg, 100.f, 100.f);
        Weapon stolen;
        stolen.bullet_speed = 500.f;
        stolen.tier = Weapon::Tier::Legendary;
        auto pickup = make_pickup(reg, 103.f, 100.f, stolen);
        systems::update_pickups(reg);
        REQUIRE_FALSE(reg.valid(pickup));

        auto stab = reg.create();
        reg.emplace<Transform2D>(stab, 103.f, 100.f);
        reg.emplace<CircleHitbox>(stab, 8.f);
        reg.emplace<StabilizerPickup>(stab);

        systems::update_pickups(reg);

        // Stabilizer should NOT be collected
        REQUIRE(reg.valid(stab));

        // WeaponDecay should still be present
        REQUIRE(reg.any_of<WeaponDecay>(player));
    }

    SECTION("Stabilizer ignored when player has no WeaponDecay") {
        auto player = make_player(reg, 100.f, 100.f);

        auto stab = reg.create();
        reg.emplace<Transform2D>(stab, 103.f, 100.f);
        reg.emplace<CircleHitbox>(stab, 8.f);
        reg.emplace<StabilizerPickup>(stab);

        systems::update_pickups(reg);

        // Stabilizer should NOT be collected
        REQUIRE(reg.valid(stab));
    }

    SECTION("Stabilizer works for Common and Rare tiers") {
        for (auto tier : {Weapon::Tier::Common, Weapon::Tier::Rare}) {
            entt::registry r;
            auto player = make_player(r, 100.f, 100.f);
            Weapon stolen;
            stolen.bullet_speed = 500.f;
            stolen.tier = tier;
            auto pickup = make_pickup(r, 103.f, 100.f, stolen);
            systems::update_pickups(r);
            REQUIRE_FALSE(r.valid(pickup));

            auto stab = r.create();
            r.emplace<Transform2D>(stab, 103.f, 100.f);
            r.emplace<CircleHitbox>(stab, 8.f);
            r.emplace<StabilizerPickup>(stab);

            systems::update_pickups(r);

            REQUIRE_FALSE(r.valid(stab));
            REQUIRE_FALSE(r.any_of<WeaponDecay>(player));
        }
    }
}

// ── Stabilizer drops ───────────────────────────────────────────

TEST_CASE("Stabilizer drop on enemy death", "[pickups][stabilizer]") {
    SECTION("Boss enemy always drops stabilizer") {
        entt::registry reg;
        reg.ctx().emplace<std::mt19937>(42u); // seed doesn't matter for boss
        PatternLibrary patterns;

        auto enemy = reg.create();
        reg.emplace<Transform2D>(enemy, 50.f, 75.f);
        reg.emplace<CircleHitbox>(enemy, 6.f);
        reg.emplace<Enemy>(enemy, Enemy::Type::Boss);
        reg.emplace<Health>(enemy, 0.f, 3.f);

        systems::update_damage(reg, patterns);

        auto stab_view = reg.view<StabilizerPickup, Transform2D, Lifetime>();
        int stab_count = 0;
        for (auto [ent, tf, life] : stab_view.each()) {
            ++stab_count;
            REQUIRE(tf.x == Catch::Approx(50.f));
            REQUIRE(tf.y == Catch::Approx(87.f)); // 75 + 12
            REQUIRE(life.remaining == Catch::Approx(8.f));
        }
        REQUIRE(stab_count == 1);
    }

    SECTION("Grunt enemy never drops stabilizer") {
        entt::registry reg;
        reg.ctx().emplace<std::mt19937>(42u);
        PatternLibrary patterns;

        auto enemy = reg.create();
        reg.emplace<Transform2D>(enemy, 50.f, 75.f);
        reg.emplace<CircleHitbox>(enemy, 6.f);
        reg.emplace<Enemy>(enemy, Enemy::Type::Grunt);
        reg.emplace<Health>(enemy, 0.f, 3.f);

        systems::update_damage(reg, patterns);

        auto stab_view = reg.view<StabilizerPickup>();
        int stab_count = 0;
        for ([[maybe_unused]] auto ent : stab_view) {
            ++stab_count;
        }
        REQUIRE(stab_count == 0);
    }

    SECTION("Mid enemy drops stabilizer with rigged RNG") {
        // Seed RNG to produce a value < 0.15 on first draw
        // We test many seeds to find one that works, or just use a known approach
        entt::registry reg;
        // Seed 0 with mt19937: first uniform_real_distribution<float>(0,1) ~ 0.548814
        // We need a seed that gives < 0.15. Seed 5 gives ~0.0994 on first draw.
        // Rather than guessing, we rig the test by running it with many enemies
        // and checking the count is probabilistically correct.
        // For a deterministic test, we set the RNG to always produce low values.
        // Actually, let's just use a fixed seed and brute-force check.
        PatternLibrary patterns;

        // Spawn 100 Mid enemies and check that some drop stabilizers
        int stabilizer_count = 0;
        reg.ctx().emplace<std::mt19937>(12345u);
        for (int i = 0; i < 100; ++i) {
            entt::registry r;
            // Share the same RNG state via the original registry — actually,
            // each iteration needs its own registry but same RNG.
            // Let's use a single registry instead.
        }

        // Better approach: single registry, 100 enemies
        reg = entt::registry{};
        reg.ctx().emplace<std::mt19937>(12345u);
        for (int i = 0; i < 100; ++i) {
            auto enemy = reg.create();
            reg.emplace<Transform2D>(enemy, 50.f, 75.f);
            reg.emplace<CircleHitbox>(enemy, 6.f);
            reg.emplace<Enemy>(enemy, Enemy::Type::Mid);
            reg.emplace<Health>(enemy, 0.f, 3.f);
        }

        systems::update_damage(reg, patterns);

        auto stab_view = reg.view<StabilizerPickup>();
        for ([[maybe_unused]] auto ent : stab_view) {
            ++stabilizer_count;
        }
        // With 15% chance and 100 enemies, expect ~15. Check it's > 0 and < 50.
        REQUIRE(stabilizer_count > 0);
        REQUIRE(stabilizer_count < 50);
    }
}

// ── Weapon tier from PatternDef ────────────────────────────────

TEST_CASE("Weapon tier flows from PatternDef to pickup", "[pickups][tier]") {
    entt::registry reg;
    PatternLibrary patterns;

    nlohmann::json j = {{"name", "rare_pattern"},
                        {"tier", "rare"},
                        {"emitters",
                         {{{"type", "radial"},
                           {"count", 3},
                           {"speed", 150.f},
                           {"fire_rate", 0.2f},
                           {"spread_angle", 45.f}}}}};
    patterns.load_from_json(j);

    auto enemy = reg.create();
    reg.emplace<Transform2D>(enemy, 50.f, 75.f);
    reg.emplace<CircleHitbox>(enemy, 6.f);
    reg.emplace<Enemy>(enemy);
    reg.emplace<Health>(enemy, 0.f, 3.f);
    BulletEmitter emitter;
    emitter.pattern_name = "rare_pattern";
    reg.emplace<BulletEmitter>(enemy, std::move(emitter));

    systems::update_damage(reg, patterns);

    auto pickup_view = reg.view<WeaponPickup>();
    int count = 0;
    for (auto [ent, wp] : pickup_view.each()) {
        ++count;
        REQUIRE(wp.weapon.tier == Weapon::Tier::Rare);
    }
    REQUIRE(count == 1);
}

// ── Pattern tier parsing ───────────────────────────────────────

TEST_CASE("PatternDef tier parsed from JSON", "[pickups][tier]") {
    PatternLibrary patterns;

    SECTION("Explicit tier fields") {
        nlohmann::json j_common = {
            {"name", "common_pat"}, {"tier", "common"}, {"emitters", {{{"count", 1}}}}};
        nlohmann::json j_rare = {
            {"name", "rare_pat"}, {"tier", "rare"}, {"emitters", {{{"count", 1}}}}};
        nlohmann::json j_legendary = {
            {"name", "legend_pat"}, {"tier", "legendary"}, {"emitters", {{{"count", 1}}}}};

        patterns.load_from_json(j_common);
        patterns.load_from_json(j_rare);
        patterns.load_from_json(j_legendary);

        REQUIRE(patterns.get("common_pat")->tier == Weapon::Tier::Common);
        REQUIRE(patterns.get("rare_pat")->tier == Weapon::Tier::Rare);
        REQUIRE(patterns.get("legend_pat")->tier == Weapon::Tier::Legendary);
    }

    SECTION("Missing tier defaults to Common") {
        nlohmann::json j = {{"name", "no_tier"}, {"emitters", {{{"count", 1}}}}};
        patterns.load_from_json(j);

        REQUIRE(patterns.get("no_tier")->tier == Weapon::Tier::Common);
    }
}
