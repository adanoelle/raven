#include "ecs/systems/emitter_system.hpp"

#include "core/string_id.hpp"
#include "ecs/components.hpp"
#include "ecs/systems/bullet_spawn.hpp"

#include <cmath>

namespace raven::systems {

namespace {

constexpr float PI = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = PI / 180.f;

/// @brief Find the player position, if any player exists.
/// @param reg The ECS registry.
/// @param out_x Output X position.
/// @param out_y Output Y position.
/// @return True if a player was found.
bool find_player_position(const entt::registry& reg, float& out_x, float& out_y) {
    auto view = reg.view<Player, Transform2D>();
    for (auto [entity, player, tf] : view.each()) {
        out_x = tf.x;
        out_y = tf.y;
        return true;
    }
    return false;
}

/// @brief Fire a burst of bullets from an emitter at a given angle.
void fire_burst(entt::registry& reg, const EmitterDef& emitter, float center_angle_deg,
                float origin_x, float origin_y) {
    float center_rad = center_angle_deg * DEG_TO_RAD;

    if (emitter.count <= 1) {
        BulletSpawnParams params;
        params.origin_x = origin_x;
        params.origin_y = origin_y;
        params.angle_rad = center_rad;
        params.speed = emitter.speed;
        params.damage = emitter.damage;
        params.lifetime = emitter.lifetime;
        params.hitbox_radius = emitter.hitbox_radius;
        params.owner = Bullet::Owner::Enemy;
        params.sheet_id = emitter.bullet_sheet;
        params.frame_x = emitter.bullet_frame_x;
        params.frame_y = emitter.bullet_frame_y;
        params.width = emitter.bullet_width;
        params.height = emitter.bullet_height;
        spawn_bullet(reg, params);
        return;
    }

    float spread_rad = emitter.spread_angle * DEG_TO_RAD;
    float step = spread_rad / static_cast<float>(emitter.count);
    float start = center_rad - spread_rad / 2.f;

    for (int i = 0; i < emitter.count; ++i) {
        float angle = start + step * static_cast<float>(i) + step / 2.f;

        BulletSpawnParams params;
        params.origin_x = origin_x;
        params.origin_y = origin_y;
        params.angle_rad = angle;
        params.speed = emitter.speed;
        params.damage = emitter.damage;
        params.lifetime = emitter.lifetime;
        params.hitbox_radius = emitter.hitbox_radius;
        params.owner = Bullet::Owner::Enemy;
        params.sheet_id = emitter.bullet_sheet;
        params.frame_x = emitter.bullet_frame_x;
        params.frame_y = emitter.bullet_frame_y;
        params.width = emitter.bullet_width;
        params.height = emitter.bullet_height;
        spawn_bullet(reg, params);
    }
}

} // anonymous namespace

void update_emitters(entt::registry& reg, const PatternLibrary& patterns, float dt) {
    const auto& interner = reg.ctx().get<StringInterner>();

    float player_x = 0.f;
    float player_y = 0.f;
    bool has_player = find_player_position(reg, player_x, player_y);

    auto view = reg.view<Transform2D, BulletEmitter>();
    for (auto [entity, tf, emitter] : view.each()) {
        if (!emitter.active) {
            continue;
        }

        const auto* pattern = patterns.get(interner.resolve(emitter.pattern_name));
        if (!pattern) {
            continue;
        }

        // Initialize parallel vectors on first use
        auto num_emitters = pattern->emitters.size();
        if (emitter.cooldowns.size() != num_emitters) {
            emitter.cooldowns.resize(num_emitters, 0.f);
            emitter.current_angles.resize(num_emitters);
            for (size_t i = 0; i < num_emitters; ++i) {
                emitter.current_angles[i] = pattern->emitters[i].start_angle;
            }
        }

        for (size_t i = 0; i < num_emitters; ++i) {
            const auto& edef = pattern->emitters[i];

            // Tick rotation
            emitter.current_angles[i] += edef.angular_velocity * dt;

            // Tick cooldown
            emitter.cooldowns[i] -= dt;
            if (emitter.cooldowns[i] > 0.f) {
                continue;
            }
            emitter.cooldowns[i] = edef.fire_rate;

            // Determine center angle for this burst
            float center_angle = emitter.current_angles[i];

            if (edef.type == EmitterDef::Type::Aimed) {
                if (!has_player) {
                    continue;
                }
                float dx = player_x - tf.x;
                float dy = player_y - tf.y;
                center_angle = std::atan2(dy, dx) / DEG_TO_RAD;
            }

            fire_burst(reg, edef, center_angle, tf.x, tf.y);
        }
    }
}

} // namespace raven::systems
