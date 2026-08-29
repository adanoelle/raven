# 22. Salvage Materials and Engineer Weapon Upgrades

Date: 2026-08-29 Status: Proposed

## Context

Raven currently has no meta-progression. A run ends and nothing carries
forward except `SaveData::best_score`. Per-run power acquisition exists
(melee weapon steal, ADR-0008) and more is planned (coin-based NPC shop
rooms, ADR-0014), but there is no reason to come back after a failed run
beyond mastery itself.

ADR-0008 also left a known sore spot: enemies killed without being disarmed
yield nothing. The disarm mechanic must stay the *only* way to take a
weapon — that risk/reward loop is the combat identity — but a kill that
drops nothing at all makes ranged play feel unrewarded.

ADR-0014's open question 2 ("Should items be lost on death? ... A
meta-progression layer is out of scope for this ADR but could be a future
addition") explicitly deferred a persistent layer. This ADR is that layer.

**The reference model is Hades**, which makes death itself part of the
loop rather than a failure screen: every run ends back in a hub, a
character there acknowledges what happened, currencies collected during
the run are spent on permanent growth, and two currency tiers feed two
kinds of progression (Darkness → incremental Mirror ranks; Titan Blood →
weapon aspects that change how a weapon plays). Raven's version is far
smaller, but keeps those four properties.

The fiction writes itself from the existing enemy design: the machines the
player destroys are built from exotic alloys. An engineer NPC studying the
enemy's technology wants those materials, and pays for them in kind — by
machining permanent improvements into the player classes' own weapons.

## Decision

**Enemies drop salvage — a rare-alloy material — on death; bosses also
drop cores. Every run, won or lost, ends back at the engineer's workshop:
a small walkable hub room where the engineer reacts to the run, banked
salvage buys permanent stat ranks for the class's own kit, and cores
unlock weapon mods that change how a class weapon behaves. All of it
persists in `SaveData`.**

Three deliberate boundaries:

1. **Salvage is not the ADR-0014 coin.** Coins (if/when implemented) are a
   volatile in-run currency for in-run items. Salvage and cores are
   persistent meta-currencies. The two layers coexist; this ADR neither
   blocks nor depends on ADR-0014.
2. **Death drops never include weapons** (ADR-0008 unchanged). Salvage is
   the reward for the kill; the disarm remains the only path to a weapon.
3. **Upgrades apply to the class's own kit only** — the starting `Weapon`
   values and `MeleeStats` — never to stolen enemy weapons. Stolen weapons
   stay exactly as strong as their pattern defines, so upgrade progression
   cannot inflate the weapon-steal loop.

### The loop

```
kill enemy ──chance──> salvage drop ──walk over──> GameState::salvage (run wallet)
kill boss  ──always──> core drop    ──walk over──> GameState::cores

        run ends — death OR victory
                    ▼
   bank into SaveData, then wake in the
        ENGINEER'S WORKSHOP (hub room)
   engineer reacts to the run · spend salvage
   on stat ranks · spend cores on weapon mods
                    ▼
        door out → next run (char select)
                    ▼
   spawn_player: class recipe, then apply_upgrades()
   mutates Weapon/MeleeStats from SaveData
```

Death is no longer a dead end that dumps to the title screen — like
Hades, it delivers you (dented, but paid) to the person who benefits from
your failed expedition. Victory routes through the same room so the
relationship and the spending moment are constants of the loop.

### Drop model

- `SalvageValue { int amount; float chance; }` on enemies, parallel to
  `ScoreValue`, populated from the stage JSON (optional `salvage` field)
  with per-tier defaults when absent:
  - Grunt: 35% chance of 1
  - Mid: 100% chance of 1–2
  - Boss: 100% chance of 6–10, **plus exactly one core**
- Rolls use the registry-context `std::mt19937` and follow the stabilizer
  convention in `handle_enemy_death`: no RNG in context → no drop (keeps
  headless tests deterministic).
