#include "ecs/systems/ground_slam_system.hpp"

#include "ecs/components.hpp"
#include "ecs/systems/ability_hits.hpp"

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

            auto hits = collect_enemies_in_circle(reg, tf.x, tf.y, slam.radius);
            apply_ability_hits(reg, hits, slam.damage, slam.knockback);
        }

        // Tick duration
        slam.remaining -= dt;
        if (slam.remaining <= 0.f) {
            reg.remove<GroundSlam>(entity);
        }
    }
}

} // namespace raven::systems
