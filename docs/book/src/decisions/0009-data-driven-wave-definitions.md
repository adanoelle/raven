# 9. Data-Driven Wave Definitions

Date: 2026-02-11 Status: Accepted

## Context

Enemy spawning was hardcoded in the game scene, making it difficult to tune
difficulty, add new rooms, or test wave logic in isolation. LDtk provides
spatial data (tile layers, spawn positions, exit locations) but has no built-in
way to define enemy composition, wave ordering, or difficulty curves. The game
needed a progression system that could be authored, iterated on, and tested
independently of the level editor.

The existing `PatternLibrary` already demonstrated a working model for
data-driven definitions: a JSON manifest lists files, a loader parses them into
structs, and systems consume those structs at runtime. A wave definition system
could follow the same pattern.

## Decision

**JSON stage files define waves (what/when); LDtk provides spawn positions
(where).** Each stage file names an LDtk level and lists ordered waves of
enemies. Each enemy entry specifies type, AI archetype, bullet pattern, HP,
score value, contact damage, and a `spawn_index` that maps to the Nth
`EnemySpawn` entity in the tilemap.

A `stage_manifest.json` lists stage files in play order, matching the
`PatternLibrary` manifest pattern. `StageLoader` loads and parses the manifest,
exposing stages by index via `get(index)`.

The format:

```json
{
  "name": "stage_01",
  "level": "Test_Room",
  "waves": [
    {
      "enemies": [
        {
          "spawn_index": 0,
          "type": "grunt",
          "pattern": "spiral_3way",
          "hp": 1.0,
          "score": 100,
          "ai": "chaser",
          "contact_damage": true
        }
      ]
    }
  ]
}
```

`spawn_index` is resolved at spawn time by looking up the tilemap's `EnemySpawn`
list in order. If the index exceeds the number of available spawn points, it
clamps to the last one.

## Consequences

**Positive:**

- Level layout is decoupled from difficulty balancing — spawn points can be
  rearranged in LDtk without editing wave data, and vice versa
- Adding or reordering stages requires only editing JSON files, no recompilation
- Wave logic is testable without LDtk: unit tests construct `StageDef` structs
  directly and pass an empty `Tilemap` (enemies fall back to center position)
- The manifest pattern is already familiar from `PatternLibrary`, reducing
  cognitive overhead for contributors
- Stage files are human-readable and diffable, making balance changes easy to
  review

**Negative:**

- `spawn_index` is order-dependent: reordering `EnemySpawn` entities in LDtk
  silently reassigns which enemy spawns where
- There is no visual editor for wave design — authors must edit JSON by hand and
  playtest to verify
- Two files must be kept in sync per room (LDtk level for layout, JSON stage for
  waves), which increases the surface area for desync bugs
