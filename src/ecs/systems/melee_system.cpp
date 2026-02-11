#include "ecs/systems/melee_system.hpp"

#include "core/string_id.hpp"
#include "ecs/components.hpp"
#include "ecs/systems/hitbox_math.hpp"
#include "ecs/systems/pickup_system.hpp"

#include <cmath>
#include <vector>

namespace raven::systems {

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
        attack.aim_x = aim.x;
        attack.aim_y = aim.y;
        attack.hit_checked = false;
        reg.emplace<MeleeAttack>(entity, attack);
        cooldown.remaining = cooldown.rate;
    }

    // Process active melee attacks
    auto attack_view = reg.view<Player, Transform2D, MeleeAttack>();
    for (auto [entity, player, tf, attack] : attack_view.each()) {
        // Hit check runs exactly once
        if (!attack.hit_checked) {
            attack.hit_checked = true;

            auto enemy_view = reg.view<Transform2D, CircleHitbox, Enemy, Health>();
            // Collect hit enemies to avoid iteration issues with component modification
            struct HitInfo {
                entt::entity ent;
                float dir_x;
                float dir_y;
            };
            std::vector<HitInfo> hits;

            for (auto [e_ent, e_tf, e_hb, enemy, e_hp] : enemy_view.each()) {
                // Use range + enemy hitbox radius for generosity
                float effective_range = attack.range + e_hb.radius;
                if (point_in_cone(tf.x, tf.y, attack.aim_x, attack.aim_y, e_tf.x + e_hb.offset_x,
                                  e_tf.y + e_hb.offset_y, effective_range, attack.half_angle)) {
                    // Direction away from player for knockback
                    float dx = e_tf.x - tf.x;
                    float dy = e_tf.y - tf.y;
                    float dist = std::sqrt(dx * dx + dy * dy);
                    float kb_x = 0.f;
                    float kb_y = 0.f;
                    if (dist > 0.f) {
                        kb_x = dx / dist;
                        kb_y = dy / dist;
                    }
                    hits.push_back({e_ent, kb_x, kb_y});
                }
            }

            for (auto& hit : hits) {
                auto& e_hp = reg.get<Health>(hit.ent);
                e_hp.current -= attack.damage;

                reg.emplace_or_replace<Knockback>(hit.ent, hit.dir_x * attack.knockback,
                                                  hit.dir_y * attack.knockback, 0.15f);

                // Disarm: remove emitter, spawn weapon pickup
                if (auto* emitter = reg.try_get<BulletEmitter>(hit.ent)) {
                    auto* e_tf = reg.try_get<Transform2D>(hit.ent);
                    if (e_tf && emitter->pattern_name.valid()) {
                        const auto* pattern = patterns.get(interner.resolve(emitter->pattern_name));
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
                            reg.emplace<WeaponPickup>(pickup_ent, WeaponPickup{std::move(weapon)});
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
