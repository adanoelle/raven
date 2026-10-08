#include "core/input.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>

namespace raven {

namespace {

/// @brief Where one button action lives: its held and pressed flags in
/// InputState, and its bindings in each layout. Indexes EdgeLatch::actions.
struct ActionSlots {
    bool InputState::* held;                      ///< Held flag.
    bool InputState::* pressed;                   ///< Press-edge flag.
    GamepadLayout::Controls GamepadLayout::* pad; ///< Gamepad bindings.
    KeyboardLayout::Keys KeyboardLayout::* keys;  ///< Keyboard bindings.
};

constexpr std::array<ActionSlots, Input::ACTION_COUNT> ACTIONS = {{
    {&InputState::shoot, &InputState::shoot_pressed, &GamepadLayout::shoot, &KeyboardLayout::shoot},
    {&InputState::melee, &InputState::melee_pressed, &GamepadLayout::melee, &KeyboardLayout::melee},
    {&InputState::dash, &InputState::dash_pressed, &GamepadLayout::dash, &KeyboardLayout::dash},
    {&InputState::bomb, &InputState::bomb_pressed, &GamepadLayout::bomb, &KeyboardLayout::bomb},
    {&InputState::pause, &InputState::pause_pressed, &GamepadLayout::pause, &KeyboardLayout::pause},
    {&InputState::confirm, &InputState::confirm_pressed, &GamepadLayout::confirm,
     &KeyboardLayout::confirm},
    {&InputState::cancel, &InputState::cancel_pressed, &GamepadLayout::cancel,
     &KeyboardLayout::cancel},
}};

/// @brief ACTIONS indexes for the mouse buttons' fixed bindings.
constexpr std::size_t SHOOT = 0;
constexpr std::size_t MELEE = 1;
static_assert(ACTIONS[SHOOT].held == &InputState::shoot);
static_assert(ACTIONS[MELEE].held == &InputState::melee);

/// @brief Deadzone radius for both sticks.
constexpr float STICK_DEADZONE = 0.2f;

/// @brief Read a gamepad axis scaled to [-1, 1] (triggers: [0, 1]).
float read_axis(SDL_Gamepad* gamepad, SDL_GamepadAxis axis) {
    return static_cast<float>(SDL_GetGamepadAxis(gamepad, axis)) / 32767.f;
}

} // namespace

Input::Input() = default;

void Input::init() {
    keyboard_ = SDL_GetKeyboardState(nullptr);

    // Try to open first available gamepad
    int count = 0;
    SDL_JoystickID* gamepads = SDL_GetGamepads(&count);
    if (gamepads) {
        for (int i = 0; i < count; ++i) {
            gamepad_ = SDL_OpenGamepad(gamepads[i]);
            if (gamepad_) {
                spdlog::info("Gamepad connected: {}", SDL_GetGamepadName(gamepad_));
                break;
            }
        }
        SDL_free(gamepads);
    }
}

Input::~Input() {
    shutdown();
}

void Input::shutdown() {
    if (gamepad_) {
        SDL_CloseGamepad(gamepad_);
        gamepad_ = nullptr;
    }
}

void Input::consume_pressed() {
    latched_ = EdgeLatch{};
    for (const auto& action : ACTIONS) {
        current_.*action.pressed = false;
    }
    current_.up_pressed = false;
    current_.down_pressed = false;
    current_.left_pressed = false;
    current_.right_pressed = false;
}

void Input::set_renderer(SDL_Renderer* renderer) {
    renderer_ = renderer;
}

void Input::set_window(SDL_Window* window) {
    window_ = window;
}

void Input::begin_frame() {
    bool was_mouse_active = current_.mouse_active;
    float prev_mouse_x = current_.mouse_x;
    float prev_mouse_y = current_.mouse_y;
    previous_ = current_;
    current_ = InputState{};
    current_.mouse_active = was_mouse_active;
    current_.mouse_x = prev_mouse_x;
    current_.mouse_y = prev_mouse_y;
}

void Input::process_event(const SDL_Event& event) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
        quit_ = true;
        break;

    case SDL_EVENT_GAMEPAD_ADDED:
        if (!gamepad_) {
            gamepad_ = SDL_OpenGamepad(event.gdevice.which);
            if (gamepad_) {
                spdlog::info("Gamepad connected: {}", SDL_GetGamepadName(gamepad_));
            }
        }
        break;

    case SDL_EVENT_GAMEPAD_REMOVED:
        if (gamepad_ && event.gdevice.which == SDL_GetGamepadID(gamepad_)) {
            SDL_CloseGamepad(gamepad_);
            gamepad_ = nullptr;
            trigger_held_ = {};
            spdlog::info("Gamepad disconnected");
        }
        break;

