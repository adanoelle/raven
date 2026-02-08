#pragma once

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Move all entities that have Velocity and clamp players to screen bounds.
/// @param reg The ECS registry containing entities to update.
/// @param dt Fixed timestep delta in seconds (typically 1/120).
void update_movement(entt::registry& reg, float dt);

} // namespace raven::systems
