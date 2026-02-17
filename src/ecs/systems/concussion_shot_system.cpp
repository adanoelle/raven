#include "ecs/systems/concussion_shot_system.hpp"

#include "ecs/components.hpp"
#include "ecs/systems/hitbox_math.hpp"

#include <cmath>
#include <vector>

namespace raven::systems {

void update_concussion_shot(entt::registry& reg, const InputState& input, float dt) {
    // Tick cooldowns
    auto cd_view = reg.view<ConcussionShotCooldown>();
    for (auto [entity, cooldown] : cd_view.each()) {
        cooldown.remaining -= dt;
    }

    // Activate concussion shot on bomb_pressed
    auto player_view = reg.view<Player, Transform2D, ConcussionShotCooldown>();
    for (auto [entity, player, tf, cooldown] : player_view.each()) {
        if (!input.bomb_pressed) {
            continue;
        }
        if (cooldown.remaining > 0.f) {
            continue;
        }
        if (reg.any_of<ConcussionShot>(entity)) {
            continue;
        }
        if (reg.any_of<Dash>(entity)) {
            continue;
        }

        reg.emplace<ConcussionShot>(entity);
        cooldown.remaining = cooldown.rate;
    }

    // Process active concussion shots
    auto shot_view = reg.view<Player, Transform2D, ConcussionShot>();
    for (auto [entity, player, tf, shot] : shot_view.each()) {
        // Hit check runs exactly once
        if (!shot.hit_checked) {
            shot.hit_checked = true;

            auto enemy_view = reg.view<Transform2D, CircleHitbox, Enemy, Health>();
            struct HitInfo {
                entt::entity ent;
                float dir_x;
                float dir_y;
            };
            std::vector<HitInfo> hits;

            for (auto [e_ent, e_tf, e_hb, enemy, e_hp] : enemy_view.each()) {
                if (circles_overlap(tf.x, tf.y, shot.radius, e_tf.x + e_hb.offset_x,
                                    e_tf.y + e_hb.offset_y, e_hb.radius)) {
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
                e_hp.current -= shot.damage;

                reg.emplace_or_replace<Knockback>(hit.ent, hit.dir_x * shot.knockback,
                                                  hit.dir_y * shot.knockback, 0.15f);
            }
        }

        // Tick duration
        shot.remaining -= dt;
        if (shot.remaining <= 0.f) {
            reg.remove<ConcussionShot>(entity);
        }
    }
}

} // namespace raven::systems
