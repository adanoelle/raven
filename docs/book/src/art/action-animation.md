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

## What About a Full 360° Spin?

A spin attack was considered for the knight's swipe, and the tradeoffs
are worth recording because they cut in opposite directions.

**What it buys — radial symmetry kills every direction problem at
once.** The arc VFX becomes an expanding *ring* (the one VFX shape with
zero rotation issues — the concussion ring is the precedent), the
`flip_x` sword-hand question disappears, and the dash-spin looks
identical whatever direction the dash travels. A spinning character
trailing afterimages is the best-case visual interaction with the dash
of any melee shape.

**The hidden art cost — this is a side-facing game.** Every sprite is
drawn facing right and mirrored; a true spin needs back and front views
that exist nowhere else, and an honest turnaround for one animation
commits every future character to the same debt. The escape hatch is
the classic tiny-sprite trick (Zelda's spin attack at similar scale):
**wind-up in side view** (sword pulled back, big anticipation) → **1–2
frames of spin blur** — a smear disc with a sword ring, more VFX than
anatomy — → **recover in side view**. Personality lives in the wind-up
and recovery; the middle is pure motion. It also happens to obey the
craft rules above: one frame of maximum distortion, and the crisp
leading edge *is* the ring.

**The real cost is the economy.** Melee is the risk lever of the steal
loop ([ADR-0008](../decisions/0008-melee-disarm-over-death-drops.md)):
closing distance *and aiming the cone* is the price of a weapon. A 360
removes the aiming half of that price — and the visual must match the
hitbox (a 360-looking swing with a 90° hitbox is exactly the "it felt
unfair" bug), so it can't be faked visually. The Brawler's Ground Slam
also already owns "the radial move," and a radial basic melee would
spend some of that contrast.

**Where it landed: the spin is the dash-slash.** The standing melee
keeps its aimed cone — the economy stays priced — and a melee started
*mid-dash* widens to a full circle. The safer 360 hitbox is gated
behind the dash's own commitment and cooldown, and the dash-slash gets
a real mechanical identity, not just a visual one.

> **Status:** the mechanic is prototyped as a one-branch combo. A dash
> from neutral grants a single *follow-up token* (`DashFollowUp`),
> spendable on either a **second dash** (`dash_system` lets it bypass
> the cooldown) or the **360° spin** (`melee_system` widens
> `half_angle` to a full circle and consumes the token). Whichever is
> used first ends the chain — mobility or the spin, never both — so
> the spin's price is the escape it replaces. The chain is the
> **knight's class talent**: other classes get a single dash and the
> aimed cone. The feel can be judged
> with placeholder art before any smear frame is drawn; if it survives
> playtesting, the dash-attack variant row is cheap: its middle frames
> are smear-disc, not anatomy.

## Open Questions

- **Arc direction vs. the no-rotation rule.** Melee aims anywhere, but
  [spec §8](art-spec.md#8-pixel-art-rules-for-crisp-scaling) forbids
  sprite rotation. The starting position: quantize the visual arc to
  left/right via `flip_x` (the knight's swipe is horizontal anyway), and
  only add a hand-drawn up/down variant — or allow rotation for soft
  additive VFX only — if flip-only bothers anyone in playtests.
- **Does the dash-spin keep its hitbox advantage?** The follow-up
  token already prices it — spinning forfeits the second dash, so the
  360 costs an escape. If playtesting still shows it crowding out the
  aimed swipe, the remaining levers are dash cooldown, the follow-up
  window length, spin damage, or reduced spin range — the shape can
  stay even if the numbers move.
- **Should melee during the *second* dash spin too?** It doesn't (the
  token is spent), which keeps dash-dash-spin from collecting both
  rewards. But a mid-dash slash with the aimed cone might read oddly
  next to the spinning first-dash version — watch for it in playtests.
