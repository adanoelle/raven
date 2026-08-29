# Game Feel — Feedback Anchors

How Raven's mechanics are built to receive art and sound, and exactly where
each cue attaches. This page is for artists and musicians deciding *what to
make* and for developers deciding *where to wire it*. The engine-level
rationale lives in
[ADR-0022](decisions/0022-ability-latch-and-feedback-anchors.md).

The goal, in one sentence: **the game should feel physical** — every action
displaces something, every impact is seen and heard, and the player can read
the state of a fight without looking at the HUD.

---

## 1. The Principle: Gameplay Emits Facts, Presentation Reads Them

Raven's simulation never draws or plays anything itself. Instead, it leaves
evidence:

- It **pushes events** — "an enemy was hit", "a dash started" — into a queue
  that the presentation layer drains each tick. Sound already works this way
  end to end.
- It **leaves state visible** — an ability that lasts 0.15 seconds exists as
  data for those 0.15 seconds, with a built-in countdown. Animation already
  works this way: the animation system picks the melee pose simply by
  noticing a melee attack exists.

The practical consequence for contributors: **every mechanic below already
has a hook**. Adding a sound or an effect never requires redesigning
gameplay — the attachment points are part of the design.

## 2. The Four Anchors

Every timed action in the game (melee, dash, slam, concussion, weapon decay,
…) exposes the same four moments where feedback can attach:

| # | Anchor          | The moment                              | What attaches here                                        |
| - | --------------- | --------------------------------------- | --------------------------------------------------------- |
| 1 | **Activation**  | The action starts                       | Swing/launch sounds, muzzle flashes, animation switches   |
| 2 | **Impact**      | The hit resolves — targets are known    | Hit sounds, hit flashes, impact particles, screen shake   |
| 3 | **Active window** | Every moment the action is live; a free 1→0 progress value | Duration-driven effects: expanding rings, trails, glows |
| 4 | **Expiry**      | The action ends                         | Cooldown UI pulses, "ready again" cues, cleanup           |

Anchor 2 is special: it is the only moment that knows *what was hit, where,
and in which direction* — exactly the data impact feedback needs (per-enemy
flash positions, knockback directions for directional particles, hit counts
for scaling screen shake).

## 3. Event Catalog

