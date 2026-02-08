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

    // Clamp player to screen bounds
    auto players = reg.view<Transform2D, Player, Sprite>();
    for (auto [entity, tf, player, sprite] : players.each()) {
        float half_w = static_cast<float>(sprite.width) / 2.f;
        float half_h = static_cast<float>(sprite.height) / 2.f;

        tf.x = std::clamp(tf.x, half_w,
                           static_cast<float>(Renderer::VIRTUAL_WIDTH) - half_w);
        tf.y = std::clamp(tf.y, half_h,
                           static_cast<float>(Renderer::VIRTUAL_HEIGHT) - half_h);
    }
}

} // namespace raven::systems
