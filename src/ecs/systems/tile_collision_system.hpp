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

/// @brief Destroy bullets whose centre is inside a solid tile.
///
/// Bullets have no RectHitbox, so update_tile_collision never sees them.
/// Run after movement and before update_collision, so a bullet that hits a
/// wall can't also hit something standing behind it. Piercing bullets stop
/// at walls too.
/// @param reg The ECS registry.
/// @param tilemap The tilemap with collision grid data.
void update_bullet_walls(entt::registry& reg, const Tilemap& tilemap);

} // namespace raven::systems
