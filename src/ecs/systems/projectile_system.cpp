#include "ecs/systems/projectile_system.hpp"
#include "ecs/components.hpp"

namespace raven::systems {

void update_projectiles(entt::registry& reg, float dt) {
    // Tick down lifetimes
    auto lifetime_view = reg.view<Lifetime>();
    for (auto [entity, life] : lifetime_view.each()) {
        life.remaining -= dt;
    }
}

} // namespace raven::systems
