#pragma once

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Process damage from collisions, apply invulnerability, and destroy dead entities.
///
/// Reads DamageOnContact results flagged by the collision system.
/// Reduces Health, triggers invulnerability frames, and destroys
/// entities whose health reaches zero.
/// @param reg The ECS registry containing damageable entities.
void update_damage(entt::registry& reg);

} // namespace raven::systems
