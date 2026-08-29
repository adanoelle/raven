# Enemy Archetypes

What each enemy actually *does*, and what that behavior asks of its art and
sound. Enemies in Raven are built from two independent axes:

- **Type** — the tier: Grunt, Mid, Boss. This sets size, health, and score
  (sprite sizes per tier are in the [art spec](art/art-spec.md#3-sprite-dimensions)).
- **Archetype** — the movement brain: Chaser, Drifter, Stalker, or Coward.
  This sets the *personality*.

Any type can carry any archetype (stage JSON decides), so the archetype —
not the tier — is what a player learns to read at a glance. **Silhouette
and animation should communicate the archetype; size and detail communicate
the tier.**

---

## Shared Behavior (all archetypes)

These rules apply to every enemy and create feedback moments of their own
(see [Game Feel](game-feel.md)):

- **Enemies start dormant.** An enemy activates only when the player is
  within its activation range *and* has an unobstructed line of sight
  (walls block the check). Until then it stands motionless and holds fire.
  → Art wants an *idle* that reads as dormant-but-alive, and ideally a brief
  wake-up tell so activation feels fair.
- **Knockback suspends the brain.** While being knocked back (from melee,
  slam, concussion, or bullet impacts) an enemy has no control — it slides,
  and if a wall is in the way it slams into it. → The hurt pose can serve as
  the "helpless slide" look.
- **Disarm enrages.** When melee knocks the weapon out of an enemy, it
  abandons its archetype entirely and charges the player at **1.5× speed**.
  A disarmed Coward — the fastest enemy in the game — becomes a missile.
  → This state change must be visible: weapon gone, posture flipped from
  cautious to berserk.
- **Weapons are visible threats.** Armed enemies fire bullet patterns; the
  pattern (and its tier — Common/Rare/Legendary) is exactly what the player
  steals via disarm. If enemy designs show their weapon, the disarm read
  comes for free.

## The Four Archetypes

Numbers below are the current defaults (`make_ai()` in
`src/ecs/systems/wave_system.cpp`); the player moves at 100–150 px/s for
comparison.

### Chaser — the pressure

| Property | Value |
| -------- | ----- |
| Speed | 70 px/s |
| Wakes at | 160 px |
| Fires when | within 80 px |
| Typical loadout | Grunt, `spiral_3way`, **contact damage** (15 per touch, 0.5s cooldown) |

Beelines straight at the player, always. Chasers are the game's melee
pressure: their body *is* the weapon, and they force the player to keep
moving while dodging everyone else's bullets.

**Design language:** aggression, forward lean. The run cycle should reach —
lunging strides, jaws or spikes leading. The silhouette points at you.
Touching it must look like it would hurt.

### Drifter — the hazard

| Property | Value |
| -------- | ----- |
| Speed | 40 px/s (slowest) |
| Wakes at | 200 px |
| Fires when | within 100 px |
| Typical loadout | Grunt, `spiral_3way` |

Wanders in a random direction, picking a new one every 1–2.5 seconds — it
never looks at the player. A Drifter is a mobile bullet fountain: the threat
is its pattern, not its intent.

**Design language:** aimless, floaty. Hovering or bobbing motion, no strong
facing — think a drone, spore, or lantern. Direction changes should look
undeliberate. Its glow/weapon should be the visually loudest part, because
the pattern is the actual danger.

### Stalker — the duelist

| Property | Value |
| -------- | ----- |
| Speed | 90 px/s |
| Wakes at | 160 px |
| Keeps distance of | 90 px; retreats if the player closes within 45 px |
| Fires when | within 120 px |
| Typical loadout | Mid, `aimed_burst` |

The most deliberate brain: advances to its preferred range, then **strafes
sideways** around the player, reversing direction every 2 seconds, firing
aimed bursts. If the player charges in, it backpedals for half a second and
resumes. Stalkers punish standing still and resist melee — closing the
distance for a disarm is a real fight.

**Design language:** predatory poise. Because it spends most of its life
moving *laterally* while facing the player, its **side-stepping animation
matters more than its run** — a sidestep or scuttle with the head/weapon
tracking sideways. Aimed shots deserve a visible wind-up so bursts feel
dodgeable.

### Coward — the chase

| Property | Value |
| -------- | ----- |
| Speed | 110 px/s (fastest — faster than the Brawler) |
| Wakes at | 200 px |
| Fires | **always**, at any range |
| Typical loadout | **Boss**, `nova_legendary` |

Runs directly away from the player, probing ahead for walls and sliding
along them to avoid getting cornered; if truly boxed in, it darts back
*past* the player to escape. All the while it never stops firing.

The current boss is a Coward with a Legendary nova pattern and 10× grunt
health: a fight where the player chases a fleeing gun through its own
bullet curtain — and where melee-disarming it means catching the fastest
thing in the game. That is the intended experience: **the boss fight is the
disarm economy at maximum stakes.**

**Design language:** skittish, evasive. Runs *away*-facing with glances
back over its shoulder; panic in the run cycle. The boss variant keeps the
panicked personality but adds bulk and an overwhelming weapon — a coward
with a cannon. Wake and flee tells should read instantly, because the
player's first lesson is "it runs."

## Composition Today (test-scale)

Current stage content is deliberately minimal while art is in production —
three stages, seven waves, built from: Chaser grunts (contact damage),
Drifter grunts, Stalker mids, and one Coward boss. The archetype system,
stage JSON, and pattern library are all data-driven, so the roster grows by
adding JSON and art, not code.

## Key Files (for developers)

| File | Role |
| ---- | ---- |
| `src/ecs/systems/ai_system.cpp` | All four archetype brains, activation/line-of-sight, contact damage |
| `src/ecs/components.hpp` | `AiBehavior`, `Enemy::Type`, `ContactDamage`, `Disarmed`, `Knockback` |
| `src/ecs/systems/wave_system.cpp` | `make_ai()` per-archetype stats, wave spawning |
| `assets/data/stages/*.json` | Which type/archetype/pattern appears in each wave |
