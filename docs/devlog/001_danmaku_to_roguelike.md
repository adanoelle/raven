# Pivoting Genre: Bullet Hell to Roguelike

Date: 2026-02-08 Tags: design, refactoring, genre

## What I Built

Converted the project from a vertical-scrolling bullet hell (danmaku) to a
twin-stick roguelike inspired by Blazing Beaks. This was a mechanical
refactoring — the engine architecture stayed mostly intact, but the content and
some systems changed significantly.

## The Interesting Problem

The core engine (ECS, renderer, scene system, collision) was genre-agnostic and
transferred cleanly. The challenge was identifying what to keep, what to remove,
and what to rename without breaking the test suite.

**Kept as-is:**

- EnTT ECS architecture and all component structs
- SDL2 renderer with virtual resolution scaling
- Stack-based scene manager
- Collision system (circle-circle + AABB)
- Input abstraction (added twin-stick aim later)
- Build system and dev environment

**Removed:**

- `BulletPatternSource` component (enemies had fixed emitter attachments)
- `GrazeTarget` component (graze scoring is a danmaku mechanic)
- Focus mode speed reduction (replaced with dodge roll in the future)
- Bomb system (will become a roguelike ability slot)
- Vertical scrolling background system

**Changed:**

- Project name: danmaku → raven
- Resolution: 384x448 (roughly 7:8, vertical) → 480x270 (16:9, horizontal)
- Player movement: 4-directional with focus → twin-stick 8-directional
- Screen orientation: vertical playfield → horizontal rooms

## How It Works

The rename was done with a combination of `sed` across all source files and
manual fixups for CMake targets, the Nix flake package name, and README
references. The resolution change required updating:

- `Renderer::VIRTUAL_WIDTH` and `VIRTUAL_HEIGHT` constants
- All entity spawn positions (player start, enemy spawns)
- Screen-bounds clamping in `movement_system`
- Off-screen despawn margins in `cleanup_system`
- Test expectations for boundary behaviour in `test_collision.cpp`

The pattern library was kept because roguelikes still have projectile patterns —
they're just fired by the player at enemies instead of by enemies at the player.
The `Bullet::Owner` enum already distinguished player vs enemy bullets, so
collision rules worked without changes.

## What I'd Do Differently

I should have written ADRs for the original danmaku design decisions. When
pivoting, I had to reconstruct the rationale for several choices from memory.
Architecture Decision Records are most valuable when they capture the context at
decision time, not retroactively.

## Next

Design the room-based level structure and implement procedural room connections.
The player needs somewhere to explore.
