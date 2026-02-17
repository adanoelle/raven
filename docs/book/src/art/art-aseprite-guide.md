# Aseprite Setup Guide

Step-by-step guide for setting up Aseprite templates, canvas guides, and
anchoring conventions for every sprite tier in Raven.

---

## 1. Frame Size vs. Body Size

Raven uses **uniform-grid sprite sheets** — every cell in a sheet is the same
pixel dimensions. But the drawn character does not fill the entire cell. We
deliberately leave transparent padding around the body so that action frames
(melee swings, weapon holdouts, dash trails) stay within the same grid without
needing a separate overlay system.

| Tier        | Frame Size | Body Size | Padding      | Used For                            |
| ----------- | ---------- | --------- | ------------ | ----------------------------------- |
| Small       | 16x16      | 14x14     | 1px per side | Grunt enemies, small pickups        |
| Medium      | 32x32      | 24x24     | 4px per side | Player characters, mid-tier enemies |
| Large       | 48x48      | 36x36     | 6px per side | Bosses                              |
| Extra-large | 64x64      | 48x48     | 8px per side | Mega-bosses, special encounters     |

**The body size is a guideline, not a hard crop.** Idle and walk frames stay
inside the body zone. Action frames (attacks, dashes) are expected to break into
the padding — that is what the padding is for.

---

## 2. Template Setup

Create one Aseprite template file per tier. Reuse it for every character, enemy,
or boss at that size.

### 2.1 Creating a template (Medium tier example: 32x32)

1. **File > New Sprite.** Set width and height to **32x32**. Color mode: RGBA.
   Background: Transparent.

2. **Add guide lines.** View > Guides, or drag from the rulers (View > Show
   Rulers if hidden). Add four guides to mark the 24x24 body zone:

   - Vertical guide at **x = 4** (left body edge)
   - Vertical guide at **x = 28** (right body edge)
   - Horizontal guide at **y = 4** (top body edge)
   - Horizontal guide at **y = 28** (bottom body edge)

3. **Add a center guide.** These help with symmetry and anchoring:

   - Vertical guide at **x = 16** (horizontal center)
   - Horizontal guide at **y = 16** (vertical center)

4. **Set grid size.** Edit > Preferences > Grid: set to **1x1** with visible
   grid for precise pixel placement. Alternatively, set to **8x8** subdivisions
   for quick spatial reference.

5. **Save as template.** File > Save As: `templates/medium_32x32.aseprite`. Keep
   this file clean — no art, just guides. Duplicate it to start each new
   character.

### 2.2 Guide placement for each tier

#### Small (16x16 frame, 14x14 body)

```
Guides: x=1, x=15, y=1, y=15 (body zone)
Center: x=8, y=8
```

```
  0               15
  ┌────────────────┐
  │░░░░░░░░░░░░░░░░│ 0
  │░┌────────────┐░│ 1  ← body top
  │░│            │░│
  │░│   14x14    │░│
  │░│   body     │░│    ← x=8 center
  │░│            │░│
  │░│            │░│
  │░└────────────┘░│ 15 ← body bottom
  └────────────────┘
    1              15
```

The 1px padding is tight. Grunt enemies are simple shapes — the padding handles
minor animation overshoot (a bounce on idle, a slight lean on walk). Do not plan
for weapon extensions at this size.

#### Medium (32x32 frame, 24x24 body)

```
Guides: x=4, x=28, y=4, y=28 (body zone)
Center: x=16, y=16
```

```
  0                               31
  ┌────────────────────────────────┐
  │░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░│ 0
  │░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░│
  │░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░│
  │░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░│ 4  ← body top
  │░░░░┌────────────────────┐░░░░░░│
  │░░░░│                    │░░░░░░│
  │░░░░│      24x24         │░░░░░░│
  │░░░░│      body          │░░░░░░│    ← y=16 center
  │░░░░│                    │░░░░░░│
  │░░░░│                    │░░░░░░│
  │░░░░│                    │░░░░░░│
  │░░░░└────────────────────┘░░░░░░│ 28 ← body bottom
  │░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░│
  │░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░│
  │░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░│
  └────────────────────────────────┘
    4                          28
```

The 4px padding per side is where melee weapons, gun holdouts, and dash effects
extend. The Brawler's swing arc and the Sharpshooter's weapon barrel both fit
within this zone during action frames.

