# Room Progression and Waves

Raven's room progression system drives the gameplay loop: the player enters a
room, fights through waves of enemies, clears the room to open exits, and
transitions to the next room. Stages are defined in JSON files that specify
enemy composition per wave, while LDtk provides the spatial layout. A HUD
overlay shows health, lives, score, weapon decay, and wave progress throughout.

## Components

### Exit

Defined in `src/ecs/components.hpp`:

```cpp
struct Exit {
    std::string target_level; // LDtk level name to load.
    bool open = false;        // Active only after room is cleared.
};
```

Created from LDtk `Exit` entities during `enter_room`. Starts closed; opened by
`update_waves` when all waves are exhausted. `check_exit_overlap` detects player
collision with open exits to trigger room transitions.

### GameState

```cpp
struct GameState {
    int score = 0;             // Accumulated score for the session.
    int current_wave = 0;      // Index of the currently active wave.
    int total_waves = 0;       // Total number of waves in the current stage.
    bool room_cleared = false; // True when all waves are exhausted.
    bool game_over = false;    // True when the player has lost all lives.
};
```

Stored in registry context (`reg.ctx()`) as a singleton. Persists across rooms
for the entire session. Reset only when exiting `GameOverScene`.

## Stage data format

A `stage_manifest.json` lists stage files in play order:

```json
{
  "stages": [
    "assets/data/stages/stage_01.json",
    "assets/data/stages/stage_02.json",
    "assets/data/stages/stage_03.json"
  ]
}
```

Each stage file names an LDtk level and defines ordered waves:

```json
{
  "name": "stage_01",
  "level": "Test_Room",
  "waves": [
    {
      "enemies": [
        {
          "spawn_index": 0,
          "type": "grunt",
          "pattern": "spiral_3way",
          "hp": 1.0,
          "score": 100,
          "ai": "chaser",
          "contact_damage": true
        },
        {
          "spawn_index": 1,
          "type": "grunt",
          "pattern": "spiral_3way",
          "hp": 1.0,
          "score": 100,
          "ai": "drifter"
        }
      ]
    },
    {
      "enemies": [
        {
          "spawn_index": 0,
          "type": "mid",
          "pattern": "aimed_burst",
          "hp": 3.0,
          "score": 300,
          "ai": "stalker"
        }
      ]
    }
  ]
}
```

Enemy fields map to components: `type` becomes `Enemy::Type`, `ai` becomes
`AiBehavior::Archetype`, `pattern` resolves to a `BulletEmitter` via the
`PatternLibrary`, and `spawn_index` selects the Nth `EnemySpawn` position from
the tilemap.

## StageLoader

`StageLoader` in `src/ecs/systems/wave_system.hpp` follows the same manifest
pattern as `PatternLibrary`:

- `load_manifest(path)` — reads `stage_manifest.json`, calls `load_file` for
  each entry
- `load_file(path)` — parses a single stage JSON into a `StageDef`
- `load_from_json(j)` — loads from an already-parsed JSON object (used by tests)
- `get(index)` — returns `const StageDef*` by index, or `nullptr` if out of
  range
- `count()` — returns the number of loaded stages

Parsing maps JSON strings to enums via helper functions: `"grunt"/"mid"/"boss"`
to `Enemy::Type`, `"chaser"/"drifter"/"stalker"/"coward"` to
`AiBehavior::Archetype`. Default AI stats (speed, ranges) are assigned per
archetype by `make_ai()`.

## Wave system functions

Three free functions in `raven::systems`:

### spawn_wave

```
spawn_wave(reg, tilemap, stage, wave_index, patterns)
```

Creates enemy entities for the given wave index. For each `WaveEnemyDef`:

1. Resolve spawn position from tilemap `EnemySpawn` list (clamped to bounds;
   falls back to center if no spawns loaded)
2. Create entity with `Transform2D`, `Velocity`, `Enemy`, `Health`,
   `CircleHitbox`, `RectHitbox`, `Sprite`, `ScoreValue`, `AiBehavior`
3. If the pattern exists in `PatternLibrary`, add `BulletEmitter`
4. If `contact_damage` is true, add `ContactDamage`

### update_waves

```
update_waves(reg, tilemap, stage, patterns)
```

Called once per tick from `GameScene::update`. If `GameState` exists and the
room isn't already cleared or game over:

1. Count remaining `Enemy` entities — if any exist, return (wave in progress)
2. Increment `current_wave`
3. If more waves remain, call `spawn_wave` for the next wave
4. Otherwise, set `room_cleared = true` and open all `Exit` entities

### check_exit_overlap

```
check_exit_overlap(reg) -> std::string
```

Finds the player position, then iterates all `Exit` entities. Returns the
`target_level` of the first open exit whose position overlaps the player
(circle-circle check with 12 px exit radius and 6 px player radius). Returns an
empty string if no transition should occur.

## Room transitions

`GameScene::enter_room(game, level)` manages the transition:

1. **Clear non-player entities** — `clear_room_entities` iterates all entities,
   collects those without `Player`, and destroys them. This preserves the
   player's health, weapon, cooldowns, and score across rooms.
2. **Reload tilemap** — constructs a fresh `Tilemap` and loads the target LDtk
   level.
3. **Reposition player** — moves the player to the new room's `PlayerStart`
   spawn point (or center if none found).
