#pragma once

#include "core/input.hpp"

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Resolve player aim direction and spawn bullets when shooting.
///
/// Updates AimDirection from right stick or mouse position, then spawns
/// bullet entities when the shoot button is held and the cooldown has
/// elapsed.
/// @param reg The ECS registry containing the player entity.
/// @param input The current frame's input state snapshot.
/// @param dt Fixed timestep delta in seconds (typically 1/120).
void update_shooting(entt::registry& reg, const InputState& input, float dt);

} // namespace raven::systems