#### Large (48x48 frame, 36x36 body)

```
Guides: x=6, x=42, y=6, y=42 (body zone)
Center: x=24, y=24
```

The 6px padding per side gives bosses room for dramatic attack wind-ups,
trailing capes, particle anchor points, and slam effects. Boss art should fill
the body zone with presence — these are the biggest things on screen.

#### Extra-Large (64x64 frame, 48x48 body)

```
Guides: x=8, x=56, y=8, y=56 (body zone)
Center: x=32, y=32
```

Reserved for final-stage bosses or screen-dominating special encounters. At
64x64 on a 480x270 canvas, this entity occupies **13.3% of screen width** and
**23.7% of screen height** — genuinely imposing. The 8px padding handles
large-scale attack animations (wing spreads, beam charges, phase-change
explosions).

---

## 3. Chibi Proportion Guides

In addition to body zone and center guides, each template should include
**horizontal proportion guides** that define anatomy zones — like the
horizontal lines on a character turnaround sheet. These keep the head size,
eye placement, and body divisions consistent across every frame and every
character at that tier.

The game uses **2-head chibi proportions** for player characters and most
enemies: the head height roughly equals the body height. This maximizes
expressiveness at small pixel scales while leaving enough body for weapons
and movement.

### 3.1 Medium tier proportions (32x32 frame)

The character is ~22px tall within the 24px body zone, bottom-anchored.
Add these five horizontal proportion guides to the medium template:

| Guide | Y Position | Zone                | Purpose                             |
| ----- | ---------- | ------------------- | ----------------------------------- |
| HT    | y = 6      | Head top            | Where the skull/hair volume begins  |
| EY    | y = 11     | Eye line            | Center of eye placement             |
| CH    | y = 17     | Chin / neck         | Where head meets body               |
| WA    | y = 22     | Waist / belt        | Separates torso from legs           |
| FT    | y = 27     | Feet / ground       | Anchor point — locked every frame   |

```
  0                               31
  ┌────────────────────────────────┐
  │           padding              │ 0
  │                                │
  │                                │
  │                                │ 4  body zone top
  │                                │
  │· · · · · · · · · · · · · · · · │ 6  HT ── head top
  │            ████████            │     ─┐
  │          ██████████            │      │ Forehead
  │          ██████████            │      │ (big for chibi)
  │· · · · ··██·●●··●●·██· · · · ·│ 11 EY ── eye line
  │          ██████████            │      │
  │          ██████████            │      │ Lower face
  │           ████████             │      │
  │· · · · · · ██████ · · · · · · │ 17 CH ── chin / neck
  │            ██████              │      │
  │            █ ██ █              │      │ Torso
  │            ██████              │      │
  │· · · · · · ██████ · · · · · · │ 22 WA ── waist / belt
  │             █  █               │      │
  │            ██  ██              │      │ Legs
  │            ██  ██              │      │
  │· · · · · ·███··███· · · · · · │ 27 FT ── feet / ground
  │           padding              │ 28 body zone bottom
  │                                │
  │                                │
  └────────────────────────────────┘
```

**Zone pixel budgets (2-head chibi, 22px tall):**

| Zone      | Y Range  | Height | What fits                              |
| --------- | -------- | ------ | -------------------------------------- |
| Crown     | 4 – 6    | 2px    | Accessories extending above head       |
| Hair/head | 6 – 11   | 5px    | Hair volume, forehead (large for chibi)|
| Face      | 11 – 17  | 6px    | Eyes, nose (opt), mouth (opt), cheeks  |
| Torso     | 17 – 22  | 5px    | Shirt/armor color, 1 costume detail    |
| Legs/feet | 22 – 27  | 5px    | Legs, boots as color accent            |

The **eye line at y=11** is placed below center of the head (head runs y=6
to y=17 = 11px, midpoint would be y=11.5). This low eye placement is what
creates the chibi "big forehead" look — the forehead area above the eyes is
larger than the face below.

### 3.2 Small tier proportions (16x16 frame)

Grunt enemies at 2-head chibi, ~12px tall, bottom-anchored:

| Guide | Y Position | Zone          |
| ----- | ---------- | ------------- |
| HT    | y = 2      | Head top      |
| EY    | y = 5      | Eye line      |
| CH    | y = 8      | Chin / neck   |
| WA    | y = 11     | Waist         |
| FT    | y = 14     | Feet / ground |

