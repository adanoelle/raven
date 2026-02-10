#include "ecs/systems/shooting_system.hpp"

#include "ecs/components.hpp"

#include <cmath>

namespace raven::systems {

namespace {

constexpr float AIM_DEADZONE = 0.2f;
constexpr float BULLET_SPEED = 300.f;

} // namespace

void update_shooting(entt::registry& reg, const InputState& input, float dt) {
    // Tick cooldowns
    auto cooldown_view = reg.view<ShootCooldown>();
    for (auto [entity, cd] : cooldown_view.each()) {
        cd.remaining -= dt;
    }

    // Resolve aim direction
    auto aim_view = reg.view<Player, Transform2D, AimDirection>();
    for (auto [entity, player, tf, aim] : aim_view.each()) {
        float stick_mag = input.aim_x * input.aim_x + input.aim_y * input.aim_y;
        if (stick_mag > AIM_DEADZONE * AIM_DEADZONE) {
            float inv_len = 1.f / std::sqrt(stick_mag);
            aim.x = input.aim_x * inv_len;
            aim.y = input.aim_y * inv_len;
        } else if (input.mouse_active) {
            float dx = input.mouse_x - tf.x;
            float dy = input.mouse_y - tf.y;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len > 1.f) {
                aim.x = dx / len;
                aim.y = dy / len;
            }
        }
        // else: retain previous aim direction
    }

    // Spawn bullets
    auto shoot_view = reg.view<Player, Transform2D, AimDirection, ShootCooldown>();
    for (auto [entity, player, tf, aim, cd] : shoot_view.each()) {
        if (input.shoot && cd.remaining <= 0.f) {
            cd.remaining = cd.rate;

            auto bullet = reg.create();
            float rotation = std::atan2(aim.y, aim.x);

            reg.emplace<Transform2D>(bullet, tf.x, tf.y, rotation);
            reg.emplace<PreviousTransform>(bullet, tf.x, tf.y);
            reg.emplace<Velocity>(bullet, aim.x * BULLET_SPEED, aim.y * BULLET_SPEED);
            reg.emplace<Bullet>(bullet, Bullet::Owner::Player);
            reg.emplace<DamageOnContact>(bullet, 1.f);
            reg.emplace<Lifetime>(bullet, 3.f);
            reg.emplace<CircleHitbox>(bullet, 2.f);
            reg.emplace<Sprite>(bullet, std::string{"projectiles"}, 1, 0, 8, 8, 5);
            reg.emplace<OffScreenDespawn>(bullet);
        }
    }
}

} // namespace raven::systems
