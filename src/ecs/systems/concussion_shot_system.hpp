#pragma once

#include "core/input.hpp"

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Process concussion shot ability: radial knockback blast.
///
/// Ticks ConcussionShotCooldown, activates ConcussionShot on bomb_pressed
/// (if not dashing), performs a circle-based hit check against enemies,
/// applies minor damage and strong knockback, and removes expired components.
/// @param reg The ECS registry.
/// @param input The current frame's input state snapshot.
/// @param dt Fixed timestep delta in seconds.
void update_concussion_shot(entt::registry& reg, const InputState& input, float dt);

} // namespace raven::systems
