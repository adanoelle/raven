#include "ecs/systems/ai_system.hpp"

#include "ecs/components.hpp"
#include "ecs/systems/hitbox_math.hpp"
#include "ecs/systems/player_utils.hpp"

#include <cmath>
#include <random>

namespace raven::systems {

namespace {

/// @brief Normalize a 2D vector in-place.
/// @param x X component (modified in-place).
/// @param y Y component (modified in-place).
/// @return The original length of the vector.
float normalize(float& x, float& y) {
    float len = std::sqrt(x * x + y * y);
    if (len > 0.f) {
        x /= len;
        y /= len;
    }
    return len;
}

/// @brief Check line-of-sight between two points using the tilemap.
///
/// Steps along the line in half-cell increments and checks for solid cells.
/// @param tilemap The tilemap with collision data.
/// @param x1 Start X position.
/// @param y1 Start Y position.
/// @param x2 End X position.
/// @param y2 End Y position.
/// @return True if there is an unobstructed line of sight.
bool has_line_of_sight(const Tilemap& tilemap, float x1, float y1, float x2, float y2) {
    if (!tilemap.is_loaded()) {
        return true;
    }

    float dx = x2 - x1;
    float dy = y2 - y1;
    float dist = normalize(dx, dy);

    float step_size = static_cast<float>(tilemap.cell_size()) * 0.5f;
    int steps = static_cast<int>(dist / step_size);

    for (int i = 1; i <= steps; ++i) {
        float px = x1 + dx * step_size * static_cast<float>(i);
        float py = y1 + dy * step_size * static_cast<float>(i);
        int gx = static_cast<int>(px) / tilemap.cell_size();
        int gy = static_cast<int>(py) / tilemap.cell_size();
        if (tilemap.is_cell_solid(gx, gy)) {
            return false;
        }
    }
    return true;
}

/// @brief Update a Chaser enemy: beeline toward the player.
void update_chaser(Velocity& vel, const AiBehavior& ai, float dir_x, float dir_y) {
    vel.dx = dir_x * ai.move_speed;
    vel.dy = dir_y * ai.move_speed;
}

/// @brief Update a Drifter enemy: wander randomly, changing direction on timer.
void update_drifter(Velocity& vel, AiBehavior& ai, float dt, std::mt19937& rng) {
    ai.phase_timer -= dt;
    if (ai.phase_timer <= 0.f) {
        std::uniform_real_distribution<float> angle_dist(0.f, 2.f * 3.14159265358979323846f);
        std::uniform_real_distribution<float> interval_dist(1.f, 2.5f);
        float angle = angle_dist(rng);
        vel.dx = std::cos(angle) * ai.move_speed;
        vel.dy = std::sin(angle) * ai.move_speed;
        ai.phase_timer = interval_dist(rng);
    }
}

/// @brief Update a Stalker enemy: approach, strafe at preferred range, retreat if too close.
void update_stalker(Velocity& vel, AiBehavior& ai, float dir_x, float dir_y, float dist, float dt) {
    float retreat_threshold = ai.preferred_range * 0.5f;

    switch (ai.phase) {
    case AiBehavior::Phase::Advance:
        if (dist <= ai.preferred_range) {
            ai.phase = AiBehavior::Phase::Attack;
            ai.phase_timer = 0.f;
            break;
        }
        vel.dx = dir_x * ai.move_speed;
        vel.dy = dir_y * ai.move_speed;
        break;

    case AiBehavior::Phase::Attack: {
        // Strafe perpendicular to the player direction
        float perp_x = -dir_y * ai.strafe_dir;
        float perp_y = dir_x * ai.strafe_dir;

        // Slight range correction to maintain preferred distance
        float range_correction = (dist - ai.preferred_range) * 0.5f;
        vel.dx = (perp_x + dir_x * range_correction / ai.move_speed) * ai.move_speed;
        vel.dy = (perp_y + dir_y * range_correction / ai.move_speed) * ai.move_speed;

        // Periodically reverse strafe direction
        ai.phase_timer += dt;
        if (ai.phase_timer >= 2.f) {
            ai.strafe_dir = -ai.strafe_dir;
            ai.phase_timer = 0.f;
        }

        // Retreat if player closes in
        if (dist < retreat_threshold) {
            ai.phase = AiBehavior::Phase::Retreat;
            ai.phase_timer = 0.f;
        }
        break;
    }

    case AiBehavior::Phase::Retreat:
        vel.dx = -dir_x * ai.move_speed;
        vel.dy = -dir_y * ai.move_speed;
        ai.phase_timer += dt;
        if (ai.phase_timer >= 0.5f) {
            ai.phase = AiBehavior::Phase::Attack;
            ai.phase_timer = 0.f;
        }
        break;

    default: // Idle handled by caller
        break;
    }
}

/// @brief Check if a position is blocked by a solid tile.
/// @param tilemap The tilemap with collision data.
/// @param x Probe X position in world pixels.
/// @param y Probe Y position in world pixels.
/// @return True if the cell at that position is solid.
bool is_probe_blocked(const Tilemap& tilemap, float x, float y) {
    if (!tilemap.is_loaded()) {
        return false;
    }
    int gx = static_cast<int>(x) / tilemap.cell_size();
    int gy = static_cast<int>(y) / tilemap.cell_size();
    return tilemap.is_cell_solid(gx, gy);
}

/// @brief Update a Coward enemy: flee away from the player, wall-sliding to avoid corners.
void update_coward(Velocity& vel, AiBehavior& ai, float dir_x, float dir_y, float entity_x,
                   float entity_y, const Tilemap& tilemap) {
    float flee_x = -dir_x;
    float flee_y = -dir_y;

    // Probe one cell ahead in the flee direction
    float probe_dist = tilemap.is_loaded() ? static_cast<float>(tilemap.cell_size()) : 16.f;
    float probe_x = entity_x + flee_x * probe_dist;
    float probe_y = entity_y + flee_y * probe_dist;

    if (!is_probe_blocked(tilemap, probe_x, probe_y)) {
        // Clear path: flee directly
        vel.dx = flee_x * ai.move_speed;
        vel.dy = flee_y * ai.move_speed;
        return;
    }

    // Blocked: try the two perpendicular directions, prefer the one
    // that has a larger component away from the player
    float perp1_x = -flee_y;
    float perp1_y = flee_x;
    float perp2_x = flee_y;
    float perp2_y = -flee_x;

    bool p1_blocked =
        is_probe_blocked(tilemap, entity_x + perp1_x * probe_dist, entity_y + perp1_y * probe_dist);
    bool p2_blocked =
        is_probe_blocked(tilemap, entity_x + perp2_x * probe_dist, entity_y + perp2_y * probe_dist);

    if (!p1_blocked && !p2_blocked) {
        // Both open: pick the one with a larger flee-from-player component
        float dot1 = perp1_x * flee_x + perp1_y * flee_y;
        float dot2 = perp2_x * flee_x + perp2_y * flee_y;
        if (dot1 >= dot2) {
            vel.dx = perp1_x * ai.move_speed;
            vel.dy = perp1_y * ai.move_speed;
        } else {
            vel.dx = perp2_x * ai.move_speed;
            vel.dy = perp2_y * ai.move_speed;
        }
    } else if (!p1_blocked) {
        vel.dx = perp1_x * ai.move_speed;
        vel.dy = perp1_y * ai.move_speed;
    } else if (!p2_blocked) {
        vel.dx = perp2_x * ai.move_speed;
        vel.dy = perp2_y * ai.move_speed;
    } else {
        // Fully cornered: reverse toward player to escape
        vel.dx = dir_x * ai.move_speed;
        vel.dy = dir_y * ai.move_speed;
    }
}

} // anonymous namespace

void update_ai(entt::registry& reg, const Tilemap& tilemap, float dt) {
    float player_x = 0.f;
    float player_y = 0.f;
    if (!find_player_position(reg, player_x, player_y)) {
        return;
    }

    auto* rng = reg.ctx().find<std::mt19937>();

    auto view = reg.view<Transform2D, Velocity, AiBehavior>();
    for (auto [entity, tf, vel, ai] : view.each()) {
        // Knockback overrides AI
        if (auto* kb = reg.try_get<Knockback>(entity)) {
            vel.dx = kb->dx;
            vel.dy = kb->dy;
            kb->remaining -= dt;
            if (kb->remaining <= 0.f) {
                reg.remove<Knockback>(entity);
            }
            continue;
        }

        float dx = player_x - tf.x;
        float dy = player_y - tf.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        // Normalized direction toward player
        float dir_x = 0.f;
        float dir_y = 0.f;
        if (dist > 0.f) {
            dir_x = dx / dist;
            dir_y = dy / dist;
        }

        // Idle: wait for activation
        if (ai.phase == AiBehavior::Phase::Idle) {
            if (dist > ai.activation_range ||
                !has_line_of_sight(tilemap, tf.x, tf.y, player_x, player_y)) {
                vel.dx = 0.f;
                vel.dy = 0.f;
                // Emitter off while idle
                if (auto* emitter = reg.try_get<BulletEmitter>(entity)) {
                    emitter->active = false;
                }
                continue;
            }
            // Activate
            ai.phase = AiBehavior::Phase::Advance;
        }

        // Dispatch to archetype handler
        switch (ai.archetype) {
        case AiBehavior::Archetype::Chaser:
            update_chaser(vel, ai, dir_x, dir_y);
            break;
        case AiBehavior::Archetype::Drifter:
            if (rng) {
                update_drifter(vel, ai, dt, *rng);
            }
            break;
        case AiBehavior::Archetype::Stalker:
            update_stalker(vel, ai, dir_x, dir_y, dist, dt);
            break;
        case AiBehavior::Archetype::Coward:
            update_coward(vel, ai, dir_x, dir_y, tf.x, tf.y, tilemap);
            break;
        }

        // Disarmed enemies become aggressive Chasers
        if (reg.any_of<Disarmed>(entity)) {
            vel.dx = dir_x * ai.move_speed * 1.5f;
            vel.dy = dir_y * ai.move_speed * 1.5f;
        }

        // Toggle emitter based on attack range
        if (auto* emitter = reg.try_get<BulletEmitter>(entity)) {
            bool in_attack_range = dist <= ai.attack_range;
            // Coward always fires; others respect attack_range
            emitter->active = (ai.archetype == AiBehavior::Archetype::Coward) || in_attack_range;
        }
    }

    // Tick all contact damage cooldowns independently
    auto contact_tick_view = reg.view<ContactDamage>();
    for (auto [e_ent, contact] : contact_tick_view.each()) {
        contact.timer -= dt;
    }

    // Contact damage vs player
    auto players = reg.view<Transform2D, CircleHitbox, Player, Health>();
    auto contact_view = reg.view<Transform2D, CircleHitbox, ContactDamage>();

    for (auto [p_ent, p_tf, p_hb, player, p_hp] : players.each()) {
        // Skip if invulnerable
        if (auto* inv = reg.try_get<Invulnerable>(p_ent)) {
            if (inv->remaining > 0.f) {
                continue;
            }
        }

        for (auto [e_ent, e_tf, e_hb, contact] : contact_view.each()) {
            if (contact.timer > 0.f) {
                continue;
            }

            if (circles_overlap(p_tf.x + p_hb.offset_x, p_tf.y + p_hb.offset_y, p_hb.radius,
                                e_tf.x + e_hb.offset_x, e_tf.y + e_hb.offset_y, e_hb.radius)) {
                p_hp.current -= contact.damage;
                contact.timer = contact.cooldown;
                reg.emplace_or_replace<Invulnerable>(p_ent, 2.f);
                break; // one contact hit per frame
            }
        }
    }
}

} // namespace raven::systems
