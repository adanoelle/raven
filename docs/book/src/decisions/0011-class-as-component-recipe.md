# 11. Player Classes as Component Recipes

Date: 2026-02-12 Status: Accepted

## Context

The design matrix calls for four player classes (Brawler, Sharpshooter,
Tactician, Alchemist), each with distinct stats, abilities, and gameplay
identity. We needed a pattern for applying class-specific behaviour to the
player entity that works well with EnTT's composition model.

Three approaches were considered:

1. **Entity templates** — pre-built archetype snapshots loaded from data files.
   EnTT supports prototype registries, but our class differences are primarily
   in component _values_ and _presence_, not structure. A data-driven approach
   adds serialisation complexity for marginal benefit at this stage.

2. **Inheritance** — a `PlayerClass` base with virtual methods for ability
   activation. This fights the ECS model: behaviour would live in class
   hierarchies instead of systems, and hot-path dispatch would go through
   vtables instead of component views.

3. **Component recipes** — free functions that take a base player entity and
   emplace class-specific components with tuned values. Systems discover
   abilities via component presence (e.g. `reg.view<ChargedShot>`) rather than
   enum checks. New classes require a new recipe function and any new component
   types, but zero changes to existing systems unless the new class reuses
   existing ability components with different values.

## Decision

**Free functions assemble class-specific components onto a base entity.** Each
recipe (e.g. `apply_brawler`, `apply_sharpshooter`) assumes universal components
already exist on the entity and adds class-specific ones on top:

- Override shared component values (`Player::speed`, `Health::max`,
  `Weapon::bullet_damage`)
- Emplace a `ClassId` for UI purposes
- Emplace a `MeleeStats` component to override melee attack defaults
- Emplace ability cooldown components that gate system activation
  (`GroundSlamCooldown` for Brawler, `ConcussionShotCooldown` + `ChargedShot`
  for Sharpshooter)

Systems gate on component presence, not runtime class checks:

- `update_ground_slam` only runs for entities with `GroundSlamCooldown`
- `update_charged_shot` only runs for entities with `ChargedShot`
- `update_concussion_shot` only runs for entities with `ConcussionShotCooldown`
- `update_melee` checks `try_get<MeleeStats>` to override defaults
- `update_shooting` skips entities with `ChargedShot`
- `update_input` checks `try_get<ChargedShot>` for the speed penalty

## Consequences

**Positive:**

- Adding a new class requires only a new recipe function and any new components
  — existing systems and classes are untouched unless the new class shares
  ability types
- Systems self-gate on component presence, so there are no cascading switch
  statements or enum checks in hot paths
- `ClassId` is minimal (UI/death messages only) and doesn't couple gameplay
  logic to an enum
- The pattern is consistent with how EnTT composition works elsewhere in the
  codebase (e.g. `BulletEmitter` presence gates emitter updates)
- Recipe functions are trivially testable — create entity, apply recipe, assert
  component values

**Negative:**

- Ordering dependency: the base entity must be fully constructed before the
  recipe runs, since recipes call `reg.get<Player>` and `reg.get<Health>`.
  Calling a recipe on an incomplete entity would crash
- There is no shared "ability" abstraction — `GroundSlam` and `ConcussionShot`
  are structurally similar but defined as separate types. If a third AoE ability
  is added, the duplication may warrant a generic `AoeBurst` component
- Both classes use the bomb button for their active ability. The input is
  disambiguated by component presence (only one cooldown component exists per
  entity), but this means a hypothetical class with _two_ active abilities would
  need a different input scheme
