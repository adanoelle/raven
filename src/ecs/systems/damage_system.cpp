#include "ecs/systems/damage_system.hpp"

#include "core/string_id.hpp"
#include "ecs/components.hpp"
#include "ecs/systems/pickup_system.hpp"

#include <spdlog/spdlog.h>

#include <random>
#include <vector>

namespace {

void tick_invulnerability(entt::registry& reg, float dt) {
    auto inv_view = reg.view<raven::Invulnerable>();
    for (auto [entity, inv] : inv_view.each()) {
        inv.remaining -= dt;
        if (inv.remaining <= 0.f) {
            reg.remove<raven::Invulnerable>(entity);
        }
    }
}

void handle_player_death(entt::registry& reg, entt::entity entity, raven::Health& hp,
                         raven::Player& player) {
    player.lives--;
    if (player.lives > 0) {
        hp.current = hp.max;
        reg.emplace_or_replace<raven::Invulnerable>(entity, 3.f);
        spdlog::info("Player died, {} lives remaining", player.lives);
    } else {
        spdlog::info("Game over!");
        // TODO: transition to game over scene
    }
}

void handle_enemy_death(entt::registry& reg, entt::entity entity,
                        const raven::PatternLibrary& patterns, raven::StringInterner& interner) {
    if (auto* score = reg.try_get<raven::ScoreValue>(entity)) {
        // TODO: add to score counter
        spdlog::debug("Enemy destroyed, +{} points", score->points);
    }

    // Spawn weapon pickup if enemy had a bullet emitter
    if (auto* emitter = reg.try_get<raven::BulletEmitter>(entity)) {
        auto* tf = reg.try_get<raven::Transform2D>(entity);
        if (tf && emitter->pattern_name.valid()) {
            const auto* pattern = patterns.get(interner.resolve(emitter->pattern_name));
            if (pattern && !pattern->emitters.empty()) {
                auto pickup_ent = reg.create();
                reg.emplace<raven::Transform2D>(pickup_ent, tf->x, tf->y);
                reg.emplace<raven::PreviousTransform>(pickup_ent, tf->x, tf->y);
                reg.emplace<raven::CircleHitbox>(pickup_ent, 8.f);
                reg.emplace<raven::Lifetime>(pickup_ent, 5.f);
                reg.emplace<raven::Sprite>(pickup_ent, interner.intern("pickups"), 0, 0, 16, 16, 5);
                auto weapon = raven::systems::weapon_from_emitter(pattern->emitters[0]);
                weapon.tier = pattern->tier;
                reg.emplace<raven::WeaponPickup>(pickup_ent,
                                                 raven::WeaponPickup{std::move(weapon)});
            }
        }
    }

    // Spawn stabilizer pickup based on enemy type
    if (auto* enemy_comp = reg.try_get<raven::Enemy>(entity)) {
        auto* tf = reg.try_get<raven::Transform2D>(entity);
        if (tf) {
            bool spawn_stabilizer = false;
            if (enemy_comp->type == raven::Enemy::Type::Boss) {
                spawn_stabilizer = true;
            } else if (enemy_comp->type == raven::Enemy::Type::Mid) {
                auto* rng = reg.ctx().find<std::mt19937>();
                if (rng) {
                    std::uniform_real_distribution<float> dist(0.f, 1.f);
                    spawn_stabilizer = dist(*rng) < 0.15f;
                }
            }
            // Grunt: never drops stabilizer

            if (spawn_stabilizer) {
                auto stab_ent = reg.create();
                reg.emplace<raven::Transform2D>(stab_ent, tf->x, tf->y + 12.f);
                reg.emplace<raven::PreviousTransform>(stab_ent, tf->x, tf->y + 12.f);
                reg.emplace<raven::CircleHitbox>(stab_ent, 8.f);
                reg.emplace<raven::Lifetime>(stab_ent, 8.f);
                reg.emplace<raven::Sprite>(stab_ent, interner.intern("pickups"), 1, 0, 16, 16, 5);
                reg.emplace<raven::StabilizerPickup>(stab_ent);
            }
        }
    }
}

} // namespace

namespace raven::systems {

void update_damage(entt::registry& reg, const PatternLibrary& patterns, float dt) {
    auto& interner = reg.ctx().get<StringInterner>();

    tick_invulnerability(reg, dt);

    // Check for dead entities
    std::vector<entt::entity> to_destroy;
    auto health_view = reg.view<Health>();
    for (auto [entity, hp] : health_view.each()) {
        if (hp.current <= 0.f) {
            if (auto* player = reg.try_get<Player>(entity)) {
                handle_player_death(reg, entity, hp, *player);
            } else {
                handle_enemy_death(reg, entity, patterns, interner);
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
