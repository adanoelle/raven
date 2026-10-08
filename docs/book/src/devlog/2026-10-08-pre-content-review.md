# Full Code Review Before the Content Phase

Date: 2026-10-08 Tags: review, content-pipeline, ldtk, combat, input, tests

## Why

The next stretch of work is art and gameplay. Before switching over, the
whole tree got a correctness review aimed at one question: what will get
in the way of making content, and what can safely wait?

Six reviewers each took one area: core and build, scenes, combat
systems, world systems and rendering, audio and tests, and readiness for
art and gameplay work. Every item in "Fix Before Switching" was then
checked against the code by hand. The rest were traced through the code
by the reviewer who found them.

Baseline at the start: the Debug build is clean, 118 of 118 tests pass,
and clang-tidy reports nothing in project code. Line numbers below refer
to the code as reviewed, at commit `6377427`.

## The Verdict

The engine is done enough to stop working on it. What held up:

- **EnTT usage.** Destroys are deferred and checked with `valid()`.
  Removals during iteration only touch the entity being visited. No
  component stores a stale entity handle (the AI looks the player up
  every tick), and `Piercing::hit` compares versioned handles, so
  recycled ids never match.
- **Fixed timestep.** The step cap drains the accumulator, so alpha stays
  in [0, 1]. Previous transforms are snapshotted every tick, and spawns,
  room entry and teleports all set the previous position too.
- **Scene stack.** Deferred transitions never free the scene that asked
  for them mid-update. Pause's pop-then-swap applies in order. Shutdown
  pops every scene before the renderer is destroyed.
- **Files.** Saves use an atomic temp-and-rename write, asset paths are
  relative to the executable, and there is no `std::filesystem`.
- **Audio.** Everything runs on the main thread, voices are capped at
  32, finished streams are reaped, and a missing device or file
  degrades to silence.
- **Combat rules.** Team filtering, invulnerability frames, and one hit
  per source per tick are all correct and tested.

The problems are where content meets code. The shipped map can't
support the stage loop. The LDtk importer breaks under ordinary editor
habits. Bad data fails without saying so. And some class mechanics
bypass the weapon data they are supposed to use.

## Fix Before Switching

These block playtesting, or will bite on the first day of making levels
and art.

### B1. A run can't get past stage 1

- **Where:** `assets/maps/raven.ldtk`, `assets/data/stages/stage_02.json:3`,
  `stage_03.json:3`, `src/scenes/game_scene.cpp:158-170, 325-327`,
  `src/ecs/systems/wave_system.cpp:291-311`.
- **Problem:** The LDtk project has one level, `Test_Room`, and defines one
  entity type, `PlayerStart`. There is no `Exit` and no `EnemySpawn`.
  Stages 2 and 3 name levels `Room_02` and `Room_03`, which don't exist.
  When stage 1 is cleared there is no door to open, so the player is
  stuck, and the boss and victory screen are unreachable.
- **Also:** `check_exit_overlap` returns the exit's `target_level`, and the
  scene treats an empty string as "no overlap". An exit placed without
  that field is a dead door. When the field is set, the scene ignores
  its value anyway and goes to the next stage index.
- **Fix:** Add `EnemySpawn` and `Exit` entity definitions and the two
  missing rooms. Make the exit check report "touched an open exit"
  separately from the target. Log an error when a level fails to load or
  has no exit.

### B2. Enemies spawn on top of the player

- **Where:** `src/ecs/systems/wave_system.cpp:221-228`,
  `src/ecs/systems/ai_system.cpp:292-316`.
- **Problem:** With no spawn points, every enemy appears at the screen
  centre, (240, 135). `PlayerStart` is at (248, 136), 8.5 px away, and
  the two hit circles reach 13 px. Stage 1's chaser deals 15 contact
  damage on the first tick. Contact damage ignores the AI's idle phase
  and there is no spawn grace, so even with real spawn points a later
  wave can land on the player.
