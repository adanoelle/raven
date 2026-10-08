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

Created from LDtk `Exit` entities during `enter_room`, with a placeholder
sprite from the `props` sheet. Starts closed; opened by `update_waves` when all
waves are exhausted, which also switches the sprite to its open frame.
`check_exit_overlap` detects player collision with open exits to trigger room
transitions.

Rooms follow the stage list, not `target_level`. When the player takes an
exit whose `target_level` doesn't match the next stage's level, the game logs
a warning and follows the stage list; `tests/test_content.cpp` checks the
shipped map for this.

### GameState

```cpp
struct GameState {
    int score = 0;             // Accumulated score for the session.
    int current_wave = 0;      // Index of the currently active wave.
    int total_waves = 0;       // Total number of waves in the current stage.
    bool room_cleared = false; // True when all waves are exhausted.
    bool game_over = false;    // True when the player has lost all lives.
    ClassId::Id player_class = ClassId::Id::Brawler; // Class used this session.
};
```

Stored in registry context (`reg.ctx()`) as a singleton. Persists across rooms
for the entire session. `GameScene::on_enter` erases and recreates it, so every
run starts fresh.

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

Each stage file names an LDtk level and defines ordered waves. A wave entry
places an enemy by name at a spawn point:

```json
{
  "name": "stage_01",
  "level": "Test_Room",
  "waves": [
    {
      "enemies": [
        { "spawn_index": 0, "enemy": "grunt_chaser" },
        { "spawn_index": 1, "enemy": "grunt_drifter" }
      ]
    },
    {
      "enemies": [{ "spawn_index": 0, "enemy": "mid_stalker" }]
    }
  ]
}
```

`spawn_index` selects the Nth `EnemySpawn` position from the tilemap.
`enemy` names a definition in `assets/data/enemies.json`
([ADR-0025](../decisions/0025-named-enemy-definitions.md)), which holds
everything about that kind of enemy:

```json
"grunt_chaser": {
    "tier": "grunt",
    "sheet": "enemies",
    "offset_y": -3,
    "hitbox": { "radius": 7, "rect": [12, 14] },
    "hp": 1,
    "score": 100,
    "ai": { "archetype": "chaser", "move_speed": 70, "activation_range": 160, "attack_range": 80 },
    "pattern": "spiral_3way",
    "contact_damage": 15
}
```

| Key               | Required | Default / meaning                                                   |
| ----------------- | -------- | ------------------------------------------------------------------- |
| `tier`            | yes      | `grunt`, `mid` or `boss`                                            |
| `sheet`           | yes      | Sprite sheet id from `config.json`                                  |
| `ai.archetype`    | yes      | `chaser`, `drifter`, `stalker` or `coward`                          |
| `ai.move_speed`, `ai.activation_range`, `ai.preferred_range`, `ai.attack_range` | no | The archetype's defaults (`default_ai()`) |
| `size`            | no       | `[w, h]` drawn size; omitted draws the sheet's frame size           |
| `offset_y`        | no       | 0; negative moves the sprite up to line up the feet                 |
| `hitbox`          | no       | `radius` 7, `rect` [12, 14]                                         |
| `hp`, `score`     | no       | 1 and 100                                                           |
| `pattern`         | no       | None: the enemy doesn't shoot                                       |
| `contact_damage`  | no       | 0: no body damage. Otherwise damage per hit                         |
| `stabilizer_drop` | no       | Tier default: boss 1, mid 0.15, grunt 0                             |

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

A wave entry with no `enemy` name (such as one still in the old inline
format) is reported and skipped. `EnemyLibrary` (`src/ecs/enemy_library.hpp`)
loads `enemies.json`: a definition with an unknown tier or archetype, or a
value out of range, is reported and skipped, and an unknown key gets a
"typo?" warning. `GameScene` reloads both at the start of every run.

## Wave system functions

Three free functions in `raven::systems`:

### spawn_wave

```
spawn_wave(reg, tilemap, stage, wave_index, patterns, enemies)
```

Creates enemy entities for the given wave index. For each `WaveEnemyDef`,
it looks up the named `EnemyDef` (skipping, with a warning, a name that isn't
defined), then:

1. Resolve spawn position from tilemap `EnemySpawn` list (clamped to bounds;
   falls back to the room centre if the level has none). A missing spawn list
   or an out-of-range `spawn_index` logs a warning.
2. Create entity with `Transform2D`, `Velocity`, `Enemy`, `StabilizerDrop`,
   `Health`, `CircleHitbox`, `RectHitbox`, `Sprite`, `ScoreValue`,
   `AiBehavior` and an `Animation` on the `idle` clip, all from the
   definition
3. If the pattern exists in `PatternLibrary`, add `BulletEmitter`. An unknown
   pattern name logs a warning; an empty one means the enemy doesn't shoot.