```
  0               15
  ┌────────────────┐
  │                │ 0
  │· · · · · · · · │ 2  HT
  │    ████████    │
  │· · █·●··●·█ · ·│ 5  EY
  │    ████████    │
  │· · ·██████ · · │ 8  CH
  │     █ ██ █     │
  │· · ·██████· · ·│ 11 WA
  │      █  █      │
  │· · ·██··██· · ·│ 14 FT
  │                │ 15
  └────────────────┘
```

At 16x16, there are very few pixels per zone. The eyes may be just 2
vertical pixels or a 2x2 block. Personality comes from silhouette shape and
color rather than facial detail.

### 3.3 Large tier proportions (48x48 frame)

Bosses can use chibi proportions or deviate toward more menacing ratios.
Two options:

**Option A — 2-head chibi boss** (cute but threatening, like Cult of the Lamb):

| Guide | Y Position | Zone          |
| ----- | ---------- | ------------- |
| HT    | y = 9      | Head top      |
| EY    | y = 17     | Eye line      |
| CH    | y = 25     | Chin / neck   |
| WA    | y = 33     | Waist         |
| FT    | y = 40     | Feet / ground |

**Option B — 2.5-head boss** (more imposing, conventional boss proportions):

| Guide | Y Position | Zone          |
| ----- | ---------- | ------------- |
| HT    | y = 8      | Head top      |
| EY    | y = 14     | Eye line      |
| CH    | y = 21     | Chin / neck   |
| WA    | y = 31     | Waist         |
| FT    | y = 40     | Feet / ground |

The 2.5-head ratio gives bosses a smaller head relative to their body,
making them look more physically imposing and less cute. Choose based on the
boss's personality — a comedic mini-boss might use Option A while a
final-stage threat uses Option B.

### 3.4 Using proportion guides in Aseprite

1. **Add guides after body zone guides.** In the medium template, you should
   have 11 guides total: 4 body zone edges, 2 center lines, and 5 proportion
   lines.

2. **Color-code if possible.** Aseprite guide colors can be changed by
   right-clicking a guide. Use one color for body zone boundaries (e.g.,
   cyan) and another for proportion lines (e.g., red or orange) so they are
   easy to distinguish at a glance.

3. **Proportion guides are targets, not walls.** The eye line means "eyes
   go here" — not "eyes must touch this exact pixel." A 3px tall eye centered
   on the eye line guide is correct usage.

4. **Hair and accessories break above HT.** The head-top guide marks where
   the skull starts. Hair volume, crowns, horns, and hats extend above it
   into the crown zone (between body zone top and HT). This is expected and
   desirable — distinctive headwear is the #1 silhouette differentiator.

5. **Check proportions across frames.** Toggle onion skinning and compare
   frames. The chin line should not drift between idle and walk. The eye line
   stays fixed. Only the feet line is an absolute lock — other guides may
   shift ±1px during squash/stretch, but should average to their guide
   position.

---

## 4. Silhouette Blocking

Before drawing detail, block out character volumes as filled shapes — like a
sculptor roughing in clay before refining. Aseprite offers two approaches:
custom brushes for fast stamping and reference layers for persistent guides.

### 4.1 Custom silhouette brushes

Create reusable brushes that stamp body-part silhouettes in a single click.

**Creating a brush:**

1. On a scratch canvas, draw the silhouette shape in black (e.g., a 12x11px
   rounded rectangle for a chibi head).
2. Select the shape with the marquee tool (**M**).
3. **Edit > New Brush** (or **Ctrl+B**). The selection becomes your active
   brush.
4. To save permanently: click the brush dropdown in the toolbar (small arrow
   next to the brush preview), then click **"Save Brush Here"** at the bottom
   of an empty slot. **Lock the slot** (click the lock icon) so it persists
   across sessions.

**Brush drawing modes:**

After selecting a custom brush, a dropdown appears on the context bar:

- **Pattern mode** — stamps the exact pixels and colors from the original
  selection. Useful for multi-color templates.
- **Foreground color mode** — stamps only the brush *shape*, filled with
  your current foreground color. This is the mode to use for silhouette
  blocking: pick black, stamp the head; pick a different shade, stamp the
  body.

