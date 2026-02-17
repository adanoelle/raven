#pragma once

#include "core/input.hpp"

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Process charged shot mechanic: hold-to-charge, release-to-fire.
///
/// For entities with ChargedShot, accumulates charge while shoot is held,
/// fires a scaled bullet on release, and resets charge during dash.
/// @param reg The ECS registry.
/// @param input The current frame's input state snapshot.
/// @param dt Fixed timestep delta in seconds.
void update_charged_shot(entt::registry& reg, const InputState& input, float dt);

} // namespace raven::systems