- **Fix:** Start each spawned enemy's contact cooldown running, and warn
  when a level has no spawn points.

### B3. The LDtk importer breaks under normal LDtk habits

- **Where:** `src/rendering/tilemap_loader.cpp:67, 75, 113-125`.
- **Problem:**
  - Every IntGrid layer, and every value above 0, becomes solid. An
    IntGrid "Floor" layer for auto-tiling turns the whole floor into
    walls.
  - Layers hidden with LDtk's eye toggle are skipped completely,
    including collision and entities. That toggle is an editor
    convenience, so hiding the collision layer while painting removes
    collision from the game.
  - A second IntGrid layer resizes the grid without clearing it, so
    cells from the two layers are combined, and rows scramble if the
    sizes differ.
  - Only the first tileset's texture is loaded. Tiles from a second
    tileset are drawn from the first one's image, with no warning.
- **Fix:** Read collision only from the IntGrid layer named `Collision`,
  build the grid fresh, apply the visibility flag to drawing only, and
  give each tile its own tileset texture.

### B4. Bad data fails without saying so

- **Where:** `src/core/game.cpp:90-124`, `src/ecs/systems/wave_system.cpp:19-36, 245`,
  `src/patterns/pattern_library.cpp:144-150, 240-243`,
  `src/rendering/sprite_sheet.cpp:54-57`, `assets/data/config.json`.
- **Problem:**
  - All of `config.json` is read inside one `try`. One bad sound entry
    (`"hit": 5`) or one sprite sheet missing `frame_h` throws, and every
    sprite sheet after it is skipped. `init` still succeeds, and the
    only sign is a single warning line.
  - An unknown enemy `type` or `ai` string becomes a grunt or chaser. A
    misspelled `pattern` gives an enemy that never fires. An unknown
    pattern `type` becomes radial, and `"linear"` parses but also
    behaves as radial.
  - A sprite frame outside its sheet is skipped without a log, so a
    sheet with fewer frames than the code expects makes sprites blink
    out.
  - The `"pickups"` sheet is used for weapon pickups and stabilizers
    (`melee_system.cpp:104`, `damage_system.cpp:74`) but never listed
    in `config.json`, so every pickup draws as the same grey box.
  - No test loads the shipped data, so none of this fails a build.
- **Fix:** Load each config entry on its own, warn on every unknown
  string, missing pattern, and out-of-range frame, register a pickups
  sheet, and add a test that loads the real stages, patterns, config
  and map and checks them against each other.

### B5. Pause can quit the run

- **Where:** `src/scenes/pause_scene.hpp:27`, `pause_scene.cpp:26-31`; the
  same pattern in the title and options menus.
- **Problem:** The menu's previous-direction value starts at 0, so a
  direction already held when the menu opens counts as a fresh press.
  Holding up while pressing Esc moves the cursor straight to QUIT TO
  TITLE, and confirm then ends the run with no prompt and no score.
- **Fix:** Move a menu cursor only when a direction is newly pushed, not
  when it was already held as the menu opened.

### B6. Bullets fly through walls

- **Where:** `src/ecs/systems/bullet_spawn.cpp:16-25`,
  `src/ecs/systems/tile_collision_system.cpp:12`.
- **Problem:** Bullets have no `RectHitbox`, so tile collision never sees
  them, and the bullet collision system never sees the tilemap. Cover
  does nothing. Enemies need line of sight to wake up, but then fire
  through walls.
- **Fix:** After movement, destroy any bullet whose centre is in a solid
  cell.

## Gameplay Bugs to Fix While Tuning

### G1. The charged shot ignores the weapon

`src/ecs/systems/charged_shot_system.cpp:40-68`. The Sharpshooter's
charged shot always fires one bullet. It ignores `bullet_count`,
`spread_angle` and `piercing`, so a stolen 16-bullet nova fires a single
shot and a spiral pickup fires one bullet instead of three. Releasing
while the cooldown is running also throws the whole charge away with no
shot. **Fix:** share the spread loop from `shooting_system.cpp:71-83`,
use `weapon.piercing || full_charge`, and buffer a release made during
the cooldown.

