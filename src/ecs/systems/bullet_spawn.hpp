#pragma once

#include "ecs/components.hpp"

#include <entt/entt.hpp>

#include <string>

namespace raven::systems {

/// @brief Parameters for spawning a single bullet entity.
struct BulletSpawnParams {
    float origin_x = 0.f;                        ///< Spawn X position in pixels.
    float origin_y = 0.f;                        ///< Spawn Y position in pixels.
    float angle_rad = 0.f;                       ///< Travel direction in radians.
    float speed = 300.f;                         ///< Speed in pixels/sec.
    float damage = 1.f;                          ///< Damage on contact.
    float lifetime = 3.f;                        ///< Lifetime in seconds.
    float hitbox_radius = 2.f;                   ///< Collision radius in pixels.
    Bullet::Owner owner = Bullet::Owner::Player; ///< Who fired this bullet.
    std::string sheet_id = "projectiles";        ///< Sprite sheet identifier.
    int frame_x = 1;                             ///< Frame column in the sheet.
    int frame_y = 0;                             ///< Frame row in the sheet.
    int width = 8;                               ///< Pixel width of bullet frame.
    int height = 8;                              ///< Pixel height of bullet frame.
    bool piercing = false;                       ///< Whether the bullet passes through targets.
};

/// @brief Create a bullet entity with all required components.
/// @param reg The ECS registry.
/// @param params Bullet configuration.
/// @return The newly created bullet entity.
entt::entity spawn_bullet(entt::registry& reg, const BulletSpawnParams& params);

} // namespace raven::systems
