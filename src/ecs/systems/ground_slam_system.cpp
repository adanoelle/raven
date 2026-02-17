#include "ecs/systems/ground_slam_system.hpp"

#include "ecs/components.hpp"
#include "ecs/systems/hitbox_math.hpp"

#include <cmath>
#include <vector>

namespace raven::systems {

void update_ground_slam(entt::registry& reg, const InputState& input, float dt) {
    // Tick cooldowns
    auto cd_view = reg.view<GroundSlamCooldown>();
    for (auto [entity, cooldown] : cd_view.each()) {
        cooldown.remaining -= dt;
    }

    // Activate ground slam on bomb_pressed
    auto player_view = reg.view<Player, Transform2D, GroundSlamCooldown>();
    for (auto [entity, player, tf, cooldown] : player_view.each()) {
        if (!input.bomb_pressed) {
            continue;
        }
        if (cooldown.remaining > 0.f) {
            continue;
        }
        if (reg.any_of<GroundSlam>(entity)) {
            continue;
        }
        if (reg.any_of<Dash>(entity)) {
            continue;
        }

        reg.emplace<GroundSlam>(entity);
        cooldown.remaining = cooldown.rate;
    }

    // Process active ground slams
    auto slam_view = reg.view<Player, Transform2D, GroundSlam>();
    for (auto [entity, player, tf, slam] : slam_view.each()) {
        // Hit check runs exactly once
        if (!slam.hit_checked) {
            slam.hit_checked = true;

            auto enemy_view = reg.view<Transform2D, CircleHitbox, Enemy, Health>();
            struct HitInfo {
                entt::entity ent;
                float dir_x;
                float dir_y;
            };
            std::vector<HitInfo> hits;

            for (auto [e_ent, e_tf, e_hb, enemy, e_hp] : enemy_view.each()) {
                if (circles_overlap(tf.x, tf.y, slam.radius, e_tf.x + e_hb.offset_x,
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
                e_hp.current -= slam.damage;

                reg.emplace_or_replace<Knockback>(hit.ent, hit.dir_x * slam.knockback,
                                                  hit.dir_y * slam.knockback, 0.15f);
            }
        }

        // Tick duration
        slam.remaining -= dt;
        if (slam.remaining <= 0.f) {
            reg.remove<GroundSlam>(entity);
        }
    }
}

} // namespace raven::systems
