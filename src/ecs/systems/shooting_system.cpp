#include "ecs/systems/shooting_system.hpp"

#include "ecs/components.hpp"
#include "ecs/systems/bullet_spawn.hpp"

#include <cmath>

namespace raven::systems {

namespace {

constexpr float AIM_DEADZONE = 0.2f;
constexpr float PI = 3.14159265358979323846f;

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
    auto shoot_view = reg.view<Player, Transform2D, AimDirection, ShootCooldown, Weapon>();
    for (auto [entity, player, tf, aim, cd, weapon] : shoot_view.each()) {
        if (reg.any_of<ChargedShot>(entity)) {
            continue;
        }
        if (input.shoot && cd.remaining <= 0.f) {
            cd.remaining = weapon.fire_rate;

            float base_angle = std::atan2(aim.y, aim.x);

            BulletSpawnParams params;
            params.origin_x = tf.x;
            params.origin_y = tf.y;
            params.speed = weapon.bullet_speed;
            params.damage = weapon.bullet_damage;
            params.lifetime = weapon.bullet_lifetime;
            params.hitbox_radius = weapon.bullet_hitbox;
            params.owner = Bullet::Owner::Player;
            params.sheet_id = weapon.bullet_sheet;
            params.frame_x = weapon.bullet_frame_x;
            params.frame_y = weapon.bullet_frame_y;
            params.width = weapon.bullet_width;
            params.height = weapon.bullet_height;
            params.piercing = weapon.piercing;

            if (weapon.bullet_count <= 1) {
                params.angle_rad = base_angle;
                spawn_bullet(reg, params);
            } else {
                float spread_rad = weapon.spread_angle * PI / 180.f;
                float step = spread_rad / static_cast<float>(weapon.bullet_count);
                float start = base_angle - spread_rad / 2.f;

                for (int i = 0; i < weapon.bullet_count; ++i) {
                    params.angle_rad = start + step * static_cast<float>(i) + step / 2.f;
                    spawn_bullet(reg, params);
                }
            }
        }
    }
}

} // namespace raven::systems