### G2. Stolen weapons keep enemy damage

`src/ecs/systems/pickup_system.cpp:111-125`. `weapon_from_emitter`
copies the enemy's bullet damage (10 to 25), which is tuned against
player health of 60 to 150. The player's default is 1 and stage enemies
have 1 to 10 HP, so a stolen spiral does about 360 damage per second and
every stolen weapon one-shots everything. **Fix, if not intended:** add
a `player_damage` field or scale factor to the pattern JSON.

### G3. Dying can count as winning

`src/scenes/game_scene.cpp:325` runs before `:340`. The exit check comes
before the game-over check, and the player entity still exists after
the final death. Dying on the last stage on the same tick the player
touches an open exit shows the victory screen. On other stages it loads
the next room, then game over a tick later. **Fix:** check game over
first.

### G4. Abandoned runs record no score

`src/scenes/pause_scene.cpp:44-45`, `src/core/game.cpp:267`. Only the
game-over and victory scenes call `record_score`. Quitting to the title
or closing the window drops the run. **Decide** whether abandoned runs
count. If they do, record in `GameScene::on_exit` only when the run is
not already ending through game over or victory.

### G5. Melee, slam and concussion only hit on their first tick

`src/ecs/systems/melee_system.cpp:72`, `ground_slam_system.cpp:39`,
`concussion_shot_system.cpp:39`. Hits are checked once, when the attack
starts. `MeleeStats::duration`, documented as the active hitbox duration,
only drives the animation and lockout. Enemies that walk into the arc
are never hit, and the Knight's dash-spin only hits where it started.
**Fix:** keep a per-attack hit list and re-check every tick while
active, or rename the field.

### G6. Enemies killed by abilities act for the rest of the tick

`src/scenes/game_scene.cpp:212-218` vs `:293-296`. The damage system
reaps the dead at the end of the tick, so an enemy killed by a slam can
still fire one emitter burst, deal contact damage, and absorb player
bullets. **Fix:** skip `Health.current <= 0` in the emitter, contact and
collision loops, or reap right after the ability systems.

### G7. Cooldowns drop the overshoot

`src/ecs/systems/shooting_system.cpp:51`, `emitter_system.cpp:122`, and
the melee and dash cooldowns. Cooldowns reset with `= rate`, which
throws away the time past zero, so every rate rounds up to whole ticks.
A 0.1 s fire rate takes 13 ticks (0.108 s, 8 % slow). **Fix:**
`remaining += rate`, floored.

### G8. Quick taps are lost at low frame rates

`src/core/input.cpp:99-104, 145-216`. Buttons are polled once per
frame. A press and release inside one frame never appear, so at 30 to 40
fps (a Steam Deck limit) or during a load hitch a quick dash, melee or
pause tap is lost. **Fix:** also latch presses from key, gamepad and
mouse button-down events in `process_event`.

### G9. Gamepad aim can drift toward a hidden cursor

`src/core/input.cpp:132-134`. Mouse movement is detected by comparing
positions. The stored position is 0 until `GameScene::on_enter` sets the
window, and a resize or fullscreen toggle changes the mapping, so both
count as mouse movement. A gamepad player's aim then follows a cursor
they never touched. **Fix:** mark the mouse as moved only from mouse
motion and button events.

### G10. End screens are easy to skip, and Start does nothing

`src/scenes/game_over_scene.cpp:33`, `victory_scene.cpp:34`. Only confirm
is checked, and confirm shares a button with shoot, so a held or tapped
shot skips the score screen within milliseconds. Start maps to pause,
which these screens ignore, although they say PRESS START. **Fix:**
ignore input for about 0.75 s and accept pause as well.

### G11. Alt-tab doesn't pause

`src/core/game.cpp:208-223`. Only the mobile and console background
event suspends the game. Alt-tab, minimising and the Steam overlay
don't, so the player can die off-screen. **Fix:** also suspend on focus
loss and minimise.

