# Character Animation Process

The working method for taking a character from first idea to animated
sprite sheet in the game — the order of operations and the reasoning
behind it. This page is the shared process reference for everyone drawing
for Raven; the tool mechanics (template files, guides, brushes, export
settings) live in the [Aseprite Setup Guide](art-aseprite-guide.md), and
the numbers (sizes, frame counts, timings) live in the
[Art Specification](art-spec.md).

The short version: **silhouette first, at final size, in the game as
early as possible.** Everything below is that sentence expanded.

---

## Why Silhouette First

Raven's second design pillar is *readable chaos*: the screen fills with
bullets and the player must never be confused. At gameplay size a player
character is a 24x24 body — the game never shows more pixels than that,
so a design that only works with detail visible doesn't work. The
silhouette — the solid black shape — is the part of the design the game
actually renders at combat distance, which is why it comes first and why
everything else is built on top of it.

Two rules follow directly:

- **Draw at final size.** Concept work at 64x64 or on paper is great for
  finding ideas, but the design isn't real until it exists at 24x24.
  Never scale art down — redraw it at size. (The
  [spec](art-spec.md#1-the-canvas-virtual-resolution) says the same:
  author at 1:1, one Aseprite pixel = one game pixel.)
- **Judge zoomed out.** Work zoomed in, evaluate at 1x and 2x. 2x is
  close to what a Switch player sees in handheld mode; if it reads
  there, it reads everywhere.

---

## The Process, Step by Step

Each step produces something checkable before the next step builds on
it. The expensive mistakes in pixel animation are the ones discovered
after twenty frames are drawn — this order exists to catch them at one
frame, or zero.

### 1. Silhouette thumbnails

Open the medium template (32x32 frame, 24x24 body — see
[template setup](art-aseprite-guide.md#2-template-setup)) and draw
**5–10 solid black silhouettes** of the character, one per frame or one
per file, directly at final size. No color, no interior detail, no
outline — just the shape, feet on the FT guide.

The [silhouette brushes and mannequin](art-aseprite-guide.md#4-silhouette-blocking)
make this fast: stamp head + torso + legs in three clicks, then push the
shape around — a wider stance, bigger pauldrons, different hair mass, a
weapon at rest vs. ready. The proportion guides
([2-head chibi](art-aseprite-guide.md#3-chibi-proportion-guides): ~11px
head, ~22px total height for the medium tier) keep every variation
honest.

Then pick a winner by squinting: which shape reads as *this character
and no other* at arm's length? The
[Knight page](characters/knight.md#silhouette) shows what the answer
looks like in words — pauldrons, medium build, compact hair, a
deliberately over-wide broadsword.

**Decide the mirroring question now.** Gameplay sprites are drawn facing
right and the engine mirrors them with `flip_x`
([spec §6](art-spec.md#6-facing-directions)) — so a sword in the right
hand switches hands when the character walks left. This is normal for
the genre and invisible in play, but it means strongly asymmetric
designs (a shield arm, an off-center emblem) should be chosen knowing
they'll mirror. Settle it at the silhouette stage, not at frame 20.

### 2. One fully rendered idle key frame

Take the winning silhouette and render it completely: palette, outline,
interior detail. This single frame is where the character's colors get
decided — target 6–10 colors, per the working method on the
[Knight page](characters/knight.md#color-palette-working-draft), with
one signature color that pops ([Color Theory](color-theory.md) covers
the hierarchy).

This frame is the **key frame** — the reference every other frame will
be checked against. Time spent making it right is multiplied across the
whole sheet.

### 3. Context checks, before animating anything

One rendered frame is cheap to change; a full sheet is not. So the key
frame gets tested in context first:

- **1x and 2x zoom** — does it read at game size and handheld size?
- **Black-fill test** — flood the frame to solid black; the silhouette
  should still identify the character.
- **Against the palettes** — drop the frame onto swatches of the darkest
  and brightest environment palettes
  ([Tileset Palette Design](tileset-palette.md)). The
  [readability tests on the Knight page](characters/knight.md#readability-tests)
  are the full checklist.
- **Next to the neighbors** — beside a grunt enemy and (eventually) the
  other player characters, at 1x.

Anything that fails here gets fixed in one frame instead of twenty-four.

### 4. Idle animation — 4 frames at 4 fps

Idle is the first animation because it's the smallest test of the whole
pipeline: few frames, subtle motion, and the character spends a lot of
screen time in it.

- **Breathing, not bouncing.** 1–2px of vertical movement in the torso
  and head. Feet stay welded to the FT guide —
  [anchoring](art-aseprite-guide.md#5-anchoring) is what keeps the
  character from jittering in place.
- **Pose to pose.** Draw the two extremes first (breath in, breath out),
  confirm they read, then add the in-betweens. This is the working
  method for every animation on the sheet: extremes first, in-betweens
  after the extremes are approved-by-squint.
- **Overlapping action.** Hair, cloth, and tassels lag 1–2 frames behind
  the body ([spec §5](art-spec.md#5-making-animation-feel-alive)). Even
  on an idle, this is what separates "alive" from "sliding."

Onion skinning ([guide §7.2](art-aseprite-guide.md#72-using-onion-skinning-for-anchoring))
and a looping 200% preview window are the tools here.

### 5. Walk cycle — 6 frames at 10 fps

The classic cycle at this size: **contact → down → passing → contact
(other foot) → down → passing**. At a 24x24 body the legs are ~5px
tall, so the cycle is really about the *body*: 1px of rise and fall
between passing and contact, arms or weapon counter-swinging, hair
following one frame late.

The feet still respect the FT baseline on contact frames; the body does
the bobbing. Fewer distinct, readable poses beat more mushy ones — the
spec's principle of *fewer frames with longer holds* applies to every
row.

### 6. Into the game — immediately

With idle and walk done, the sheet goes in the game **before** any
action frames are drawn. Export rows 0 (idle) and 1 (walk) per the
[export settings](art-aseprite-guide.md#73-exporting-the-sprite-sheet);
a correctly named PNG drops into place with zero code changes
([file names do the wiring](../getting-work-into-the-game.md#file-names-do-the-wiring)).
The engine already runs idle and walk from those two rows — melee and
dash temporarily reuse the walk row until the action rows exist, so
nothing breaks while the sheet is half-finished.

This step is the whole reason the order matters. An animation that loops
beautifully in Aseprite can die in-engine: too subtle under bullets, too
noisy over dark tiles, wrong personality at actual speed. Ten minutes of
running around a real room answers questions no amount of preview
squinting can. Iterate here — on two rows — until the character feels
right, *then* invest in the expensive frames.

### 7. Action rows, in screen-time order

The remaining rows, ordered by how often the player sees them: **attack
(4f @ 12 fps) → dodge (3f @ 15 fps) → hurt (2f) → death (5f)** — exact
timings in the [spec's player table](art-spec.md#4-animation-frame-counts-and-timing).

- **Anticipation and follow-through.** One wind-up frame before the
  swing, 1–2 settle frames after. The engine holds non-looping
  animations until their last frame, so follow-through always plays out
  even when the gameplay window is shorter than the animation.
- **Use the padding.** Attack and dodge frames are what the 4px padding
  zone is *for* — the sword arc and dash lean break out of the body
  zone by design.
- **Keep checking in-engine.** Each row goes into the game as it's
  finished, not at the end.

---

## Traps Worth Skipping

Each of these has burned someone before; they're cheap to avoid:

- **Polishing every frame before the first in-engine look.** Step 6
  exists precisely so this can't happen — get two rough rows in the game
  before rendering twenty-four finished frames.
- **Only ever seeing the art zoomed in.** At 800% everything looks bold
  and readable. Keep a 1x/2x preview window open the entire session.
- **Starting with the attack animation.** It's the exciting one, but it
  depends on decisions (silhouette, palette, anchor, timing feel) that
  idle and walk settle first.
- **Scaling down instead of redrawing.** Downscaled art always looks
  downscaled. Ideas can start big; pixels start at final size.
- **Palette drift.** Adding "just one more shade" mid-animation
  fragments the palette across frames. New colors get added to the
  palette file deliberately, then used — see
  [organization](art-organization.md) for where the palette file lives.

---

## Where to Go Next

- The concrete route through the steps above — every file path, tool,
  and command in order, with the knight as the example — is the
  [First Character Walkthrough](first-character-walkthrough.md).
- Sources, sketches, exports, and the notes that accumulate around them
  all have a home — the directory layout and naming conventions are in
  [Art Files and Organization](art-organization.md).
