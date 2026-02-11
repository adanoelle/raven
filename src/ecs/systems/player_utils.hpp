#pragma once

#include "ecs/components.hpp"

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Find the player position, if any player exists.
/// @param reg The ECS registry.
/// @param out_x Output X position.
/// @param out_y Output Y position.
/// @return True if a player was found.
inline bool find_player_position(const entt::registry& reg, float& out_x, float& out_y) {
    auto view = reg.view<Player, Transform2D>();
    for (auto [entity, player, tf] : view.each()) {
        out_x = tf.x;
        out_y = tf.y;
        return true;
    }
    return false;
}

} // namespace raven::systems
