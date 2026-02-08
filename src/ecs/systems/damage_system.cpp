#include "ecs/systems/damage_system.hpp"
#include "ecs/components.hpp"

#include <spdlog/spdlog.h>
#include <vector>

namespace raven::systems {

void update_damage(entt::registry& reg) {
    // Tick down invulnerability
    auto inv_view = reg.view<Invulnerable>();
    for (auto [entity, inv] : inv_view.each()) {
        inv.remaining -= 1.f / 120.f; // fixed timestep
        if (inv.remaining <= 0.f) {
            reg.remove<Invulnerable>(entity);
        }
    }

    // Check for dead entities
    std::vector<entt::entity> to_destroy;
    auto health_view = reg.view<Health>();
    for (auto [entity, hp] : health_view.each()) {
        if (hp.current <= 0.f) {
            // Player death handling
            if (auto* player = reg.try_get<Player>(entity)) {
                player->lives--;
                if (player->lives > 0) {
                    hp.current = hp.max;
                    reg.emplace_or_replace<Invulnerable>(entity, 3.f);
                    spdlog::info("Player died, {} lives remaining", player->lives);
                } else {
                    spdlog::info("Game over!");
                    // TODO: transition to game over scene
                }
            } else {
                // Enemy died — collect score, spawn items, etc.
                if (auto* score = reg.try_get<ScoreValue>(entity)) {
                    // TODO: add to score counter
                    spdlog::debug("Enemy destroyed, +{} points", score->points);
                }
                to_destroy.push_back(entity);
            }
        }
    }

    for (auto entity : to_destroy) {
        if (reg.valid(entity)) {
            reg.destroy(entity);
        }
    }
}

} // namespace raven::systems
