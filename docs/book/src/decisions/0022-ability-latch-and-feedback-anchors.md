# 22. One-Shot Ability Hits via Component Latch, with Feedback Anchors

Date: 2026-08-28 Status: Accepted

## Context

Three player abilities resolve as an instantaneous area query: melee (cone),
ground slam (circle), and concussion shot (circle). All three share a shape:

1. On button press, an ability component (`MeleeAttack`, `GroundSlam`,
   `ConcussionShot`) is emplaced on the player.
2. The component lives for a `remaining` duration measured in seconds.
3. The gameplay effect — damage plus radial knockback — must apply **exactly
   once**, even though the component exists for many simulation ticks (a 0.3s
   slam spans 36 ticks at 120Hz; without protection it would deal its damage
   36 times).

Three designs were considered for the exactly-once guarantee:

1. **Per-tick damage with per-target cooldowns** — every tick, damage all
   enemies in range that haven't been hit "recently". This is the right model
   for *lingering* damage zones (fire pools), but for a one-shot impact it
   converts a simple guarantee into per-enemy bookkeeping and tuning
   (how long is "recently"?).

2. **Short-lived hitbox entity** — spawn an entity carrying a hitbox and a
   damage component, let the collision system detect overlaps, destroy the
   entity after one tick. This is the most general design: hitboxes can move,
   persist, and be authored as data. But for an instantaneous query it adds a
   full entity lifecycle (create, tag so it hits enemies but not the player,
   deferred destroy) and pushes ability semantics (knockback direction,
   disarm-on-melee) into the collision system's responsibilities.

3. **Latch flag on the ability component** — a `hit_checked` bool on the
   component. The first tick of processing runs the area query and applies
   effects, then sets the flag; later ticks only count down `remaining`.

## Decision

**Latch flag, plus a shared collect/apply helper**
(`src/ecs/systems/ability_hits.hpp`).

The latch keeps the exactly-once guarantee local to the ability system that
owns the semantics. The helper splits resolution into two phases:

- `collect_enemies_in_circle()` — a **read-only pass** over the enemy view
  that returns `AbilityHit{entity, dir_x, dir_y}` records. Melee substitutes
  its own cone-shaped collect but fills the same record type.
- `apply_ability_hits()` — a **write pass** over the records: subtract
  `Health`, `emplace_or_replace<Knockback>` scaled by the ability's knockback
  stat with the shared `ABILITY_KNOCKBACK_DURATION`.

Collect-then-apply avoids mutating component storage while iterating a view
over it (the same reasoning as deferred destruction, ADR-0007). The helper
never destroys entities and never decides death: death is resolved solely by
the damage system, so score, drops, and death SFX behave identically no
matter what dealt the final hit.

Knockback semantics encoded by the helper:

- The push direction is a **unit vector** from ability origin to enemy
  centre, so knockback distance is exactly
  `knockback_speed x ABILITY_KNOCKBACK_DURATION` regardless of how far away
  the enemy stood.
- `emplace_or_replace` means overlapping knockbacks **replace, never stack**:
  the most recent hit wins and its timer restarts.
- If origin and target coincide exactly (zero distance), the direction is
  `(0, 0)`: damage applies, knockback is skipped. This is a NaN guard, not a
  gameplay branch — collision keeps entities from perfectly stacking, so the
  case is effectively unreachable today. If a future mechanic makes it
  reachable (teleports, spawn-on-player effects), a motionless point-blank
  hit would read as the enemy *withstanding* the blow, which is the wrong
  message. The fallback policy, should it be needed: prefer the **most
  intentional direction available** — the player's aim direction for aimed
  abilities (melee already carries `aim_x/aim_y`), a random outward direction
  drawn from the registry's seeded `std::mt19937` for radial abilities, and
  "no knockback" only as the final resort.

### The ability component as a feedback timeline

The deeper reason the ability component outlives its one-tick gameplay effect:
its lifecycle provides the **anchor points where game feel attaches**. The
theory: *gameplay systems emit facts; presentation reads them.* Simulation
code never calls into audio or rendering directly — it either pushes an event
or leaves component state that presentation layers observe. Each ability
exposes four anchors:

1. **Activation edge** — the tick the component is emplaced. This is the
   "the player did a thing" moment: swing/launch SFX (melee already pushes
   `Sfx::Melee` here), muzzle-flash-style VFX, animation state switches
   (the animation system selects Melee/Dash states purely by observing which
   components exist on the player — presence *is* the API).

2. **Impact tick** — inside the latch, where the hit list is known. This is
   the only place that knows *what was hit, where, and in which direction* —
   exactly the data impact feedback needs: per-enemy hit flashes and hit SFX,
   spawn positions for impact particles, screen shake scaled by hit count,
   and hit-stop (a few-tick freeze) if we adopt it. `AbilityHit` records are
   the natural payload; a future impact-event queue can carry them the same
   way `AudioQueue` carries `Sfx` values.

3. **Active window** — every tick while the component exists, with
   `remaining` as a free 1→0 progress value. Duration-based VFX key off it:
   ADR-0012's expanding concussion ring maps `remaining` to ring radius and
   frame index. No separate VFX timer needs to exist or be kept in sync.

4. **Expiry** — the tick the component is removed. Cleanup, cooldown UI
   pulses, "ability ready" cues.

Sound already follows this theory end to end: systems call
`push_sfx(reg, Sfx::...)` at the anchor, the request lands in a registry-
context `AudioQueue`, and the scene drains the queue after the system
pipeline and forwards it to the `AudioEngine`. Simulation stays deterministic
and headless-testable (the queue is simply absent in unit tests, and
`push_sfx` tolerates that). VFX should arrive the same way when the art is
ready: either **observe** ability components for duration-driven effects
(anchor 3) or **consume** a drained event queue for one-shot effects
(anchors 1, 2, 4) — never by simulation code reaching into the renderer.

Knockback itself is the first and cheapest "physicality" channel, and it
lives entirely in simulation: while a `Knockback` component exists, the AI
system pins the enemy's velocity to the impulse and suspends its brain
(component presence as state machine). Because knockbacked enemies still run
tile collision, they visibly slam into walls — feedback for free.

## Consequences

**Positive:**

- Exactly-once damage with one bool; no per-target cooldown bookkeeping, no
  auxiliary entity lifecycle.
- Damage, knockback direction math, stacking policy, and duration live in
  one header; a fourth ability inherits all of them by filling a hit vector.
- The component-lifetime anchors mean VFX/SFX can be added later without
  restructuring gameplay code: the hooks already exist and two (audio queue,
  animation-by-presence) are already exercised.
- Simulation remains presentation-free and fully unit-testable.

**Negative:**

- The latch resolves everything on one tick, so it cannot express sweeping
  or multi-hit attacks (a sword arc that hits early targets before late
  ones, a projectile AoE that travels). When such an ability is designed,
  the hitbox-entity approach (option 2) becomes the right tool — the
  collect/apply helper still applies to its resolution, but the query timing
  moves into the collision system.
- Impact feedback (anchor 2) currently has no event queue; only audio does.
  When impact VFX/hit-stop/screen shake arrive, an `AbilityHit`-carrying
  queue (mirroring `AudioQueue`) should be added rather than letting VFX
  code re-derive hits from health deltas.
- `ABILITY_KNOCKBACK_DURATION` is shared across all abilities by design; if
  a future ability needs a distinct knockback duration, it must become a
  per-ability stat rather than a second constant.
