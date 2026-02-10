# Input Handling

Raven abstracts keyboard, gamepad, and mouse input behind a unified `InputState`
struct with normalised axes, edge-detected button presses, and
virtual-resolution mouse coordinates.

## InputState

Defined in `src/core/input.hpp`, `InputState` is a plain struct snapshot of the
current frame's input:

```cpp
struct InputState {
    // Movement axes, normalised to [-1, 1]
    float move_x = 0.f;
    float move_y = 0.f;

    // Right stick aim axes, normalised to [-1, 1]
    float aim_x = 0.f;
    float aim_y = 0.f;

    // Mouse position in virtual resolution (480x270) pixels
    float mouse_x = 0.f;
    float mouse_y = 0.f;
    bool mouse_active = false;

    // Held buttons (true while held)
    bool shoot, focus, bomb, pause, confirm, cancel;

    // Edge flags (true only on the frame the button was first pressed)
    bool shoot_pressed, bomb_pressed, pause_pressed;
    bool confirm_pressed, cancel_pressed;
};
```

Systems read `InputState` by const reference — they never write to it or call
SDL directly. This keeps all SDL coupling inside the `Input` class.

## Input class lifecycle

The `Input` class owns the SDL keyboard state pointer and an optional gamepad
handle. Each frame follows a three-step sequence:

```
begin_frame()     Reset edge flags, preserve mouse state
process_event()   Handle SDL_QUIT, controller hot-plug  (called per event)
update()          Poll keyboard + gamepad + mouse, compute edges
```

### begin_frame

Saves the current `InputState` as `previous_`, then resets `current_` to
defaults. Mouse position and `mouse_active` are carried forward so they persist
across frames without mouse movement.

### process_event

Handles three event types:

- `SDL_QUIT` — sets the quit flag.
- `SDL_CONTROLLERDEVICEADDED` — opens the first available gamepad.
- `SDL_CONTROLLERDEVICEREMOVED` — closes the gamepad if it was the active one.

All other events are ignored. Keyboard and gamepad button state is read via
polling in `update()`, not from events.

### update

Calls four private methods in sequence:

1. `update_from_keyboard()` — read `SDL_GetKeyboardState` array.
2. `update_from_gamepad()` — read `SDL_GameControllerGetAxis` and
   `SDL_GameControllerGetButton`.
3. `update_mouse()` — read `SDL_GetMouseState`, convert to virtual coordinates,
   merge left-click into `shoot`.
4. `compute_edges()` — compare `current_` against `previous_` to set `_pressed`
   edge flags, clamp movement axes, resolve `mouse_active`.

## Keyboard mapping

| Action     | Keys           |
| ---------- | -------------- |
| Move left  | `A` / `Left`   |
| Move right | `D` / `Right`  |
| Move up    | `W` / `Up`     |
| Move down  | `S` / `Down`   |
| Shoot      | `Z`            |
| Focus      | `Left Shift`   |
| Bomb       | `X`            |
| Pause      | `Escape`       |
| Confirm    | `Z` / `Return` |
| Cancel     | `X` / `Escape` |

All keyboard inputs are OR'd into the `InputState` fields, so keyboard and
gamepad can be used simultaneously.

## Gamepad mapping

| Action         | Gamepad                    |
| -------------- | -------------------------- |
| Move           | Left stick (deadzone 0.2)  |
| Move (digital) | D-pad                      |
| Aim            | Right stick (deadzone 0.2) |
| Shoot          | A button                   |
| Bomb           | B button                   |
| Focus          | Right shoulder             |
| Pause          | Start                      |
| Confirm        | A button                   |
| Cancel         | B button                   |

Stick axes are divided by `32767` to normalise to `[-1, 1]`. Values below the
deadzone threshold (`0.2`) are ignored. D-pad buttons add `-1` or `+1` to the
movement axes, stacking with the left stick.

When the right stick exceeds the deadzone, `mouse_moved_` is set to `false`,
causing `mouse_active` to become `false` after edge computation. This gives the
right stick priority over stale mouse coordinates.

## Mouse handling

`update_mouse()` performs a manual window-to-virtual resolution conversion (see
[Aiming and Shooting](shooting.md#mouse-coordinate-conversion) for the full
math). The left mouse button is merged into `InputState::shoot` so mouse users
can aim and fire with the mouse alone.

The `mouse_active` flag is resolved in `compute_edges()`:

- Mouse movement sets `mouse_active = true`.
- Right stick magnitude > 0.04 (squared deadzone) sets `mouse_active = false`.

This lets the shooting system choose between mouse aim and stick aim without
explicit mode switching.

## Edge detection

Edge flags detect the _rising edge_ of a button press — `true` only on the first
frame the button is held:

```cpp
current_.shoot_pressed = current_.shoot && !previous_.shoot;
```

`begin_frame()` copies `current_` into `previous_` before any polling, so the
comparison always spans exactly one frame. This is used for discrete actions
like pause and bomb that should fire once per press.

## Key files

| File                               | Role                                                       |
| ---------------------------------- | ---------------------------------------------------------- |
| `src/core/input.hpp`               | `InputState` struct, `Input` class declaration             |
| `src/core/input.cpp`               | Keyboard, gamepad, mouse polling and edge detection        |
| `src/ecs/systems/input_system.hpp` | `update_input()` — applies `InputState` to player velocity |
