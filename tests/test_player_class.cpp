#include "core/input.hpp"
#include "core/string_id.hpp"
#include "ecs/components.hpp"
#include "ecs/player_class.hpp"
#include "ecs/systems/charged_shot_system.hpp"
#include "ecs/systems/concussion_shot_system.hpp"
#include "ecs/systems/ground_slam_system.hpp"
#include "ecs/systems/hitbox_math.hpp"
#include "ecs/systems/input_system.hpp"
#include "ecs/systems/melee_system.hpp"
#include "ecs/systems/shooting_system.hpp"
#include "patterns/pattern_library.hpp"

#include <entt/entt.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace raven;
using Catch::Approx;

namespace {

/// @brief Create a bare player entity with universal components (pre-recipe).
entt::entity make_bare_player(entt::registry& reg, float x, float y) {
    auto& interner = reg.ctx().get<StringInterner>();
    auto player = reg.create();
    reg.emplace<Transform2D>(player, x, y);
    reg.emplace<PreviousTransform>(player, x, y);
    reg.emplace<Velocity>(player);
    reg.emplace<Player>(player);
    reg.emplace<Health>(player, 1.f, 1.f);
    reg.emplace<CircleHitbox>(player, 6.f);
    reg.emplace<AimDirection>(player, 1.f, 0.f);
    reg.emplace<ShootCooldown>(player, 0.f, 0.2f);
    reg.emplace<MeleeCooldown>(player);
    reg.emplace<DashCooldown>(player);
    auto& weapon = reg.emplace<Weapon>(player);
    weapon.bullet_sheet = interner.intern("projectiles");
    return player;
}

/// @brief Create an enemy entity for ability tests.
entt::entity make_enemy(entt::registry& reg, float x, float y) {
    auto enemy = reg.create();
    reg.emplace<Transform2D>(enemy, x, y);
    reg.emplace<PreviousTransform>(enemy, x, y);
    reg.emplace<Velocity>(enemy);
    reg.emplace<Enemy>(enemy);
    reg.emplace<Health>(enemy, 10.f, 10.f);
    reg.emplace<CircleHitbox>(enemy, 7.f);
    return enemy;
}

InputState no_input() {
    return InputState{};
}

InputState bomb_input() {
    InputState input{};
    input.bomb = true;
    input.bomb_pressed = true;
    return input;
}

} // namespace

// ── Class recipe tests ──────────────────────────────────────────────

TEST_CASE("Brawler recipe sets correct stats", "[class]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_brawler(reg, player);

    auto& p = reg.get<Player>(player);
    REQUIRE(p.speed == Approx(100.f));
    REQUIRE(p.lives == 3);

    auto& hp = reg.get<Health>(player);
    REQUIRE(hp.current == Approx(150.f));
    REQUIRE(hp.max == Approx(150.f));

    REQUIRE(reg.any_of<ClassId>(player));
    REQUIRE(reg.get<ClassId>(player).id == ClassId::Id::Brawler);

    REQUIRE(reg.any_of<MeleeStats>(player));
    auto& ms = reg.get<MeleeStats>(player);
    REQUIRE(ms.damage == Approx(3.f));
    REQUIRE(ms.range == Approx(35.f));

    REQUIRE(reg.any_of<GroundSlamCooldown>(player));
    REQUIRE_FALSE(reg.any_of<ChargedShot>(player));
    REQUIRE_FALSE(reg.any_of<ConcussionShotCooldown>(player));
}

TEST_CASE("Sharpshooter recipe sets correct stats", "[class]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_sharpshooter(reg, player);

    auto& p = reg.get<Player>(player);
    REQUIRE(p.speed == Approx(150.f));
    REQUIRE(p.lives == 2);

    auto& hp = reg.get<Health>(player);
    REQUIRE(hp.current == Approx(60.f));
    REQUIRE(hp.max == Approx(60.f));

    REQUIRE(reg.any_of<ClassId>(player));
    REQUIRE(reg.get<ClassId>(player).id == ClassId::Id::Sharpshooter);

    REQUIRE(reg.any_of<MeleeStats>(player));
    auto& ms = reg.get<MeleeStats>(player);
    REQUIRE(ms.damage == Approx(1.f));
    REQUIRE(ms.range == Approx(20.f));

    REQUIRE(reg.any_of<ChargedShot>(player));
    REQUIRE(reg.any_of<ConcussionShotCooldown>(player));
    REQUIRE_FALSE(reg.any_of<GroundSlamCooldown>(player));

    auto& weapon = reg.get<Weapon>(player);
    REQUIRE(weapon.bullet_damage == Approx(1.5f));
    REQUIRE(weapon.fire_rate == Approx(0.3f));
}

