# Walk Cycle: Blocking Strategy

How to get from a blank 6-frame timeline to a working walk cycle using
color blocks — no detail, no rendering, just the motion. This page
expands on the walk step in the
[Character Animation Process](character-animation-process.md#5-walk-cycle--6-frames-at-10-fps).
The frame count and timing (6 frames @ 10 fps, 0.6s cycle, looping) are
owned by the
[spec's player table](art-spec.md#4-animation-frame-counts-and-timing);
the brush set and proportion guide values (HT, CH, FT) are owned by the
[Aseprite Setup Guide](art-aseprite-guide.md#3-chibi-proportion-guides).
Numbers repeated below are for convenience — if they ever disagree with
those pages, those pages win.

---

## Why Block Before Drawing

At 24x24, a walk cycle is really a body-rhythm problem. The legs are
~5px tall — there isn't room for anatomical leg animation, so the
readability comes from **weight, bob, and counter-swing** in the torso
and head. Blocking with flat color rectangles isolates those forces
before any rendering detail can distract from them. If the blocks don't
walk, the finished sprite won't either.

---

## Setup in Aseprite

1. Open the medium template (`art/templates/medium_32x32.aseprite`) or
   the character's working file.
2. **Add frames** until you have 6 total (Insert Frame ×5).
3. **Tag the frames** — select all 6, right-click the timeline → New
   Tag, name it `walk`, set it to loop.
4. **Create a `blocking` layer** above the guides layer. All block
   shapes go here.
5. **Turn on Onion Skinning** (View → Onion Skinning) so adjacent
   frames are visible while drawing — essential for checking the bob
   and foot placement.
6. Set frame duration to **100ms** (10 fps) — the walk row's timing from
   the [spec](art-spec.md#4-animation-frame-counts-and-timing).

---

## The Three Blocks

Use a distinct flat color for each block so you can read the mass
distribution at a glance (e.g. red head, blue body, green legs). At
this scale, three blocks are enough — arms don't get their own piece.
Brushes are the medium-tier set from the
[guide's silhouette brushes](art-aseprite-guide.md#41-custom-silhouette-brushes);
guide rows are the
[medium-tier proportions](art-aseprite-guide.md#31-medium-tier-proportions-32x32-frame).

| Block | Brush / Size | Placement |
|-------|-------------|-----------|
| **Head** | `head_round`, 12×11 px | Centered at x = 16, top at HT guide (y = 6) |
| **Body** | `body_pear`, 10×10 px | Top at CH guide (y = 17) |
| **Legs** | `legs_pair`, 8×5 px | Bottom anchored at FT guide (y = 27) |

The pear is taller than the 5px torso zone on purpose: it runs from CH
down through the hips (y = 17–26) and **overlaps the leg block**. Stamp
the legs *on top* of the body (legs layer above body, or stamp them
last) so the leg split stays readable; the body's lower half is the
hip mass the legs hang from, not a second torso.

### Why No Arm Block?

The torso zone is 5px tall. Arms at this size are 1–2px sticks — they
disappear into the body block rather than reading as separate shapes.
Instead, **widen or shift the body block 1–2px** in the direction of
the counter-swing on each frame. That captures the arm mass without
tracking a fourth tiny piece. Arms get carved out of the body
silhouette later, during the sculpting pass.

---

## Pose-to-Pose Order

Draw the extremes first, then fill in the in-betweens. This is the
same principle as the idle (§4 of the process) applied to a longer
cycle.

| Draw order | Frame | Pose | Why this order |
|:----------:|:-----:|------|----------------|
| **1st** | 1 | Contact (right foot forward) | Most extreme leg spread — defines the range of motion |
| **2nd** | 4 | Contact (left foot forward) | Mirror of frame 1 — confirms the range works both ways |
| **3rd** | 3 | Passing (right foot) | Legs together, body at highest point — defines the bob height |
| **4th** | 6 | Passing (left foot) | Mirror of frame 3 |
| **5th** | 2 | Down | In-between frames 1 and 3 — weight settles onto front foot |
| **6th** | 5 | Down | In-between frames 4 and 6 — mirror of frame 2 |

The two contacts set the range. The two passing frames set the high
point. With onion skinning on, the down frames almost draw themselves
once those four key poses are in.

---

## Frame-by-Frame Blocking Notes

The body and head bob; the feet do not. All vertical offsets below are
for the **body and head blocks only**, measured from the contact pose,
with positive y pointing down (Aseprite's convention):

| Pose | Frames | Body / head y-offset | Where in the cycle |
|------|:------:|:--------------------:|--------------------|
| **Contact** | 1, 4 | **0** (baseline) | Feet on FT, widest stance |
| **Down** | 2, 5 | **+1 px** | Lowest point — weight settles |
| **Passing** | 3, 6 | **−1 px** | Highest point — legs together |

Total travel is 2 px (Down to Passing). Every note below refers to this
table; don't re-derive the offsets per frame.

### Contact (frames 1 and 4)

- **Legs** split wide — the widest stance in the cycle.
- **Body** at the baseline (offset 0 in the table).
- **Head** sits on the body at the baseline.
- **Counter-swing**: shift the body block 1–2px opposite the leading
  foot to suggest arm/weapon motion.
- Feet **lock to FT** (y = 27). This is the anchor frame — if the feet
  drift here, the whole cycle slides.

### Down (frames 2 and 5)

- **Body** drops to +1 px — the lowest point in the cycle. Weight
  commits to the front foot.
- **Legs** begin to close from the contact spread.
- **Head** drops with the body, or lags 1 frame (overlapping action).
- The body block's counter-swing is at its most extreme here, matching
  the weight shift.

### Passing (frames 3 and 6)

- **Legs** together, directly under the body — the narrowest stance.
- **Body** rises to −1 px — the highest point in the cycle.
- **Head** rides up with the body.
- **Counter-swing** is neutral — body block centered, between the two
  extremes.

---

## Checking the Blocking

Before moving to sculpting or rendering:

1. **Play the loop at 10 fps.** The bob should feel like weight, not
   vibration. If the body movement reads as jitter, drop the Down
   offset from the table (Down = 0, same as Contact) and keep only the
   Contact → Passing rise, 1 px total.
2. **Squint test at 1x zoom.** The three blocks should move as a unit
   with visible rhythm. If one block dominates or disappears, adjust
   its size.
3. **Check the feet.** Contact frames must land on FT (y = 27). If the
   feet float on contacts, the character looks like it's skating.
4. **Check the counter-swing.** The body block should shift opposite
   the leading foot. If it stays centered the walk looks stiff; if it
   shifts too much it looks drunk.

Once the blocks walk convincingly, the sculpting pass — carving
silhouette detail, splitting legs, adding hair lag — is just refining
shapes that already move correctly.

---

## Where to Go Next

- The overall process that this step fits into:
  [Character Animation Process](character-animation-process.md).
- Stretch and smear strategy for the action rows (attack, dash) that
  come after the walk cycle:
  [Action Animation: Stretch and Smears](action-animation.md).
- Template files, proportion guides, and export settings:
  [Aseprite Setup Guide](art-aseprite-guide.md).
