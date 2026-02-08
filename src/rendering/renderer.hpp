#pragma once

#include <SDL2/SDL.h>

#include <memory>
#include <string>

namespace raven {

/// Handles SDL window + renderer with virtual resolution scaling.
/// Renders to a low-res target (e.g. 384x448) then scales to window.
class Renderer {
public:
    // 16:9 pixel art roguelike (1920/4 x 1080/4)
    static constexpr int VIRTUAL_WIDTH = 480;
    static constexpr int VIRTUAL_HEIGHT = 270;

    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool init(const std::string& title, int window_scale = 2);
    void shutdown();

    /// Begin frame — clears render target
    void begin_frame();

    /// End frame — scales virtual target to window (does not present)
    void end_frame();

    /// Present — call after any overlays have rendered
    void present();

    [[nodiscard]] SDL_Renderer* sdl_renderer() const { return renderer_; }
    [[nodiscard]] SDL_Window* sdl_window() const { return window_; }
    [[nodiscard]] int virtual_width() const { return VIRTUAL_WIDTH; }
    [[nodiscard]] int virtual_height() const { return VIRTUAL_HEIGHT; }

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* render_target_ = nullptr; // virtual resolution target
};

} // namespace raven
