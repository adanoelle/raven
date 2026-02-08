#pragma once

#include <SDL2/SDL.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace raven {

/// @brief Manages a single texture atlas (sprite sheet) with uniform frame sizes.
class SpriteSheet {
public:
    SpriteSheet() = default;
    ~SpriteSheet();

    SpriteSheet(const SpriteSheet&) = delete;
    SpriteSheet& operator=(const SpriteSheet&) = delete;

    /// @brief Load a sprite sheet image from disk and configure frame dimensions.
    /// @param renderer SDL_Renderer used to create the texture.
    /// @param path File path to the PNG image.
    /// @param frame_width Width of a single frame in pixels.
    /// @param frame_height Height of a single frame in pixels.
    /// @return True on success, false if the image could not be loaded.
    bool load(SDL_Renderer* renderer, const std::string& path,
              int frame_width, int frame_height);

    /// @brief Draw one frame from the sprite sheet at the given position.
    /// @param renderer SDL_Renderer to draw with.
    /// @param frame_x Frame column index (0-based).
    /// @param frame_y Frame row index (0-based).
    /// @param dest_x Destination X position in virtual pixels.
    /// @param dest_y Destination Y position in virtual pixels.
    /// @param flip_x If true, flip the sprite horizontally.
    void draw(SDL_Renderer* renderer,
              int frame_x, int frame_y,
              int dest_x, int dest_y,
              bool flip_x = false) const;

    /// @brief Get the width of a single frame.
    /// @return Frame width in pixels.
    [[nodiscard]] int frame_width() const { return frame_w_; }

    /// @brief Get the height of a single frame.
    /// @return Frame height in pixels.
    [[nodiscard]] int frame_height() const { return frame_h_; }

private:
    SDL_Texture* texture_ = nullptr;
    int frame_w_ = 0;
    int frame_h_ = 0;
    int sheet_w_ = 0;
    int sheet_h_ = 0;
};

/// @brief Registry of named sprite sheets. Owns all loaded SpriteSheet instances.
class SpriteSheetManager {
public:
    /// @brief Load a sprite sheet and register it under a string ID.
    /// @param renderer SDL_Renderer used to create the texture.
    /// @param id Unique identifier for later retrieval.
    /// @param path File path to the PNG image.
    /// @param frame_w Width of a single frame in pixels.
    /// @param frame_h Height of a single frame in pixels.
    /// @return True on success, false if loading failed.
    bool load(SDL_Renderer* renderer, const std::string& id,
              const std::string& path, int frame_w, int frame_h);

    /// @brief Retrieve a loaded sprite sheet by ID.
    /// @param id The identifier used when the sheet was loaded.
    /// @return Pointer to the SpriteSheet, or nullptr if not found.
    [[nodiscard]] const SpriteSheet* get(const std::string& id) const;

private:
    std::unordered_map<std::string, std::unique_ptr<SpriteSheet>> sheets_;
};

} // namespace raven
