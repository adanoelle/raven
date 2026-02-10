#include "ecs/systems/pickup_system.hpp"

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

void update_pickups(entt::registry& reg) {
    auto players = reg.view<Transform2D, CircleHitbox, Player, Weapon>();
    auto pickups = reg.view<Transform2D, CircleHitbox, WeaponPickup>();

    std::vector<entt::entity> pickups_to_destroy;

    for (auto [p_ent, p_tf, p_hb, player, weapon] : players.each()) {
        for (auto [pk_ent, pk_tf, pk_hb, pickup] : pickups.each()) {
            if (circles_overlap(p_tf.x + p_hb.offset_x, p_tf.y + p_hb.offset_y, p_hb.radius,
                                pk_tf.x + pk_hb.offset_x, pk_tf.y + pk_hb.offset_y, pk_hb.radius)) {
                // Save current weapon if not already decaying
                if (!reg.any_of<WeaponDecay>(p_ent)) {
                    reg.emplace_or_replace<DefaultWeapon>(p_ent, DefaultWeapon{weapon});
                }

                // Equip the pickup's weapon
                weapon = pickup.weapon;
                reg.emplace_or_replace<WeaponDecay>(p_ent, 10.f);

                pickups_to_destroy.push_back(pk_ent);
                break; // one pickup per frame
            }
        }
    }

    for (auto pk_ent : pickups_to_destroy) {
        if (reg.valid(pk_ent)) {
            reg.destroy(pk_ent);
        }
    }

    // Stabilizer pickup collection
    auto stabilizers = reg.view<Transform2D, CircleHitbox, StabilizerPickup>();
    std::vector<entt::entity> stabilizers_to_destroy;

    for (auto [p_ent, p_tf, p_hb, player, weapon] : players.each()) {
        if (!reg.any_of<WeaponDecay>(p_ent))
            continue;
        if (weapon.tier == Weapon::Tier::Legendary)
            continue;

        for (auto [s_ent, s_tf, s_hb] : stabilizers.each()) {
            if (circles_overlap(p_tf.x + p_hb.offset_x, p_tf.y + p_hb.offset_y, p_hb.radius,
                                s_tf.x + s_hb.offset_x, s_tf.y + s_hb.offset_y, s_hb.radius)) {
                reg.remove<WeaponDecay>(p_ent);
                if (reg.any_of<DefaultWeapon>(p_ent)) {
                    reg.remove<DefaultWeapon>(p_ent);
                }
                stabilizers_to_destroy.push_back(s_ent);
                break; // one stabilizer per frame
            }
        }
    }

    for (auto s_ent : stabilizers_to_destroy) {
        if (reg.valid(s_ent)) {
            reg.destroy(s_ent);
        }
    }
}

void update_weapon_decay(entt::registry& reg, float dt) {
    auto view = reg.view<WeaponDecay, Weapon>();

    std::vector<entt::entity> expired;

    for (auto [entity, decay, weapon] : view.each()) {
        decay.remaining -= dt;
        if (decay.remaining <= 0.f) {
            // Explosion damage if player is not invulnerable
            if (reg.any_of<Player>(entity) && !reg.any_of<Invulnerable>(entity)) {
                if (auto* hp = reg.try_get<Health>(entity)) {
                    hp->current -= 1.f;
                }
                reg.emplace_or_replace<Invulnerable>(entity, 2.f);
            }

            // Spawn explosion VFX at entity position
            if (auto* tf = reg.try_get<Transform2D>(entity)) {
                auto vfx = reg.create();
                reg.emplace<Transform2D>(vfx, tf->x, tf->y);
                reg.emplace<Lifetime>(vfx, 0.5f);
                reg.emplace<ExplosionVfx>(vfx);
            }

            // Revert to default weapon
            if (auto* def = reg.try_get<DefaultWeapon>(entity)) {
                weapon = def->weapon;
                reg.remove<DefaultWeapon>(entity);
            }
            expired.push_back(entity);
        }
    }

    for (auto entity : expired) {
        if (reg.valid(entity) && reg.any_of<WeaponDecay>(entity)) {
            reg.remove<WeaponDecay>(entity);
        }
    }
}

Weapon weapon_from_emitter(const EmitterDef& emitter) {
    Weapon weapon;
    weapon.bullet_speed = emitter.speed;
    weapon.bullet_damage = emitter.damage;
    weapon.bullet_lifetime = emitter.lifetime;
    weapon.bullet_hitbox = emitter.hitbox_radius;
    weapon.fire_rate = emitter.fire_rate;
    weapon.bullet_count = emitter.count;
    weapon.spread_angle = emitter.spread_angle;
    weapon.bullet_sheet = emitter.bullet_sheet;
    weapon.bullet_frame_x = emitter.bullet_frame_x;
    weapon.bullet_frame_y = emitter.bullet_frame_y;
    weapon.bullet_width = emitter.bullet_width;
    weapon.bullet_height = emitter.bullet_height;
    return weapon;
}

} // namespace raven::systems
