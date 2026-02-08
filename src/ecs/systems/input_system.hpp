#pragma once

#include "core/input.hpp"

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Map the current input state to player entity velocity and actions.
///
/// Reads Player and Velocity components. Smoothly approaches the target
/// velocity using an exponential ease to eliminate hard snaps on direction
/// changes. Triggers shooting and other actions based on edge-detected
/// button presses.
/// @param reg The ECS registry containing the player entity.
/// @param input The current frame's input state snapshot.
/// @param dt Fixed timestep delta in seconds (typically 1/120).
void update_input(entt::registry& reg, const InputState& input, float dt);

} // namespace raven::systems