**Suggested brush set for medium tier (32x32 frame, 24x24 body):**

| Brush         | Size   | Shape                          | Purpose                           |
| ------------- | ------ | ------------------------------ | --------------------------------- |
| `head_round`  | 12x11  | Rounded rect / oval            | Standard chibi head mass          |
| `head_tall`   | 10x12  | Taller oval                    | Alternate for serious characters  |
| `body_pear`   | 10x10  | Pear / tapered trapezoid       | Standard chibi torso              |
| `body_wide`   | 12x10  | Wider trapezoid                | Stocky Brawler-type torso         |
| `legs_pair`   | 8x5    | Two small blocks with 2px gap  | Stubby chibi legs                 |

**The 3-click blocking workflow:**

1. Open the character template (with proportion and body zone guides).
2. Select `head_round` brush, foreground color black.
3. Click once in the head zone — centered on x=16, top edge on HT guide.
4. Switch to `body_pear` brush.
5. Click once in the torso zone — top edge on CH guide.
6. Switch to `legs_pair` brush.
7. Click once in the leg zone — bottom edge on FT guide.
8. You now have a blocked silhouette. Merge the parts, then sculpt: carve
   the neck, add hair, refine the profile.

For subsequent animation frames, stamp the same brushes to maintain
consistent volumes, then adjust the pose. This is faster than redrawing
the full silhouette each frame and keeps proportions locked.

### 4.2 Reference layer mannequin

For the first character at each tier, a reference layer provides a
persistent silhouette guide underneath your working layers.

**Creating a mannequin file:**

1. In the medium template, draw a clean front-facing chibi silhouette in
   solid black on the proportion guides. Include the head, torso, and legs
   at the correct 2-head ratio. No detail — just the mass.
2. Save as `templates/chibi_mannequin_32x32.aseprite`.

**Using the mannequin:**

1. Duplicate the tier template to start a new character.
2. **Layer > New > New Reference Layer from File.** Select the mannequin
   file.
3. The mannequin appears as a semi-transparent overlay — visible while you
   draw but non-editable and **excluded from export**.
4. Draw on a normal layer above it. The mannequin acts as a volume target:
   match the head mass, align the shoulders, lock the feet.
5. Delete the reference layer when you no longer need it, or keep it for
   the entire animation process.

**Advantages over brushes:**

- The mannequin stays visible *under* your art as you sculpt. A stamped
  brush disappears into your drawing once you start editing pixels.
- Useful for checking that an action frame's body doesn't shrink or grow
  unexpectedly — toggle the reference layer on to compare volumes.
- Multiple reference layers can coexist. Add a side-view mannequin for
  walk cycles or an action-pose mannequin for attack frames.

### 4.3 Combining both approaches

Use the reference layer mannequin for your first character at each tier to
establish proportions. Once you are confident in the shapes, save the
individual body-part silhouettes as custom brushes. From then on, use the
brushes for fast blocking on new characters and frames, and bring the
mannequin back only when you need to double-check proportions.

**Suggested file organization:**

```
templates/
├── medium_32x32.aseprite            Blank template with guides
├── chibi_mannequin_32x32.aseprite   Full silhouette mannequin
├── small_16x16.aseprite             Blank template with guides
├── chibi_mannequin_16x16.aseprite   Grunt silhouette mannequin
├── large_48x48.aseprite             Blank template with guides
└── boss_mannequin_48x48.aseprite    Boss silhouette mannequin
```

Custom brushes are saved in Aseprite's user configuration directory
(`user.aseprite-brushes`) and persist across all files automatically.

---

## 5. Anchoring

Anchoring determines where the character "sits" within the frame across all
animation poses. If the anchor drifts between frames, the character appears to
float or jitter during playback.

### 5.1 Bottom-center anchor (characters and ground enemies)

Used for the **player, ground-based enemies, and walking bosses**. The
character's feet are pinned to a consistent Y position near the bottom of the
body zone, horizontally centered.

```
Medium tier (32x32 frame):

  ┌────────────────────────────────┐
  │            padding             │
  │        ┌──────────────┐        │
  │        │   head/hat   │        │
  │        │              │        │
  │        │    torso     │        │
  │        │              │        │
  │        │    legs      │        │
  │        └────┤feet├────┘        │  ← feet on this line, every frame
  │           ▲                    │
  │     anchor (x=16, y=27)        │
  │            padding             │
  └────────────────────────────────┘
```