- Multiple pieces scatter with small positional offsets so a boss kill
  visibly showers salvage.
- Drop entities use the established pickup recipe (`Transform2D`,
  `PreviousTransform`, `CircleHitbox`, `Lifetime{8}`, `Sprite` on the
  `pickups` sheet, payload component `SalvagePickup { int amount; }` —
  cores are `CorePickup{}` with a longer `Lifetime{15}`; losing a boss
  core to a despawn timer would be brutal).

### Collection and run wallet

- A third pass in `update_pickups` collects every overlapping
  `SalvagePickup`/`CorePickup` (no one-per-frame limit; a shower should
  hoover up in one step), adds to `GameState::salvage` / `::cores`, plays
  `Sfx::Pickup` (a distinct salvage sound is polish, see below), and
  defers destruction (ADR-0007).
- The HUD shows a salvage counter top-right under the score, tinted to
  match the pickup sprite so the association is learned visually. Cores
  appear beside it only when nonzero.

### Banking

- On run end — death or victory — the run's salvage and cores are added
  to `SaveData` and saved immediately (same pref-path `save.json`,
  ADR-0017 conventions), before the workshop scene enters.
- **The player keeps 100% on death.** That is the Hades contract: dying
  hurts, but the trip still paid. Score remains the prestige metric. A
  death penalty stays available as a tuning lever if progression proves
  too fast.
- The death/victory splash (existing scenes, kept brief) shows
  `SALVAGE RECOVERED: n` so the bank moment is legible, then confirms
  into the workshop instead of the title screen.

### The engineer's workshop (hub room)

Not a menu — a small walkable room, entered after every run and visitable
from the title screen:

- An LDtk level (`workshop`) with a `PlayerStart`, an `Engineer` spawn
  point, and an `Exit` door that leads to character select / the next
  run. The scene runs the movement slice of the pipeline only —
  `update_input`, `update_movement`, `update_tile_collision`,
  `update_animation`, render systems — no combat, no waves, no damage.
  This reuses `Tilemap::find_spawn` and the `Exit` idiom exactly as
  `GameScene::enter_room` does.
- Walking up to the engineer shows a one-line interaction prompt;
  confirm pushes an **upgrade overlay** on the scene stack (the
  `PauseScene` overlay pattern: dim the room, render rows over it), so
  the room stays visible behind the catalog.
- **Evolving dialogue, Hades-style.** The engineer greets every arrival
  with the highest-priority line whose conditions match, drawn from
  `assets/data/npcs/engineer.json`. Conditions cover: first meeting,
  arrived-after-death vs. arrived-after-victory, salvage delivered this
  run, lifetime salvage thresholds (her research milestones — this is
  the "thank you for funding the work" beat), first core, class played,
  fully-upgraded. Entries carry `id`, `conditions`, `priority`, and a
  `once` flag; `SaveData` records seen ids and a visit counter. Writing
  more flavor never touches code.

### Two currencies, two kinds of growth

Mirroring Hades' Darkness/Titan Blood split, with fields that already
exist on `Weapon` and `MeleeStats`:

- **Salvage → stat ranks** (the Mirror of Night analog). Incremental,
  multi-rank tracks per class: `Weapon::bullet_damage`, `fire_rate`,
  `bullet_speed`, `MeleeStats::damage`, `range`. Escalating costs,
  modest ceilings — a maxed class should feel sharper, not trivialized.
- **Cores → weapon mods** (the weapon-aspect analog). One-time unlocks
  that change behavior rather than numbers, toggleable in the workshop
  (equip at most one per class to start). Launch candidates use existing
  `Weapon` fields no upgrade currently touches: a Sharpshooter piercing
  mod (`piercing = true`), a Brawler scatter mod (`bullet_count = 3`,
  `spread_angle` up, `bullet_damage` down). Mods are where class
  identity deepens; ranks are where it solidifies.