    // Only real mouse use makes the mouse the aiming device. Comparing
    // positions instead would count a window resize, or the first frame,
    // as movement and swing a controller player's aim to a hidden cursor.
    case SDL_EVENT_MOUSE_MOTION:
        mouse_moved_ = true;
        break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        mouse_moved_ = true;
        if (event.button.button == SDL_BUTTON_LEFT) {
            latched_.actions[SHOOT] = true;
        } else if (event.button.button == SDL_BUTTON_RIGHT) {
            latched_.actions[MELEE] = true;
        }
        break;

    case SDL_EVENT_KEY_DOWN:
        if (!event.key.repeat) {
            for (std::size_t i = 0; i < ACTIONS.size(); ++i) {
                const auto& keys = keyboard_layout_.*ACTIONS[i].keys;
                if (std::ranges::find(keys, event.key.scancode) != keys.end()) {
                    latched_.actions[i] = true;
                }
            }
        }
        break;

    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        if (gamepad_ && event.gbutton.which == SDL_GetGamepadID(gamepad_)) {
            latch_pad(PadControl::Kind::Button, event.gbutton.button);
        }
        break;

    case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        // A trigger pulled from rest. One already held doesn't re-latch
        // while it wobbles above the threshold.
        if (gamepad_ && event.gaxis.which == SDL_GetGamepadID(gamepad_) &&
            (event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER ||
             event.gaxis.axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) &&
            !trigger_held_[event.gaxis.axis] &&
            trigger_pressed(static_cast<float>(event.gaxis.value) / 32767.f, false)) {
            latch_pad(PadControl::Kind::Trigger, event.gaxis.axis);
        }
        break;

    default:
        break;
    }
}

void Input::update() {
    update_from_keyboard();
    update_from_gamepad();
    update_mouse();
    compute_edges();
}

void Input::update_mouse() {
    if (!window_)
        return;

    float wx = 0.f;
    float wy = 0.f;
    SDL_MouseButtonFlags buttons = SDL_GetMouseState(&wx, &wy);

    // Manual window-to-virtual resolution conversion (480x270).
    // This avoids SDL_RenderWindowToLogical which gives incorrect results
    // when a render-target texture is combined with SDL_RenderSetLogicalSize.
    int win_w = 0;
    int win_h = 0;
    SDL_GetWindowSize(window_, &win_w, &win_h);

    constexpr double VIRTUAL_W = 480.0;
    constexpr double VIRTUAL_H = 270.0;

    double scale =
        std::min(static_cast<double>(win_w) / VIRTUAL_W, static_cast<double>(win_h) / VIRTUAL_H);
    double offset_x = (static_cast<double>(win_w) - VIRTUAL_W * scale) / 2.0;
    double offset_y = (static_cast<double>(win_h) - VIRTUAL_H * scale) / 2.0;

    float lx = static_cast<float>((static_cast<double>(wx) - offset_x) / scale);
    float ly = static_cast<float>((static_cast<double>(wy) - offset_y) / scale);

    current_.mouse_x = lx;
    current_.mouse_y = ly;

    // Left mouse button also triggers shoot
    current_.shoot = current_.shoot || (buttons & SDL_BUTTON_LMASK);

    // Right mouse button triggers melee
    current_.melee = current_.melee || (buttons & SDL_BUTTON_RMASK);
}

void Input::update_from_keyboard() {
    if (!keyboard_)
        return;

    // Movement
    if (keyboard_[SDL_SCANCODE_LEFT] || keyboard_[SDL_SCANCODE_A])
        current_.move_x -= 1.f;
    if (keyboard_[SDL_SCANCODE_RIGHT] || keyboard_[SDL_SCANCODE_D])
        current_.move_x += 1.f;
    if (keyboard_[SDL_SCANCODE_UP] || keyboard_[SDL_SCANCODE_W])
        current_.move_y -= 1.f;
    if (keyboard_[SDL_SCANCODE_DOWN] || keyboard_[SDL_SCANCODE_S])
        current_.move_y += 1.f;

    // Buttons
    for (const auto& action : ACTIONS) {
        for (SDL_Scancode key : keyboard_layout_.*action.keys) {
            if (key != SDL_SCANCODE_UNKNOWN && keyboard_[key]) {
                current_.*action.held = true;
            }
        }
    }
}

