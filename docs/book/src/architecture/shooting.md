# Aiming and Shooting

Raven's shooting system resolves an aim direction each tick from gamepad right
stick or mouse position, then spawns bullet entities when the shoot button is
held and the cooldown has elapsed.

## Components

### AimDirection

Defined in `src/ecs/components.hpp`:

```cpp
struct AimDirection {
    float x = 1.f; // Aim X component (unit vector).
    float y = 0.f; // Aim Y component (unit vector).
};
```

Stores the resolved aim as a normalised direction vector. Defaults to facing
right. The shooting system writes this every tick; bullet spawning reads it to
set velocity and rotation.

### ShootCooldown

```cpp
struct ShootCooldown {
    float remaining = 0.f; // Time until next shot allowed (seconds).
    float rate = 0.2f;     // Minimum interval between shots (seconds).
};
```

`remaining` is decremented by `dt` every tick. When the player fires,
`remaining` is reset to `rate`. At the default rate of 0.2 s the player fires 5
shots per second.

## Aim resolution

`update_shooting()` resolves the aim direction with a priority chain:

1. **Right stick** — if the stick magnitude exceeds the deadzone (`0.2`),
   normalise and use as the aim vector. This takes highest priority so gamepad
   players always get immediate response.
2. **Mouse** — if `mouse_active` is true and the stick is idle, compute the
   direction from the player's world position to the mouse cursor. If the
   distance is less than 1 pixel (cursor on top of the player), skip the update
   to avoid a degenerate zero vector.
3. **Retain previous** — if neither input source is active, the previous
   `AimDirection` is kept. This means the player always has a valid aim even
   when no input is present.

```cpp
float stick_mag = input.aim_x * input.aim_x + input.aim_y * input.aim_y;
if (stick_mag > AIM_DEADZONE * AIM_DEADZONE) {
    // Use right stick (normalised)
} else if (input.mouse_active) {
    // Aim toward mouse position
} // else: retain previous aim
```

The deadzone comparison uses squared magnitude to avoid a square root on every
tick.

## Mouse coordinate conversion

The `Input` class converts raw window-space mouse coordinates to the 480x270
virtual resolution manually in `update_mouse()`. This is necessary because
`SDL_RenderWindowToLogical` gives incorrect results when a render-target texture
is combined with `SDL_RenderSetLogicalSize` (the engine renders to an offscreen
texture, then scales to the window).

The conversion is letterbox-aware:

```cpp
double scale = std::min(double(win_w) / 480.0, double(win_h) / 270.0);
double offset_x = (double(win_w) - 480.0 * scale) / 2.0;
double offset_y = (double(win_h) - 270.0 * scale) / 2.0;

float lx = float((double(wx) - offset_x) / scale);
float ly = float((double(wy) - offset_y) / scale);
```

`scale` is the uniform scale factor that fits 480x270 into the window. The
offsets account for letterbox/pillarbox bars when the window aspect ratio does
not match 16:9.

The `mouse_active` flag tracks which input device was used most recently: mouse
movement sets it to `true`, right stick input sets it to `false`. This prevents
stale mouse coordinates from overriding the gamepad when the mouse is
stationary.

## Bullet spawning

When `input.shoot` is held and `remaining <= 0`:

1. Reset `remaining` to `rate`.
2. Create a new bullet entity with:

| Component           | Value                                               |
| ------------------- | --------------------------------------------------- |
| `Transform2D`       | Player position; rotation = `atan2(aim.y, aim.x)`   |
| `PreviousTransform` | Player position (for render interpolation)          |
| `Velocity`          | `aim * 300` pixels/s                                |
| `Bullet`            | `Owner::Player`                                     |
| `DamageOnContact`   | 1.0                                                 |
| `Lifetime`          | 3.0 s                                               |
| `CircleHitbox`      | radius 2 px                                         |
| `Sprite`            | `"projectiles"` sheet, frame (1,0), 8x8 px, layer 5 |
| `OffScreenDespawn`  | (tag)                                               |

The bullet speed constant (`BULLET_SPEED = 300`) is defined at the top of
`shooting_system.cpp`.

## Input bindings

| Action       | Keyboard | Gamepad     | Mouse           |
| ------------ | -------- | ----------- | --------------- |
| Shoot (hold) | `Z`      | A button    | Left click      |
| Aim          | —        | Right stick | Cursor position |

All three shoot sources are OR'd together in `InputState::shoot`. The left mouse
button is merged in `Input::update_mouse()`.

## System execution order

`update_shooting()` runs second in the `GameScene::update()` pipeline,
immediately after input:

```
update_input            read keyboard/gamepad → target velocity
update_shooting         aim resolution + bullet spawn
animation state logic   velocity → idle/walk switch
update_animation        tick frames
update_movement         velocity → position
update_tile_collision   resolve wall overlaps
update_projectiles      tick bullet lifetimes
update_collision        circle-circle hit tests
update_damage           apply DamageOnContact
update_cleanup          despawn off-screen / expired entities
```

Running shooting right after input ensures the aim direction reflects the
current tick's input. Bullets spawned this tick are moved by `update_movement`
on the same tick, so they never appear at the player's position for more than
one frame.

## Tests

`tests/test_shooting.cpp` covers the shooting system with Catch2:

| Test case                                           | What it verifies                                   |
| --------------------------------------------------- | -------------------------------------------------- |
| bullet spawned when shoot held and cooldown expired | A bullet entity is created with correct components |
| no bullet when cooldown still active                | Cooldown blocks firing                             |
| cooldown resets after firing                        | `remaining` is set back to `rate`                  |
| no bullet when shoot not held                       | No fire without input                              |
| right stick sets aim direction                      | Stick input produces correct unit vector           |
| mouse position sets aim when stick inactive         | Mouse fallback works                               |
| aim direction retained when no input                | Previous aim persists                              |
| aim direction is normalized                         | Non-unit stick input is normalised                 |
| mouse aim (right/left/up/down)                      | Cardinal mouse directions produce correct aim      |
| mouse aim diagonal is normalized                    | Diagonal mouse aim is unit length                  |
| mouse on player retains previous aim                | Degenerate zero-distance case is handled           |
| shoot fires bullet toward mouse                     | Bullet velocity matches mouse aim                  |
| right stick overrides mouse                         | Priority order is respected                        |
| bullet velocity matches aim direction               | `Velocity = aim * BULLET_SPEED`                    |

## Key files

| File                                  | Role                                                 |
| ------------------------------------- | ---------------------------------------------------- |
| `src/ecs/components.hpp`              | `AimDirection`, `ShootCooldown`, `Bullet` components |
| `src/ecs/systems/shooting_system.hpp` | `update_shooting()` declaration                      |
| `src/ecs/systems/shooting_system.cpp` | Aim resolution, cooldown, bullet spawn               |
| `src/core/input.cpp`                  | Mouse coordinate conversion, `mouse_active` flag     |
| `src/scenes/game_scene.cpp`           | System execution order                               |
| `tests/test_shooting.cpp`             | Catch2 tests for aim and cooldown                    |
