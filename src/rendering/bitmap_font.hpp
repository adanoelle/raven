#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <string_view>

namespace raven {

/// @brief Map a character to its glyph index in the font atlas.
///
/// The atlas covers printable ASCII (32-126) in row-major order,
/// 16 glyphs per row (see tools/gen_font.py for the layout contract).
/// @param c The character to look up.
/// @return Zero-based glyph index, or -1 if the atlas has no glyph for c.
[[nodiscard]] constexpr int font_glyph_index(char c) {
    auto u = static_cast<unsigned char>(c);
    if (u < 32 || u > 126) {
        return -1;
    }
    return u - 32;
}

/// @brief Monospace bitmap font renderer backed by a glyph atlas texture.
///
/// Glyphs are white in the atlas; draw() tints them per call via SDL
/// color modulation. All drawing is unloaded-safe: a font that failed to
/// load simply draws nothing, matching the sprite fallback philosophy.
class BitmapFont {
  public:
    static constexpr int FIRST_CHAR = 32; ///< First ASCII code in the atlas.
    static constexpr int LAST_CHAR = 126; ///< Last ASCII code in the atlas.
    static constexpr int COLUMNS = 16;    ///< Glyphs per atlas row.

    BitmapFont() = default;
    ~BitmapFont();

    BitmapFont(const BitmapFont&) = delete;
    BitmapFont& operator=(const BitmapFont&) = delete;
    BitmapFont(BitmapFont&& other) noexcept;
    BitmapFont& operator=(BitmapFont&& other) noexcept;

    /// @brief Load the glyph atlas texture.
    /// @param renderer The SDL renderer that will own the texture.
    /// @param path Path to the atlas PNG (use paths::asset()).
    /// @param glyph_w Cell width in pixels, including spacing.
    /// @param glyph_h Cell height in pixels, including spacing.
    /// @return True on success.
    bool load(SDL_Renderer* renderer, const std::string& path, int glyph_w, int glyph_h);

    /// @brief Whether the atlas texture is loaded and drawable.
    /// @return True if load() succeeded.
    [[nodiscard]] bool is_loaded() const { return texture_ != nullptr; }

    /// @brief Cell width in pixels (glyph advance).
    /// @return Glyph cell width, 0 if not loaded.
    [[nodiscard]] int glyph_width() const { return glyph_w_; }

    /// @brief Cell height in pixels (line height).
    /// @return Glyph cell height, 0 if not loaded.
    [[nodiscard]] int glyph_height() const { return glyph_h_; }

    /// @brief Compute the rendered width of a string.
    /// @param text The text to measure.
    /// @param scale Integer pixel scale factor.
    /// @return Width in pixels (monospace: length x cell width x scale).
    [[nodiscard]] int measure(std::string_view text, int scale = 1) const {
        return static_cast<int>(text.size()) * glyph_w_ * scale;
    }

    /// @brief Draw text with its top-left corner at (x, y).
    /// @param renderer The SDL renderer to draw with.
    /// @param text The text to draw; characters outside the atlas draw '?'.
    /// @param x Left edge in virtual-resolution pixels.
    /// @param y Top edge in virtual-resolution pixels.
    /// @param color Tint color (glyphs are white in the atlas).
    /// @param scale Integer pixel scale factor.
    void draw(SDL_Renderer* renderer, std::string_view text, float x, float y,
              SDL_Color color = {255, 255, 255, 255}, int scale = 1) const;

    /// @brief Draw text horizontally centered on center_x.
    /// @param renderer The SDL renderer to draw with.
    /// @param text The text to draw.
    /// @param center_x Horizontal center in virtual-resolution pixels.
    /// @param y Top edge in virtual-resolution pixels.
    /// @param color Tint color.
    /// @param scale Integer pixel scale factor.
    void draw_centered(SDL_Renderer* renderer, std::string_view text, float center_x, float y,
                       SDL_Color color = {255, 255, 255, 255}, int scale = 1) const;

  private:
    SDL_Texture* texture_ = nullptr;
    int glyph_w_ = 0;
    int glyph_h_ = 0;
};

} // namespace raven
