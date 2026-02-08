#pragma once

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Destroy entities tagged with OffScreenDespawn that have left the play area.
/// @param reg The ECS registry containing entities to check.
/// @param screen_w Virtual screen width in pixels (Renderer::VIRTUAL_WIDTH).
/// @param screen_h Virtual screen height in pixels (Renderer::VIRTUAL_HEIGHT).
void update_cleanup(entt::registry& reg, int screen_w, int screen_h);

} // namespace raven::systems
