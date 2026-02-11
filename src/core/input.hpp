#pragma once

#include <SDL2/SDL.h>

#include <cstdint>

namespace raven {

/// @brief Abstract input state — works for keyboard and gamepad.
/// Designed for easy porting to Switch Pro Controller.
struct InputState {
    float move_x = 0.f; ///< Horizontal movement axis, normalised to [-1, 1].
    float move_y = 0.f; ///< Vertical movement axis, normalised to [-1, 1].

    float aim_x = 0.f; ///< Right stick aim X, normalised [-1, 1].
    float aim_y = 0.f; ///< Right stick aim Y, normalised [-1, 1].

    float mouse_x = 0.f;       ///< Mouse X in virtual resolution pixels.
    float mouse_y = 0.f;       ///< Mouse Y in virtual resolution pixels.
    bool mouse_active = false; ///< True if mouse moved since last right-stick input.

    bool shoot = false;   ///< Shoot button held this frame.
    bool focus = false;   ///< Focus button held (slow movement + show hitbox).
    bool bomb = false;    ///< Bomb button held this frame.
    bool melee = false;   ///< Melee button held this frame.
    bool dash = false;    ///< Dash button held this frame.
    bool pause = false;   ///< Pause button held this frame.
    bool confirm = false; ///< Confirm/accept button held this frame.
    bool cancel = false;  ///< Cancel/back button held this frame.

    bool shoot_pressed = false;   ///< Shoot button pressed this frame (edge).
    bool bomb_pressed = false;    ///< Bomb button pressed this frame (edge).
    bool melee_pressed = false;   ///< Melee button pressed this frame (edge).
    bool dash_pressed = false;    ///< Dash button pressed this frame (edge).
    bool pause_pressed = false;   ///< Pause button pressed this frame (edge).
    bool confirm_pressed = false; ///< Confirm button pressed this frame (edge).
    bool cancel_pressed = false;  ///< Cancel button pressed this frame (edge).
};

/// @brief Manages keyboard and gamepad input with per-frame edge detection.
class Input {
  public:
    Input();
    ~Input();

    /// @brief Reset per-frame edge flags. Call once per frame before polling events.
    void begin_frame();

    /// @brief Process a single SDL event (quit, controller hot-plug).
    ///
    /// Does not poll keyboard or gamepad state; call update() after the
    /// event loop for that.
    /// @param event The SDL event to handle.
    void process_event(const SDL_Event& event);

    /// @brief Poll keyboard and gamepad state and compute edge flags.
    ///
    /// Must be called exactly once per frame, after the event loop, to
    /// ensure input axes reflect currently held keys even on frames with
    /// no pending SDL events.
    void update();

    /// @brief Get the current input state snapshot.
    /// @return Const reference to the current InputState.
    [[nodiscard]] const InputState& state() const { return current_; }

    /// @brief Store the SDL renderer for mouse coordinate conversion.
    /// @param renderer The SDL_Renderer used for SDL_RenderWindowToLogical.
    void set_renderer(SDL_Renderer* renderer);

    /// @brief Store the SDL window for manual mouse coordinate conversion.
    /// @param window The SDL_Window used for window-to-virtual resolution mapping.
    void set_window(SDL_Window* window);

    /// @brief Check whether a quit event was received.
    /// @return True if the user requested quit (window close or quit key).
    [[nodiscard]] bool quit_requested() const { return quit_; }

  private:
    InputState current_;
    InputState previous_;
    bool quit_ = false;

    const Uint8* keyboard_ = nullptr;

    SDL_GameController* gamepad_ = nullptr;

    SDL_Renderer* renderer_ = nullptr;
    SDL_Window* window_ = nullptr;
    bool mouse_moved_ = false; ///< Mouse moved this frame.

    void update_from_keyboard();
    void update_from_gamepad();
    void update_mouse();
    void compute_edges();
};

} // namespace raven
