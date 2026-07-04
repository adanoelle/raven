#pragma once

#include "rendering/bitmap_font.hpp"

#include <SDL3/SDL.h>
#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Render the in-game HUD overlay (health bar, lives, score, decay timer, wave indicator).
/// @param reg The ECS registry containing player and game state.
/// @param renderer SDL renderer for drawing primitives.
/// @param font Bitmap font for text elements (score).
void render_hud(entt::registry& reg, SDL_Renderer* renderer, const BitmapFont& font);

} // namespace raven::systems
