#include "rendering/sprite_sheet.hpp"

#include <SDL3_image/SDL_image.h>
#include <spdlog/spdlog.h>

namespace raven {

SpriteSheet::~SpriteSheet() {
    if (texture_) {
        SDL_DestroyTexture(texture_);
    }
}

bool SpriteSheet::load(SDL_Renderer* renderer, const std::string& path, int frame_width,
                       int frame_height) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        spdlog::error("Failed to load sprite sheet '{}': {}", path, SDL_GetError());
        return false;
    }

    texture_ = SDL_CreateTextureFromSurface(renderer, surface);
    sheet_w_ = surface->w;
    sheet_h_ = surface->h;
    SDL_DestroySurface(surface);

    if (!texture_) {
        spdlog::error("Failed to create texture from '{}': {}", path, SDL_GetError());
        return false;
    }

    SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_PIXELART);

    frame_w_ = frame_width;
    frame_h_ = frame_height;

    spdlog::debug("Loaded sprite sheet '{}': {}x{}, frames {}x{}", path, sheet_w_, sheet_h_,
                  frame_w_, frame_h_);

    return true;
}

void SpriteSheet::draw(SDL_Renderer* renderer, int frame_x, int frame_y, int dest_x, int dest_y,
                       bool flip_x) const {
    if (!texture_)
        return;

    SDL_FRect src{static_cast<float>(frame_x * frame_w_), static_cast<float>(frame_y * frame_h_),
                  static_cast<float>(frame_w_), static_cast<float>(frame_h_)};

    SDL_FRect dst{static_cast<float>(dest_x), static_cast<float>(dest_y),
                  static_cast<float>(frame_w_), static_cast<float>(frame_h_)};

    SDL_FlipMode flip = flip_x ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderTextureRotated(renderer, texture_, &src, &dst, 0.0, nullptr, flip);
}

// ── SpriteSheetManager ───────────────────────────────────────────

bool SpriteSheetManager::load(SDL_Renderer* renderer, const std::string& id,
                              const std::string& path, int frame_w, int frame_h) {
    auto sheet = std::make_unique<SpriteSheet>();
    if (!sheet->load(renderer, path, frame_w, frame_h)) {
        return false;
    }
    sheets_[id] = std::move(sheet);
    return true;
}

const SpriteSheet* SpriteSheetManager::get(const std::string& id) const {
    auto it = sheets_.find(id);
    return it != sheets_.end() ? it->second.get() : nullptr;
}

} // namespace raven
