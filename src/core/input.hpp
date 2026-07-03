#pragma once

#include <SDL3/SDL.h>

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

    // Press edges. Latched from the frame the press happens until a fixed
    // tick consumes them (Input::consume_pressed), so presses are never
    // dropped on frames that run zero fixed ticks (>120 Hz displays) and
    // never replayed into multiple ticks of the same frame.
    bool shoot_pressed = false;   ///< Shoot button press edge.
    bool bomb_pressed = false;    ///< Bomb button press edge.
    bool melee_pressed = false;   ///< Melee button press edge.
    bool dash_pressed = false;    ///< Dash button press edge.
    bool pause_pressed = false;   ///< Pause button press edge.
    bool confirm_pressed = false; ///< Confirm button press edge.
    bool cancel_pressed = false;  ///< Cancel button press edge.
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

    /// @brief Clear latched press edges after a fixed tick has seen them.
    ///
    /// Call after each fixed-timestep update. Edges latch on the frame the
    /// press happens and survive frames that run zero fixed ticks, so a
    /// press always drives exactly one tick regardless of display refresh
    /// rate relative to the tick rate.
    void consume_pressed();

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

    /// @brief Release input devices. Call before SDL_Quit().
    void shutdown();

  private:
    /// @brief Press edges awaiting consumption by a fixed tick.
    struct EdgeLatch {
        bool shoot = false;
        bool bomb = false;
        bool melee = false;
        bool dash = false;
        bool pause = false;
        bool confirm = false;
        bool cancel = false;
    };

    InputState current_;
    InputState previous_;
    EdgeLatch latched_;
    bool quit_ = false;

    const bool* keyboard_ = nullptr;

    SDL_Gamepad* gamepad_ = nullptr;

    SDL_Renderer* renderer_ = nullptr;
    SDL_Window* window_ = nullptr;
    bool mouse_moved_ = false; ///< Mouse moved this frame.

    void update_from_keyboard();
    void update_from_gamepad();
    void update_mouse();
    void compute_edges();
};

} // namespace raven