// ── MeleeStats integration tests ────────────────────────────────────

TEST_CASE("MeleeStats overrides default MeleeAttack values", "[class][melee]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    PatternLibrary patterns;

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_brawler(reg, player);

    InputState input{};
    input.melee = true;
    input.melee_pressed = true;

    systems::update_melee(reg, input, patterns, 1.f / 120.f);

    REQUIRE(reg.any_of<MeleeAttack>(player));
    auto& attack = reg.get<MeleeAttack>(player);
    REQUIRE(attack.damage == Approx(3.f));
    REQUIRE(attack.range == Approx(35.f));
    REQUIRE(attack.knockback == Approx(300.f));
}

TEST_CASE("Sharpshooter has weaker melee via MeleeStats", "[class][melee]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();
    PatternLibrary patterns;

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_sharpshooter(reg, player);

    auto enemy = make_enemy(reg, 115.f, 100.f); // Within sharpshooter's shorter range

    InputState input{};
    input.melee = true;
    input.melee_pressed = true;

    systems::update_melee(reg, input, patterns, 1.f / 120.f);

    REQUIRE(reg.any_of<MeleeAttack>(player));
    auto& attack = reg.get<MeleeAttack>(player);
    REQUIRE(attack.damage == Approx(1.f));
    REQUIRE(attack.range == Approx(20.f));

    auto& e_hp = reg.get<Health>(enemy);
    REQUIRE(e_hp.current == Approx(9.f)); // 10 - 1 damage
}

// ── Ground slam tests ───────────────────────────────────────────────

TEST_CASE("Ground slam hits nearby enemies", "[class][ground_slam]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_brawler(reg, player);

    auto enemy = make_enemy(reg, 130.f, 100.f); // 30px away, within 50px radius

    auto input = bomb_input();
    systems::update_ground_slam(reg, input, 1.f / 120.f);

    auto& e_hp = reg.get<Health>(enemy);
    REQUIRE(e_hp.current < 10.f); // Took damage
    REQUIRE(reg.any_of<Knockback>(enemy));
}

TEST_CASE("Ground slam misses distant enemies", "[class][ground_slam]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_brawler(reg, player);

    auto enemy = make_enemy(reg, 200.f, 200.f); // Far outside 50px radius

    auto input = bomb_input();
    systems::update_ground_slam(reg, input, 1.f / 120.f);

    auto& e_hp = reg.get<Health>(enemy);
    REQUIRE(e_hp.current == Approx(10.f)); // No damage
}

TEST_CASE("Ground slam cooldown prevents spam", "[class][ground_slam]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_brawler(reg, player);

    auto enemy = make_enemy(reg, 130.f, 100.f);

    // First slam
    auto input = bomb_input();
    systems::update_ground_slam(reg, input, 1.f / 120.f);
    float hp_after_first = reg.get<Health>(enemy).current;
    REQUIRE(hp_after_first < 10.f);

    // Expire the slam
    auto empty = no_input();
    for (int i = 0; i < 25; ++i) {
        systems::update_ground_slam(reg, empty, 1.f / 120.f);
    }

    // Second slam immediately — cooldown should prevent it
    systems::update_ground_slam(reg, input, 1.f / 120.f);
    REQUIRE(reg.get<Health>(enemy).current == Approx(hp_after_first));
}

TEST_CASE("Ground slam blocked during dash", "[class][ground_slam]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_brawler(reg, player);

    // Simulate being mid-dash
    reg.emplace<Dash>(player);

    auto input = bomb_input();
    systems::update_ground_slam(reg, input, 1.f / 120.f);

    REQUIRE_FALSE(reg.any_of<GroundSlam>(player));
}

// ── Charged shot tests ──────────────────────────────────────────────

