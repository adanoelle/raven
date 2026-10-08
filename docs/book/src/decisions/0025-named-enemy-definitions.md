# 25. Named Enemy Definitions

Date: 2026-10-08 Status: Accepted (amends the enemy entries of [ADR-0009](0009-data-driven-wave-definitions.md))

## Context

[ADR-0009](0009-data-driven-wave-definitions.md) made waves data, but each
wave entry spelled out its enemy inline: tier, AI archetype, pattern, HP,
score and a contact-damage flag. Everything else about an enemy was C++:
sprite sheet, drawn size, hitboxes and render offset per tier
(`enemy_visuals`), movement tuning per archetype (`make_ai`), and the
stabilizer drop chance per tier (`handle_enemy_death`). So:

- Adding an enemy type meant editing C++ in four places.
- Two grunts couldn't look or move differently.
- The same enemy was copy-pasted into every stage that used it, so retuning
  it meant editing every copy.
- Unknown strings quietly became a grunt or a chaser.

The [pre-content review](../devlog/2026-10-08-pre-content-review.md) listed
this as the second thing slowing down content work, after animation.

## Decision

**Enemies are defined once, by name, in `assets/data/enemies.json`, and
stages place them by name.**

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

A stage wave entry is now `{ "spawn_index": 0, "enemy": "grunt_chaser" }`.

- Only `tier`, `sheet` and `ai.archetype` are required. Everything else has
  a default:
  - `size` is the sheet's frame size. Drawn size 0 now means "the frame's
    own size" for any `Sprite`.
  - AI tuning comes from `default_ai()` for the archetype.
  - `pattern` is empty, so the enemy doesn't shoot.
  - `contact_damage` is 0 (no body damage).
  - `stabilizer_drop` comes from `default_stabilizer_drop()` for the tier.
- `EnemyLibrary` (`src/ecs/enemy_library.hpp`) loads the file. A definition
  with a missing or unknown tier or archetype, or a value out of range, is
  reported and skipped. An unknown key gets a "typo?" warning. A wave entry
  without an `enemy` name, or naming one that isn't defined, is reported and
  skipped.
- `GameScene` rebuilds the library on every run, so edits apply from the next
  run without restarting the game.
- The stabilizer drop chance travels with the enemy as a `StabilizerDrop`
  component. Enemies without one keep the tier default.
- `tests/test_content.cpp` checks that every definition loads, uses a known
  pattern and a registered sheet, and that every stage names only defined
  enemies.

## Consequences

**Positive:**

- New enemies, and variants of existing ones, are JSON and art. Changing an
  enemy changes it everywhere it appears.
- Movement tuning is visible and editable in data instead of hidden in a
  `switch`.
- Stage files say what appears where, nothing more.
- Grunts now draw at their sheet's frame size (16 px placeholders), no longer
  stretched to 24 px. They will draw at 24 px when 24 px art lands, with no
  data change.

**Negative:**

- One more file to keep in sync. The content test catches a stage naming a
  missing enemy, but not an enemy that no stage uses.
- No per-entry overrides: a tougher grunt in a later stage is its own
  definition. Overrides can be added if variants multiply.
- The archetype behaviors themselves (the four brains, and timers such as the
  stalker's strafe interval) are still C++.
