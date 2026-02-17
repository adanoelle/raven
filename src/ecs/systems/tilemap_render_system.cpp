#include "ecs/systems/tilemap_render_system.hpp"

namespace raven::systems {

void render_tilemap(const Tilemap& tilemap, SDL_Renderer* renderer) {
    if (!tilemap.is_loaded() || !tilemap.texture()) {
        return;
    }

    SDL_Texture* tex = tilemap.texture();
    for (const auto& tile : tilemap.tiles()) {
        SDL_FRect dest{static_cast<float>(tile.dest_x), static_cast<float>(tile.dest_y),
                       static_cast<float>(tile.src.w), static_cast<float>(tile.src.h)};

        SDL_FlipMode flip = SDL_FLIP_NONE;
        if (tile.flip_x) {
            flip = static_cast<SDL_FlipMode>(flip | SDL_FLIP_HORIZONTAL);
        }
        if (tile.flip_y) {
            flip = static_cast<SDL_FlipMode>(flip | SDL_FLIP_VERTICAL);
        }

        SDL_FRect src{static_cast<float>(tile.src.x), static_cast<float>(tile.src.y),
                      static_cast<float>(tile.src.w), static_cast<float>(tile.src.h)};
        SDL_RenderTextureRotated(renderer, tex, &src, &dest, 0.0, nullptr, flip);
    }
}

} // namespace raven::systems