Rules:

- **Feet stay on the same Y pixel in every frame.** Idle bob moves the head and
  torso up, not the feet down.
- **Horizontally centered** in the body zone. Weapon extensions go left/right
  into padding; the body stays centered.
- The engine's `Sprite::offset_y` shifts the visual upward so the feet align
  with the entity's collision center. Tune this value once per character size,
  not per frame.

### 5.2 Center anchor (flying enemies and hovering bosses)

Used for enemies and bosses that **float, fly, or have no clear ground
contact**. The body center stays at the frame center.

```
Large tier (48x48 frame):

  ┌────────────────────────────────────────────────┐
  │                  padding                       │
  │          ┌──────────────────────┐              │
  │          │     wing / horn      │              │
  │          │                      │              │
  │          │   ● body center ●    │              │ ← y=24
  │          │                      │              │
  │          │     tail / trail     │              │
  │          └──────────────────────┘              │
  │                  padding                       │
  └────────────────────────────────────────────────┘
                      ▲
                anchor (x=24, y=24)
```

Rules:

- **Body center stays at frame center in every frame.** Wings, tendrils, and
  other appendages animate around this fixed point.
- Bobbing motion moves the whole body ±1–2px vertically, but the _average_
  position stays at center.
- Set `Sprite::offset_y = 0` and `Sprite::offset_x = 0` for center-anchored
  entities.

### 5.3 Choosing the right anchor

| Entity type                    | Anchor        | offset_y       |
| ------------------------------ | ------------- | -------------- |
| Player (Brawler, Sharpshooter) | Bottom-center | Negative (~-5) |
| Grunt enemies (ground)         | Bottom-center | Negative       |
| Mid enemies (ground)           | Bottom-center | Negative       |
| Flying enemies                 | Center        | 0              |
| Ground bosses                  | Bottom-center | Negative       |
| Hovering / flying bosses       | Center        | 0              |
| Projectiles                    | Center        | 0              |
| VFX (explosions, sparks)       | Center        | 0              |

---

## 6. Bosses: Design Considerations

Bosses are the most complex sprites in the game. They occupy **48x48 or 64x64
frames** on a 480x270 canvas, making them visually dominant.

### 6.1 Screen presence

| Frame Size | % Screen Width | % Screen Height | Feel                   |
| ---------- | -------------- | --------------- | ---------------------- |
| 48x48      | 10.0%          | 17.8%           | Threatening, imposing  |
| 64x64      | 13.3%          | 23.7%           | Dominating, final-boss |

For reference, the 32x32 player at 24x24 body occupies ~5% width / ~8.9% height.
A 48x48 boss is roughly **twice the player's visual size** in each dimension —
large enough to read as a clear threat without overwhelming the play space.

### 6.2 Boss sprite sheet layout

Boss sheets are wider than character sheets because bosses have more animation
states and more frames per state.

```
Example boss sheet (48x48 frames, 8 columns wide):

Row 0: Idle         [4–6 frames]  ██████░░    Menacing presence
Row 1: Phase 2 idle [4–6 frames]  ██████░░    Visually distinct from phase 1
Row 2: Attack A     [5–6 frames]  ██████░░    Clear telegraph + payoff
Row 3: Attack B     [5–6 frames]  ██████░░    Visually distinct from A
Row 4: Hurt         [2–3 frames]  ███░░░░░    Brief stagger, sells impact
Row 5: Transition   [4–6 frames]  ██████░░    Phase change (dramatic)
Row 6: Death        [6–8 frames]  ████████    Spectacular, slower pace
```

### 6.3 Aseprite setup for bosses

1. **One `.aseprite` file per boss.** Bosses are complex enough to warrant their
   own file rather than sharing a sheet with other enemies.

2. **Use frame tags** for each animation state (Idle, Attack A, Attack B, Hurt,
   Transition, Death). This keeps the timeline organized and allows selective
   export.

3. **Use layers** to separate body, effects, and attack overlays:

   - `body` — the boss silhouette, always present
   - `fx` — charge glow, aura, phase-change particles
   - `attack` — weapon/appendage extensions into padding zone

   Flatten on export. The layers are for your editing convenience only.