### G12. One bad field wipes a save file

`src/core/settings.cpp:13-17`, `save_data.cpp:13`, `game.cpp:45-46`. One
field of the wrong type (`"fullscreen": 1`, `"best_score": "500"`)
throws, and the whole file falls back to defaults. Settings then write
those defaults straight back over the user's file, and the next score
overwrites the save. A huge float converted to int is undefined
behaviour. This matters before ADR-0023 puts progression in the save.
**Fix:** read each field on its own with a type check and fallback, and
rename an unreadable file to `.bad` before overwriting it.

### G13. The decay explosion barely exists

`src/ecs/systems/pickup_system.cpp:82-92`. It deals 1 damage against
health pools of 60 to 150, has no sprite, and plays no sound. The header
promises 2 s of invulnerability; the code gives 0.5 s.

### G14. Some damage is silent

Contact damage (`ai_system.cpp:311`), the decay explosion, and melee,
slam and concussion hits (`ability_hits.hpp:76`) push no sound. **Fix:**
route player damage through one helper that grants invulnerability and
plays the hit sound.

### G15. Animation timing

- The final frame of a one-shot animation shows for about one tick: the
  hold stops when the end frame is reached, not after it has played
  (`game_scene.cpp:239`, `animation_system.cpp:15`).
- Melee and dash still use the walk row; the art spec puts attack on
  row 2 and dodge on row 3.
- A `frame_duration` of 0 or less loops forever in
  `animation_system.cpp:12`. Guard it before animations become data.

### G16. Tile collision cancels the whole move

`src/ecs/systems/tile_collision_system.cpp:42-48`. A blocked axis undoes
the move instead of pushing out to the wall, so entities stop up to
3 px short, more while dashing. Anything that starts inside a solid
cell can never move again; a boss spawned next to a wall freezes.
**Fix:** snap the blocked axis to the cell edge and push out of any
overlap that already exists.

### Smaller issues

- Grunts are drawn at 24 px from 16 px frames, a 1.5× scale
  (`wave_system.cpp:96` vs `config.json`).
- Bullets use layer 5 and characters 10, so bullets draw underneath
  characters; `art-integration.md` says the opposite.
- The per-tick sound dedupe doesn't stop the same sound stacking across
  several ticks in one frame (`game_scene.cpp:309-322`). Add a short
  retrigger window in `AudioEngine::play`.
- A level with no cell size divides by zero in the AI
  (`tilemap_loader.cpp:187`, `ai_system.cpp:45, 143`).
- The 360° melee spin misses about a fifth of targets exactly behind,
  from float rounding (`hitbox_math.hpp:63`); cones test the enemy's
  centre only, so large enemies are missed at the edges.
- The debug overlay's "Total" entity count shows destroyed entities, not
  live ones (`debug_overlay.cpp:93`).
- The RNG isn't reseeded on later runs, because `ctx().emplace` keeps an
  existing value (`game_scene.cpp:55`). The seed is never logged.
- While ImGui has the mouse, quit and gamepad hotplug events are
  dropped, but game input isn't, so dragging a debug slider fires shots
  (`game.cpp:147-160`).
- The music volume slider does nothing; there is no music yet.
- Character select has no back button.
- `SceneManager::pop` calls `on_exit` before removing the scene, so a
  transition requested from `on_exit` would pop the wrong scene. Nothing
  does this today.
- A missing WAV logs a warning on every play.
- Only one gamepad is tracked; unplugging it leaves a second one
  unused.
- Dead config that looks tunable: `ShootCooldown::rate`,
  `Dash::duration` (dash i-frames are hard-coded to 0.18 s),
  `Weapon::piercing` (never set outside tests), and the `gameplay`,
  `sprite_defs`, `tilemap` and `debug` blocks of `config.json`.

## Infrastructure for the Content Phase

What would most reduce friction once art and gameplay are the main
work, in priority order.

