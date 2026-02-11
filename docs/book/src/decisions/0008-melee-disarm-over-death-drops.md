# 8. Melee Disarm Over Death-Based Weapon Drops

Date: 2026-02-10 Status: Accepted

## Context

The initial weapon steal system had enemies drop their weapon on death. While
functional, this made weapon acquisition passive — the player just shoots from
a distance and picks up whatever falls. This clashes with the Blazing
Beaks-inspired design goal where the player must take deliberate risk to gain
power.

With the enemy AI archetypes (Chaser, Drifter, Stalker, Coward) now creating
varied movement pressure, there was an opportunity to add a melee system that
creates risk/reward tension: the player must close distance and swing an arc
attack to steal weapons, exposing themselves to bullets and contact damage.

## Decision

**Weapons are only obtainable via melee disarm.** Enemies no longer drop weapons
on death. Instead:

1. The player performs a directional melee attack (cone-shaped arc hitbox).
2. If the arc hits an enemy that has a `BulletEmitter`, the emitter is removed,
   a `WeaponPickup` is spawned at the enemy's position, and the enemy is tagged
   `Disarmed`.
3. Disarmed enemies become aggressive Chasers at 1.5x their base move speed,
   creating a new threat.
4. Stabilizer drops on death remain unchanged.

A dash ability complements melee by giving the player a burst-movement tool with
brief invulnerability to close gaps and dodge bullets.

## Consequences

**Positive:**

- Creates a clear risk/reward loop: closing distance to melee is dangerous but
  rewarding
- Disarmed enemies becoming aggressive Chasers adds a second-order consequence
  to the decision to disarm
- Dash + melee together form a "close-dodge-strike" combat flow that rewards
  skilled play
- Weapon acquisition is now an active player choice, not a passive loot drop
- The cone hitbox reuses the same spatial primitives as the collision system
  (`point_in_cone` alongside `circles_overlap`)

**Negative:**

- Players must learn the melee mechanic — passive weapon drops were more
  discoverable
- Balance is more complex: melee range, cooldown, dash timing, and disarmed
  enemy aggression all interact
- Enemies that die before being disarmed yield no weapon, which can feel
  punishing if the player accidentally kills them with ranged fire