4. **Spawn exit entities** — creates `Exit` entities from tilemap `Exit` spawn
   points, reading `target_level` from the spawn's fields map.
5. **Reset wave state** — sets `current_wave = 0`, `total_waves` from the stage
   definition, `room_cleared = false`.
6. **Spawn wave 0** — calls `spawn_wave` for the first wave.

When the player clears the final stage and steps on an exit, `GameScene::update`
increments `current_stage_` and checks for a next stage. If none exists, it
swaps to `TitleScene` (victory).

## Score and game over

**Score accumulation** happens in `damage_system.cpp`. When an enemy's health
reaches zero, `handle_enemy_death` checks for a `ScoreValue` component and adds
its points to `GameState::score`.

**Game over** triggers in `handle_player_death` when `player.lives` reaches
zero. It sets `GameState::game_over = true`. On the next tick,
`GameScene::update` detects the flag and swaps to `GameOverScene`.

`GameOverScene` captures the final score from `GameState` in `on_enter`, then
renders a dark red background, a title placeholder, score digits, and a blinking
restart prompt. Pressing confirm swaps to `TitleScene`. `on_exit` clears the
registry and erases `GameState` for a fresh start.

## HUD

`render_hud()` in `src/ecs/systems/hud_system.cpp` draws all HUD elements using
SDL primitives at the 480x270 virtual resolution:

| Element      | Position            | Visual                                                                      |
| ------------ | ------------------- | --------------------------------------------------------------------------- |
| Health bar   | Top-left (4, 4)     | 40x4 px, dark gray background, red fill (white when invulnerable)           |
| Lives pips   | Right of health bar | 4x4 px white squares, one per remaining life                                |
| Weapon decay | Below health bar    | 30x3 px, yellow fill proportional to remaining time                         |
| Score        | Top-right           | 5x7 px digit rectangles, right-aligned, brightness varies by digit value    |
| Wave dots    | Top-center          | 3x3 px dots — bright gray (completed), yellow (current), hollow (remaining) |

The decay timer only renders when the player has a `WeaponDecay` component
(i.e., is carrying a stolen weapon). All other elements are always visible.

## System pipeline order

```
update_input
update_melee
update_dash
update_shooting
update_emitters
update_ai
animation state logic
update_animation
update_movement
update_tile_collision
update_collision
update_pickups
update_weapon_decay
update_damage           <- score accumulation, game over flag
update_cleanup
update_waves            <- wave clear check, next wave spawn
check_exit_overlap      <- room transition trigger
game_over check         <- scene swap to GameOverScene
render_tilemap
render_sprites
render_hud              <- HUD overlay on top of gameplay
```

`update_waves` runs after `update_damage` so that enemies destroyed in the
current tick are already gone when the wave-clear check runs.
`check_exit_overlap` runs after waves so that exits opened this tick can be
detected immediately. `render_hud` runs last in the render pass to draw on top
of all gameplay sprites.

## Tests

`tests/test_waves.cpp` covers the wave, room, and scoring systems:

| Test case                                                             | What it verifies                                                   |
| --------------------------------------------------------------------- | ------------------------------------------------------------------ |
| `spawn_wave` creates correct enemy count                              | 3 enemies spawned from a 3-enemy wave                              |
| `spawn_wave` assigns contact damage to first enemy only               | `ContactDamage` on first, absent on second                         |
| `update_waves` advances to next wave when all enemies dead            | `current_wave` incremented, new enemies spawned                    |
| `update_waves` sets `room_cleared` when all waves exhausted           | `room_cleared = true` after last wave cleared                      |
| Exit entities marked open when room cleared                           | `Exit::open` set to true                                           |
| `check_exit_overlap` returns empty when exit closed                   | No transition from closed exit                                     |
| `check_exit_overlap` returns `target_level` when open and overlapping | Correct level string returned                                      |
| `check_exit_overlap` returns empty when player far from exit          | Distance check works                                               |
| Score accumulates on enemy death via `update_damage`                  | `GameState::score` incremented                                     |
| Game over flag set when player loses all lives                        | `GameState::game_over = true`                                      |
| `StageLoader` parses stage JSON correctly                             | All fields round-trip through JSON parsing                         |
| Enemy type strings map to correct enums                               | `"grunt"/"mid"/"boss"` and `"chaser"/"drifter"/"stalker"/"coward"` |

## Key files

| File                                     | Role                                                              |
| ---------------------------------------- | ----------------------------------------------------------------- |
| `src/ecs/components.hpp`                 | `Exit`, `GameState`, `ScoreValue`                                 |
| `src/ecs/systems/wave_system.hpp/.cpp`   | `StageLoader`, `spawn_wave`, `update_waves`, `check_exit_overlap` |
| `src/ecs/systems/hud_system.hpp/.cpp`    | `render_hud` (health, lives, score, decay, waves)                 |
| `src/ecs/systems/damage_system.cpp`      | Score accumulation, game over trigger                             |
| `src/scenes/game_scene.hpp/.cpp`         | `enter_room`, `clear_room_entities`, system wiring                |
| `src/scenes/game_over_scene.hpp/.cpp`    | Game over screen with score display                               |
| `assets/data/stages/stage_manifest.json` | Stage file manifest                                               |
| `assets/data/stages/stage_01.json`       | First stage definition                                            |
| `tests/test_waves.cpp`                   | Catch2 tests                                                      |
