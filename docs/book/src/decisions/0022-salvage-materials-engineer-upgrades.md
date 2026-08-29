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

The fiction writes itself from the existing enemy design: the machines the
player destroys are built from exotic alloys. An engineer NPC studying the
enemy's technology wants those materials, and pays for them in kind — by
machining permanent improvements into the player classes' own weapons.

## Decision

**Enemies have a chance to drop salvage — a rare-alloy material — on
death. Salvage collected during a run is banked when the run ends, and
spent between runs at the engineer's workshop on permanent, per-class
weapon upgrades stored in `SaveData`.**

Three deliberate boundaries:

1. **Salvage is not the ADR-0014 coin.** Coins (if/when implemented) are a
   volatile in-run currency for in-run items. Salvage is a persistent
   meta-currency. The two coexist; this ADR neither blocks nor depends on
   ADR-0014.
2. **Death drops never include weapons** (ADR-0008 unchanged). Salvage is
   the reward for the kill; the disarm remains the only path to a weapon.
3. **Upgrades apply to the class's own kit only** — the starting `Weapon`
   values and `MeleeStats` — never to stolen enemy weapons. Stolen weapons
   stay exactly as strong as their pattern defines, so upgrade progression
   cannot inflate the weapon-steal loop.

### The loop

```
kill enemy ──chance──> salvage drop ──walk over──> GameState::salvage (run wallet)
                                                        │ run ends (death or victory)
                                                        ▼
                                          SaveData::salvage_banked (persistent)
                                                        │ between runs
                                                        ▼
                                   Engineer's Workshop (from title menu)
                                     spend salvage → upgrade_levels
                                                        │ next run
                                                        ▼
                                   spawn_player: class recipe, then
                                   apply_upgrades() mutates Weapon/MeleeStats
```

### Drop model

- `SalvageValue { int amount; float chance; }` on enemies, parallel to
  `ScoreValue`, populated from the stage JSON (optional `salvage` field)
  with per-tier defaults when absent:
  - Grunt: 35% chance of 1
  - Mid: 100% chance of 1–2
  - Boss: 100% chance of 6–10
- Rolls use the registry-context `std::mt19937` and follow the stabilizer
  convention in `handle_enemy_death`: no RNG in context → no drop (keeps
  headless tests deterministic).
- Multiple pieces scatter with small positional offsets so a boss kill
  visibly showers salvage.
- Drop entities use the established pickup recipe (`Transform2D`,
  `PreviousTransform`, `CircleHitbox`, `Lifetime{8}`, `Sprite` on the
  `pickups` sheet, payload component `SalvagePickup { int amount; }`).
  They despawn on room exit like everything else non-player — collect it
  before you leave.

### Collection and run wallet

- A third pass in `update_pickups` collects every overlapping
  `SalvagePickup` (no one-per-frame limit; a shower should hoover up in
  one step), adds to `GameState::salvage`, plays `Sfx::Pickup` (a distinct
  salvage sound is polish, see below), and defers destruction (ADR-0007).
- The HUD shows a salvage counter top-right under the score, tinted to
  match the pickup sprite so the association is learned visually.

### Banking

- On run end — `GameOverScene::on_enter` and `VictoryScene::on_enter` —
  the run's salvage is added to `SaveData::salvage_banked` and saved
  immediately (same pref-path `save.json`, ADR-0017 conventions).
- **The player keeps 100% of collected salvage on death.** Score is the
  prestige metric; salvage is the retention metric, and taxing it on death
  punishes exactly the players the meta-layer exists to keep. A death
  penalty (e.g. bank 60%) stays available as a tuning lever if
  progression proves too fast.
- Both end screens show a `SALVAGE RECOVERED: n` line so the bank moment
  is legible.

### Engineer's workshop

- A new `WorkshopScene`, entered from a `WORKSHOP` item on the title menu
  (between runs — the engineer's lab is not part of the run map; ADR-0014
  shop *rooms* remain the in-run venue).