TEST_CASE("Charged shot fires on release with scaled damage", "[class][charged_shot]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_sharpshooter(reg, player);

    float dt = 1.f / 120.f;

    // Start charging (shoot pressed)
    InputState charge_start{};
    charge_start.shoot = true;
    charge_start.shoot_pressed = true;
    systems::update_charged_shot(reg, charge_start, dt);

    // Hold for several frames to build charge
    InputState hold{};
    hold.shoot = true;
    for (int i = 0; i < 48; ++i) { // 48 ticks = 0.4s = 50% charge
        systems::update_charged_shot(reg, hold, dt);
    }

    auto& cs = reg.get<ChargedShot>(player);
    REQUIRE(cs.charge > 0.4f);
    REQUIRE(cs.charging == true);

    // Release — should fire
    InputState release{};
    release.shoot = false;
    systems::update_charged_shot(reg, release, dt);

    // A bullet should have been spawned
    int bullet_count = 0;
    for (auto entity : reg.view<Bullet>()) {
        (void)entity;
        ++bullet_count;
    }
    REQUIRE(bullet_count >= 1);
}

TEST_CASE("Tap-fire as Sharpshooter fires weak shot", "[class][charged_shot]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_sharpshooter(reg, player);

    float dt = 1.f / 120.f;

    // Press shoot
    InputState press{};
    press.shoot = true;
    press.shoot_pressed = true;
    systems::update_charged_shot(reg, press, dt);

    // Immediately release
    InputState release{};
    release.shoot = false;
    systems::update_charged_shot(reg, release, dt);

    // A bullet should have been spawned (weak)
    int bullet_count = 0;
    for (auto entity : reg.view<Bullet>()) {
        (void)entity;
        ++bullet_count;
    }
    REQUIRE(bullet_count >= 1);

    // The bullet should have weak damage (near min_damage_mult * bullet_damage = 0.5 * 1.5 = 0.75)
    // One tick of charge accumulates, so damage is slightly above 0.75
    for (auto [entity, bullet, dmg] : reg.view<Bullet, DamageOnContact>().each()) {
        if (bullet.owner == Bullet::Owner::Player) {
            REQUIRE(dmg.damage < 1.0f);   // Well below full damage
            REQUIRE(dmg.damage >= 0.75f); // At or near minimum
        }
    }
}

TEST_CASE("Full charge produces piercing shot", "[class][charged_shot]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_sharpshooter(reg, player);

    float dt = 1.f / 120.f;

    // Press shoot
    InputState press{};
    press.shoot = true;
    press.shoot_pressed = true;
    systems::update_charged_shot(reg, press, dt);

    // Hold until full charge (0.8s = 96 ticks)
    InputState hold{};
    hold.shoot = true;
    for (int i = 0; i < 100; ++i) {
        systems::update_charged_shot(reg, hold, dt);
    }

    auto& cs = reg.get<ChargedShot>(player);
    REQUIRE(cs.charge >= cs.full_charge_threshold);

    // Release
    InputState release{};
    release.shoot = false;
    systems::update_charged_shot(reg, release, dt);

    // The bullet should be piercing
    int piercing_count = 0;
    for (auto entity : reg.view<Bullet, Piercing>()) {
        (void)entity;
        ++piercing_count;
    }
    REQUIRE(piercing_count >= 1);
}

TEST_CASE("Charge resets on dash", "[class][charged_shot]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_sharpshooter(reg, player);

    float dt = 1.f / 120.f;

    // Start charging
    InputState press{};
    press.shoot = true;
    press.shoot_pressed = true;
    systems::update_charged_shot(reg, press, dt);

    // Hold to build some charge
    InputState hold{};
    hold.shoot = true;
    for (int i = 0; i < 30; ++i) {
        systems::update_charged_shot(reg, hold, dt);
    }

    auto& cs = reg.get<ChargedShot>(player);
    REQUIRE(cs.charge > 0.f);

    // Simulate dash
    reg.emplace<Dash>(player);
    systems::update_charged_shot(reg, hold, dt);

    REQUIRE(cs.charge == Approx(0.f));
    REQUIRE(cs.charging == false);
}

TEST_CASE("Charging applies movement speed penalty", "[class][charged_shot]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_sharpshooter(reg, player);

    float dt = 1.f / 120.f;

    // Start charging
    InputState press{};
    press.shoot = true;
    press.shoot_pressed = true;
    systems::update_charged_shot(reg, press, dt);

    // Verify charging state
    auto& cs = reg.get<ChargedShot>(player);
    REQUIRE(cs.charging == true);

    // Move while charging
    InputState move_charge{};
    move_charge.shoot = true;
    move_charge.move_x = 1.f;
    systems::update_input(reg, move_charge, dt);

    auto& vel = reg.get<Velocity>(player);
    // With penalty=0.5 and speed=150, target velocity should approach 75
    // One tick won't reach it, but it should be < normal speed target
    REQUIRE(std::abs(vel.dx) < 150.f);
}

