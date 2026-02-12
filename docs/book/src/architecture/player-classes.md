# Player Classes

Raven's player class system lets the player choose a combat archetype before
entering the game. Each class is implemented as a "component recipe" — a free
function that assembles class-specific components onto a base player entity.
Systems gate their behaviour on component presence rather than runtime enum
checks, so adding a new class means writing a new recipe function and any
class-specific components it needs. Two classes ship in the initial
implementation: **Brawler** (melee-focused tank) and **Sharpshooter** (ranged
glass cannon).

## Design matrix relationship

The design matrix in `docs/design/entity_database.csv` describes an
aspirational, full-game vision with four classes and normalised stats. The
current implementation covers two of those classes with concrete, tuned values.
Where the implementation diverges from the design matrix, the difference
reflects prototype-stage tuning decisions that will converge toward the design
as content and balancing matures.

| Stat   | Design (Brawler) |                 Impl (Brawler) | Design (Sharpshooter) |              Impl (Sharpshooter) |
| ------ | ---------------: | -----------------------------: | --------------------: | -------------------------------: |
| HP     |              150 |                            150 |                    60 |                               60 |
| Lives  |                — |                              3 |                     — |                                2 |
| Speed  |              100 |                            100 |                    80 |                              150 |
| Damage |  10 (normalised) | 3 per melee tick, 1 per bullet |       25 (normalised) | 1 per melee tick, 1.5 per bullet |

