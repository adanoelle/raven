#include "ecs/systems/collision_system.hpp"

#include "ecs/components.hpp"

#include <cmath>

namespace raven::systems {

namespace {

bool circles_overlap(float x1, float y1, float r1, float x2, float y2, float r2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dist_sq = dx * dx + dy * dy;
    float radii = r1 + r2;
    return dist_sq <= radii * radii;
}

} // anonymous namespace

void update_collision(entt::registry& reg) {
    // Player vs enemy bullets
    auto players = reg.view<Transform2D, CircleHitbox, Player, Health>();
    auto enemy_bullets = reg.view<Transform2D, CircleHitbox, Bullet, DamageOnContact>();

    for (auto [p_ent, p_tf, p_hb, player, p_hp] : players.each()) {
        // Skip if invulnerable
        if (auto* inv = reg.try_get<Invulnerable>(p_ent)) {
            if (inv->remaining > 0.f)
                continue;
        }

        for (auto [b_ent, b_tf, b_hb, bullet, dmg] : enemy_bullets.each()) {
            if (bullet.owner != Bullet::Owner::Enemy)
                continue;

            if (circles_overlap(p_tf.x + p_hb.offset_x, p_tf.y + p_hb.offset_y, p_hb.radius,
                                b_tf.x + b_hb.offset_x, b_tf.y + b_hb.offset_y, b_hb.radius)) {
                p_hp.current -= dmg.damage;
                reg.destroy(b_ent);

                // Grant invulnerability frames
                reg.emplace_or_replace<Invulnerable>(p_ent, 2.f);
                break; // one hit per frame
            }
        }
    }

    // TODO: Player bullets vs enemies (similar pattern with RectHitbox)
    // TODO: Spatial hash for broad-phase optimization
}

} // namespace raven::systems
