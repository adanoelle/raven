#pragma once

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Snapshot previous positions, integrate velocity, and clamp players.
///
/// Before integration, copies Transform2D into PreviousTransform for
/// any entity that has both, enabling render interpolation between ticks.
/// Then applies Velocity to Transform2D and clamps players to screen bounds.
/// @param reg The ECS registry containing entities to update.
/// @param dt Fixed timestep delta in seconds (typically 1/120).
void update_movement(entt::registry& reg, float dt);

} // namespace raven::systems
