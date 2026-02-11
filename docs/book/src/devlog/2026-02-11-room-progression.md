# Room Progression and HUD

Added data-driven wave spawning, multi-room progression, score tracking, game
over handling, and an in-game HUD — completing the gameplay loop from room entry
through victory or defeat.

## What changed

**Wave system** — enemies now spawn in waves defined by JSON stage files rather
than being hardcoded. A `StageLoader` reads a manifest of stage files (one per
room), each listing ordered waves of enemies with type, AI archetype, bullet
pattern, HP, score value, and spawn position index. When a wave is cleared
(all `Enemy` entities destroyed), the next wave spawns automatically.

**Room transitions** — when all waves in a stage are exhausted, `Exit` entities
open. Walking into an open exit triggers `enter_room`, which destroys all
non-player entities, reloads the tilemap for the next LDtk level, repositions
the player to `PlayerStart`, and spawns the first wave of the new stage. After
the final stage, the game returns to the title screen.

**Score tracking** — the `damage_system` awards `ScoreValue::points` to
`GameState::score` on enemy death. Score persists across rooms and is displayed
on the game over screen.

**Game over** — when the player's last life is lost, `damage_system` sets
`GameState::game_over = true`. `GameScene::update` detects the flag and swaps
to `GameOverScene`, which shows the final score and a blinking restart prompt.
Pressing confirm returns to the title screen.

**HUD** — a minimal SDL-primitive overlay renders health bar (red, white when
invulnerable), lives pips, weapon decay timer, score digits (right-aligned),
and wave progress dots (top-center). All drawing uses `SDL_RenderFillRect` and
`SDL_RenderDrawRect` — no font rendering needed.

**Stage JSON format** — a `stage_manifest.json` lists ordered stage file paths.
Each stage file names a level and defines waves of enemies. The `spawn_index`
field maps to the Nth `EnemySpawn` entity in the tilemap, decoupling enemy
composition from LDtk level layout.

## Design rationale

LDtk handles spatial layout (tile layers, spawn positions, exits) while JSON
stage files handle enemy composition and difficulty balancing. This separation
means level artists can rearrange spawn points without breaking wave tuning,
and designers can adjust enemy counts and types without editing the map.

Room transitions happen inside `GameScene` rather than through the scene
manager because the player entity must survive across rooms. `clear_room_entities`
uses an entity filter (keep anything with `Player`, destroy everything else) so
the player's health, weapon, and cooldowns carry over.

`GameState` lives in registry context as a singleton rather than as a component
because it represents session-wide state (score, wave index, game-over flag)
that doesn't belong to any single entity. This follows the same pattern EnTT
uses for `StringInterner` and `std::mt19937`.

## New files

- `src/ecs/systems/wave_system.hpp/.cpp` — `StageLoader`, `spawn_wave`, `update_waves`, `check_exit_overlap`
- `src/ecs/systems/hud_system.hpp/.cpp` — `render_hud` (health, lives, score, decay, waves)
- `src/scenes/game_over_scene.hpp/.cpp` — game over screen with score and blink prompt
- `assets/data/stages/stage_manifest.json` — stage file manifest
- `assets/data/stages/stage_01.json` — first stage (2 waves)
- `assets/data/stages/stage_02.json` — second stage
- `assets/data/stages/stage_03.json` — third stage
- `tests/test_waves.cpp` — 12 new Catch2 tests
- `docs/book/src/decisions/0009-data-driven-wave-definitions.md` — ADR
- `docs/book/src/architecture/room-progression.md` — architecture chapter

## Test results

72/72 tests pass, including 12 new tests covering wave spawning, wave
progression, room clear and exit logic, score accumulation, game-over flag,
and stage JSON parsing.
