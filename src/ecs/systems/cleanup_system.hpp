#pragma once

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Tick entity lifetimes and destroy expired or off-screen entities.
/// @param reg The ECS registry containing entities to check.
/// @param dt Fixed timestep delta in seconds (typically 1/120).
/// @param screen_w Virtual screen width in pixels (Renderer::VIRTUAL_WIDTH).
/// @param screen_h Virtual screen height in pixels (Renderer::VIRTUAL_HEIGHT).
void update_cleanup(entt::registry& reg, float dt, int screen_w, int screen_h);

} // namespace raven::systems
