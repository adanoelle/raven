# 4. Data-Driven Patterns via JSON

Date: 2026-02-08
Status: Accepted

## Context

Bullet patterns (and eventually enemy wave scripts) need to be tunable without recompiling. Designers and playtesters should be able to adjust bullet counts, speeds, spread angles, and fire rates by editing data files.

## Decision

Define bullet patterns as JSON files loaded at runtime by `PatternLibrary`.

Each pattern file contains:
- A pattern name (unique identifier)
- One or more emitter definitions, each specifying: type (Radial/Aimed/Linear), bullet count, speed, angular velocity, fire rate, spread angle, sprite, lifetime, damage, and hitbox radius

A `manifest.json` file lists all pattern files to load. The library supports loading individual files or parsed JSON objects (useful for tests).

```json
{
  "name": "aimed_burst",
  "emitters": [
    {
      "type": "Aimed",
      "count": 3,
      "speed": 120,
      "fire_rate": 0.5,
      "spread_angle": 30
    }
  ]
}
```

## Consequences

**Positive:**
- Patterns can be tuned without recompilation
- Test suite can validate pattern parameters in isolation
- Enables future hot-reload: watch pattern files and re-parse on change
- Clear separation between game logic (how bullets behave) and content (which patterns exist)

**Negative:**
- JSON parsing adds a runtime dependency (nlohmann/json)
- No compile-time validation of pattern data — errors are caught at load time
- Complex patterns may eventually need a scripting language (deferred decision)
