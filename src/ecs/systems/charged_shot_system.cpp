#include "ecs/systems/charged_shot_system.hpp"

#include "ecs/components.hpp"
#include "ecs/systems/bullet_spawn.hpp"

#include <cmath>

namespace raven::systems {

void update_charged_shot(entt::registry& reg, const InputState& input, float dt) {
    auto view = reg.view<Player, ChargedShot, Weapon, AimDirection, ShootCooldown, Transform2D>();

    for (auto [entity, player, cs, weapon, aim, cd, tf] : view.each()) {
        // Reset charge during dash
        if (reg.any_of<Dash>(entity)) {
            cs.charge = 0.f;
            cs.charging = false;
            cs.was_shooting = false;
            continue;
        }

        // Start charging on press edge
        if (input.shoot && !cs.was_shooting) {
            cs.charging = true;
            cs.charge = 0.f;
        }

        // Accumulate charge while held
        if (input.shoot && cs.charging) {
            cs.charge += dt / cs.charge_rate;
            if (cs.charge > 1.f) {
                cs.charge = 1.f;
            }
        }

        // Fire on release
        if (!input.shoot && cs.was_shooting && cs.charging) {
            float t = cs.charge;
            float damage_mult = cs.min_damage_mult + (cs.max_damage_mult - cs.min_damage_mult) * t;
            float speed_mult = cs.min_speed_mult + (cs.max_speed_mult - cs.min_speed_mult) * t;

            float base_angle = std::atan2(aim.y, aim.x);

            BulletSpawnParams params;
            params.origin_x = tf.x;
            params.origin_y = tf.y;
            params.angle_rad = base_angle;
            params.speed = weapon.bullet_speed * speed_mult;
            params.damage = weapon.bullet_damage * damage_mult;
            params.lifetime = weapon.bullet_lifetime;
            params.hitbox_radius = weapon.bullet_hitbox;
            params.owner = Bullet::Owner::Player;
            params.sheet_id = weapon.bullet_sheet;
            params.frame_x = weapon.bullet_frame_x;
            params.frame_y = weapon.bullet_frame_y;
            params.width = weapon.bullet_width;
            params.height = weapon.bullet_height;
            params.piercing = cs.charge >= cs.full_charge_threshold;

            spawn_bullet(reg, params);
            cd.remaining = weapon.fire_rate;

            cs.charge = 0.f;
            cs.charging = false;
        }

        // Cancel charging if shoot not held and not just released
        if (!input.shoot) {
            cs.charging = false;
        }

        cs.was_shooting = input.shoot;
    }
}

} // namespace raven::systems