Every feedback-worthy event in the current game, with the cues each one
wants. **Sound status** refers to the
[SFX catalog](audio-spec.md#2-sound-effects-catalog); no visual effects
exist yet, so every VFX column is open work — sizes and frame counts are in
the [art spec](art/art-spec.md#4-animation-frame-counts-and-timing).

### The core loop: steal, hold, keep-or-lose

| Event | What happens mechanically | Anchor | Sound | Visual |
| ----- | ------------------------- | ------ | ----- | ------ |
| **Melee swing** | Cone sweeps in the aim direction (90° wide for Brawler, ~46° for Sharpshooter) | 1 | `melee.wav` (shipping) | Arc slash sprite following the cone |
| **Melee connect** | Each enemy in the cone takes damage and is shoved outward | 2 | needed: `melee_connect.wav` | Hit flash on each struck enemy |
| **Disarm** | The struck enemy's weapon flies loose as a pickup; the enemy turns and charges at 1.5× speed | 2 | needed: `disarm.wav` — **the signature reward sound** | Weapon-pickup sprite popping loose; enraged flash on the disarmed enemy |
| **Weapon pickup** | Player gains the stolen weapon; a 10-second decay timer starts | 1 | `pickup.wav` (shipping) | Pickup shimmer while on the ground (8x8, specced) |
| **Decay countdown** | Timer runs; HUD shows a shrinking yellow bar | 3 | needed: `decay_warning.wav` over the final ~3s | Weapon sprite flicker as time runs out |
| **Decay explosion** | Weapon explodes: 1 HP damage (if not invulnerable), reverts to default gun | 2 | needed: `decay_explosion.wav` | Explosion animation — the engine already spawns a 0.5s explosion entity at the player; **no art or renderer exists for it yet** |
| **Stabilizer drop** | Mid enemies 15%, bosses always; 8s lifetime on the ground | 1 | — (covered by enemy death) | Distinct pickup sprite; must read as *rarer* than a weapon |
| **Stabilizer collected** | Decay timer removed — the weapon is permanent | 2 | needed: `stabilizer.wav`, warmer/more final than pickup | "Locked in" flash or glint on the player |

### Abilities

| Event | What happens mechanically | Anchor | Sound | Visual |
| ----- | ------------------------- | ------ | ----- | ------ |
| **Dash** | 0.12s burst at 400 px/s, invulnerable for 0.18s | 1, 3 | `dash.wav` (shipping) | Motion trail / afterimages during the window |
| **Ground slam** (Brawler) | 50px-radius shockwave, 4 damage, heavy 350 px/s knockback; 0.15s window | 1, 2, 3 | needed: `ground_slam.wav` | Expanding shockwave keyed to the window's 1→0 progress; screen shake scaled by enemies hit |
| **Concussion shot** (Sharpshooter) | 45px-radius blast, only 1 damage but 400 px/s knockback — a space-maker | 1, 2, 3 | needed: `concussion_shot.wav` | Expanding ring, 96x96, 4 frames, additive glow — fully specced in [ADR-0012](decisions/0012-concussion-shot-vfx.md) |
| **Charging a shot** (Sharpshooter) | Hold to charge over 0.8s; movement halved while charging; HUD bar fills | 3 | optional rising tone | Glow building on the player/weapon |
| **Full charge reached** | Crossing 95% makes the shot pierce; HUD bar turns yellow | edge of 3 | needed: `charge_ready.wav` | Bright pulse at the moment of crossing |
| **Charged release** | Bullet fires with 0.5–3.0× damage by charge level | 1 | `shoot.wav` (shipping; a fuller variant is desirable) | Muzzle flash scaled by charge |

### Getting hit, and hitting back

| Event | What happens mechanically | Anchor | Sound | Visual |
| ----- | ------------------------- | ------ | ----- | ------ |
| **Enemy hit by bullet** | Damage, plus a 0.1s knockback nudge along the bullet's path | 2 | `enemy_hit.wav` (shipping) | Hit spark (16x16, specced); brief white flash on the enemy |
| **Enemy knocked back** | Enemy's brain suspends; it slides and — because knockback still collides with walls — visibly **slams into them** | 3 | wall-slam thud (future) | Dust puff on wall impact (future) |
| **Enemy death** | Score added, drops rolled | 2 | `enemy_down.wav` (shipping) | Death animation per enemy (specced); explosion VFX (specced) |
| **Player hit** | Damage, then 2 full seconds of invulnerability | 2, then 3 | `player_hit.wav` (shipping) | Hurt flash on impact; **blink/ghost during the entire 2s invulnerability** — the player must be able to *see* they are safe (the HUD health bar already turns white) |
| **Player loses a life** | HP refills, a life pip disappears, 3s invulnerability | 2 | needed: `player_death.wav` | Death + respawn animation (specced rows) |

### Rooms and progression

| Event | What happens mechanically | Anchor | Sound | Visual |
| ----- | ------------------------- | ------ | ----- | ------ |
| **Enemy wakes** | Enemies idle until the player is in range *and* in line of sight, then activate | 1 | — (subtle, optional) | A wake-up tell: eyes lighting, a twitch — gives the player a fair warning |
| **Wave spawns** | Next wave appears when the previous one is cleared | 1 | — | Spawn-in effect so enemies don't pop from nothing |
| **Room cleared** | Final wave down — exits open | 2 | needed: `wave_clear.wav` | Exit visibly opening/glowing; currently exits are invisible logic |
| **Boss appears** | Stage 3's final wave: a fleeing boss that fires a Legendary nova pattern constantly | 1 | needed: `boss_entrance.wav` | Entrance animation (specced); see [Enemy Archetypes](enemy-archetypes.md) |
| **Game over / victory** | Scene transitions | — | `mus_game_over.ogg` / `mus_victory.ogg` ([track list](music-track-list.md)) | Scene art |

## 4. Physicality Channels

Cues stack into a hierarchy of "how hard did that hit":

1. **Knockback** (shipping) — every melee hit, slam, concussion, and bullet
   displaces enemies. This is the base layer, and it's pure simulation: it
   works today with zero art.
2. **Hit flash + spark** (art needed) — confirms *contact* the same frame it
   happens.
3. **Screen shake** (engine work + tuning) — reserved for the heavy hits:
   ground slam, decay explosion, boss moments. ADR-0012 speccs 1–2px for
   2–3 frames. If everything shakes, nothing does.
4. **Hit-stop** (future) — a few frozen ticks on the biggest impacts.
   The engine's fixed-timestep design supports it cleanly; treat it as the
   strongest spice in the rack.

A useful calibration: **concussion shot should feel like air pressure**
(big push, little damage, airy sound, expanding ring) while **ground slam
feels like mass** (boom, shake, dust). Same mechanic shape, opposite
material character — that contrast is what makes the two classes feel
different in the hands.

## 5. For Developers: Wiring New Feedback

- One-shot cues (anchors 1, 2, 4) go through the event queue:
  `push_sfx(reg, Sfx::…)` today; an equivalent impact-event queue carrying
  hit records is the planned path for one-shot VFX
  ([ADR-0022](decisions/0022-ability-latch-and-feedback-anchors.md)).
- Duration cues (anchor 3) **observe components**: read the ability's
  `remaining` field and map it to radius, frame index, or alpha. Never add
  a parallel VFX timer that can drift out of sync.
- Simulation code must never call into the renderer or audio engine
  directly — that's what keeps the game deterministic and headless-testable.