Both live in `assets/data/upgrades/upgrade_manifest.json`, loaded by an
`UpgradeCatalog` class copying the `PatternLibrary` loader shape,
including the `load_from_json` overload so tests build catalogs inline.
Schema per entry: `id`, `class`, `kind` (`rank` | `mod`), `name`, `desc`,
`stat`/`effects`, `per_level` or values, `max_level`, `costs[]`
(salvage) or `cost_cores`.

### Applying upgrades

Following the class-recipe pattern (ADR-0011): a free function
`apply_upgrades(reg, entity, catalog, save_data)` runs in `spawn_player`
*after* the class recipe and mutates component values in place — ranks
first, then the equipped mod. Systems need zero changes — they already
read `Weapon` and `MeleeStats` values each tick. The function is
trivially testable headlessly: build registry, apply recipe, apply
upgrades, assert values.

## ECS integration

**New components** (`components.hpp`):

- `SalvageValue { int amount = 1; float chance = 1.f; }` — on enemies
- `SalvagePickup { int amount = 1; }` / `CorePickup {}` — on drop entities
- `Engineer {}` — tag on the workshop NPC entity, drives the interaction
  prompt
- `GameState` gains `int salvage = 0; int cores = 0;` (run wallet;
  existing erase-then-emplace lifecycle in `GameScene::on_enter` already
  handles the reset)

**Changed systems** (no new system files):

- `damage_system.cpp` / `handle_enemy_death` — spawn salvage/core drops
  at the existing "no death drops" comment, beside the stabilizer roll
- `pickup_system.cpp` / `update_pickups` — salvage/core collection pass
- `hud_system.cpp` — salvage (+ core) counter
- `wave_system.cpp` — read optional `salvage` field from stage JSON,
  emplace `SalvageValue`

**New modules:**

- `src/ecs/upgrades.{hpp,cpp}` — `UpgradeCatalog` (manifest +
  `load_from_json`), purchase/unlock math, and `apply_upgrades` (sits
  beside `player_class.cpp`; added to both `CMakeLists.txt` and
  `tests/CMakeLists.txt` so tests reach the math without a scene)
- `src/scenes/workshop_scene.{hpp,cpp}` — the hub room: movement-slice
  pipeline, engineer proximity prompt, dialogue line via `BitmapFont`
- `src/scenes/upgrade_overlay_scene.{hpp,cpp}` — catalog browse/buy
  overlay, `PauseScene` idiom

**Changed core:**

- `save_data.{hpp,cpp}` — `salvage_banked`, `cores_banked`,
  `upgrade_levels` (map `"class/upgrade_id"` → rank), `mods_unlocked`,
  `mod_equipped` per class, `dialogue_seen`, `workshop_visits`; JSON
  round-trip, defensive load (corrupt file → defaults, never a crash)
- `title_scene.cpp` — `WORKSHOP` menu entry (visit without running)
- `game_over_scene.cpp` / `victory_scene.cpp` — banking + recovered
  line + confirm routes to `WorkshopScene` instead of the title

**New data/assets** (no CMake changes needed):

- `assets/data/upgrades/upgrade_manifest.json`
- `assets/data/npcs/engineer.json`
- A `workshop` level in `assets/maps/raven.ldtk` with an `Engineer`
  entity
- Stage JSONs optionally gain per-enemy `salvage` fields

**Polish (not v1-blocking):**

- A real `pickups.png` sprite sheet + `config.json` entry — today weapon
  and stabilizer drops already render as gray fallback rects; salvage
  and cores make four pickup types on one missing sheet, which is the
  moment to draw it
- Distinct `Sfx::Salvage` (new enum value before `Count`, name mapping,
  `config.json` sound entry, WAV) — until then, `Sfx::Pickup`
- Magnet drift (pickups within a radius drift toward the player)
- Engineer idle animation, portrait, workshop ambience track

## Sequencing and the hit-mechanics branch

