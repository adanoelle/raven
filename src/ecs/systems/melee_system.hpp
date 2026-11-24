#pragma once

#include "core/input.hpp"
#include "patterns/pattern_library.hpp"
#include "rendering/tilemap.hpp"

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Process melee attacks: arc hitbox, enemy disarm, and weapon pickup spawning.
///
/// Ticks MeleeCooldown, spawns MeleeAttack on melee_pressed, performs a
/// cone-shaped hit check against enemies, applies damage and knockback,
/// disarms enemies with BulletEmitters (removing the emitter and spawning
/// a WeaponPickup), and removes expired MeleeAttack components. An enemy
/// with a wall between it and the attacker is not hit.
/// @param reg The ECS registry.
/// @param input The current frame's input state snapshot.
/// @param tilemap The tilemap whose walls block the swing.
/// @param patterns The loaded pattern library (for deriving weapon pickups).
/// @param dt Fixed timestep delta in seconds.
void update_melee(entt::registry& reg, const InputState& input, const Tilemap& tilemap,
                  const PatternLibrary& patterns, float dt);

} // namespace raven::systems