TEST_CASE("Shooting system skips ChargedShot entities", "[class][charged_shot]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_sharpshooter(reg, player);

    float dt = 1.f / 120.f;

    // Hold shoot — normal shooting system should NOT fire
    InputState shoot{};
    shoot.shoot = true;
    systems::update_shooting(reg, shoot, dt);

    int bullet_count = 0;
    for (auto entity : reg.view<Bullet>()) {
        (void)entity;
        ++bullet_count;
    }
    REQUIRE(bullet_count == 0);
}

// ── Concussion shot tests ───────────────────────────────────────────

TEST_CASE("Concussion shot hits nearby enemies with knockback", "[class][concussion_shot]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_sharpshooter(reg, player);

    auto enemy = make_enemy(reg, 130.f, 100.f); // 30px away, within 45px radius

    auto input = bomb_input();
    systems::update_concussion_shot(reg, input, 1.f / 120.f);

    auto& e_hp = reg.get<Health>(enemy);
    REQUIRE(e_hp.current < 10.f);
    REQUIRE(reg.any_of<Knockback>(enemy));

    auto& kb = reg.get<Knockback>(enemy);
    REQUIRE(kb.dx > 0.f); // Knocked away from player (player at 100, enemy at 130)
}

TEST_CASE("Concussion shot misses distant enemies", "[class][concussion_shot]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_sharpshooter(reg, player);

    auto enemy = make_enemy(reg, 200.f, 200.f); // Far outside 45px radius

    auto input = bomb_input();
    systems::update_concussion_shot(reg, input, 1.f / 120.f);

    auto& e_hp = reg.get<Health>(enemy);
    REQUIRE(e_hp.current == Approx(10.f));
}

TEST_CASE("Concussion shot cooldown prevents spam", "[class][concussion_shot]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_sharpshooter(reg, player);

    auto enemy = make_enemy(reg, 130.f, 100.f);

    // First shot
    auto input = bomb_input();
    systems::update_concussion_shot(reg, input, 1.f / 120.f);
    float hp_after_first = reg.get<Health>(enemy).current;
    REQUIRE(hp_after_first < 10.f);

    // Expire the shot
    auto empty = no_input();
    for (int i = 0; i < 15; ++i) {
        systems::update_concussion_shot(reg, empty, 1.f / 120.f);
    }

    // Second shot immediately — cooldown should prevent it
    systems::update_concussion_shot(reg, input, 1.f / 120.f);
    REQUIRE(reg.get<Health>(enemy).current == Approx(hp_after_first));
}

TEST_CASE("Concussion shot blocked during dash", "[class][concussion_shot]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_sharpshooter(reg, player);

    // Simulate being mid-dash
    reg.emplace<Dash>(player);

    auto input = bomb_input();
    systems::update_concussion_shot(reg, input, 1.f / 120.f);

    REQUIRE_FALSE(reg.any_of<ConcussionShot>(player));
}

// ── Cross-class ability isolation tests ─────────────────────────────

TEST_CASE("Brawler has no ChargedShot or ConcussionShotCooldown", "[class]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_brawler(reg, player);

    // Bomb input should not trigger concussion shot (no ConcussionShotCooldown)
    auto input = bomb_input();
    systems::update_concussion_shot(reg, input, 1.f / 120.f);
    REQUIRE_FALSE(reg.any_of<ConcussionShot>(player));

    // Charged shot system should not affect Brawler (no ChargedShot component)
    InputState shoot{};
    shoot.shoot = true;
    shoot.shoot_pressed = true;
    systems::update_charged_shot(reg, shoot, 1.f / 120.f);
    // No crash, no bullet from charged system
}

TEST_CASE("Sharpshooter has no GroundSlamCooldown", "[class]") {
    entt::registry reg;
    reg.ctx().emplace<StringInterner>();

    auto player = make_bare_player(reg, 100.f, 100.f);
    apply_sharpshooter(reg, player);

    // Bomb input should not trigger ground slam (no GroundSlamCooldown)
    auto input = bomb_input();
    systems::update_ground_slam(reg, input, 1.f / 120.f);
    REQUIRE_FALSE(reg.any_of<GroundSlam>(player));
}
