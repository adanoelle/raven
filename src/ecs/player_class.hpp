#pragma once

#include <entt/entt.hpp>

namespace raven {

/// @brief Apply Brawler class stats and components to a player entity.
/// @param reg The ECS registry.
/// @param entity The player entity (must already have universal components).
void apply_brawler(entt::registry& reg, entt::entity entity);

/// @brief Apply Sharpshooter class stats and components to a player entity.
/// @param reg The ECS registry.
/// @param entity The player entity (must already have universal components).
void apply_sharpshooter(entt::registry& reg, entt::entity entity);

} // namespace raven