**HP** values match the design matrix exactly. **Speed** diverges for
Sharpshooter: the design uses 80 (relative to Brawler's 100), but the
implementation uses 150 to make the glass cannon feel meaningfully faster during
prototype playtesting. The difference will narrow once enemies scale in speed.

**Damage** in the design matrix is a single normalised number representing
overall damage output. The implementation splits this into per-tick melee damage
(`MeleeStats::damage`) and per-bullet damage (`Weapon::bullet_damage`) because
the ECS needs concrete values per contact event. Brawler's 3 melee damage per
tick and Sharpshooter's 1.5 bullet damage with charge multipliers (0.5x--3.0x)
approximate the design intent of Brawler as sustained melee and Sharpshooter as
burst ranged.

The design matrix also lists a Tactician and Alchemist class. These are not yet
implemented; see `docs/design/raven_design_matrix_template.md` Phase 1 for the
roadmap.

## Class as component recipe

The recipe pattern works in two steps:

1. **Base entity** — `GameScene::spawn_player` creates a player entity with all
   universal components: `Transform2D`, `Velocity`, `Player`, `Health`,
   `CircleHitbox`, `RectHitbox`, `Sprite`, `Animation`, `AnimationState`,
   `AimDirection`, `ShootCooldown`, `MeleeCooldown`, `DashCooldown`, `Weapon`.
   At this point the entity is a generic player with default stats.

2. **Recipe overlay** — a `switch` on `selected_class_` calls either
   `apply_brawler` or `apply_sharpshooter`. The recipe function overrides
   universal component values (speed, HP, lives) and emplaces class-specific
   components (abilities, stat modifiers).

Both recipe functions live in `src/ecs/player_class.hpp/.cpp`. They take a
registry and entity, assume the universal components already exist, and add
class-specific ones on top.

## Components

### ClassId

```cpp
struct ClassId {
    enum class Id : uint8_t { Brawler, Sharpshooter };
    Id id = Id::Brawler;
};
```

Identifies the player's chosen class. Used only for UI/death messages and
persisted in `GameState::player_class` for the game-over screen. Systems never
branch on this enum in hot paths — they gate on the presence of ability
components instead.

### MeleeStats

```cpp
struct MeleeStats {
    float damage = 2.f;
    float range = 30.f;
    float half_angle = 0.785f;
    float knockback = 250.f;
    float duration = 0.1f;
};
```

Persistent stat overrides for the melee system. When `update_melee` creates a
`MeleeAttack`, it checks for `MeleeStats` on the entity and copies its values
into the attack instead of using `MeleeAttack` defaults. This lets each class
have different melee characteristics without subclassing.

### GroundSlam

```cpp
struct GroundSlam {
    float damage = 4.f;
    float radius = 50.f;
    float knockback = 350.f;
    float remaining = 0.15f;
    bool hit_checked = false;
};
```

Transient component representing an active ground slam attack (Brawler ability).
Emplaced when the player presses the bomb button, removed after `remaining`
expires. The hit check runs exactly once on the first tick, dealing AoE damage
and knockback to all enemies within `radius`.

### GroundSlamCooldown

```cpp
struct GroundSlamCooldown {
    float remaining = 0.f;
    float rate = 2.0f;
};
```

Cooldown timer for ground slam. Present only on Brawler entities; its existence
gates whether `update_ground_slam` activates. At 2.0 s cooldown the Brawler gets
one slam every two seconds.

### ChargedShot

```cpp
struct ChargedShot {
    float charge = 0.f;
    float charge_rate = 0.8f;
    float min_damage_mult = 0.5f;
    float max_damage_mult = 3.0f;
    float min_speed_mult = 1.0f;
    float max_speed_mult = 1.8f;
    float move_penalty = 0.5f;
    float full_charge_threshold = 0.95f;
    bool charging = false;
    bool was_shooting = false;
};
```

Persistent component that replaces the Sharpshooter's normal shooting with a
charge-and-release mechanic. While the shoot button is held, charge accumulates
at `charge_rate` (0.8 s to full). On release, a single bullet is fired with
damage and speed scaled by the charge level. Full charge produces a `Piercing`
bullet. The `move_penalty` (0.5x) slows movement while charging, applied by the
input system.

### ConcussionShot

```cpp
struct ConcussionShot {
    float radius = 45.f;
    float knockback = 400.f;
    float damage = 1.f;
    float remaining = 0.1f;
    bool hit_checked = false;
};
```

Transient component representing an active concussion shot blast (Sharpshooter
ability). Structurally identical to `GroundSlam` — AoE hit check on first tick,
expires after `remaining`. Emphasises knockback (400 px/s) over damage (1) for a
crowd-control role.

### ConcussionShotCooldown

```cpp
struct ConcussionShotCooldown {
    float remaining = 0.f;
    float rate = 3.0f;
};
```

Cooldown timer for concussion shot. Present only on Sharpshooter entities. At
3.0 s cooldown the Sharpshooter gets one blast every three seconds.

## Class recipes

| Attribute             | Brawler                             | Sharpshooter                                |
| --------------------- | ----------------------------------- | ------------------------------------------- |
| Speed                 | 100                                 | 150                                         |
| Lives                 | 3                                   | 2                                           |
| HP                    | 150                                 | 60                                          |
| MeleeStats damage     | 3                                   | 1                                           |
| MeleeStats range      | 35                                  | 20                                          |
| MeleeStats half_angle | 0.785 rad (~45 deg)                 | 0.4 rad (~23 deg)                           |
| MeleeStats knockback  | 300                                 | 150                                         |
| MeleeStats duration   | 0.12 s                              | 0.08 s                                      |
| Active ability        | GroundSlam (via GroundSlamCooldown) | ConcussionShot (via ConcussionShotCooldown) |
| Passive ability       | —                                   | ChargedShot                                 |
| Weapon: bullet_damage | 1 (default)                         | 1.5                                         |
| Weapon: fire_rate     | 0.2 s (default)                     | 0.3 s                                       |

## Character select scene

The title screen now transitions through a character select screen before
entering gameplay:

```
TitleScene  ──(confirm)──>  CharacterSelectScene  ──(confirm)──>  GameScene
```

`CharacterSelectScene` renders two coloured boxes (red for Brawler, blue for
Sharpshooter) at the centre of the 480x270 virtual resolution. Left/right input
toggles selection. The selected box renders in a bright colour; the unselected
one is dimmed. A blinking white underline beneath the active box and a blinking
"press confirm" prompt guide the player. On confirm, the scene passes the
selected `ClassId::Id` to `GameScene`'s constructor via swap.

## Systems

### Ground slam system

`update_ground_slam()` in `src/ecs/systems/ground_slam_system.cpp`:

1. **Tick cooldowns** — decrement all `GroundSlamCooldown::remaining` by `dt`.
2. **Activate** — on `bomb_pressed` with cooldown elapsed, no active
   `GroundSlam`, and no active `Dash`, emplace `GroundSlam` and reset cooldown.
3. **Hit check** — on the first tick of an active `GroundSlam` (`!hit_checked`):
   - Iterate all enemies with `(Transform2D, CircleHitbox, Enemy, Health)`.
   - For each enemy within `radius` (circle-circle overlap): deal damage, apply
     knockback radially away from the player (350 px/s, 0.15 s).
   - Set `hit_checked = true`.
4. **Expire** — tick `remaining`, remove `GroundSlam` when expired.

Enemies are collected into a `std::vector<HitInfo>` before applying effects,
matching the pattern used by `update_melee`.

### Charged shot system

`update_charged_shot()` in `src/ecs/systems/charged_shot_system.cpp`:

1. **Dash cancel** — if the entity has `Dash`, reset charge to zero and skip.
2. **Start charging** — on the shoot press edge
   (`input.shoot && !was_shooting`), set `charging = true` and `charge = 0`.
3. **Accumulate** — while shoot is held and `charging`, increment charge by
   `dt / charge_rate`. Clamp to 1.0.
4. **Fire on release** — when shoot is released while `was_shooting` and
   `charging`:
   - Compute `damage_mult` and `speed_mult` by linearly interpolating between
     min/max multipliers based on charge level.
   - Spawn a bullet via `spawn_bullet` with scaled damage and speed.
   - If charge >= `full_charge_threshold` (0.95), the bullet gets `Piercing`.
   - Reset charge and charging state.
5. **Track state** — store `was_shooting = input.shoot` for next-frame edge
   detection.

### Concussion shot system

`update_concussion_shot()` in `src/ecs/systems/concussion_shot_system.cpp`:

1. **Tick cooldowns** — decrement all `ConcussionShotCooldown::remaining` by
   `dt`.
2. **Activate** — on `bomb_pressed` with cooldown elapsed, no active
   `ConcussionShot`, and no active `Dash`, emplace `ConcussionShot` and reset
   cooldown.
3. **Hit check** — on the first tick (`!hit_checked`):
   - Iterate all enemies with `(Transform2D, CircleHitbox, Enemy, Health)`.
   - For each enemy within `radius` (circle-circle overlap): deal damage, apply
     knockback radially (400 px/s, 0.15 s).
   - Set `hit_checked = true`.
4. **Expire** — tick `remaining`, remove `ConcussionShot` when expired.

## Modified systems

| System            | Modification                                                                                                                                        |
| ----------------- | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| `update_melee`    | If entity has `MeleeStats`, copies its values into the new `MeleeAttack` instead of using defaults. See [Melee Attack and Dash](melee-and-dash.md). |
| `update_shooting` | Skips entities with `ChargedShot` — the charged shot system handles all bullet spawning for Sharpshooter.                                           |
| `update_input`    | If entity has `ChargedShot` and `charging == true`, multiplies movement speed by `move_penalty` (0.5x).                                             |

## HUD additions

Two new HUD elements are drawn by `render_hud()`:

**Ability cooldown bar** (bottom-left, 30x3 px) — shows the readiness of the
class active ability. Checks for `GroundSlamCooldown` (Brawler) or
`ConcussionShotCooldown` (Sharpshooter), whichever is present. Fills from empty
(on cooldown) to full (ready). Uses a dim cyan while recharging and a bright
cyan when fully ready.

**Charge indicator** (bottom-left, above ability bar, 30x3 px) — visible only
while `ChargedShot::charging` is true. Shows current charge level as an orange
fill. Turns bright yellow when charge reaches `full_charge_threshold`,
signalling that the shot will pierce.

## System pipeline order

```
update_charged_shot      <- charge accumulation, fire-on-release
update_input             (skip velocity during Dash, charge speed penalty)
update_melee             <- arc check, disarm, spawn pickups, MeleeStats override
update_dash              <- burst velocity, invulnerability
update_ground_slam       <- AoE damage, knockback
update_concussion_shot   <- AoE knockback
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
update_damage            <- score accumulation, game over flag
update_cleanup
update_waves             <- wave clear check, next wave spawn
check_exit_overlap       <- room transition trigger
game_over check          <- scene swap to GameOverScene
render_tilemap
render_sprites
render_hud               <- HUD overlay on top of gameplay
```

`update_charged_shot` runs first so it can set the `charging` flag before
`update_input` applies the movement speed penalty. The three ability systems
(`ground_slam`, `concussion_shot`, `charged_shot`) all run before
`update_shooting` so that the shooting system can correctly skip `ChargedShot`
entities.

## Tests

`tests/test_player_class.cpp` covers the class system with 20 test cases:

| Test case                                            | What it verifies                                                                                                            |
| ---------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------- |
| Brawler recipe sets correct stats                    | Speed, lives, HP, ClassId, MeleeStats, GroundSlamCooldown present, no ChargedShot or ConcussionShotCooldown                 |
| Sharpshooter recipe sets correct stats               | Speed, lives, HP, ClassId, MeleeStats, ChargedShot, ConcussionShotCooldown present, no GroundSlamCooldown, weapon overrides |
| MeleeStats overrides default MeleeAttack values      | Brawler melee uses MeleeStats damage (3), range (35), knockback (300)                                                       |
| Sharpshooter has weaker melee via MeleeStats         | MeleeStats damage (1), range (20), enemy takes 1 damage                                                                     |
| Ground slam hits nearby enemies                      | Enemy within radius takes damage and gets knockback                                                                         |
| Ground slam misses distant enemies                   | Enemy outside radius takes no damage                                                                                        |
| Ground slam cooldown prevents spam                   | Second slam blocked by 2.0 s cooldown                                                                                       |
| Ground slam blocked during dash                      | No GroundSlam emplaced while Dash is active                                                                                 |
| Charged shot fires on release with scaled damage     | Partial charge produces bullet with intermediate damage                                                                     |
| Tap-fire as Sharpshooter fires weak shot             | Immediate release fires bullet near min_damage_mult                                                                         |
| Full charge produces piercing shot                   | Charge >= threshold yields Piercing tag                                                                                     |
| Charge resets on dash                                | Dash clears charge and charging state                                                                                       |
| Charging applies movement speed penalty              | Input system velocity < normal speed while charging                                                                         |
| Shooting system skips ChargedShot entities           | Normal shooting produces no bullets for Sharpshooter                                                                        |
| Concussion shot hits nearby enemies with knockback   | Enemy within radius takes damage and knockback                                                                              |
| Concussion shot misses distant enemies               | Enemy outside radius takes no damage                                                                                        |
| Concussion shot cooldown prevents spam               | Second shot blocked by 3.0 s cooldown                                                                                       |
| Concussion shot blocked during dash                  | No ConcussionShot emplaced while Dash is active                                                                             |
| Brawler has no ChargedShot or ConcussionShotCooldown | Bomb input does not trigger concussion, charged shot system is inert                                                        |
| Sharpshooter has no GroundSlamCooldown               | Bomb input does not trigger ground slam                                                                                     |

## Key files

| File                                              | Role                                                                                                                   |
| ------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------- |
| `src/ecs/player_class.hpp/.cpp`                   | `apply_brawler`, `apply_sharpshooter` recipe functions                                                                 |
| `src/ecs/components.hpp`                          | `ClassId`, `MeleeStats`, `GroundSlam`, `GroundSlamCooldown`, `ChargedShot`, `ConcussionShot`, `ConcussionShotCooldown` |
| `src/ecs/systems/ground_slam_system.hpp/.cpp`     | Ground slam system                                                                                                     |
| `src/ecs/systems/charged_shot_system.hpp/.cpp`    | Charged shot system                                                                                                    |
| `src/ecs/systems/concussion_shot_system.hpp/.cpp` | Concussion shot system                                                                                                 |
| `src/ecs/systems/melee_system.cpp`                | MeleeStats override logic                                                                                              |
| `src/ecs/systems/shooting_system.cpp`             | ChargedShot skip                                                                                                       |
| `src/ecs/systems/input_system.cpp`                | Charge speed penalty                                                                                                   |
| `src/ecs/systems/hud_system.cpp`                  | Ability cooldown bar, charge indicator                                                                                 |
| `src/scenes/char_select_scene.hpp/.cpp`           | Character selection scene                                                                                              |
| `src/scenes/game_scene.hpp/.cpp`                  | Class recipe application, system wiring                                                                                |
| `tests/test_player_class.cpp`                     | Catch2 tests (20 cases)                                                                                                |
