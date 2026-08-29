#include "ecs/systems/concussion_shot_system.hpp"

#include "ecs/components.hpp"
#include "ecs/systems/ability_hits.hpp"

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

            auto hits = collect_enemies_in_circle(reg, tf.x, tf.y, shot.radius);
            apply_ability_hits(reg, hits, shot.damage, shot.knockback);
        }

        // Tick duration
        shot.remaining -= dt;
        if (shot.remaining <= 0.f) {
            reg.remove<ConcussionShot>(entity);
        }
    }
}

} // namespace raven::systems
