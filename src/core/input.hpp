#pragma once

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
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
    bool bomb = false;    ///< Class ability button held this frame.
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
    bool bomb_pressed = false;    ///< Class ability button press edge.
    bool melee_pressed = false;   ///< Melee button press edge.
    bool dash_pressed = false;    ///< Dash button press edge.
    bool pause_pressed = false;   ///< Pause button press edge.
    bool confirm_pressed = false; ///< Confirm button press edge.
    bool cancel_pressed = false;  ///< Cancel button press edge.

    // Menu navigation edges: a movement axis pushed past half-way. Latched
    // and consumed like the button edges, so a direction that was already
    // held when a menu opened does not move its cursor.
    bool up_pressed = false;    ///< Move-up press edge.
    bool down_pressed = false;  ///< Move-down press edge.
    bool left_pressed = false;  ///< Move-left press edge.
    bool right_pressed = false; ///< Move-right press edge.
};

/// @brief Whether a movement axis was pushed past the menu threshold this frame.
/// @param previous Axis value on the previous frame.
/// @param current Axis value on this frame.
/// @param direction +1 to test the positive direction, -1 for the negative one.
/// @return True if the axis is past half-way in that direction now, but was not before.
[[nodiscard]] constexpr bool axis_pressed(float previous, float current, float direction) {
    constexpr float THRESHOLD = 0.5f;
    return current * direction > THRESHOLD && previous * direction <= THRESHOLD;
}

/// @brief Whether an analog trigger counts as pressed, with hysteresis.
///
/// A trigger resting near one threshold would flicker between pressed and
/// released (firing a stray Sharpshooter shot on each release), so a held
/// trigger has to come back further than a released one has to travel.
/// @param value Trigger position in [0, 1].
/// @param was_pressed Whether the trigger counted as pressed last time.
/// @return Whether it counts as pressed now.
[[nodiscard]] constexpr bool trigger_pressed(float value, bool was_pressed) {
    constexpr float PRESS = 0.5f;
    constexpr float RELEASE = 0.3f;
    return was_pressed ? value > RELEASE : value > PRESS;
}

/// @brief A stick position after its deadzone, each axis in [-1, 1].
struct StickValue {
    float x = 0.f; ///< Horizontal position.
    float y = 0.f; ///< Vertical position.
};

/// @brief Apply a round deadzone to a stick and rescale the rest of its travel.
///
/// One round deadzone keeps shallow diagonals intact, where a deadzone per
/// axis would zero the smaller component and pull movement onto the axes.
/// Rescaling starts movement from zero at the edge of the deadzone instead
/// of jumping straight to the deadzone's value.
/// @param x Raw horizontal position in [-1, 1].
/// @param y Raw vertical position in [-1, 1].
/// @param deadzone Radius below which the stick reads as centred.
/// @return The rescaled position, or zero inside the deadzone.
[[nodiscard]] inline StickValue apply_radial_deadzone(float x, float y, float deadzone) {
    const float magnitude = std::sqrt(x * x + y * y);
    if (magnitude <= deadzone) {
        return {};
    }
    const float scaled = std::min(1.f, (magnitude - deadzone) / (1.f - deadzone));
    return {x / magnitude * scaled, y / magnitude * scaled};
}

/// @brief One physical gamepad control that can drive an action.
struct PadControl {
    /// @brief Which kind of control this is.
    enum class Kind : uint8_t {
        None,    ///< Empty slot.
        Button,  ///< A digital button; code is an SDL_GamepadButton.
        Trigger, ///< An analog trigger read as a button; code is an SDL_GamepadAxis.
    };
    Kind kind = Kind::None; ///< Kind of control.
    int code = 0;           ///< SDL_GamepadButton or SDL_GamepadAxis, by kind.
};

/// @brief A gamepad button as a PadControl.
/// @param button The button.
/// @return The control.
[[nodiscard]] constexpr PadControl pad_button(SDL_GamepadButton button) {
    return {PadControl::Kind::Button, static_cast<int>(button)};
}

/// @brief An analog trigger as a PadControl.
/// @param axis SDL_GAMEPAD_AXIS_LEFT_TRIGGER or SDL_GAMEPAD_AXIS_RIGHT_TRIGGER.
/// @return The control.
[[nodiscard]] constexpr PadControl pad_trigger(SDL_GamepadAxis axis) {
    return {PadControl::Kind::Trigger, static_cast<int>(axis)};
}

/// @brief Gamepad controls for each action, up to two each.
///
/// The defaults are the standard layout (ADR-0027). Combat actions sit on
/// the shoulders and triggers, so the right thumb never leaves the aim
/// stick, and the face buttons repeat them for players who prefer those.
/// Back/Select is left free for the playtest marker (ADR-0026).
struct GamepadLayout {
    using Controls = std::array<PadControl, 2>; ///< Controls for one action.

