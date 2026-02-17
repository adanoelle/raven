# Melee Attack and Dash

Raven's melee and dash systems form the core close-combat loop. The player
swings a directional arc to damage and disarm enemies, and dashes to close
distance or dodge bullets. Together they create the risk/reward tension central
to the Blazing Beaks-inspired design: weapons are only obtainable by getting
close and striking with melee.

## Components

### MeleeAttack

Defined in `src/ecs/components.hpp`:

```cpp
struct MeleeAttack {
    float damage = 2.f;        // Per-enemy damage.
    float range = 30.f;        // Arc reach in pixels.
    float half_angle = 0.785f; // ~45 deg half-angle (90 deg total cone).
    float knockback = 250.f;   // Knockback speed on hit.
    float remaining = 0.1f;    // Active hitbox duration.
    float aim_x = 1.f;         // Aim direction snapshot X.
    float aim_y = 0.f;         // Aim direction snapshot Y.
    bool hit_checked = false;   // Ensures arc check runs exactly once.
};
```

This is a transient component — emplaced when the player presses the melee
button and removed after `remaining` expires (~0.1 s). The aim direction is
snapshotted at creation so the arc doesn't follow the player's aim during the
swing.

### MeleeCooldown

```cpp
struct MeleeCooldown {
    float remaining = 0.f; // Time until next melee allowed.
    float rate = 0.4f;     // Minimum interval between attacks.
};
```

Prevents spam-swinging. At 0.4 s cooldown the player gets ~2.5 attacks per
second.

### Dash

```cpp
struct Dash {
    float speed = 400.f;     // Dash travel speed in pixels/s.
    float duration = 0.12f;  // Total dash duration.
    float remaining = 0.12f; // Time remaining in the dash.
    float dir_x = 1.f;       // Dash direction X.
    float dir_y = 0.f;       // Dash direction Y.
};
```

Transient component that overrides the player's velocity for its duration.
Direction is locked at activation (movement input if moving, else aim
direction).

### DashCooldown

```cpp
struct DashCooldown {
    float remaining = 0.f;
    float rate = 0.6f; // 0.6 s between dashes.
};
```

### Disarmed (tag)

```cpp
struct Disarmed {};
```

Applied to enemies whose `BulletEmitter` was removed by a melee hit. The AI
system uses this tag to override the enemy's behavior to aggressive Chaser at
1.5x move speed.

## Input bindings

| Action | Keyboard | Mouse       | Gamepad |
| ------ | -------- | ----------- | ------- |
| Melee  | C        | Right click | X       |
| Dash   | Space    | —           | LB      |

Both have edge-detected `_pressed` variants for single-activation.

## Arc hitbox geometry

The melee hit check uses `point_in_cone()` from
`src/ecs/systems/hitbox_math.hpp`:

```
point_in_cone(origin, aim_direction, target, range, half_angle)
```

1. **Range check**: `dist_sq <= range * range`
2. **Angle check**:
   `dot(normalize(aim), normalize(dir_to_target)) >= cos(half_angle)`

The effective range for each enemy is `attack.range + enemy.hitbox_radius`,
giving a generous margin so the arc feels good to use.

## Melee system

`update_melee()` in `src/ecs/systems/melee_system.cpp`:

1. **Tick cooldowns** — decrement all `MeleeCooldown::remaining` by `dt`.
2. **Initiate** — on `melee_pressed` with cooldown elapsed and no active
   `MeleeAttack`, emplace `MeleeAttack` with current `AimDirection` snapshot.
3. **Hit check** — on the first tick of an active `MeleeAttack`
   (`!hit_checked`):
   - Iterate all enemies with `(Transform2D, CircleHitbox, Enemy, Health)`.
   - For each enemy inside the cone: deal damage, apply knockback (away from
     player, 250 px/s, 0.15 s).
   - If the enemy has a `BulletEmitter`: look up pattern, call
     `weapon_from_emitter()`, spawn `WeaponPickup` at enemy position, remove
     `BulletEmitter`, emplace `Disarmed` tag.
   - Set `hit_checked = true`.
4. **Expire** — tick `remaining`, remove `MeleeAttack` when expired.

If the player entity has a `MeleeStats` component (emplaced by a class recipe),
its values are copied into the `MeleeAttack` at creation instead of using the
struct defaults. This allows each class to have different melee characteristics.
See [Player Classes](player-classes.md) for the per-class stat table.

Hit enemies are collected into a `std::vector` before applying effects, avoiding
issues with component modification during iteration.

## Dash system

`update_dash()` in `src/ecs/systems/dash_system.cpp`:

1. **Tick cooldowns** — decrement all `DashCooldown::remaining` by `dt`.
2. **Initiate** — on `dash_pressed` with cooldown elapsed and no active `Dash`:
   - Direction = movement input if magnitude >= 0.01, else `AimDirection`.
   - Emplace `Dash` with normalised direction.
   - Grant `Invulnerable` (0.18 s — slightly longer than the 0.12 s dash for a
     grace period).
