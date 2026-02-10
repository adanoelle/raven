#include "ecs/systems/damage_system.hpp"

#include "ecs/components.hpp"
#include "ecs/systems/pickup_system.hpp"

#include <spdlog/spdlog.h>

#include <random>
#include <vector>

namespace raven::systems {

void update_damage(entt::registry& reg, const PatternLibrary& patterns) {
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

                // Spawn weapon pickup if enemy had a bullet emitter
                if (auto* emitter = reg.try_get<BulletEmitter>(entity)) {
                    auto* tf = reg.try_get<Transform2D>(entity);
                    if (tf && !emitter->pattern_name.empty()) {
                        const auto* pattern = patterns.get(emitter->pattern_name);
                        if (pattern && !pattern->emitters.empty()) {
                            auto pickup_ent = reg.create();
                            reg.emplace<Transform2D>(pickup_ent, tf->x, tf->y);
                            reg.emplace<CircleHitbox>(pickup_ent, 8.f);
                            reg.emplace<Lifetime>(pickup_ent, 5.f);
                            auto weapon = weapon_from_emitter(pattern->emitters[0]);
                            weapon.tier = pattern->tier;
                            reg.emplace<WeaponPickup>(pickup_ent, WeaponPickup{std::move(weapon)});
                        }
                    }
                }

                // Spawn stabilizer pickup based on enemy type
                if (auto* enemy_comp = reg.try_get<Enemy>(entity)) {
                    auto* tf = reg.try_get<Transform2D>(entity);
                    if (tf) {
                        bool spawn_stabilizer = false;
                        if (enemy_comp->type == Enemy::Type::Boss) {
                            spawn_stabilizer = true;
                        } else if (enemy_comp->type == Enemy::Type::Mid) {
                            auto* rng = reg.ctx().find<std::mt19937>();
                            if (rng) {
                                std::uniform_real_distribution<float> dist(0.f, 1.f);
                                spawn_stabilizer = dist(*rng) < 0.15f;
                            }
                        }
                        // Grunt: never drops stabilizer

                        if (spawn_stabilizer) {
                            auto stab_ent = reg.create();
                            reg.emplace<Transform2D>(stab_ent, tf->x, tf->y + 12.f);
                            reg.emplace<CircleHitbox>(stab_ent, 8.f);
                            reg.emplace<Lifetime>(stab_ent, 8.f);
                            reg.emplace<StabilizerPickup>(stab_ent);
                        }
                    }
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