1. **Data-driven animation clips.** Player rows, frame counts and
   timings are a switch in `game_scene.cpp:247-275`. Enemies never
   animate: their frame is always 0. Aseprite tag timings are thrown
   away, and the export script ARCHITECTURE.md mentions doesn't exist.
   Per-sheet clips in `config.json`, ideally generated from Aseprite
   tags, plus an `Animation` on enemies, is the biggest art unblocker.
2. **Enemy definitions in JSON.** A new enemy type needs C++ in four
   places: the `Enemy::Type` enum, `parse_enemy_type`, `enemy_visuals`
   and `make_ai`. Two grunts can't look different. Move sheet, size,
   hitbox, clips, AI tuning, contact damage and drop rates into data,
   and have stages refer to enemies by id. Class stats can move to
   `classes.json` at the same time.
3. **Renderer basics.** `Transform2D.rotation` is set but drawn at 0.
   There is no per-sprite tint, alpha or blend mode, so no hit flash and
   no ADR-0012 additive ring. There is no camera shake. Layers are bare
   numbers. Exits and the decay explosion have no sprites. A
   `spawn_vfx` helper and the ADR-0022 impact-event queue would cover
   most feedback effects.
4. **Hot reload.** A debug key that reloads sprite sheets, patterns and
   stages in place. Assets are already symlinked into the build, so this
   turns Aseprite-to-game into seconds.
5. **A tuning overlay.** The overlay shows FPS, entity counts and the
   player position only. `debug.show_hitboxes` is never read. Add
   hitboxes, editing the selected entity's weapon, melee and AI values,
   spawning an enemy by id, room skip, and god mode.
6. **New sounds without C++.** `Sfx` is a fixed seven-entry enum plus a
   switch, so every sound listed as needed in `game-feel.md` needs code.

## Tests

- **Brittle numbers.** About 40 assertions check exact tuning values:
  class stats (`test_player_class.cpp:71-159`), dash speed and
  invulnerability (`test_melee_dash.cpp`), decay time and bullet speed
  (`test_pickups.cpp`), and the bullet sprite frame
  (`test_shooting.cpp:67-70`). The same numbers are repeated as magic
  constants in the systems. Name each constant once, have tests use the
  component defaults, and test class recipes by relative order.
- **No coverage:** losing a life and respawning (`damage_system.cpp:23-30`),
  `cleanup_system.cpp` (not even compiled into the tests),
  `GameScene::update` as a whole, `Clock::advance`, the scene manager's
  deferred transitions, and input press edges.
- **Tests that can't fail:** `test_shooting.cpp:313-331` loops over
  bullets without requiring one exists; `test_ai.cpp:115-149` never
  compares old and new velocity; `test_ecs.cpp:30-81` and
  `test_patterns.cpp:19-64` re-implement the formula instead of calling
  game code; `test_bitmap_font.cpp:202-210` and
  `test_player_class.cpp:598-603` have no real assertions;
  `test_pickups.cpp:476-484` has a dead loop; `test_audio.cpp` never
  plays a sound.
- **Build:** `tests/CMakeLists.txt` lists 27 game sources by hand, so
  each compiles twice, test code gets none of the project warnings, and
  LDtkLoader isn't linked. A `raven_core` static library would fix all
  three later.
- **Highest-value tests to add:** shipped-data validation; a headless
  gameplay smoke test (once the scene's system list is a free function);
  life loss and respawn; `update_cleanup`; `Clock::advance`.

## ADR-0023 Needs Revising Before It's Built

The salvage and upgrades plan conflicts with the current code in
several places:

- The Sharpshooter's piercing mod would do nothing (G1's root cause),
  and the ADR's claim that systems need zero changes is false.
- `GameState` is erased in the end scenes' `on_exit`, before the
  workshop's `on_enter`, so the arrival dialogue has no run data.
- Mods that *set* a field run after ranks, so they overwrite upgrades
  the player paid for. Apply mods as multipliers or deltas.
