# Action Animation: Stretch and Smears

How the melee attack and dash get their energy — and specifically, how a
horizontal sword swipe can look right both standing still and mid-dash
without drawing two versions of it. This page is the strategy for the
attack/dodge rows described in the
[Character Animation Process](character-animation-process.md#7-action-rows-in-screen-time-order);
the frame counts and timings live in the
[spec's player table](art-spec.md#4-animation-frame-counts-and-timing).

---

## Two Engine Facts That Do Most of the Work

The consistency problem — "won't the slash look different when dashing?"
— is mostly dissolved by how the engine already behaves:

1. **Melee outranks dash in the animation state machine**
   (`game_scene.cpp`, priority Melee > Dash > Walk > Idle). Slashing
   mid-dash switches the body to the *attack row*. There is no
   "dash-stretched body playing a slash" frame to draw — the standing
   slash and the dashing slash use the **same body animation**, always.

2. **The slash arc is its own sprite, not part of the body sheet.** The
   [Ability VFX table](art-spec.md#ability-vfx) specs it as a separate
   48x48 overlay (3–4 frames, one lifetime of the melee window). Once
   it's in, the swipe reads *identically* in both contexts because it is
   literally the same pixels, spawned on top of whatever the body is
   doing.

   > **Status:** the arc VFX is specced but not yet spawned by
   > `melee_system` — melee is currently an invisible hit check. The
   > arc sheet is safe to draw now; it gets wired when it exists.

## One Subject Carries the Stretch

The classic mistake with squash-and-stretch at this scale is distorting
everything at once. Split the budget — each action stretches *one*
subject:

- **Dash: the body stretches.** Elongate along the motion vector for one
  frame — ~24px of character becomes ~29px long and ~20px tall — then
  recover. The 3-frame dash row is: **squash** (anticipation crouch) →
  **stretch** (the smear) → **recover**.
- **Melee: the sword smears, the body stays solid.** The 4-frame attack
  row is: **anticipation** (torso twist back, slight squash) →
  **contact** → **follow-through** (overshoot) → **settle**. The *arc
  VFX* is the stretch — the sword effectively becomes a crescent smear —
  while the body barely deforms.

Because the two actions distort different subjects, the dash-slash
composes instead of compounding: an interrupted body stretch, a smeared
arc, and real translation across the screen. Nothing fights.

## Stretch Craft Rules

- **Smear the trailing edge; keep the leading edge crisp.** The front
  silhouette (face, chest, sword tip at contact) stays on-model; the
  elongation and ghost pixels happen behind it. This is what keeps
  *readable chaos* intact — the player's eye tracks the crisp edge.
- **One frame of maximum distortion, never two.** At 12–15 fps, a single
  smear frame reads as speed; two read as goo. Stretch is a transient,
  not a state.
- **Stretch from the anchor.** Standing attack: feet welded to the FT
  line, the torso does the twisting. Dash: the feet line may break (it's
  the one state allowed to float), but the center of mass travels
  smoothly.
- **Conserve volume.** +25% length means roughly −20% height on the same
  frame. A stretch that only adds pixels looks like growing, not speed.

## Where the Dash-Slash Gets Its Extra Energy

Since the body animation is shared, the "maximum visual effect" version
comes from context, not from a different animation:

1. **Real velocity** — the same four attack frames swept across 40+
   pixels of screen are inherently more dramatic. Free.
2. **The arc VFX riding along** — spawned at the player, it naturally
   trails the motion.
3. **Afterimages** — 2–3 fading ghost copies of the body sprite spawned
   during the dash. Ghosts *are* stretch, delivered by the engine
   instead of the pixels, and they layer under the slash without anyone
   drawing a combined frame. *(Planned: the engine has the pattern for
   short-lived visual entities; the spawner is a small addition.)*

## Open Questions

- **Arc direction vs. the no-rotation rule.** Melee aims anywhere, but
  [spec §8](art-spec.md#8-pixel-art-rules-for-crisp-scaling) forbids
  sprite rotation. The starting position: quantize the visual arc to
  left/right via `flip_x` (the knight's swipe is horizontal anyway), and
  only add a hand-drawn up/down variant — or allow rotation for soft
  additive VFX only — if flip-only bothers anyone in playtests.
- **A dedicated dash-attack row?** Some games have one; we deliberately
  don't draw it up front. Ship the shared attack row, judge the
  dash-slash in-engine, and only add a variant row if it feels wrong in
  play. The bet: velocity + ghosts + arc make it feel great without one.
