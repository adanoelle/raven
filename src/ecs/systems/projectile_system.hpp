#pragma once

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Update projectile lifetimes and despawn expired bullets.
///
/// Decrements Lifetime::remaining for all Bullet entities and destroys
/// those whose lifetime has elapsed.
/// @param reg The ECS registry containing bullet entities.
/// @param dt Fixed timestep delta in seconds (typically 1/120).
void update_projectiles(entt::registry& reg, float dt);

} // namespace raven::systems
