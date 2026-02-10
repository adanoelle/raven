#pragma once

#include "ecs/components.hpp"
#include "patterns/pattern_library.hpp"

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Detect player overlap with weapon and stabilizer pickups.
///
/// Weapon pickups: saves the player's current weapon to DefaultWeapon (if not
/// already decaying), sets the player's Weapon to the pickup's weapon,
/// emplaces WeaponDecay, and destroys the pickup entity.
///
/// Stabilizer pickups: if the player has WeaponDecay and the weapon tier is
/// not Legendary, removes WeaponDecay and DefaultWeapon (making the weapon
/// permanent) and destroys the stabilizer entity.
/// @param reg The ECS registry.
void update_pickups(entt::registry& reg);

/// @brief Tick WeaponDecay timers and handle expiry with explosion.
///
/// When remaining reaches zero: if the player is not invulnerable, deals 1
/// damage and grants 2s invulnerability. Spawns an ExplosionVfx entity at
/// the player position. Restores the player's Weapon from DefaultWeapon and
/// removes both WeaponDecay and DefaultWeapon.
/// @param reg The ECS registry.
/// @param dt Fixed timestep delta in seconds.
void update_weapon_decay(entt::registry& reg, float dt);

/// @brief Convert an EmitterDef's stats into a Weapon component.
/// @param emitter The emitter definition to convert.
/// @return A Weapon with properties derived from the emitter.
[[nodiscard]] Weapon weapon_from_emitter(const EmitterDef& emitter);

} // namespace raven::systems
