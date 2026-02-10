#include "core/input.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>

namespace raven {

Input::Input() {
    keyboard_ = SDL_GetKeyboardState(nullptr);

    // Try to open first available gamepad
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            gamepad_ = SDL_GameControllerOpen(i);
            if (gamepad_) {
                spdlog::info("Gamepad connected: {}", SDL_GameControllerName(gamepad_));
                break;
            }
        }
    }
}

Input::~Input() {
    if (gamepad_) {
        SDL_GameControllerClose(gamepad_);
    }
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
    case SDL_QUIT:
        quit_ = true;
        break;

    case SDL_CONTROLLERDEVICEADDED:
        if (!gamepad_) {
            gamepad_ = SDL_GameControllerOpen(event.cdevice.which);
            if (gamepad_) {
                spdlog::info("Gamepad connected: {}", SDL_GameControllerName(gamepad_));
            }
        }
        break;

    case SDL_CONTROLLERDEVICEREMOVED:
        if (gamepad_ && event.cdevice.which ==
                            SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gamepad_))) {
            SDL_GameControllerClose(gamepad_);
            gamepad_ = nullptr;
            spdlog::info("Gamepad disconnected");
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

    int wx, wy;
    Uint32 buttons = SDL_GetMouseState(&wx, &wy);

    // Manual window-to-virtual resolution conversion (480x270).
    // This avoids SDL_RenderWindowToLogical which gives incorrect results
    // when a render-target texture is combined with SDL_RenderSetLogicalSize.
    int win_w, win_h;
    SDL_GetWindowSize(window_, &win_w, &win_h);

    constexpr double VIRTUAL_W = 480.0;
    constexpr double VIRTUAL_H = 270.0;

    double scale =
        std::min(static_cast<double>(win_w) / VIRTUAL_W, static_cast<double>(win_h) / VIRTUAL_H);
    double offset_x = (static_cast<double>(win_w) - VIRTUAL_W * scale) / 2.0;
    double offset_y = (static_cast<double>(win_h) - VIRTUAL_H * scale) / 2.0;

    float lx = static_cast<float>((static_cast<double>(wx) - offset_x) / scale);
    float ly = static_cast<float>((static_cast<double>(wy) - offset_y) / scale);

    spdlog::debug("mouse: win=({},{}) logical=({:.1f},{:.1f}) scale={:.2f}", wx, wy, lx, ly, scale);

    if (lx != current_.mouse_x || ly != current_.mouse_y) {
        mouse_moved_ = true;
    }
    current_.mouse_x = lx;
    current_.mouse_y = ly;

    // Left mouse button also triggers shoot
    current_.shoot = current_.shoot || (buttons & SDL_BUTTON_LMASK);
}

void Input::update_from_keyboard() {
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
    current_.shoot = current_.shoot || keyboard_[SDL_SCANCODE_Z];
    current_.focus = current_.focus || keyboard_[SDL_SCANCODE_LSHIFT];
    current_.bomb = current_.bomb || keyboard_[SDL_SCANCODE_X];
    current_.pause = current_.pause || keyboard_[SDL_SCANCODE_ESCAPE];
    current_.confirm =
        current_.confirm || keyboard_[SDL_SCANCODE_Z] || keyboard_[SDL_SCANCODE_RETURN];
    current_.cancel =
        current_.cancel || keyboard_[SDL_SCANCODE_X] || keyboard_[SDL_SCANCODE_ESCAPE];
}

void Input::update_from_gamepad() {
    if (!gamepad_)
        return;

    // Left stick
    constexpr float DEADZONE = 0.2f;
    float lx = static_cast<float>(SDL_GameControllerGetAxis(gamepad_, SDL_CONTROLLER_AXIS_LEFTX)) /
               32767.f;
    float ly = static_cast<float>(SDL_GameControllerGetAxis(gamepad_, SDL_CONTROLLER_AXIS_LEFTY)) /
               32767.f;

    if (std::abs(lx) > DEADZONE)
        current_.move_x += lx;
    if (std::abs(ly) > DEADZONE)
        current_.move_y += ly;

    // Right stick (aim)
    float rx = static_cast<float>(SDL_GameControllerGetAxis(gamepad_, SDL_CONTROLLER_AXIS_RIGHTX)) /
               32767.f;
    float ry = static_cast<float>(SDL_GameControllerGetAxis(gamepad_, SDL_CONTROLLER_AXIS_RIGHTY)) /
               32767.f;
    if (std::abs(rx) > DEADZONE || std::abs(ry) > DEADZONE) {
        current_.aim_x = rx;
        current_.aim_y = ry;
        mouse_moved_ = false; // stick takes priority
    }

    // D-pad
    if (SDL_GameControllerGetButton(gamepad_, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
        current_.move_x -= 1.f;
    if (SDL_GameControllerGetButton(gamepad_, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
        current_.move_x += 1.f;
    if (SDL_GameControllerGetButton(gamepad_, SDL_CONTROLLER_BUTTON_DPAD_UP))
        current_.move_y -= 1.f;
    if (SDL_GameControllerGetButton(gamepad_, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
        current_.move_y += 1.f;

    // Buttons (A = shoot, B = bomb, RB = focus)
    current_.shoot =
        current_.shoot || SDL_GameControllerGetButton(gamepad_, SDL_CONTROLLER_BUTTON_A);
    current_.bomb = current_.bomb || SDL_GameControllerGetButton(gamepad_, SDL_CONTROLLER_BUTTON_B);
    current_.focus = current_.focus ||
                     SDL_GameControllerGetButton(gamepad_, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    current_.pause =
        current_.pause || SDL_GameControllerGetButton(gamepad_, SDL_CONTROLLER_BUTTON_START);
    current_.confirm =
        current_.confirm || SDL_GameControllerGetButton(gamepad_, SDL_CONTROLLER_BUTTON_A);
    current_.cancel =
        current_.cancel || SDL_GameControllerGetButton(gamepad_, SDL_CONTROLLER_BUTTON_B);
}

void Input::compute_edges() {
    current_.shoot_pressed = current_.shoot && !previous_.shoot;
    current_.bomb_pressed = current_.bomb && !previous_.bomb;
    current_.pause_pressed = current_.pause && !previous_.pause;
    current_.confirm_pressed = current_.confirm && !previous_.confirm;
    current_.cancel_pressed = current_.cancel && !previous_.cancel;

    // Clamp movement
    current_.move_x = std::clamp(current_.move_x, -1.f, 1.f);
    current_.move_y = std::clamp(current_.move_y, -1.f, 1.f);

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
