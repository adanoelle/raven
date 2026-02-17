# Player Class System

Added the player class system with two initial classes — Brawler and
Sharpshooter. Each class is implemented as a component recipe that overlays
class-specific stats and abilities onto the base player entity. A character
select scene lets the player choose before entering gameplay. Three new ability
systems (ground slam, charged shot, concussion shot) and modifications to three
existing systems complete the class differentiation.

## What changed

**Class recipes** — `apply_brawler` and `apply_sharpshooter` in
`src/ecs/player_class.cpp`. Each function takes a base player entity and
emplaces class-specific components: `ClassId`, `MeleeStats`, ability cooldowns,
and weapon overrides. Brawler gets 150 HP / 3 lives / 100 speed with strong
melee. Sharpshooter gets 60 HP / 2 lives / 150 speed with 1.5x bullet damage.

**Ground slam** (Brawler active) — AoE blast around the player dealing 4 damage
with 350 px/s knockback in a 50 px radius. Activated on bomb input with a 2.0 s
cooldown. Blocked during dash.

**Charged shot** (Sharpshooter passive) — replaces normal shooting with a
charge-and-release mechanic. Holding shoot accumulates charge over 0.8 s.
Release fires a single bullet with damage scaled from 0.5x to 3.0x and speed
from 1.0x to 1.8x. Full charge produces a piercing shot. Charge resets on dash.

**Concussion shot** (Sharpshooter active) — AoE blast dealing 1 damage with 400
px/s knockback in a 45 px radius. Activated on bomb input with a 3.0 s cooldown.
Emphasises crowd control over damage.

**MeleeStats override** — the melee system now checks for a `MeleeStats`
component and copies its values into `MeleeAttack` on creation. Brawler melee
deals 3 damage with 35 px range; Sharpshooter deals 1 damage with 20 px range.

**Character select scene** — `CharacterSelectScene` inserted between title and
game. Two coloured boxes (red/blue) with left/right selection. Passes the chosen
`ClassId::Id` to `GameScene`'s constructor.

**HUD additions** — ability cooldown bar (bottom-left, cyan fill) shows active
ability readiness. Charge indicator (above ability bar, orange/yellow fill)
appears while charging a shot.

**Shooting system skip** — `update_shooting` skips entities with `ChargedShot`
so the charged shot system has exclusive control over Sharpshooter bullet
spawning.

**Input speed penalty** — `update_input` multiplies movement speed by
`ChargedShot::move_penalty` (0.5x) while charging, creating a risk/reward
trade-off between mobility and damage.

## Design rationale

The component recipe pattern was chosen over entity templates or inheritance
because it aligns with EnTT's composition model. Systems gate on component
presence rather than class enums, so adding a third class requires only a new
recipe function and any new components — no changes to existing systems. See
[ADR-0011](../decisions/0011-class-as-component-recipe.md) for the full decision
record.

The design matrix (`docs/design/entity_database.csv`) provides the target stat
profile for all four planned classes. Current implementation values diverge in
some areas (notably Sharpshooter speed: 150 vs design's 80) due to
prototype-stage tuning. See the
[Player Classes](../architecture/player-classes.md) architecture chapter for a
detailed comparison table.

Both classes share the bomb button for their active ability. Input is
disambiguated by component presence: Brawler has `GroundSlamCooldown`,
Sharpshooter has `ConcussionShotCooldown`, and the respective systems only
activate when their cooldown component is present.

## New files

- `src/ecs/player_class.hpp/.cpp` — class recipe functions
- `src/ecs/systems/ground_slam_system.hpp/.cpp` — ground slam system
- `src/ecs/systems/charged_shot_system.hpp/.cpp` — charged shot system
- `src/ecs/systems/concussion_shot_system.hpp/.cpp` — concussion shot system
- `src/scenes/char_select_scene.hpp/.cpp` — character selection scene
- `tests/test_player_class.cpp` — 20 new Catch2 tests
- `docs/book/src/architecture/player-classes.md` — architecture chapter
- `docs/book/src/decisions/0011-class-as-component-recipe.md` — ADR

## Test results

92/92 tests pass, including 20 new tests covering class recipes, MeleeStats
integration, ground slam mechanics, charged shot charge/release/pierce/cancel,
concussion shot mechanics, and cross-class ability isolation.
