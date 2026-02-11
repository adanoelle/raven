#pragma once

#include "rendering/tilemap.hpp"

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Update enemy AI behaviors, knockback, and contact damage.
///
/// Dispatches to per-archetype handlers (Chaser, Drifter, Stalker, Coward),
/// applies knockback impulses, toggles emitters based on attack range,
/// and resolves contact damage against the player.
/// @param reg The ECS registry containing enemy entities.
/// @param tilemap The tilemap used for line-of-sight checks.
/// @param dt Fixed timestep delta in seconds.
void update_ai(entt::registry& reg, const Tilemap& tilemap, float dt);

} // namespace raven::systems
