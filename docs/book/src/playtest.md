# Playtest Playbook

A repeatable checklist for manually validating gameplay feel, visual
correctness, and system interactions that unit tests cannot cover.

## Running the game

Build and launch with:

```sh
just run
```

To enable the ImGui debug overlay, build with the `RAVEN_ENABLE_IMGUI` CMake
option. The overlay displays entity counts, component state, and frame timing.

## Controls reference

| Action | Keyboard      | Gamepad            | Mouse           |
| ------ | ------------- | ------------------ | --------------- |
| Move   | WASD / Arrows | Left stick / D-pad | —               |
| Aim    | —             | Right stick        | Cursor position |
| Shoot  | Z             | A button           | Left click      |
| Focus  | Left Shift    | Right shoulder     | —               |
| Bomb   | X             | B button           | —               |
| Pause  | Escape        | Start              | —               |

Keyboard and gamepad inputs stack — both can be used simultaneously. Mouse
movement sets aim to cursor position; right stick input overrides it.

## Playtest scenarios

Work through each group in order. Check every box before moving on.

### 1. Movement and aiming

- [ ] WASD moves the player in all eight directions with no stutter
- [ ] Left stick produces smooth diagonal movement
- [ ] D-pad moves in cardinal directions and stacks with left stick
- [ ] Right stick aims independently of movement direction
- [ ] Mouse cursor aims toward the pointer; moving the mouse overrides stick
- [ ] Right stick overrides stale mouse position when used
- [ ] Releasing all aim inputs retains the last aim direction
- [ ] Stick deadzone (0.2) filters out drift — no aim jitter when stick is
      centered

### 2. Shooting

- [ ] Holding shoot fires at a steady rate (~5 shots/s at default 0.2 s
      cooldown)
- [ ] Releasing and re-pressing shoot respects the cooldown
- [ ] Bullets travel in the current aim direction at 300 px/s
- [ ] Bullets despawn after 3 s or when leaving the screen
- [ ] Bullet rotation matches the aim direction visually

### 3. Weapon steal loop

- [ ] Killing an enemy with a `BulletEmitter` drops a weapon pickup at the death
      position
- [ ] Walking over the pickup collects it — player's fire pattern changes
      immediately
- [ ] A 10 s decay timer starts on collection
- [ ] When the timer expires: explosion VFX spawns, player takes 1 damage, 2 s
      invulnerability granted, weapon reverts to default
- [ ] Collecting a second weapon while decaying resets the timer but preserves
      the original default weapon
- [ ] After revert the player fires with the original default weapon

### 4. Stabilizer pickups

- [ ] Mid-tier enemies drop stabilizers at ~15% rate; bosses drop at 100%
- [ ] Grunts never drop stabilizers
- [ ] Collecting a stabilizer while carrying a Common or Rare stolen weapon
      removes the decay timer permanently
- [ ] Stabilizer has no effect if the player has no stolen weapon (no
      `WeaponDecay`)
- [ ] Stabilizer does not work on Legendary weapons — pickup remains in the
      world

### 5. Piercing

- [ ] Weapons with `piercing = true` pass through enemies on hit
- [ ] Non-piercing bullets stop and despawn on first contact

### 6. Enemy emitters

- [ ] Enemies with `BulletEmitter` fire patterns matching their JSON definitions
- [ ] Bullet count, spread angle, speed, and fire rate match the pattern data
- [ ] Enemy bullets have correct `Owner::Enemy` — they damage the player, not
      other enemies

### 7. Collision and damage

- [ ] Player bullets damage enemies on overlap (circle-circle hitbox)
- [ ] Enemy bullets damage the player on overlap
- [ ] Invulnerability frames prevent repeated damage for 2 s after a hit
- [ ] Damage numbers and HP changes are consistent

### 8. Edge cases

- [ ] Chained steals: collect weapon A, then weapon B before A decays — original
      default is preserved through both
- [ ] Decay while invulnerable: weapon reverts but no damage is dealt
- [ ] Stabilizer without stolen weapon: no crash, pickup stays in world
- [ ] Stabilizer on Legendary: no crash, pickup stays in world
- [ ] Rapid pickup collection: walk through multiple pickups quickly — only one
      collected per frame
- [ ] Entity cleanup: bullets, pickups, and VFX despawn correctly — no entity
      leaks over a long session

## What to look for

**Feel** — Inputs are responsive with no perceptible lag. Aim snaps to the
stick/mouse immediately. Shooting cadence feels consistent. Movement is smooth
with no hitching.

**Visual** — Bullets and pickups render at the correct position. Sprites face
the aim direction. Explosion VFX appears at the player, not at the world origin.
No Z-order glitches between layers.

**Balance** — Default fire rate and bullet speed feel appropriate. Decay timer
(10 s) gives enough time to find a stabilizer. Stabilizer drop rates feel fair
but not trivial. Legendary weapons feel worth the risk.

**Bugs** — No crashes or freezes. No entities stuck in invalid states. No
bullets spawning at (0, 0). Registry entity count stays stable over time (check
with the debug overlay).

## Reporting issues

When filing a bug:

1. State what you expected to happen
2. State what actually happened
3. List exact steps to reproduce
4. Note which input device was used (keyboard, gamepad, mouse)
5. Attach a screenshot or recording if possible