- `Weapon::fire_rate` is the interval between shots, so "+3-5 % per
  rank" would slow firing down.
- Damage and fire-rate ranks multiply, so the 1.2-1.25× offence ceiling
  is really about 1.5×. Cap the combined result.
- Salvage is banked only on death or victory, so quitting to the title
  loses it and players will learn to die instead. A single
  `Game::bank_run()` covers every way a run ends.
- The planned save fields can't support the dialogue conditions
  (lifetime salvage, deaths, first death per boss, victories) and have
  no version field, and a corrupt file wipes everything (G12).
- The v1 phasing ships no screen for spending salvage.
- Reusing `Exit` "exactly as `enter_room` does" leaves the workshop door
  locked, because exits only open when waves clear.
- `SalvageValue { int amount; float chance; }` can't hold the 1-2 and
  6-10 default ranges.
- The workshop's movement slice isn't reusable: player animation lives
  inline in `GameScene::update` and `spawn_player` is private.
- Picking up a stabilizer discards the upgraded class weapon for the
  rest of the run.
- The Decision section still says every run ends in the workshop; the
  phasing section says v1 doesn't.
- Pickup art is specified as 8×8; existing pickup frames are 16×16.

## Can Wait

- Packaging: SDL and Steam libraries are copied into `build/` but not
  installed, there is no `$ORIGIN` RPATH, the Windows exe isn't marked
  `WIN32`, and multi-config generators leave ImGui on in Release.
- Steam: initialise before the renderer and call
  `SteamAPI_RestartAppIfNecessary`.
- `fs::write_text` doesn't fsync before the rename; the header's
  power-loss promise only holds on ext4.
- ImGui draws under logical presentation; exact integer scaling;
  reloading textures on render device reset.
- `flake.nix`'s package can't build in the Nix sandbox (CPM fetches over
  the network); the dev shell forces `SDL_VIDEODRIVER=x11`.
- ADR-0005's "fs is the only file seam" is false for `IMG_Load` and
  `SDL_LoadWAV`. Only matters for the Switch port.
- The shop (ADR-0014) and salvage (ADR-0023) systems, spatial
  broadphase, bullet owner tags, atlas packing, LDtk parse caching, RNG
  portability, a visual pattern editor, and Switch toolchain work.
- `MAX_STEPS_PER_FRAME = 4` at 120 Hz means frames slower than about
  42 ms slow the game down instead of catching up. Fine for this genre,
  but a 30 fps cap sits right at the edge.

## Out-of-Date Docs

- `README.md:9` and `ARCHITECTURE.md:9` still say SDL2. ARCHITECTURE
  lists `tools/scripts/export_aseprite.sh` and `platform_desktop.cpp`,
  which don't exist.
- `architecture/room-progression.md` says `GameState` is reset only when
  leaving the game-over scene; it is now erased in `GameScene::on_enter`.
- `damage_system.hpp:13-14` says deaths drop weapons; they don't.
  `collision_system.hpp:9-10` describes AABB checks that aren't done.
- `art-integration.md:72` tells artists to add `sprite_defs`, which
  nothing reads. ADR-0004's example pattern type `"Aimed"` would become
  radial.

## Fixed in This Change

The six blockers, plus G3, which sat in the same code.

- **B1.** `raven.ldtk` gains `EnemySpawn` and `Exit` entity definitions
  (the exit has a nullable `target_level` string field), four spawn points
  and an exit in `Test_Room`, and placeholder `Room_02` and `Room_03` levels
  (copies of `Test_Room` with 2x2 pillars) for stages 2 and 3.
  `check_exit_overlap` now returns the exit itself, so a blank
  `target_level` no longer reads as "no overlap". Rooms still follow the stage
  list; a `target_level` that disagrees with it logs a warning. `enter_room`
  logs an error when a level fails to load or has no exit. Exits get a
  placeholder sprite from a new `props` sheet that switches to an open frame
  when the room clears, so players can see where to go.
