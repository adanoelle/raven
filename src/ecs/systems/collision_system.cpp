#include "ecs/systems/collision_system.hpp"

#include "ecs/components.hpp"
#include "ecs/systems/hitbox_math.hpp"

#include <vector>

namespace raven::systems {

void update_collision(entt::registry& reg) {
    // Player vs enemy bullets
    auto players = reg.view<Transform2D, CircleHitbox, Player, Health>();
    auto enemy_bullets = reg.view<Transform2D, CircleHitbox, Bullet, DamageOnContact>();

    // Collect bullets to destroy after iteration (avoid invalidating views)
    std::vector<entt::entity> enemy_bullets_to_destroy;

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
                enemy_bullets_to_destroy.push_back(b_ent);

                // Grant invulnerability frames
                reg.emplace_or_replace<Invulnerable>(p_ent, 2.f);
                break; // one hit per frame
            }
        }
    }

    for (auto b_ent : enemy_bullets_to_destroy) {
        if (reg.valid(b_ent)) {
            reg.destroy(b_ent);
        }
    }

    // Player bullets vs enemies
    auto player_bullets = reg.view<Transform2D, CircleHitbox, Bullet, DamageOnContact>();
    auto enemies = reg.view<Transform2D, CircleHitbox, Enemy, Health>();

    // Collect bullets to destroy after iteration (avoid invalidating views)
    std::vector<entt::entity> bullets_to_destroy;

    for (auto [b_ent, b_tf, b_hb, bullet, dmg] : player_bullets.each()) {
        if (bullet.owner != Bullet::Owner::Player)
            continue;

        for (auto [e_ent, e_tf, e_hb, enemy, e_hp] : enemies.each()) {
            if (circles_overlap(b_tf.x + b_hb.offset_x, b_tf.y + b_hb.offset_y, b_hb.radius,
                                e_tf.x + e_hb.offset_x, e_tf.y + e_hb.offset_y, e_hb.radius)) {
                e_hp.current -= dmg.damage;

                if (!reg.any_of<Piercing>(b_ent)) {
                    bullets_to_destroy.push_back(b_ent);
                    break; // non-piercing: one hit then destroy
                }
            }
        }
    }

    for (auto b_ent : bullets_to_destroy) {
        if (reg.valid(b_ent)) {
            reg.destroy(b_ent);
        }
    }
}

} // namespace raven::systems
