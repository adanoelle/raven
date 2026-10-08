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

    // Held buttons (true while held); bomb is the class ability
    bool shoot, melee, dash, bomb, pause, confirm, cancel;

    // Press edges, latched until a fixed tick consumes them
    bool shoot_pressed, melee_pressed, dash_pressed, bomb_pressed;
    bool pause_pressed, confirm_pressed, cancel_pressed;

    // Menu navigation edges: a movement axis pushed past half-way
    bool up_pressed, down_pressed, left_pressed, right_pressed;
};
```

Systems read `InputState` by const reference — they never write to it or call
SDL directly. This keeps all SDL coupling inside the `Input` class.

## Input class lifecycle

The `Input` class owns the SDL keyboard state pointer and an optional gamepad
handle. Each frame follows a three-step sequence:

```
begin_frame()     Reset edge flags, preserve mouse state
process_event()   Quit, hot-plug, mouse use, press latching  (called per event)
update()          Poll keyboard + gamepad + mouse, compute edges
```

### begin_frame

Saves the current `InputState` as `previous_`, then resets `current_` to
defaults. Mouse position and `mouse_active` are carried forward so they persist
across frames without mouse movement.

### process_event

- `SDL_EVENT_QUIT` sets the quit flag.
- `SDL_EVENT_GAMEPAD_ADDED` / `_REMOVED` open the first gamepad, or close the
  active one.
- `SDL_EVENT_MOUSE_MOTION` and `SDL_EVENT_MOUSE_BUTTON_DOWN` mark the mouse as
  the aiming device. Only real mouse use does this; a window resize no
  longer counts as movement.
- Key-down (not repeats), gamepad button-down, mouse button-down, and a
  trigger pulled past its press threshold all **latch a press** for every
  action bound to that control.

Held state is still read by polling in `update()`. Latching from events as
well catches a tap that goes down and up between two polls, which polling
alone misses at low frame rates.

### update

Calls four private methods in sequence:

1. `update_from_keyboard()` — read `SDL_GetKeyboardState` array.
2. `update_from_gamepad()` — read the sticks and every control in the
   gamepad layout, triggers included.
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
| Melee      | `C`            |
| Dash       | `Space`        |
| Ability    | `X`            |
| Pause      | `Escape`       |
| Confirm    | `Z` / `Return` |
| Cancel     | `X` / `Escape` |

All keyboard inputs are OR'd into the `InputState` fields, so keyboard and
gamepad can be used simultaneously.

## Gamepad mapping

The layout is decided in [ADR-0027](../decisions/0027-gamepad-layout.md).
Combat actions sit on the shoulders and triggers, so the right thumb never
leaves the aim stick, and the face buttons repeat them:

| Action           | Gamepad                          |
| ---------------- | -------------------------------- |
| Move             | Left stick (round deadzone 0.2), D-pad |
| Aim              | Right stick (round deadzone 0.2) |
| Shoot            | RT                               |
| Melee            | RB, or X                         |
| Dash             | LB, or A                         |
| Ability          | LT, or B                         |
| Pause            | Start                            |
| Confirm / Cancel | A / B                            |

Back/Select is left free for the playtest marker
([ADR-0026](../decisions/0026-record-and-replay-playtests.md)).

**Bindings are tables.** `GamepadLayout` and `KeyboardLayout` in
`core/input.hpp` list up to two controls per action. Polling, event latching
and edge detection all loop over them, so a preset or a remapped layout is a
different table, not different code.

**Triggers** are read as buttons with hysteresis (`trigger_pressed`): a pull
past 0.5 presses, and coming back below 0.3 releases, so a trigger resting
near one threshold doesn't flicker.

**Sticks** use one round deadzone (`apply_radial_deadzone`). On the left stick
the remaining travel is rescaled, so movement starts from zero at the edge
and diagonals keep both components. The right stick only needs a direction,
so it passes through raw once it is outside the deadzone. D-pad buttons add
`-1` or `+1` to the movement axes, stacking with the left stick.

When the right stick is outside its deadzone, `mouse_moved_` is set to
`false`, so `mouse_active` becomes `false` after edge computation and the
stick takes priority over the mouse.

## Mouse handling

`update_mouse()` performs a manual window-to-virtual resolution conversion (see
[Aiming and Shooting](shooting.md#mouse-coordinate-conversion) for the full
math). The left mouse button is merged into `InputState::shoot` so mouse users
can aim and fire with the mouse alone.

The `mouse_active` flag is resolved in `compute_edges()`:

- Mouse motion or a click (an SDL event) sets `mouse_active = true`.
- Right stick magnitude > 0.04 (squared deadzone) sets `mouse_active = false`.

This lets the shooting system choose between mouse aim and stick aim without
explicit mode switching.

## Edge detection and latching

Edge flags detect the _rising edge_ of a button press. The raw edge is computed
per render frame (`begin_frame()` copies `current_` into `previous_` before
polling), but the flag is **latched** rather than exposed directly:

```cpp
latched_.shoot = latched_.shoot || (current_.shoot && !previous_.shoot);
current_.shoot_pressed = latched_.shoot;
```

Latching exists because render frames and fixed ticks are decoupled. The game
renders at display rate but simulates at 120 Hz, so a frame can run **zero**
fixed ticks (on a 240 Hz display, about half of them do) or **several** (at
30 fps, four). Without latching, both directions break:

- A press whose edge lands on a zero-tick frame is cleared by the next
  `begin_frame()` before any system sees it — dropped inputs on high-refresh
  displays.
- A press seen by a multi-tick frame fires once *per tick* — dashes double-fire
  and menu confirms skip through two screens at low frame rates.

The game loop calls `Input::consume_pressed()` after each fixed tick, which
clears the latches. The result: every press drives **exactly one** tick,
regardless of the display's refresh rate relative to the tick rate. Edges that
arrive on zero-tick frames stay latched until the next tick consumes them.

Held-state fields (`shoot`, `move_x`, ...) are unaffected — they are re-polled
every frame and read directly by continuous systems like movement and
auto-fire.

## Key files

| File                               | Role                                                       |
| ---------------------------------- | ---------------------------------------------------------- |
| `src/core/input.hpp`               | `InputState` struct, `Input` class declaration             |
| `src/core/input.cpp`               | Keyboard, gamepad, mouse polling and edge detection        |
| `src/ecs/systems/input_system.hpp` | `update_input()` — applies `InputState` to player velocity |
