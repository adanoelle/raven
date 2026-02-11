#pragma once

#include <SDL2/SDL.h>
#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Render the in-game HUD overlay (health bar, lives, score, decay timer, wave indicator).
/// @param reg The ECS registry containing player and game state.
/// @param renderer SDL renderer for drawing primitives.
void render_hud(entt::registry& reg, SDL_Renderer* renderer);

} // namespace raven::systems
