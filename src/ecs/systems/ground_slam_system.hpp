#pragma once

#include "core/input.hpp"
#include "rendering/tilemap.hpp"

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Process ground slam ability: radial AoE damage and knockback.
///
/// Ticks GroundSlamCooldown, activates GroundSlam on bomb_pressed (if not
/// dashing), performs a circle-based hit check against enemies, applies
/// damage and knockback, and removes expired GroundSlam components.
/// @param reg The ECS registry.
/// @param input The current frame's input state snapshot.
/// @param tilemap The tilemap whose walls block the blast.
/// @param dt Fixed timestep delta in seconds.
void update_ground_slam(entt::registry& reg, const InputState& input, const Tilemap& tilemap,
                        float dt);

} // namespace raven::systems