A hit-mechanics branch is in flight and owns the death/damage flow this
feature hooks (`damage_system.cpp`, likely `collision_system.cpp` /
`melee_system.cpp` / `components.hpp`). Implementation therefore waits
for that merge; the only hard coupling is "where an enemy death is
detected", so the plan survives any reshaping of hit resolution short of
moving death handling out of the damage system.

Implementation order once unblocked (each step builds, tests, and could
ship alone):

1. **Drops + wallet + HUD** — `SalvageValue`, `SalvagePickup`,
   death-roll, collection pass, counter. Tests: seeded-RNG drop rolls,
   no-RNG → no drop, probability band, collection increments wallet,
   multi-pickup same-tick collection.
2. **Banking + persistence** — `SaveData` fields + round-trip, end-scene
   banking, recovered line. Tests: JSON round-trip, corrupt-file
   defaults.
3. **Catalog + application** — `UpgradeCatalog` (ranks first),
   `apply_upgrades`, wire into `spawn_player`. Tests: inline-JSON
   catalog load with clamping, purchase math, upgrade math per class,
   stolen weapons unaffected.
4. **Workshop hub** — LDtk room, movement-slice scene, engineer prompt +
   dialogue conditions, upgrade overlay, end-scene routing. Scene logic
   thin; purchase and dialogue-selection math live in `upgrades.cpp` /
   data where tests reach them.
5. **Cores + mods** — `CorePickup`, boss drop, mod entries, equip
   toggle. Tests: mod application (piercing/scatter fields), equip
   exclusivity.

## Consequences

**Positive:**

- Every kill can pay something, fixing ADR-0008's "ranged kills feel
  unrewarded" without touching the disarm loop
- Death becomes a narrative beat that pays out, not a failure screen —
  the Hades property that most changes how losing feels
- First persistent progression layer; the engineer gives the game a
  recurring face, and her research milestones give lifetime salvage a
  meaning beyond the spend
- Ranks vs. mods split gives both steady growth and build identity
  without inventing new combat systems — mods only set existing
  `Weapon` fields
- Data-driven catalog and dialogue iterate without recompiling
- Answers ADR-0014's open question 2 in a way that composes with, rather
  than replaces, its in-run shop
- Reuses established patterns end to end: pickup recipe, ctx RNG
  convention, deferred destruction, manifest loaders, class recipes,
  overlay scenes, `Exit`/spawn-point rooms, pref-path persistence

**Negative:**

- Permanent upgrades erode difficulty over time; costs must escalate
  steeply and stat ceilings stay modest — balancing this is ongoing
  work, not a one-time task
- The hub room adds real content scope a menu would not: an LDtk level,
  an NPC sprite, dialogue writing, an overlay UI — the price of the
  Hades feel
- Routing every run's end through the workshop adds a step for players
  who just want to retry; the door layout must make "straight back in"
  take under ~5 seconds
- Two meta-currencies plus ADR-0014's future coins is three currencies;
  distinct visual language and drop feel are required, not optional
- `SaveData` grows from one int to structured state — corrupt-file
  handling and forward compatibility now actually matter
- Meta-progression softens the roguelike purity ADR-0014 noted; players
  who want a fixed challenge may eventually want a "no upgrades" toggle

## Open questions

1. **Material fiction and names.** "Salvage" and "cores" are working
   mechanical terms; flavor names open.
2. **Engineer identity.** Name, look, and voice are open. Working
   assumption: she is the game's first named NPC and may later anchor
   the ADR-0014 shop cast.
3. **Upgrade respec.** Ranks: no refunds in v1 (escalating costs make
   picks matter). Mods: freely re-equippable in the workshop, Hades
   aspect-style.
4. **Gifting.** Hades deepens NPC bonds with a gift item. A Raven analog
   (donate a stolen legendary weapon to her research?) is evocative but
   out of scope; noted so the dialogue-condition system doesn't preclude
   it.
5. **Co-op split.** Shared wallet vs. per-player collection — deferred to
   the co-op milestone, but `GameState::salvage` as a single int assumes
   shared; revisit then.
