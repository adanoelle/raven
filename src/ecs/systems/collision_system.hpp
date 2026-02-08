#pragma once

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Detect and resolve collisions between hitbox-bearing entities.
///
/// Uses circle-circle checks for player vs bullet and AABB for
/// player projectiles vs enemy rect hitboxes. Applies DamageOnContact
/// and marks colliding pairs for the damage system.
/// @param reg The ECS registry containing entities with hitbox components.
void update_collision(entt::registry& reg);

} // namespace raven::systems
