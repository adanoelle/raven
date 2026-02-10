#include "ecs/systems/bullet_spawn.hpp"

#include "ecs/components.hpp"

#include <cmath>

namespace raven::systems {

entt::entity spawn_bullet(entt::registry& reg, const BulletSpawnParams& params) {
    auto entity = reg.create();

    float rotation = params.angle_rad;
    float vx = std::cos(rotation) * params.speed;
    float vy = std::sin(rotation) * params.speed;

    reg.emplace<Transform2D>(entity, params.origin_x, params.origin_y, rotation);
    reg.emplace<PreviousTransform>(entity, params.origin_x, params.origin_y);
    reg.emplace<Velocity>(entity, vx, vy);
    reg.emplace<Bullet>(entity, params.owner);
    reg.emplace<DamageOnContact>(entity, params.damage);
    reg.emplace<Lifetime>(entity, params.lifetime);
    reg.emplace<CircleHitbox>(entity, params.hitbox_radius);
    reg.emplace<Sprite>(entity, std::string{params.sheet_id}, params.frame_x, params.frame_y,
                        params.width, params.height, 5);
    reg.emplace<OffScreenDespawn>(entity);

    if (params.piercing) {
        reg.emplace<Piercing>(entity);
    }

    return entity;
}

} // namespace raven::systems
