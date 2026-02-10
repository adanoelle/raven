#pragma once

#include "patterns/pattern_library.hpp"

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Process damage from collisions, apply invulnerability, and destroy dead entities.
///
/// Reads DamageOnContact results flagged by the collision system.
/// Reduces Health, triggers invulnerability frames, and destroys
/// entities whose health reaches zero. When an enemy with a BulletEmitter
/// dies, spawns a weapon pickup derived from the first emitter.
/// @param reg The ECS registry containing damageable entities.
/// @param patterns The loaded pattern library (for deriving weapon pickups).
void update_damage(entt::registry& reg, const PatternLibrary& patterns);

} // namespace raven::systems
