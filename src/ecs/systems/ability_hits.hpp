#pragma once

#include "ecs/components.hpp"
#include "ecs/systems/hitbox_math.hpp"

#include <entt/entt.hpp>

#include <cmath>
#include <vector>

namespace raven::systems {

/// @brief Knockback duration applied by player abilities (melee, slam, concussion).
inline constexpr float ABILITY_KNOCKBACK_DURATION = 0.15f;

/// @brief An enemy struck by a player ability, with its outward knockback direction.
struct AbilityHit {
    entt::entity ent; ///< The enemy entity that was hit.
    float dir_x;      ///< Unit knockback direction X (away from the ability origin).
    float dir_y;      ///< Unit knockback direction Y (away from the ability origin).
};

/// @brief Compute the unit direction from an ability origin to a target.
/// @param from_x Ability origin X.
/// @param from_y Ability origin Y.
/// @param to_x Target X.
/// @param to_y Target Y.
/// @param out_x Receives the unit direction X (0 if origin and target coincide).
/// @param out_y Receives the unit direction Y (0 if origin and target coincide).
inline void knockback_dir(float from_x, float from_y, float to_x, float to_y, float& out_x,
                          float& out_y) {
    float dx = to_x - from_x;
    float dy = to_y - from_y;
    float dist = std::sqrt(dx * dx + dy * dy);
    out_x = 0.f;
    out_y = 0.f;
    if (dist > 0.f) {
        out_x = dx / dist;
        out_y = dy / dist;
    }
}

/// @brief Collect all enemies whose hitbox overlaps a circle centred on an ability origin.
///
/// Collecting first and mutating afterwards avoids modifying components while
/// iterating the enemy view.
/// @param reg The ECS registry.
/// @param cx Circle centre X (ability origin).
/// @param cy Circle centre Y (ability origin).
/// @param radius Area-of-effect radius in pixels.
/// @return Hit enemies with their knockback directions.
[[nodiscard]] inline std::vector<AbilityHit>
collect_enemies_in_circle(entt::registry& reg, float cx, float cy, float radius) {
    std::vector<AbilityHit> hits;
    auto enemy_view = reg.view<Transform2D, CircleHitbox, Enemy, Health>();
    for (auto [e_ent, e_tf, e_hb, enemy, e_hp] : enemy_view.each()) {
        if (circles_overlap(cx, cy, radius, e_tf.x + e_hb.offset_x, e_tf.y + e_hb.offset_y,
                            e_hb.radius)) {
            AbilityHit hit{e_ent, 0.f, 0.f};
            knockback_dir(cx, cy, e_tf.x, e_tf.y, hit.dir_x, hit.dir_y);
            hits.push_back(hit);
        }
    }
    return hits;
}

/// @brief Apply damage and radial knockback to previously collected ability hits.
/// @param reg The ECS registry.
/// @param hits Enemies to affect, as returned by a collect function.
/// @param damage Damage subtracted from each enemy's health.
/// @param knockback Knockback speed in pixels/s along each hit's direction.
inline void apply_ability_hits(entt::registry& reg, const std::vector<AbilityHit>& hits,
                               float damage, float knockback) {
    for (const auto& hit : hits) {
        auto& e_hp = reg.get<Health>(hit.ent);
        e_hp.current -= damage;

        reg.emplace_or_replace<Knockback>(hit.ent, hit.dir_x * knockback, hit.dir_y * knockback,
                                          ABILITY_KNOCKBACK_DURATION);
    }
}

} // namespace raven::systems
