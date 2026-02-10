#pragma once

#include "rendering/tilemap.hpp"

#include <SDL2/SDL.h>

namespace raven::systems {

/// @brief Render all pre-baked tiles from a loaded tilemap.
/// @param tilemap The tilemap to render.
/// @param renderer The SDL_Renderer to draw with.
void render_tilemap(const Tilemap& tilemap, SDL_Renderer* renderer);

} // namespace raven::systems
