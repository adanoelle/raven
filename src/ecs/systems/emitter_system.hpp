#pragma once

#include "patterns/pattern_library.hpp"

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Tick bullet emitters and spawn enemy bullets from pattern definitions.
///
/// For each entity with a BulletEmitter component, looks up the referenced
/// PatternDef, advances rotation and cooldown timers, and spawns bullets
/// on cooldown expiry.
/// @param reg The ECS registry containing emitter entities.
/// @param patterns The loaded pattern library.
/// @param dt Fixed timestep delta in seconds.
void update_emitters(entt::registry& reg, const PatternLibrary& patterns, float dt);

} // namespace raven::systems
