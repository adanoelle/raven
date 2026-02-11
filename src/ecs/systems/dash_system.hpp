#pragma once

#include "core/input.hpp"

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Process dash: burst velocity override with brief invulnerability.
///
/// Ticks DashCooldown, initiates Dash on dash_pressed, overrides velocity
/// during Dash, grants Invulnerable, and removes Dash when expired.
/// @param reg The ECS registry.
/// @param input The current frame's input state snapshot.
/// @param dt Fixed timestep delta in seconds.
void update_dash(entt::registry& reg, const InputState& input, float dt);

} // namespace raven::systems
