#include "rendering/bitmap_font.hpp"

#include <SDL3_image/SDL_image.h>
#include <spdlog/spdlog.h>

#include <utility>

namespace raven {

BitmapFont::~BitmapFont() {
    if (texture_) {
        SDL_DestroyTexture(texture_);
    }
}

BitmapFont::BitmapFont(BitmapFont&& other) noexcept
    : texture_(std::exchange(other.texture_, nullptr)), glyph_w_(std::exchange(other.glyph_w_, 0)),
      glyph_h_(std::exchange(other.glyph_h_, 0)) {}

BitmapFont& BitmapFont::operator=(BitmapFont&& other) noexcept {
    if (this != &other) {
        if (texture_) {
            SDL_DestroyTexture(texture_);
        }
        texture_ = std::exchange(other.texture_, nullptr);
        glyph_w_ = std::exchange(other.glyph_w_, 0);
        glyph_h_ = std::exchange(other.glyph_h_, 0);
    }
    return *this;
}

bool BitmapFont::load(SDL_Renderer* renderer, const std::string& path, int glyph_w, int glyph_h) {
    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        spdlog::error("Failed to load font atlas '{}': {}", path, SDL_GetError());
        return false;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    int sheet_w = surface->w;
    int sheet_h = surface->h;
    SDL_DestroySurface(surface);

    if (!texture) {
        spdlog::error("Failed to create font texture from '{}': {}", path, SDL_GetError());
        return false;
    }

    if (texture_) {
        SDL_DestroyTexture(texture_);
    }
    texture_ = texture;

    SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_PIXELART);

    glyph_w_ = glyph_w;
    glyph_h_ = glyph_h;

    spdlog::debug("Loaded font atlas '{}': {}x{}, glyph cell {}x{}", path, sheet_w, sheet_h,
                  glyph_w_, glyph_h_);
    return true;
}

void BitmapFont::draw(SDL_Renderer* renderer, std::string_view text, float x, float y,
                      SDL_Color color, int scale) const {
    if (!texture_ || text.empty() || scale < 1) {
        return;
    }

    SDL_SetTextureColorMod(texture_, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(texture_, color.a);

    const float w = static_cast<float>(glyph_w_ * scale);
    const float h = static_cast<float>(glyph_h_ * scale);
    float pen_x = x;

    for (char c : text) {
        int idx = font_glyph_index(c);
        if (idx < 0) {
            idx = font_glyph_index('?');
        }
        if (c != ' ') {
            SDL_FRect src{static_cast<float>((idx % COLUMNS) * glyph_w_),
                          static_cast<float>((idx / COLUMNS) * glyph_h_),
                          static_cast<float>(glyph_w_), static_cast<float>(glyph_h_)};
            SDL_FRect dst{pen_x, y, w, h};
            SDL_RenderTexture(renderer, texture_, &src, &dst);
        }
        pen_x += w;
    }
}

void BitmapFont::draw_centered(SDL_Renderer* renderer, std::string_view text, float center_x,
                               float y, SDL_Color color, int scale) const {
    draw(renderer, text, center_x - static_cast<float>(measure(text, scale)) / 2.f, y, color,
         scale);
}

} // namespace raven
