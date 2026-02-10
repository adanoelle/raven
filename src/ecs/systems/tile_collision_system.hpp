#pragma once

#include "rendering/tilemap.hpp"

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Resolve entity-vs-tilemap collision using axis-separated push-out.
///
/// Entities must have Transform2D, PreviousTransform, Velocity, and RectHitbox.
/// @param reg The ECS registry.
/// @param tilemap The tilemap with collision grid data.
void update_tile_collision(entt::registry& reg, const Tilemap& tilemap);

} // namespace raven::systems
