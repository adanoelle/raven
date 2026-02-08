#pragma once

#include <SDL2/SDL.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace raven {

/// Manages a texture atlas / sprite sheet.
class SpriteSheet {
public:
    SpriteSheet() = default;
    ~SpriteSheet();

    SpriteSheet(const SpriteSheet&) = delete;
    SpriteSheet& operator=(const SpriteSheet&) = delete;

    bool load(SDL_Renderer* renderer, const std::string& path,
              int frame_width, int frame_height);

    void draw(SDL_Renderer* renderer,
              int frame_x, int frame_y,
              int dest_x, int dest_y,
              bool flip_x = false) const;

    [[nodiscard]] int frame_width() const { return frame_w_; }
    [[nodiscard]] int frame_height() const { return frame_h_; }

private:
    SDL_Texture* texture_ = nullptr;
    int frame_w_ = 0;
    int frame_h_ = 0;
    int sheet_w_ = 0;
    int sheet_h_ = 0;
};

/// Manages all loaded sprite sheets by ID.
class SpriteSheetManager {
public:
    bool load(SDL_Renderer* renderer, const std::string& id,
              const std::string& path, int frame_w, int frame_h);

    [[nodiscard]] const SpriteSheet* get(const std::string& id) const;

private:
    std::unordered_map<std::string, std::unique_ptr<SpriteSheet>> sheets_;
};

} // namespace raven