4. **Phase changes.** If the boss visually transforms between phases, create
   separate idle/attack rows for each phase. The transition animation bridges
   them. Do not try to morph between phases in a single animation — use a
   dedicated transition row.

5. **Attack telegraphs.** Boss attacks must be readable. Use 1–2 bright wind-up
   frames before the hit frame. The wind-up can break into the padding zone (a
   raised claw, a charging beam) to signal the attack direction.

### 6.4 Hitbox vs. visual for bosses

Boss visuals are much larger than their hitbox. A 48x48 boss with a 36x36 body
might have a `CircleHitbox` radius of 16–20px or a `RectHitbox` of 24x24. The
extra visual size sells presence without making the boss unfairly easy to hit.
Communicate the hurtbox to the player through the debug overlay during
playtesting, and tune until it feels fair.

---

## 7. Animation Workflow in Aseprite

### 7.1 Starting a new character

1. **Duplicate** the tier template (`templates/medium_32x32.aseprite`).
2. **Rename** to the character name (`player_brawler.aseprite`).
3. **Draw the idle pose** in frame 1 within the body guides. Get the silhouette
   right first — fill with a single dark color and check readability at 1x zoom
   before adding detail.
4. **Add frames** for the idle animation (3–4 more). Use Aseprite's onion
   skinning (View > Onion Skinning) to keep the feet anchored.
5. **Add a frame tag** for "idle" covering those frames.
6. **Repeat** for each animation state (walk, attack, dash, hurt, death), adding
   tagged frame ranges.

### 7.2 Using onion skinning for anchoring

Enable onion skinning to see previous and next frames as translucent overlays.
This is the primary tool for maintaining consistent anchoring:

- Check that feet do not drift between frames.
- Check that the body center of mass does not jump unexpectedly.
- Use red/blue tinting (default) to distinguish previous vs. next frames.

### 7.3 Exporting the sprite sheet

File > Export Sprite Sheet with these settings:

| Setting     | Value                                        |
| ----------- | -------------------------------------------- |
| Sheet Type  | By Rows                                      |
| Constraints | Fixed # of Columns (match your widest anim)  |
| Borders     | None (no padding between frames)             |
| Output File | `assets/sprites/<name>.png`                  |
| JSON Data   | Optional — useful for verifying frame counts |

The export arranges frame tags into rows automatically when using "By Rows" with
tags. Each tag becomes one row, frames within the tag become columns.

**Batch export command** (for scripting or a `justfile` recipe):

```bash
aseprite -b player_brawler.aseprite \
  --sheet assets/sprites/player.png \
  --sheet-type rows \
  --split-tags
```

The `-b` flag runs headless. `--split-tags` assigns each tag to its own row.

### 7.4 Verifying the export

After exporting, confirm:

- The PNG dimensions divide evenly by the frame size (e.g., 192x224 for a 32x32
  sheet is 6 columns x 7 rows).
- No colored or opaque background leaked into transparent areas.
- Frame order matches the expected row layout (idle = row 0, walk = row 1,
  etc.).
- Open the PNG at 1x zoom — if the character is unreadable, the silhouette needs
  work.

---

## 8. Registering New Art in the Engine

After exporting, register the sprite sheet in `assets/data/config.json`:

```json
{
  "id": "player",
  "path": "assets/sprites/player.png",
  "frame_w": 32,
  "frame_h": 32
}
```

Then wire up the entity's `Sprite` and `Animation` components at spawn time. See
the [Art Integration Guide](art-integration.md) for the full walkthrough.

---

## 9. Checklist: New Character

- [ ] Duplicated the correct tier template
- [ ] Body zone and proportion guides visible (HT, EY, CH, WA, FT)
- [ ] Head-to-body ratio matches target (2-head chibi for player/enemies)
- [ ] Eyes centered on the EY guide line
- [ ] Idle silhouette reads clearly at 1x zoom
- [ ] Feet (or center) anchored consistently on FT guide across all frames
- [ ] Frame tags added for every animation state
- [ ] Onion skinning used to verify frame-to-frame stability
- [ ] Exported as sprite sheet (By Rows, no border padding)
- [ ] PNG dimensions divide evenly by frame size
- [ ] Registered in `config.json` with correct `frame_w` / `frame_h`
- [ ] `Sprite` and `Animation` components wired up at spawn
- [ ] Tested in-game at 1x and 4x scale