4. If `contact_damage` is above 0, add `ContactDamage` with that damage and
   its cooldown already running for `SPAWN_CONTACT_GRACE` (1 s), so an enemy
   that appears on top of the player can't hit them before they can react.

### update_waves

```
update_waves(reg, tilemap, stage, patterns, enemies)
```

Called once per tick from `GameScene::update`. If `GameState` exists and the
room isn't already cleared or game over:

1. Count remaining `Enemy` entities — if any exist, return (wave in progress)
2. Increment `current_wave`
3. If more waves remain, call `spawn_wave` for the next wave
4. Otherwise, set `room_cleared = true` and open all `Exit` entities

### check_exit_overlap

```
check_exit_overlap(reg) -> const Exit*
```

Finds the player position, then iterates all `Exit` entities. Returns the
first open exit whose position overlaps the player (circle-circle check with
12 px exit radius and 6 px player radius), or `nullptr` if no transition
should occur. An exit with an empty `target_level` still counts.

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
swaps to `VictoryScene`, which shows the final score and the high-score table.

The game-over check runs before the exit check. The player entity outlives its
final death, so dying on the same tick as touching an exit ends the run
instead of counting as clearing the stage.

`enter_room` logs an error when the level fails to load or has no `Exit`,
since either one leaves the run stuck in that room.

## Score and game over

**Score accumulation** happens in `damage_system.cpp`. When an enemy's health
reaches zero, `handle_enemy_death` checks for a `ScoreValue` component and adds
its points to `GameState::score`.

**Game over** triggers in `handle_player_death` when `player.lives` reaches
zero. It sets `GameState::game_over = true`. On the next tick,
`GameScene::update` detects the flag and swaps to `GameOverScene`.

`GameOverScene` captures the final score from `GameState` in `on_enter`, then
renders a dark red background, bitmap-font title and score text
([ADR-0018](../decisions/0018-bitmap-font-text.md)), and a blinking
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
| Score        | Top-right           | Bitmap-font text, right-aligned ([ADR-0018](../decisions/0018-bitmap-font-text.md)) |
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
| `spawn_wave` starts contact damage on a grace cooldown                | `ContactDamage::timer` starts at `SPAWN_CONTACT_GRACE`             |
| `check_exit_overlap` returns nullptr when exit closed                 | No transition from closed exit                                     |
| `check_exit_overlap` returns the exit when open and overlapping       | The overlapped exit is returned                                    |
| `check_exit_overlap` triggers for an exit with no `target_level`      | The final room's exit still works                                  |
| `check_exit_overlap` returns nullptr when player far from exit        | Distance check works                                               |
| Score accumulates on enemy death via `update_damage`                  | `GameState::score` incremented                                     |
| Game over flag set when player loses all lives                        | `GameState::game_over = true`                                      |
| `StageLoader` parses stage JSON correctly                             | All fields round-trip through JSON parsing                         |
| `StageLoader` skips wave entries that name no enemy                   | Old inline entries are reported, not silently defaulted            |
| `spawn_wave` builds enemies from their definitions                    | Every component comes from the `EnemyDef`                          |
| `spawn_wave` skips enemies with no definition                         | A misspelled name spawns nothing and logs a warning                |
| Enemy type and AI parsers reject unknown strings                      | Typos such as `"Boss"` are not silently accepted                   |

## Key files

| File                                     | Role                                                              |
| ---------------------------------------- | ----------------------------------------------------------------- |
| `src/ecs/components.hpp`                 | `Exit`, `GameState`, `ScoreValue`                                 |
| `src/ecs/systems/wave_system.hpp/.cpp`   | `StageLoader`, `spawn_wave`, `update_waves`, `check_exit_overlap` |
| `src/ecs/enemy_library.hpp/.cpp`         | `EnemyLibrary`, `EnemyDef`, `default_ai`                          |
| `assets/data/enemies.json`               | Enemy definitions, by name                                        |
| `src/ecs/systems/hud_system.hpp/.cpp`    | `render_hud` (health, lives, score, decay, waves)                 |
| `src/ecs/systems/damage_system.cpp`      | Score accumulation, game over trigger                             |
| `src/scenes/game_scene.hpp/.cpp`         | `enter_room`, `clear_room_entities`, system wiring                |
| `src/scenes/game_over_scene.hpp/.cpp`    | Game over screen with score display                               |
| `assets/data/stages/stage_manifest.json` | Stage file manifest                                               |
| `assets/data/stages/stage_01.json`       | First stage definition                                            |
| `tests/test_waves.cpp`                   | Catch2 tests                                                      |
| `tests/test_enemies.cpp`                 | Enemy definition loading and validation                           |
