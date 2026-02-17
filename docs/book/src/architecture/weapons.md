# Weapons and Pickups

Raven's weapon system lets the player steal enemy weapons via melee disarm. The
player must close distance and land a melee arc attack on an armed enemy to
knock loose their weapon as a pickup. Collected weapons run on a 10-second decay
timer. When the timer expires the weapon explodes — dealing damage and reverting
to the player's default gun. Stabilizer pickups, dropped by tougher enemies, can
freeze a stolen weapon in place permanently. A three-tier rarity system (Common,
Rare, Legendary) gates which weapons can be stabilized.

> **Note:** Enemies no longer drop weapons on death. See
> [Melee and Dash](melee-and-dash.md) for the disarm system and
> [ADR-0008](../decisions/0008-melee-disarm-over-death-drops.md) for the design
> rationale.

## Components

### Weapon

Defined in `src/ecs/components.hpp`:

```cpp
struct Weapon {
    enum class Tier : uint8_t {
        Common,   // Can be stabilized.
        Rare,     // Can be stabilized.
        Legendary // Cannot be stabilized.
    };

    Tier tier = Tier::Common;
    float bullet_speed = 300.f;
    float bullet_damage = 1.f;
    float bullet_lifetime = 3.f;
    float bullet_hitbox = 2.f;
    float fire_rate = 0.2f;
    int bullet_count = 1;
    float spread_angle = 0.f;
    std::string bullet_sheet = "projectiles";
    int bullet_frame_x = 1;
    int bullet_frame_y = 0;
    int bullet_width = 8;
    int bullet_height = 8;
    bool piercing = false;
};
```

Every player entity gets a `Weapon` on spawn. Bullet spawning in
`update_shooting()` reads these fields instead of hard-coded constants, so
equipping a stolen weapon immediately changes how the player fires.

### WeaponPickup

```cpp
struct WeaponPickup {
    Weapon weapon; // The weapon stats to give the player.
};
```

Attached to dropped weapon entities in the world. When the player's hitbox
overlaps a `WeaponPickup` entity, the weapon is transferred and the pickup is
destroyed.

### WeaponDecay

```cpp
struct WeaponDecay {
    float remaining = 10.f; // Seconds until stolen weapon expires.
};
```

Added to the player when a weapon is collected. Ticks down every frame; when it
reaches zero the stolen weapon explodes and reverts to the default.

### DefaultWeapon

```cpp
struct DefaultWeapon {
    Weapon weapon; // The player's original weapon.
};
```

Saved at collection time so the player's base weapon can be restored after
decay. Only written once per steal cycle — collecting a second weapon while
already decaying does **not** overwrite the saved default.

### StabilizerPickup (tag)

```cpp
struct StabilizerPickup {};
```

Tag component on stabilizer pickup entities. Collection removes `WeaponDecay`
and `DefaultWeapon`, making the current weapon permanent.

### ExplosionVfx (tag)

```cpp
struct ExplosionVfx {};
```

Tag on the cosmetic explosion entity spawned when a weapon decays. The entity
has a 0.5 s `Lifetime` and is cleaned up by the normal despawn system.

## Weapon steal

Weapons are obtained exclusively via melee disarm (see
[Melee and Dash](melee-and-dash.md)). When the melee arc hits an enemy with a
`BulletEmitter`, the melee system:

1. Looks up the `PatternDef` by `emitter.pattern_name` in the `PatternLibrary`.
2. Calls `weapon_from_emitter()` on the first `EmitterDef` to build a `Weapon`
   with matching bullet speed, damage, fire rate, spread, etc.
3. Copies the pattern's `tier` onto the weapon.
4. Spawns a `WeaponPickup` entity at the enemy's position with a 5 s lifetime
   and 8 px collection radius.
5. Removes the enemy's `BulletEmitter` and emplaces the `Disarmed` tag.

The tier flows from JSON all the way through to the pickup:

