#include "ecs/systems/cleanup_system.hpp"
#include "ecs/components.hpp"

#include <vector>

namespace raven::systems {

void update_cleanup(entt::registry& reg, int screen_w, int screen_h) {
    std::vector<entt::entity> to_destroy;

    // Remove entities past their lifetime
    auto lifetime_view = reg.view<Lifetime>();
    for (auto [entity, life] : lifetime_view.each()) {
        if (life.remaining <= 0.f) {
            to_destroy.push_back(entity);
        }
    }

    // Remove off-screen entities tagged for despawn
    constexpr float MARGIN = 32.f;
    auto offscreen_view = reg.view<Transform2D, OffScreenDespawn>();
    for (auto [entity, tf] : offscreen_view.each()) {
        if (tf.x < -MARGIN || tf.x > static_cast<float>(screen_w) + MARGIN ||
            tf.y < -MARGIN || tf.y > static_cast<float>(screen_h) + MARGIN) {
            to_destroy.push_back(entity);
        }
    }

    for (auto entity : to_destroy) {
        if (reg.valid(entity)) {
            reg.destroy(entity);
        }
    }
}

} // namespace raven::systems