3. **Override velocity** — while `Dash` is active, set `vel = dir * speed`.
4. **Expire** — tick `remaining`, remove `Dash` when expired.

The input system skips velocity updates for entities with `Dash`, preventing
player movement input from fighting the dash velocity.

## Disarmed enemy AI

In `update_ai()`, after the archetype dispatch block:

```cpp
if (reg.any_of<Disarmed>(entity)) {
    vel.dx = dir_x * ai.move_speed * 1.5f;
    vel.dy = dir_y * ai.move_speed * 1.5f;
}
```

This overrides whatever the archetype handler set. A disarmed Stalker or Coward
becomes an aggressive Chaser at 150% speed, creating pressure on the player
after disarming.

## Animation states

`AnimationState::State` is extended with `Melee` and `Dash` values. Priority
order: Melee > Dash > Walk > Idle. Both currently use placeholder frame values
pending an art pass.

## System pipeline order

```
update_charged_shot      <- charge accumulation, fire-on-release
update_input             (skip velocity during Dash, charge speed penalty)
update_melee             <- arc check, disarm, spawn pickups, MeleeStats override
update_dash              <- burst velocity, invulnerability
update_ground_slam       <- AoE damage, knockback (Brawler)
update_concussion_shot   <- AoE knockback (Sharpshooter)
update_shooting          (skip ChargedShot entities)
update_emitters
update_ai                (disarmed -> Chaser override)
animation state logic    (extended with Melee/Dash/GroundSlam states)
update_animation
update_movement
update_tile_collision
update_collision
update_pickups
update_weapon_decay
update_damage            (no more weapon drops on death)
update_cleanup
update_waves             <- wave clear check, next wave spawn
check_exit_overlap       <- room transition trigger
game_over check          <- scene swap to GameOverScene
```

Melee and dash run early in the pipeline so that the velocity overrides and
spawned pickups are visible to downstream systems in the same frame.
`update_charged_shot` runs before `update_input` so the `charging` flag is set
before the input system applies the movement speed penalty.

## Tests

`tests/test_melee_dash.cpp` covers the melee, dash, and related systems:

| Test case                                     | What it verifies                                                |
| --------------------------------------------- | --------------------------------------------------------------- |
| `point_in_cone`: target in front within range | Basic cone hit                                                  |
| `point_in_cone`: target behind player         | Misses behind                                                   |
| `point_in_cone`: target outside range         | Range cutoff                                                    |
| `point_in_cone`: target at boundary angle     | Inclusive boundary                                              |
| `point_in_cone`: target just outside angle    | Exclusive boundary                                              |
| `point_in_cone`: target at origin             | Degenerate case                                                 |
| Melee arc hits enemy in front                 | Damage applied, knockback added                                 |
| Melee arc misses enemy behind                 | No damage                                                       |
| Melee arc misses enemy outside range          | No damage                                                       |
| Melee disarms enemy and spawns pickup         | BulletEmitter removed, Disarmed tag added, WeaponPickup spawned |
| Melee cooldown prevents rapid attacks         | Second swing blocked by cooldown                                |
| Disarmed enemy state                          | No BulletEmitter, has Disarmed tag                              |
| Dash activates with movement direction        | Dash direction matches movement input                           |
| Dash uses aim direction when stationary       | Falls back to AimDirection                                      |
| Dash overrides velocity                       | Velocity set to dash speed \* direction                         |
| Dash expires and is removed                   | Dash component removed after duration                           |
| Dash cooldown prevents rapid dashing          | Second dash blocked                                             |
| Input system skips player during dash         | Movement input ignored while dashing                            |
| Invulnerability granted during dash           | Invulnerable component with 0.18 s                              |
| Contact damage cooldowns tick independently   | Fix verification: separate cooldown pass                        |

## Key files

| File                                    | Role                                                               |
| --------------------------------------- | ------------------------------------------------------------------ |
| `src/ecs/components.hpp`                | `MeleeAttack`, `MeleeCooldown`, `Dash`, `DashCooldown`, `Disarmed` |
| `src/ecs/systems/melee_system.hpp/.cpp` | Arc hitbox, disarm, weapon pickup spawning                         |
| `src/ecs/systems/dash_system.hpp/.cpp`  | Burst velocity, invulnerability                                    |
| `src/ecs/systems/hitbox_math.hpp`       | `point_in_cone()` geometry primitive                               |
| `src/ecs/systems/player_utils.hpp`      | Shared `find_player_position()`                                    |
| `src/ecs/systems/ai_system.cpp`         | Disarmed enemy override                                            |
| `src/ecs/systems/input_system.cpp`      | Dash velocity skip                                                 |
| `src/ecs/systems/damage_system.cpp`     | Death drops removed                                                |
| `src/scenes/game_scene.cpp`             | System wiring, animation states                                    |
| `tests/test_melee_dash.cpp`             | Catch2 tests                                                       |
