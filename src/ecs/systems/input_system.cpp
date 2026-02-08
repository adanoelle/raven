#include "ecs/systems/input_system.hpp"
#include "ecs/components.hpp"

#include <cmath>

namespace raven::systems {

void update_input(entt::registry& reg, const InputState& input) {
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

        vel.dx = mx * speed;
        vel.dy = my * speed;
    }
}

} // namespace raven::systems
