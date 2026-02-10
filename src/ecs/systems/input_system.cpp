#include "ecs/systems/input_system.hpp"

#include "ecs/components.hpp"

#include <cmath>

namespace raven::systems {

void update_input(entt::registry& reg, const InputState& input, float dt) {
    auto view = reg.view<Player, Velocity>();

    for (auto [entity, player, vel] : view.each()) {
        float speed = player.speed;

        float mx = input.move_x;
        float my = input.move_y;

        // Normalize diagonal movement
        float len = std::sqrt(mx * mx + my * my);
        if (len > 1.f) {
            mx /= len;
            my /= len;
        }

        float target_dx = mx * speed;
        float target_dy = my * speed;

        // Exponential approach: ~92% in 5 ticks (~42ms), ~98% in 8 ticks (~67ms)
        constexpr float approach_rate = 60.f;
        float t = 1.f - std::exp(-approach_rate * dt);

        vel.dx += (target_dx - vel.dx) * t;
        vel.dy += (target_dy - vel.dy) * t;
    }
}

} // namespace raven::systems