    Controls shoot{{pad_trigger(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)}}; ///< Fire (held).
    Controls melee{{pad_button(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER),
                    pad_button(SDL_GAMEPAD_BUTTON_WEST)}}; ///< Melee.
    Controls dash{{pad_button(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER),
                   pad_button(SDL_GAMEPAD_BUTTON_SOUTH)}}; ///< Dash.
    Controls bomb{{pad_trigger(SDL_GAMEPAD_AXIS_LEFT_TRIGGER),
                   pad_button(SDL_GAMEPAD_BUTTON_EAST)}};     ///< Class ability.
    Controls pause{{pad_button(SDL_GAMEPAD_BUTTON_START)}};   ///< Pause.
    Controls confirm{{pad_button(SDL_GAMEPAD_BUTTON_SOUTH)}}; ///< Menu confirm.
    Controls cancel{{pad_button(SDL_GAMEPAD_BUTTON_EAST)}};   ///< Menu cancel.
};

/// @brief Keyboard keys for each action, up to two each. Unused slots hold
/// SDL_SCANCODE_UNKNOWN.
struct KeyboardLayout {
    using Keys = std::array<SDL_Scancode, 2>; ///< Keys for one action.

    Keys shoot{SDL_SCANCODE_Z, SDL_SCANCODE_UNKNOWN};      ///< Fire (held).
    Keys melee{SDL_SCANCODE_C, SDL_SCANCODE_UNKNOWN};      ///< Melee.
    Keys dash{SDL_SCANCODE_SPACE, SDL_SCANCODE_UNKNOWN};   ///< Dash.
    Keys bomb{SDL_SCANCODE_X, SDL_SCANCODE_UNKNOWN};       ///< Class ability.
    Keys pause{SDL_SCANCODE_ESCAPE, SDL_SCANCODE_UNKNOWN}; ///< Pause.
    Keys confirm{SDL_SCANCODE_Z, SDL_SCANCODE_RETURN};     ///< Menu confirm.
    Keys cancel{SDL_SCANCODE_X, SDL_SCANCODE_ESCAPE};      ///< Menu cancel.
};

/// @brief Manages keyboard and gamepad input with per-frame edge detection.
class Input {
  public:
    Input();
    ~Input();

    // Non-copyable/movable: owns the SDL gamepad handle.
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;
    Input(Input&&) = delete;
    Input& operator=(Input&&) = delete;

    /// @brief Open input devices. Call once after SDL_Init(SDL_INIT_GAMEPAD).
    ///
    /// Device discovery lives here rather than in the constructor because
    /// Input is a Game member and is constructed before SDL is initialised.
    void init();

    /// @brief Reset per-frame edge flags. Call once per frame before polling events.
    void begin_frame();

    /// @brief Process a single SDL event.
    ///
    /// Handles quit and controller hot-plug, marks the mouse as the aiming
    /// device when it moves or clicks, and latches presses from key, button
    /// and trigger events. Latching from events catches a tap that goes
    /// down and up between two polls, which polling alone misses at low
    /// frame rates. Call update() after the event loop to poll held state.
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

    /// @brief The gamepad layout in use.
    /// @return The layout.
    [[nodiscard]] const GamepadLayout& gamepad_layout() const { return gamepad_layout_; }

    /// @brief The keyboard layout in use.
    /// @return The layout.
    [[nodiscard]] const KeyboardLayout& keyboard_layout() const { return keyboard_layout_; }

    /// @brief Number of button actions (fire, melee, dash, ability, pause, confirm, cancel).
    static constexpr std::size_t ACTION_COUNT = 7;

  private:
    /// @brief Press edges awaiting consumption by a fixed tick.
    struct EdgeLatch {
        std::array<bool, ACTION_COUNT> actions{}; ///< One per button action.
        bool up = false;
        bool down = false;
        bool left = false;
        bool right = false;
    };

    InputState current_;
    InputState previous_;
    EdgeLatch latched_;
    bool quit_ = false;

    GamepadLayout gamepad_layout_;
    KeyboardLayout keyboard_layout_;
    std::array<bool, SDL_GAMEPAD_AXIS_COUNT> trigger_held_{}; ///< Trigger hysteresis state.

    const bool* keyboard_ = nullptr;

    SDL_Gamepad* gamepad_ = nullptr;

    SDL_Renderer* renderer_ = nullptr;
    SDL_Window* window_ = nullptr;
    bool mouse_moved_ = false; ///< Mouse moved or clicked this frame.

    void update_from_keyboard();
    void update_from_gamepad();
    void update_mouse();
    void compute_edges();

    /// @brief Whether a gamepad control is down now (updates trigger hysteresis).
    bool pad_control_down(const PadControl& control);

    /// @brief Latch a press of every action bound to a gamepad control.
    void latch_pad(PadControl::Kind kind, int code);
};

} // namespace raven