```
pattern.json  →  PatternDef::tier  →  weapon_from_emitter()  →  WeaponPickup
  "tier": "rare"     Weapon::Tier::Rare                          .weapon.tier
```

Patterns without an explicit `"tier"` field default to `Common`.

### Collection

When the player overlaps a `WeaponPickup` (circle-circle test in
`update_pickups()`):

1. If the player does **not** already have `WeaponDecay`, save the current
   `Weapon` into `DefaultWeapon`.
2. Replace the player's `Weapon` with the pickup's weapon.
3. Emplace `WeaponDecay` with 10 s remaining (resets timer if already decaying).
4. Destroy the pickup entity.

Only one pickup is collected per frame (`break` after first hit).

## Decay and explosion

`update_weapon_decay()` runs every tick after `update_pickups()`:

1. Decrement `decay.remaining` by `dt`.
2. If `remaining <= 0`:
   - **Damage**: if the entity is a `Player` and not `Invulnerable`, subtract 1
     HP and grant 2 s invulnerability. If already invulnerable, skip damage
     entirely.
   - **VFX**: spawn an `ExplosionVfx` entity at the player's position with a 0.5
     s `Lifetime`.
   - **Revert**: restore the player's `Weapon` from `DefaultWeapon`, then remove
     both `WeaponDecay` and `DefaultWeapon`.

The weapon **always** reverts regardless of invulnerability state — only the
damage is conditional.

## Stabilizer pickup

### Drop rules

Stabilizer drops are handled in `update_damage()` alongside weapon pickup
spawning:

| Enemy type | Drop chance |
| ---------- | ----------- |
| Grunt      | 0%          |
| Mid        | 15%         |
| Boss       | 100%        |

The RNG is an `std::mt19937` stored in the registry context, seeded from
`std::random_device` in `GameScene::on_enter()`. Stabilizer pickups spawn 12 px
below the enemy's position with an 8 s lifetime.

### Collection rules

In `update_pickups()`, stabilizer collection is checked after weapon pickup
collection. For a stabilizer to be collected:

1. The player must have `WeaponDecay` (must be carrying a stolen weapon).
2. The weapon's tier must **not** be `Legendary`.

If both conditions are met and hitboxes overlap:

1. Remove `WeaponDecay` — the decay timer stops.
2. Remove `DefaultWeapon` — nothing to revert to; the weapon is now permanent.
3. Destroy the stabilizer entity.

Legendary weapons can never be stabilized. They always decay and explode.

## System execution order

`update_pickups()` and `update_weapon_decay()` run in the second half of the
`GameScene::update()` pipeline, after movement and collision:

```
update_input              read keyboard/gamepad → target velocity (skip during dash)
update_melee              arc check, disarm, spawn weapon pickups
update_dash               burst velocity, invulnerability
update_shooting           aim resolution + bullet spawn
update_emitters           enemy bullet pattern firing
update_ai                 archetype dispatch + disarmed override
animation state logic     Melee > Dash > Walk > Idle switch
update_animation          tick frames
update_movement           velocity → position
update_tile_collision     resolve wall overlaps
update_collision          circle-circle hit tests
update_pickups         ←  weapon + stabilizer collection
update_weapon_decay    ←  tick decay timers, handle explosion
update_damage             apply DamageOnContact, stabilizer drops (no weapon drops)
update_cleanup            tick lifetimes, despawn off-screen / expired entities
```

Key ordering detail: `update_pickups` runs before `update_weapon_decay`, so a
stabilizer collected on the same tick prevents the explosion from firing.
`update_melee` runs early so that weapon pickups spawned from disarms are
available for collection later in the same frame.

## Gameplay possibilities

The component design enables several gameplay dynamics:

- **Risk/reward loop** — stolen weapons are powerful upgrades, but the player
  must find a stabilizer or accept the explosion damage. Holding a Legendary
  weapon is pure risk: it can never be stabilized.
