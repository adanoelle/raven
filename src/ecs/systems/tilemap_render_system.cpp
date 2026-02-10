#include "ecs/systems/tilemap_render_system.hpp"

namespace raven::systems {

void render_tilemap(const Tilemap& tilemap, SDL_Renderer* renderer) {
    if (!tilemap.is_loaded() || !tilemap.texture()) {
        return;
    }

    SDL_Texture* tex = tilemap.texture();
    for (const auto& tile : tilemap.tiles()) {
        SDL_Rect dest{tile.dest_x, tile.dest_y, tile.src.w, tile.src.h};

        int flip = SDL_FLIP_NONE;
        if (tile.flip_x) {
            flip |= SDL_FLIP_HORIZONTAL;
        }
        if (tile.flip_y) {
            flip |= SDL_FLIP_VERTICAL;
        }

        SDL_Rect src = tile.src;
        SDL_RenderCopyEx(renderer, tex, &src, &dest, 0.0, nullptr,
                         static_cast<SDL_RendererFlip>(flip));
    }
}

} // namespace raven::systems