void Input::update_from_gamepad() {
    if (!gamepad_)
        return;

    // Left stick: round deadzone, rescaled so movement starts from zero
    const StickValue left =
        apply_radial_deadzone(read_axis(gamepad_, SDL_GAMEPAD_AXIS_LEFTX),
                              read_axis(gamepad_, SDL_GAMEPAD_AXIS_LEFTY), STICK_DEADZONE);
    current_.move_x += left.x;
    current_.move_y += left.y;

    // Right stick (aim): only the direction matters, so the round deadzone
    // decides whether the stick is aiming and the raw value passes through
    const float rx = read_axis(gamepad_, SDL_GAMEPAD_AXIS_RIGHTX);
    const float ry = read_axis(gamepad_, SDL_GAMEPAD_AXIS_RIGHTY);
    if (rx * rx + ry * ry > STICK_DEADZONE * STICK_DEADZONE) {
        current_.aim_x = rx;
        current_.aim_y = ry;
        mouse_moved_ = false; // stick takes priority
    }

    // D-pad
    if (SDL_GetGamepadButton(gamepad_, SDL_GAMEPAD_BUTTON_DPAD_LEFT))
        current_.move_x -= 1.f;
    if (SDL_GetGamepadButton(gamepad_, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
        current_.move_x += 1.f;
    if (SDL_GetGamepadButton(gamepad_, SDL_GAMEPAD_BUTTON_DPAD_UP))
        current_.move_y -= 1.f;
    if (SDL_GetGamepadButton(gamepad_, SDL_GAMEPAD_BUTTON_DPAD_DOWN))
        current_.move_y += 1.f;

    // Buttons and triggers. Every control is read, without stopping at the
    // first match, so each trigger's hysteresis state stays current.
    for (const auto& action : ACTIONS) {
        for (const PadControl& control : gamepad_layout_.*action.pad) {
            if (pad_control_down(control)) {
                current_.*action.held = true;
            }
        }
    }
}

bool Input::pad_control_down(const PadControl& control) {
    switch (control.kind) {
    case PadControl::Kind::Button:
        return SDL_GetGamepadButton(gamepad_, static_cast<SDL_GamepadButton>(control.code));
    case PadControl::Kind::Trigger: {
        const auto axis = static_cast<std::size_t>(control.code);
        if (axis >= trigger_held_.size()) {
            return false;
        }
        const float value = read_axis(gamepad_, static_cast<SDL_GamepadAxis>(control.code));
        trigger_held_[axis] = trigger_pressed(value, trigger_held_[axis]);
        return trigger_held_[axis];
    }
    case PadControl::Kind::None:
        break;
    }
    return false;
}

void Input::latch_pad(PadControl::Kind kind, int code) {
    for (std::size_t i = 0; i < ACTIONS.size(); ++i) {
        for (const PadControl& control : gamepad_layout_.*ACTIONS[i].pad) {
            if (control.kind == kind && control.code == code) {
                latched_.actions[i] = true;
            }
        }
    }
}

void Input::compute_edges() {
    // Clamp movement first so axis edges compare like with like
    current_.move_x = std::clamp(current_.move_x, -1.f, 1.f);
    current_.move_y = std::clamp(current_.move_y, -1.f, 1.f);

    // Latch new edges; latches persist across frames until a fixed tick
    // consumes them (see consume_pressed), so a press on a frame that runs
    // zero fixed ticks is not lost. Events may already have latched a press
    // that came and went between polls.
    for (std::size_t i = 0; i < ACTIONS.size(); ++i) {
        const auto& action = ACTIONS[i];
        latched_.actions[i] =
            latched_.actions[i] || (current_.*action.held && !(previous_.*action.held));
        current_.*action.pressed = latched_.actions[i];
    }
    latched_.up = latched_.up || axis_pressed(previous_.move_y, current_.move_y, -1.f);
    latched_.down = latched_.down || axis_pressed(previous_.move_y, current_.move_y, 1.f);
    latched_.left = latched_.left || axis_pressed(previous_.move_x, current_.move_x, -1.f);
    latched_.right = latched_.right || axis_pressed(previous_.move_x, current_.move_x, 1.f);

    current_.up_pressed = latched_.up;
    current_.down_pressed = latched_.down;
    current_.left_pressed = latched_.left;
    current_.right_pressed = latched_.right;

    // Resolve mouse_active: mouse movement activates, right stick deactivates
    if (mouse_moved_) {
        current_.mouse_active = true;
    }
    if (current_.aim_x * current_.aim_x + current_.aim_y * current_.aim_y > 0.04f) {
        current_.mouse_active = false;
    }
    mouse_moved_ = false;
}

} // namespace raven
