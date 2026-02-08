#pragma once

#include "rendering/sprite_sheet.hpp"

#include <SDL2/SDL.h>
#include <entt/entt.hpp>

namespace raven::systems {

/// @brief Render all entities that have Sprite and Transform2D components.
///
/// Sorts entities by Sprite::layer before drawing so higher layers
/// render on top. Reads Transform2D for position and Sprite for the
/// sheet ID and frame indices.
/// @param reg The ECS registry containing renderable entities.
/// @param renderer The SDL_Renderer to draw with.
/// @param sprites The SpriteSheetManager providing loaded textures.
void render_sprites(entt::registry& reg, SDL_Renderer* renderer,
                    const SpriteSheetManager& sprites);

} // namespace raven::systems
