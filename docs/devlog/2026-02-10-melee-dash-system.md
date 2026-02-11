# Melee Attack and Dash System

Added the melee attack and dash systems, completing the core close-combat loop.
Weapons are now only obtainable by meleeing enemies — closing distance, swinging
an arc attack to disarm them, and picking up the dropped weapon. This replaces
the previous passive death-drop system with an active risk/reward mechanic.

## What changed

**Melee attack** — a directional cone hitbox (90 deg arc, 30 px range) that
damages enemies, applies knockback, and disarms those with bullet emitters.
Disarmed enemies lose their ranged attack and become aggressive Chasers at
1.5x speed, creating immediate pressure on the player.

**Dash** — a burst movement ability (400 px/s for 0.12 s) with brief
invulnerability (0.18 s). Uses movement direction if the player is moving,
otherwise aim direction. The input system skips normal velocity updates during
a dash so the two don't fight.

**Input bindings** — melee on right mouse button / C / gamepad X. Dash on space
bar / gamepad LB. Both have cooldowns (0.4 s melee, 0.6 s dash).

**Code review fixes** — extracted the duplicated `find_player_position` helper
from `ai_system.cpp` and `emitter_system.cpp` into a shared
`player_utils.hpp`. Restructured the contact damage cooldown tick in
`ai_system.cpp` to run as a separate pass before the overlap check, fixing a
structural issue where cooldowns were ticked inside the player-vs-enemy nested
loop.

## Design rationale

The previous death-drop system was too passive. Shooting enemies from range and
collecting whatever fell didn't create interesting decisions. The melee disarm
forces the player to weigh:

- Do I risk getting close to steal this weapon?
- The disarmed enemy will chase me faster — is it worth the trade?
- Can I dash in, melee, and dash out before the bullet pattern closes in?

This creates the "close-dodge-strike" gameplay loop that Blazing Beaks does
well. The dash gives the player a tool to manage the risk, but with a cooldown
long enough that it can't be spammed.

## New files

- `src/ecs/systems/player_utils.hpp` — shared player position lookup
- `src/ecs/systems/melee_system.hpp/.cpp` — melee attack system
- `src/ecs/systems/dash_system.hpp/.cpp` — dash system
- `tests/test_melee_dash.cpp` — 20 new Catch2 tests
- `docs/decisions/0008-melee-disarm-over-death-drops.md` — ADR
- `docs/book/src/architecture/melee-and-dash.md` — architecture chapter

## Test results

60/60 tests pass, including 20 new tests covering arc geometry, melee damage
and disarm, dash mechanics and cooldowns, input system interaction, and the
contact damage cooldown fix.
