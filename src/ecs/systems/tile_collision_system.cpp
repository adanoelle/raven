#include "ecs/systems/tile_collision_system.hpp"

#include "ecs/components.hpp"

namespace raven::systems {

void update_tile_collision(entt::registry& reg, const Tilemap& tilemap) {
    if (!tilemap.is_loaded()) {
        return;
    }

    auto view = reg.view<Transform2D, PreviousTransform, Velocity, RectHitbox>();
    for (auto [entity, tf, prev, vel, hb] : view.each()) {
        float box_x = tf.x + hb.offset_x - hb.width / 2.f;
        float box_y = tf.y + hb.offset_y - hb.height / 2.f;

        if (!tilemap.is_solid(box_x, box_y, hb.width, hb.height)) {
            continue;
        }

        // Axis-separated resolution: try reverting each axis independently

        // Try keeping X movement, revert Y
        float try_x = tf.x;
        float try_y = prev.y;
        float try_box_x = try_x + hb.offset_x - hb.width / 2.f;
        float try_box_y = try_y + hb.offset_y - hb.height / 2.f;

        if (!tilemap.is_solid(try_box_x, try_box_y, hb.width, hb.height)) {
            tf.y = prev.y;
            vel.dy = 0.f;
            continue;
        }

        // Try keeping Y movement, revert X
        try_x = prev.x;
        try_y = tf.y;
        try_box_x = try_x + hb.offset_x - hb.width / 2.f;
        try_box_y = try_y + hb.offset_y - hb.height / 2.f;

        if (!tilemap.is_solid(try_box_x, try_box_y, hb.width, hb.height)) {
            tf.x = prev.x;
            vel.dx = 0.f;
            continue;
        }

        // Both axes blocked: revert to previous position
        tf.x = prev.x;
        tf.y = prev.y;
        vel.dx = 0.f;
        vel.dy = 0.f;
    }
}

} // namespace raven::systems