- **Chained steals** — collecting a second weapon while decaying resets the
  timer but preserves the original default, so the player can keep gambling for
  better drops.
- **Intentional hit** — a player who is already invulnerable (e.g. from a recent
  damage hit) can let the decay expire with no penalty, effectively getting a
  free weapon cycle.
- **Tier gating** — the Common/Rare/Legendary hierarchy lets level designers
  control power curves per enemy type without touching code.
- **Extensibility** — the factoring supports straightforward additions:
  - New tiers or tier-specific decay durations.
  - Decay speed modifiers (slow/fast decay pickups).
  - Weapon-specific explosion VFX (read `Weapon` fields when spawning
    `ExplosionVfx`).
  - New pickup types (health, shield) following the same tag + overlap pattern.
  - Per-weapon sprite overrides are already in the `Weapon` struct.

## Tests

`tests/test_pickups.cpp` covers the pickup and decay systems with Catch2:

| Test case                                                     | What it verifies                                               |
| ------------------------------------------------------------- | -------------------------------------------------------------- |
| Player collects weapon pickup                                 | Pickup destroyed, weapon equipped, decay starts, default saved |
| Separated pickup is not collected                             | No collection when hitboxes don't overlap                      |
| Weapon reverts after decay expires                            | Default weapon restored, decay + default removed               |
| Decay timer ticks down                                        | `remaining` decreases by `dt` each frame                       |
| Second pickup while decaying does not overwrite DefaultWeapon | Original default preserved across chained steals               |
| weapon_from_emitter conversion                                | All emitter fields map to correct weapon fields                |
| Enemy death does not spawn weapon pickup                      | Weapons only via melee disarm                                  |
| Decay expires — damage, invulnerability, revert               | 1 HP lost, 2 s invulnerability granted, weapon reverted        |
| Decay expires while invulnerable — no damage                  | HP unchanged, weapon still reverts                             |
| Explosion spawns ExplosionVfx entity                          | VFX at player position with 0.5 s lifetime                     |
| Stabilizer collected removes decay and default                | Weapon becomes permanent                                       |
| Stabilizer ignored for Legendary tier                         | Stabilizer entity not destroyed                                |
| Stabilizer ignored without WeaponDecay                        | No effect when player has no stolen weapon                     |
| Stabilizer works for Common and Rare tiers                    | Both non-Legendary tiers can stabilize                         |
| Boss always drops stabilizer                                  | 100% drop rate verified                                        |
| Grunt never drops stabilizer                                  | 0% drop rate verified                                          |
| Mid drops stabilizer probabilistically                        | Statistical check over 100 enemies                             |
| Weapon tier flows from PatternDef to melee pickup             | JSON tier string → enum → melee disarm pickup weapon tier      |
| PatternDef tier parsed from JSON                              | Explicit common/rare/legendary and missing-defaults-to-common  |

## Key files

| File                                | Role                                                                                         |
| ----------------------------------- | -------------------------------------------------------------------------------------------- |
| `src/ecs/components.hpp`            | `Weapon`, `WeaponPickup`, `WeaponDecay`, `DefaultWeapon`, `StabilizerPickup`, `ExplosionVfx` |
| `src/ecs/systems/pickup_system.hpp` | `update_pickups()`, `update_weapon_decay()`, `weapon_from_emitter()` declarations            |
| `src/ecs/systems/pickup_system.cpp` | Pickup collection, decay timer, emitter-to-weapon conversion                                 |
| `src/ecs/systems/damage_system.cpp` | Enemy death → weapon pickup + stabilizer drop spawning                                       |
| `src/patterns/pattern_library.hpp`  | `PatternDef` with `tier` field, `EmitterDef` struct                                          |
| `src/scenes/game_scene.cpp`         | System execution order, RNG seeding                                                          |
| `tests/test_pickups.cpp`            | Catch2 tests for pickups, decay, stabilizer, tiers                                           |
