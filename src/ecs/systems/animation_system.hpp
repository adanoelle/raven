#pragma once

#include "core/string_id.hpp"
#include "ecs/components.hpp"

#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Switch an Animation to a clip, starting from its first frame.
///
/// Does nothing if the clip is already playing, so it is safe to call
/// every tick.
/// @param anim The Animation to change.
/// @param clip Interned clip name.
void play_clip(Animation& anim, StringId clip);

/// @brief Advance Animation components and write each one's current frame
/// into its Sprite.
///
/// Clips come from the AnimationLibrary in the registry context; without
/// one this does nothing. A clip missing from a sheet that has animation
/// data is logged once and leaves the Sprite unchanged.
/// @param reg The ECS registry containing entities to update.
/// @param dt Fixed timestep delta in seconds (typically 1/120).
void update_animation(entt::registry& reg, float dt);

/// @brief Choose each player's clip from what they are doing, and face the
/// sprite along the aim direction.
///
/// Priority is attack (melee or ground slam), then dash, walk, idle. A
/// one-shot action clip plays to the end even after the action itself
/// ends, so wind-up and follow-through frames always show; a new action
/// interrupts it. When the sheet has no clip for a state, the choice falls
/// back to walk, then idle, and the missing tag is logged once.
/// @param reg The ECS registry containing entities to update.
void update_player_animation(entt::registry& reg);

} // namespace raven::systems