- The engineer is a named NPC with a medium-tier sprite (32x32 frame per
  ADR-0014's NPC sizing) and short, state-aware dialogue: first visit,
  fresh salvage delivered (the congratulation / "this funds my research"
  beat), nothing to spend, and fully-upgraded. Lines live in
  `assets/data/npcs/engineer.json` and are picked by condition, random
  within a condition, so writing more flavor never touches code.
- The upgrade catalog is data-driven
  (`assets/data/upgrades/upgrade_manifest.json`), loaded by an
  `UpgradeCatalog` class copying the `PatternLibrary` loader shape,
  including the `load_from_json` overload so tests build catalogs inline.
- Catalog schema per upgrade: `id`, `class` (`brawler` | `sharpshooter` |
  `all`), `name`, `desc`, `stat`, `per_level` delta (or multiplier),
  `max_level`, `costs[]` (one entry per level, escalating).
- v1 stat targets, all fields that already exist: `Weapon::bullet_damage`,
  `Weapon::fire_rate`, `Weapon::bullet_speed`, `MeleeStats::damage`,
  `MeleeStats::range`. Ability-cooldown upgrades (ground slam, concussion
  shot) are a natural second wave once the shape is proven.
- Purchases write `SaveData::upgrade_levels` (map of
  `"class_id/upgrade_id"` → level) and deduct `salvage_banked`, saving
  immediately.

### Applying upgrades

Following the class-recipe pattern (ADR-0011): a free function
`apply_upgrades(reg, entity, catalog, save_data)` runs in `spawn_player`
*after* the class recipe and mutates component values in place. Systems
need zero changes — they already read `Weapon` and `MeleeStats` values
each tick. The function is trivially testable headlessly: build registry,
apply recipe, apply upgrades, assert values.

## ECS integration

**New components** (`components.hpp`):

- `SalvageValue { int amount = 1; float chance = 1.f; }` — on enemies
- `SalvagePickup { int amount = 1; }` — on drop entities
- `GameState` gains `int salvage = 0;` (run wallet; existing
  erase-then-emplace lifecycle in `GameScene::on_enter` already handles
  the reset)

**Changed systems** (no new system files):

- `damage_system.cpp` / `handle_enemy_death` — spawn salvage drops at the
  existing "no death drops" comment, beside the stabilizer roll
- `pickup_system.cpp` / `update_pickups` — salvage collection pass
- `hud_system.cpp` — salvage counter
- `wave_system.cpp` — read optional `salvage` field from stage JSON,
  emplace `SalvageValue`

**New modules:**

- `src/ecs/upgrades.{hpp,cpp}` — `UpgradeCatalog` (manifest +
  `load_from_json`) and `apply_upgrades` (sits beside `player_class.cpp`;
  added to both `CMakeLists.txt` and `tests/CMakeLists.txt`)
- `src/scenes/workshop_scene.{hpp,cpp}` — menu-driven scene copying the
  `OptionsScene` row/cursor idiom; engineer dialogue via `BitmapFont`

**Changed core:**

- `save_data.{hpp,cpp}` — `salvage_banked`, `upgrade_levels`, JSON
  round-trip, defensive load (corrupt file → defaults, never a crash)
- `title_scene.cpp` — `WORKSHOP` menu entry
- `game_over_scene.cpp` / `victory_scene.cpp` — banking + recovered line

**New data files** (no CMake changes needed):

- `assets/data/upgrades/upgrade_manifest.json`
- `assets/data/npcs/engineer.json`
- Stage JSONs optionally gain per-enemy `salvage` fields

**Polish (not v1-blocking):**

- A real `pickups.png` sprite sheet + `config.json` entry — today weapon
  and stabilizer drops already render as gray fallback rects; salvage
  makes three pickup types on one missing sheet, which is the moment to
  draw it
- Distinct `Sfx::Salvage` (new enum value before `Count`, name mapping,
  `config.json` sound entry, WAV) — until then, `Sfx::Pickup`
- Magnet drift (pickups within a radius drift toward the player)
- Engineer portrait / animation in the workshop

## Sequencing and the hit-mechanics branch

A hit-mechanics branch is in flight and owns the death/damage flow this
feature hooks (`damage_system.cpp`, likely `collision_system.cpp` /
`melee_system.cpp` / `components.hpp`). Implementation therefore waits
for that merge; the only hard coupling is "where an enemy death is
detected", so the plan survives any reshaping of hit resolution short of
moving death handling out of the damage system.

Implementation order once unblocked (each step builds, tests, and could
ship alone):

1. **Drops + wallet + HUD** — `SalvageValue`, `SalvagePickup`, death-roll,
   collection pass, counter. Tests: seeded-RNG drop rolls, no-RNG → no
   drop, probability band, collection increments wallet, multi-pickup
   same-tick collection.
2. **Banking + persistence** — `SaveData` fields + round-trip, end-scene
   banking, recovered line. Tests: JSON round-trip, corrupt-file
   defaults.
3. **Catalog + application** — `UpgradeCatalog`, `apply_upgrades`, wire
   into `spawn_player`. Tests: inline-JSON catalog load with clamping,
   upgrade math per class, stolen weapons unaffected.
4. **Workshop scene** — title menu entry, engineer dialogue, purchase
   flow (spend, level up, save). Scene logic thin; purchase math lives in
   `upgrades.cpp` where tests reach it.

## Consequences

**Positive:**

- Every kill can pay something, fixing ADR-0008's "ranged kills feel
  unrewarded" without touching the disarm loop
- First persistent progression layer; failed runs still advance something
- The engineer gives the game a recurring face and a narrative frame for
  why the player fights machines
- Data-driven catalog and dialogue iterate without recompiling
- Answers ADR-0014's open question 2 in a way that composes with, rather
  than replaces, its in-run shop
- Reuses established patterns end to end: pickup recipe, ctx RNG
  convention, deferred destruction, manifest loaders, class recipes,
  pref-path persistence

**Negative:**

- Permanent upgrades erode difficulty over time; costs must escalate
  steeply and stat ceilings stay modest (a maxed class should feel
  sharper, not trivialized) — balancing this is ongoing work, not a
  one-time task
- Two currencies (salvage now, coins later) risk player confusion;
  distinct visual language and drop feel are required, not optional
- `SaveData` grows from one int to structured state — corrupt-file
  handling and forward compatibility now actually matter
- More art debt: salvage sprite, engineer sprite, workshop backdrop
- Meta-progression softens the roguelike purity ADR-0014 noted; players
  who want a fixed challenge may eventually want a "no upgrades" toggle

## Open questions

1. **Material fiction and name.** "Salvage" is the working mechanical
   term. The flavor name (and whether tiers of material exist — common
   scrap vs. boss-only alloy) is open; multiple material types would add
   a `type` field to `SalvageValue`/`SalvagePickup` and per-type costs in
   the catalog, deferred until one material proves too flat.
2. **Engineer identity.** Name, look, and voice are open. Working
   assumption: she is the game's first named NPC and may later anchor the
   ADR-0014 shop cast.
3. **Upgrade respec.** Can banked levels be refunded? v1 says no;
   escalating costs make early picks matter.
4. **Co-op split.** Shared wallet vs. per-player collection — deferred to
   the co-op milestone, but `GameState::salvage` as a single int assumes
   shared; revisit then.
