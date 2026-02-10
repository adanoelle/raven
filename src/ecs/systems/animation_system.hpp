#pragma once

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Advance Animation timers and sync current_frame to Sprite::frame_x.
/// @param reg The ECS registry containing entities to update.
/// @param dt Fixed timestep delta in seconds (typically 1/120).
void update_animation(entt::registry& reg, float dt);

} // namespace raven::systems
