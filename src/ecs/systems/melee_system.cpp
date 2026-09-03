#include "ecs/systems/melee_system.hpp"

#include "core/string_id.hpp"
#include "ecs/components.hpp"
#include "ecs/systems/ability_hits.hpp"
#include "ecs/systems/hitbox_math.hpp"
#include "ecs/systems/pickup_system.hpp"

#include <vector>

namespace raven::systems {

namespace {
constexpr float PI = 3.14159265358979323846f;
} // namespace

void update_melee(entt::registry& reg, const InputState& input, const PatternLibrary& patterns,
                  float dt) {
    auto& interner = reg.ctx().get<StringInterner>();

    // Tick melee cooldowns
    auto cd_view = reg.view<MeleeCooldown>();
    for (auto [entity, cooldown] : cd_view.each()) {
        cooldown.remaining -= dt;
    }

    // Initiate melee attack on press
    auto player_view = reg.view<Player, Transform2D, AimDirection, MeleeCooldown>();
    for (auto [entity, player, tf, aim, cooldown] : player_view.each()) {
        if (!input.melee_pressed) {
            continue;
        }
        if (cooldown.remaining > 0.f) {
            continue;
        }
        if (reg.any_of<MeleeAttack>(entity)) {
            continue;
        }

        MeleeAttack attack;
        if (auto* stats = reg.try_get<MeleeStats>(entity)) {
            attack.damage = stats->damage;
            attack.range = stats->range;
            attack.half_angle = stats->half_angle;
            attack.knockback = stats->knockback;
            attack.remaining = stats->duration;
        }
        attack.aim_x = aim.x;
        attack.aim_y = aim.y;
        attack.hit_checked = false;

        // Dash-spin prototype: a melee started mid-dash widens to a full
        // 360-degree spin, spending the dash's follow-up token — the same
        // token a second dash would use — so each dash chain buys either
        // the spin or a second dash, never both. Melee during a follow-up
        // dash (token already spent) is the normal aimed cone, and the
        // standing attack is untouched (the disarm economy prices in
        // aiming).
        if (reg.any_of<Dash>(entity) && reg.any_of<DashFollowUp>(entity)) {
            attack.half_angle = PI;
            reg.remove<DashFollowUp>(entity);
        }
        reg.emplace<MeleeAttack>(entity, attack);
        cooldown.remaining = cooldown.rate;
        push_sfx(reg, Sfx::Melee);
    }

    // Process active melee attacks
    auto attack_view = reg.view<Player, Transform2D, MeleeAttack>();
    for (auto [entity, player, tf, attack] : attack_view.each()) {
        // Hit check runs exactly once
        if (!attack.hit_checked) {
            attack.hit_checked = true;

            // Collect hit enemies to avoid iteration issues with component modification
            auto enemy_view = reg.view<Transform2D, CircleHitbox, Enemy, Health>();
            std::vector<AbilityHit> hits;

            for (auto [e_ent, e_tf, e_hb, enemy, e_hp] : enemy_view.each()) {
                // Use range + enemy hitbox radius for generosity
                float effective_range = attack.range + e_hb.radius;
                if (point_in_cone(tf.x, tf.y, attack.aim_x, attack.aim_y, e_tf.x + e_hb.offset_x,
                                  e_tf.y + e_hb.offset_y, effective_range, attack.half_angle)) {
                    AbilityHit hit{e_ent, 0.f, 0.f};
                    knockback_dir(tf.x, tf.y, e_tf.x, e_tf.y, hit.dir_x, hit.dir_y);
                    hits.push_back(hit);
                }
            }

            apply_ability_hits(reg, hits, attack.damage, attack.knockback);

            for (auto& hit : hits) {
                // Disarm: remove emitter, spawn weapon pickup
                if (auto* emitter = reg.try_get<BulletEmitter>(hit.ent)) {
                    auto* e_tf = reg.try_get<Transform2D>(hit.ent);
                    if (e_tf && emitter->pattern_name.valid()) {
                        const auto* pattern = patterns.get(emitter->pattern_name);
                        if (pattern && !pattern->emitters.empty()) {
                            auto pickup_ent = reg.create();
                            reg.emplace<Transform2D>(pickup_ent, e_tf->x, e_tf->y);
                            reg.emplace<PreviousTransform>(pickup_ent, e_tf->x, e_tf->y);
                            reg.emplace<CircleHitbox>(pickup_ent, 8.f);
                            reg.emplace<Lifetime>(pickup_ent, 5.f);
                            reg.emplace<Sprite>(pickup_ent, interner.intern("pickups"), 0, 0, 16,
                                                16, 5);
                            auto weapon = weapon_from_emitter(pattern->emitters[0]);
                            weapon.tier = pattern->tier;
                            reg.emplace<WeaponPickup>(pickup_ent, WeaponPickup{weapon});
                        }
                    }
                    reg.remove<BulletEmitter>(hit.ent);
                    reg.emplace_or_replace<Disarmed>(hit.ent);
                }
            }
        }

        // Tick duration
        attack.remaining -= dt;
        if (attack.remaining <= 0.f) {
            reg.remove<MeleeAttack>(entity);
        }
    }
}

} // namespace raven::systems
