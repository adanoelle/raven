#pragma once

#include <SDL2/SDL.h>
#include <cstdint>

namespace raven {

/// Abstract input state — works for keyboard and gamepad.
/// Designed for easy porting to Switch Pro Controller.
struct InputState {
    // Movement (normalized -1 to 1)
    float move_x = 0.f;
    float move_y = 0.f;

    // Buttons (current frame state)
    bool shoot = false;
    bool focus = false;   // slow movement + show hitbox
    bool bomb = false;
    bool pause = false;
    bool confirm = false;
    bool cancel = false;

    // Edge detection (pressed this frame)
    bool shoot_pressed = false;
    bool bomb_pressed = false;
    bool pause_pressed = false;
    bool confirm_pressed = false;
    bool cancel_pressed = false;
};

class Input {
public:
    Input();
    ~Input();

    /// Call once per frame before polling events
    void begin_frame();

    /// Process an SDL event
    void process_event(const SDL_Event& event);

    /// Get current input state
    [[nodiscard]] const InputState& state() const { return current_; }

    /// Is quit requested?
    [[nodiscard]] bool quit_requested() const { return quit_; }

private:
    InputState current_;
    InputState previous_;
    bool quit_ = false;

    // Keyboard state
    const Uint8* keyboard_ = nullptr;

    // Gamepad
    SDL_GameController* gamepad_ = nullptr;

    void update_from_keyboard();
    void update_from_gamepad();
    void compute_edges();
};

} // namespace raven
