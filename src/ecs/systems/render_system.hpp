#pragma once

#include "rendering/sprite_sheet.hpp"

#include <SDL2/SDL.h>
#include <entt/entt.hpp>

namespace raven::systems {

void render_sprites(entt::registry& reg, SDL_Renderer* renderer,
                    const SpriteSheetManager& sprites);

} // namespace raven::systems
