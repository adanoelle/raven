#pragma once

#include "core/input.hpp"

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Map the current input state to player entity velocity and actions.
///
/// Reads Player and Velocity components. Sets Velocity based on movement
/// axes scaled by Player::speed. Triggers shooting and other actions
/// based on edge-detected button presses.
/// @param reg The ECS registry containing the player entity.
/// @param input The current frame's input state snapshot.
void update_input(entt::registry& reg, const InputState& input);

} // namespace raven::systems
