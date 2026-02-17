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

## 3. Anchoring

Anchoring determines where the character "sits" within the frame across all
animation poses. If the anchor drifts between frames, the character appears to
float or jitter during playback.

### 3.1 Bottom-center anchor (characters and ground enemies)

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

### 3.2 Center anchor (flying enemies and hovering bosses)

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

### 3.3 Choosing the right anchor

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

## 4. Bosses: Design Considerations

Bosses are the most complex sprites in the game. They occupy **48x48 or 64x64
frames** on a 480x270 canvas, making them visually dominant.

### 4.1 Screen presence

| Frame Size | % Screen Width | % Screen Height | Feel                   |
| ---------- | -------------- | --------------- | ---------------------- |
| 48x48      | 10.0%          | 17.8%           | Threatening, imposing  |
| 64x64      | 13.3%          | 23.7%           | Dominating, final-boss |

For reference, the 32x32 player at 24x24 body occupies ~5% width / ~8.9% height.
A 48x48 boss is roughly **twice the player's visual size** in each dimension —
large enough to read as a clear threat without overwhelming the play space.

### 4.2 Boss sprite sheet layout

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

### 4.3 Aseprite setup for bosses

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

### 4.4 Hitbox vs. visual for bosses

Boss visuals are much larger than their hitbox. A 48x48 boss with a 36x36 body
might have a `CircleHitbox` radius of 16–20px or a `RectHitbox` of 24x24. The
extra visual size sells presence without making the boss unfairly easy to hit.
Communicate the hurtbox to the player through the debug overlay during
playtesting, and tune until it feels fair.

---

## 5. Animation Workflow in Aseprite

### 5.1 Starting a new character

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

### 5.2 Using onion skinning for anchoring

Enable onion skinning to see previous and next frames as translucent overlays.
This is the primary tool for maintaining consistent anchoring:

- Check that feet do not drift between frames.
- Check that the body center of mass does not jump unexpectedly.
- Use red/blue tinting (default) to distinguish previous vs. next frames.

### 5.3 Exporting the sprite sheet

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

### 5.4 Verifying the export

After exporting, confirm:

- The PNG dimensions divide evenly by the frame size (e.g., 192x224 for a 32x32
  sheet is 6 columns x 7 rows).
- No colored or opaque background leaked into transparent areas.
- Frame order matches the expected row layout (idle = row 0, walk = row 1,
  etc.).
- Open the PNG at 1x zoom — if the character is unreadable, the silhouette needs
  work.

---

## 6. Registering New Art in the Engine

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

## 7. Checklist: New Character

- [ ] Duplicated the correct tier template
- [ ] Guides visible at body zone boundaries
- [ ] Idle silhouette reads clearly at 1x zoom
- [ ] Feet (or center) anchored consistently across all frames
- [ ] Frame tags added for every animation state
- [ ] Onion skinning used to verify frame-to-frame stability
- [ ] Exported as sprite sheet (By Rows, no border padding)
- [ ] PNG dimensions divide evenly by frame size
- [ ] Registered in `config.json` with correct `frame_w` / `frame_h`
- [ ] `Sprite` and `Animation` components wired up at spawn
- [ ] Tested in-game at 1x and 4x scale
