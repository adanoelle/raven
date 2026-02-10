#include "core/input.hpp"

#include <spdlog/spdlog.h>

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

void Input::begin_frame() {
    previous_ = current_;
    current_ = InputState{};
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
    compute_edges();
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
}

} // namespace raven