- **B2.** Enemies with contact damage spawn with their contact cooldown
  already running for `SPAWN_CONTACT_GRACE` (1 s). The fallback position is
  the room centre, and a level with no spawn points or an out-of-range
  `spawn_index` logs a warning.
- **B3.** Only the IntGrid layer named `Collision` is solid, and its grid is
  built fresh. The eye toggle hides a layer's tiles but never its collision or
  entities. Each tileset gets its own texture. A level with no cell size
  fails to load instead of letting the AI divide by zero (G20's crash).
- **B4.** `config.json` loads entry by entry, and a malformed one is
  reported by its index. Unknown enemy types, AI strings, pattern names,
  emitter types and tiers all log a warning; `"linear"` warns that it isn't
  implemented. An out-of-range sprite frame and an unregistered sheet are each
  reported once. A new `pickups` sheet (weapon pickup, stabilizer) is
  registered. Sheet ids used from C++ live in `src/rendering/sheet_ids.hpp`.
  `tests/test_content.cpp` loads the shipped stages, patterns, config and map
  through the real loaders and checks them against each other and against
  those ids; run against the old map and config it fails on every problem
  listed in B1 and B4.
- **B5.** `Input` computes `up/down/left/right_pressed` edges the same way as
  the button edges, and all four menus use them, so a held direction no longer
  moves a cursor when a menu opens. This also retires the menu navigation
  code that was repeated across the title, pause and options scenes (an open
  item from the [2026-07-15 review](2026-07-15-architecture-review-fixes.md)).
- **B6.** `update_bullet_walls` destroys any bullet whose centre is in a
  solid cell. It runs after movement and before bullet collision, so a bullet
  can't hit something on the far side of a wall.
- **G3.** The game-over check now runs before the exit check.

`tools/gen_placeholder_sprites.py` generates the two new sheets, and takes
sheet names so it can regenerate some sheets without overwriting finished art
in the others.

Not verified here: playing through all three rooms by hand. The new tests
cover the pieces (exits, spawn grace, bullet walls, menu edges, the LDtk
rules and the shipped content), and the game starts cleanly with the new
sheets, but the full run still needs a playtest.

GCC's `-Wnull-dereference` reports a false positive inside EnTT's const
registry view, reached from `find_player_position` in `player_utils.hpp`
(seen in `emitter_system.cpp`, `ai_system.cpp` and `wave_system.cpp`). It
predates this change.

## Follow-up: Animation Clips

The first item on the content-phase list is done, as
[ADR-0024](../decisions/0024-animation-clips-from-aseprite.md). Animation
timing now comes from Aseprite's JSON export: `just export-art` writes each
sheet's PNG and tag data, and the game plays clips by tag name instead of
the frame table that was in `GameScene`. Enemies get an `idle` clip, so they
animate once their sheets have one. This also fixes G15: a one-shot clip's
last frame shows for its full duration, and a zero-length frame can no longer
hang the loop. The knight has only `idle` and `walk` tags so far, so its
attack and dash play the walk clip until those tags exist.

## Status

| Item | Status |
|------|--------|
| B1. Run can't pass stage 1 | Fixed |
| B2. Enemies spawn on the player | Fixed |
| B3. LDtk importer | Fixed |
| B4. Bad data fails silently | Fixed |
| B5. Pause can quit the run | Fixed |
| B6. Bullets through walls | Fixed |
| G3. Dying can count as winning | Fixed |
| G15. Animation timing | Fixed with the animation clips |
| G20. Level with no cell size divides by zero | Fixed (with B3) |
| Rest of G1-G16 and smaller issues | Open |
| 1. Data-driven animation clips | Done ([ADR-0024](../decisions/0024-animation-clips-from-aseprite.md)) |
| Rest of content-phase infrastructure | Open; next up is enemy definitions in JSON |
| Tests | Shipped-data validation added; the rest open |
| ADR-0023 revisions | Open |
| Out-of-date docs | `room-progression.md`'s `GameState` note and ARCHITECTURE's export script fixed; the rest open |
