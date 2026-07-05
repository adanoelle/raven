#pragma once

#include <SDL3/SDL.h>

#include <memory>
#include <string>

namespace raven {

/// @brief SDL window and renderer wrapper with virtual resolution scaling.
///
/// Renders to a low-res target (480x270) then scales to the window size,
/// giving pixel-perfect rendering at any display resolution.
class Renderer {
  public:
    static constexpr int VIRTUAL_WIDTH = 480;  ///< Internal render width in pixels (1920/4).
    static constexpr int VIRTUAL_HEIGHT = 270; ///< Internal render height in pixels (1080/4).

    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    /// @brief Create the SDL window and renderer.
    /// @param title Window title string.
    /// @param window_scale Integer multiplier for the window size (default 2).
    /// @param fullscreen Start in borderless fullscreen.
    /// @param vsync Request vsync; check vsync_enabled() for the result.
    /// @return True on success, false if SDL initialisation failed.
    bool init(const std::string& title, int window_scale = 2, bool fullscreen = false,
              bool vsync = true);

    /// @brief Whether vsync is actually active (the driver may refuse it).
    ///
    /// When false, the caller must limit the frame rate itself or the main
    /// loop busy-spins at uncapped speed.
    /// @return True if presentation is synced to the display.
    [[nodiscard]] bool vsync_enabled() const { return vsync_enabled_; }

    /// @brief Switch between borderless fullscreen and windowed mode.
    /// @param fullscreen True for fullscreen.
    void set_fullscreen(bool fullscreen);

    /// @brief Resize the window to scale x virtual resolution (windowed only).
    /// @param window_scale Integer multiplier, >= 1.
    void set_window_scale(int window_scale);

    /// @brief Enable or disable vsync at runtime.
    ///
    /// Updates vsync_enabled() with the actual result — the driver may
    /// refuse, in which case the game loop's frame limiter takes over.
    /// @param vsync True to request vsync.
    void set_vsync(bool vsync);

    /// @brief Destroy the SDL window, renderer, and render target.
    void shutdown();

    /// @brief Begin a frame by clearing the virtual render target.
    void begin_frame();

    /// @brief End the frame by scaling the virtual target to the window. Does not present.
    void end_frame();

    /// @brief Present the rendered frame. Call after any overlays have drawn.
    void present();

    /// @brief Handle SDL events that affect the renderer (target reset, resize).
    /// @param event The SDL event to inspect.
    void handle_event(const SDL_Event& event);

    /// @brief Get the raw SDL_Renderer pointer for direct draw calls.
    /// @return Pointer to the SDL_Renderer (never null after successful init).
    [[nodiscard]] SDL_Renderer* sdl_renderer() const { return renderer_; }

    /// @brief Get the raw SDL_Window pointer.
    /// @return Pointer to the SDL_Window (never null after successful init).
    [[nodiscard]] SDL_Window* sdl_window() const { return window_; }

    /// @brief Get the virtual render width.
    /// @return VIRTUAL_WIDTH (480).
    [[nodiscard]] int virtual_width() const { return VIRTUAL_WIDTH; }

    /// @brief Get the virtual render height.
    /// @return VIRTUAL_HEIGHT (270).
    [[nodiscard]] int virtual_height() const { return VIRTUAL_HEIGHT; }

  private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* render_target_ = nullptr; ///< Virtual resolution render target.
    bool vsync_enabled_ = false;           ///< True if the driver accepted vsync.

    /// @brief Destroy and recreate the virtual resolution render target.
    void recreate_target();
};

} // namespace raven
