#include "ecs/systems/movement_system.hpp"

#include "ecs/components.hpp"
#include "rendering/renderer.hpp"

#include <algorithm>

namespace raven::systems {

void update_movement(entt::registry& reg, float dt) {
    // Snapshot positions for render interpolation
    auto interp_view = reg.view<Transform2D, PreviousTransform>();
    for (auto [entity, tf, prev] : interp_view.each()) {
        prev.x = tf.x;
        prev.y = tf.y;
    }

    // Move all entities with velocity
    auto view = reg.view<Transform2D, Velocity>();
    for (auto [entity, tf, vel] : view.each()) {
        tf.x += vel.dx * dt;
        tf.y += vel.dy * dt;
    }

    // Clamp player to screen bounds, by the body's size. The sprite's size
    // is a fallback only: it may be 0, meaning "the sheet's frame size".
    auto players = reg.view<Transform2D, Player>();
    for (auto [entity, tf, player] : players.each()) {
        float half_w = 0.f;
        float half_h = 0.f;
        if (const auto* body = reg.try_get<RectHitbox>(entity)) {
            half_w = body->width / 2.f;
            half_h = body->height / 2.f;
        } else if (const auto* sprite = reg.try_get<Sprite>(entity)) {
            half_w = static_cast<float>(sprite->width) / 2.f;
            half_h = static_cast<float>(sprite->height) / 2.f;
        }

        tf.x = std::clamp(tf.x, half_w, static_cast<float>(Renderer::VIRTUAL_WIDTH) - half_w);
        tf.y = std::clamp(tf.y, half_h, static_cast<float>(Renderer::VIRTUAL_HEIGHT) - half_h);
    }
}

} // namespace raven::systems
