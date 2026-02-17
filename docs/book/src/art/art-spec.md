# Raven Art Specification

Reference document for sprite dimensions, animation specs, sheet layout, display
scaling, and delivery format.

---

## 1. The Canvas: Virtual Resolution

The game renders at **480x270 pixels**. This is the artist's canvas — every
sprite, tile, and UI element is authored at this resolution.

- A 24x24 character body (in a 32x32 frame) occupies ~5% of screen width —
  between Blazing Beaks and Nuclear Throne in proportion.
- A 16x16 grunt enemy occupies ~3.3% of screen width (30 tiles across).
- All art is authored at **1:1** — one pixel in Aseprite equals one game pixel.
  Never pre-scale.

---

## 2. Display Scaling

The engine scales the 480x270 canvas up to the display using
**nearest-neighbor** filtering. Every game pixel becomes a sharp block.

| Display            | Resolution | Scale | Integer? |
| ------------------ | ---------- | ----- | -------- |
| Windowed (default) | 960x540    | 2x    | Yes      |
| 1080p / Full HD    | 1920x1080  | 4x    | Yes      |
| 1440p              | 2560x1440  | 5.33x | No       |
| 4K / UHD           | 3840x2160  | 8x    | Yes      |
| Switch docked      | 1920x1080  | 4x    | Yes      |
| Switch handheld    | 1280x720   | 2.67x | No       |

At non-integer scales (1440p, Switch handheld), some pixels are one screen pixel
wider than their neighbors. This is standard for the genre and barely visible in
practice.

**Primary target: 1080p at 4x.** A 32x32 frame becomes 128x128 screen pixels;
the 24x24 character body within it renders at 96x96.

---

## 3. Sprite Dimensions

The game uses a **frame size / body size** convention. The sprite sheet frame
is larger than the drawn character, leaving transparent padding for action
frames (melee swings, weapon holdouts, dash effects). See the
[Aseprite Setup Guide](art-aseprite-guide.md) for template files and guide
placement.

| Entity            | Frame Size | Body Size | Notes                                    |
| ----------------- | ---------- | --------- | ---------------------------------------- |
| Player character  | 32x32      | 24x24     | 4px padding for weapons and dash effects |
| Standard enemies  | 16x16      | 14x14     | 1px padding, simple shapes               |
| Mid-tier enemies  | 32x32      | 24x24     | Same tier as player                      |
| Bosses            | 48x48      | 36x36     | 6px padding for dramatic attacks         |
| Mega-bosses       | 64x64      | 48x48     | 8px padding, screen-dominating           |
| Small projectiles | 8x8        | —         | Body fills frame                         |
| Large projectiles | 16x16      | —         | Special attacks                          |
| Pickups / items   | 8x8        | —         | Smaller than characters                  |
| Tiles             | 16x16      | —         | Standard tile grid                       |
| UI icons          | 8x8        | —         | Hearts, ammo pips                        |
| UI portraits      | 48x48–64x64 | —       | Stat screen, character select (see below)|
| NPC sprites       | 32x32      | 24x24     | Shop NPCs, same tier as player           |

### UI portraits (future)

Stat screens, character select, and NPC dialogue benefit from a larger,
more detailed front-facing character illustration than the gameplay sprite
can provide. These portraits are **separate assets** from the gameplay
sprite sheet — not a frame pulled from it.

| Portrait use       | Recommended size | Notes                            |
| ------------------ | ---------------- | -------------------------------- |
| Stat / skill sheet | 48x48 or 64x64  | Detailed face, hair, expression  |
| Character select   | 64x64            | Full body or bust, front-facing  |
| NPC dialogue box   | 32x32 or 48x48  | Per-NPC personality              |

At 64x64, there are enough pixels for anime-style eyes with real expression,
distinct hair rendering, and costume detail lost at gameplay scale. These
portraits are a **stretch goal** — the game is functional without them, but
they significantly improve the feel of menus and progression screens.

Portraits should share the character's **color palette** with their gameplay
sprite so the two read as the same character. Draw the portrait after
finalizing the gameplay sprite's palette and silhouette.

All frames in a single sheet must be the same size — the engine uses a uniform
grid to address frames. The body size is a guideline for idle/walk poses;
action frames may extend into the padding zone.

---

## 4. Animation Frame Counts and Timing

The game loop runs at 120Hz. Animation playback is independent — driven by
`frame_duration` (seconds per frame), not tied to the tick rate or refresh rate.

### Player (32x32 frame, 24x24 body)

| State        | Frames | Anim FPS | Duration/frame | Loop? | Cycle  |
| ------------ | ------ | -------- | -------------- | ----- | ------ |
| Idle         | 4      | 4        | 0.250s         | Yes   | 1.0s   |
| Walk         | 6      | 10       | 0.100s         | Yes   | 0.6s   |
| Run          | 6      | 12       | 0.083s         | Yes   | 0.5s   |
| Attack       | 4      | 12       | 0.083s         | No    | 0.33s  |
| Dodge / dash | 3      | 15       | 0.067s         | No    | 0.2s   |
| Hurt         | 2      | 8        | 0.125s         | No    | 0.25s  |
| Death        | 5      | 8        | 0.125s         | No    | 0.625s |

Attack and dash frames may extend into the 4px padding zone for weapon
visuals. Idle and walk frames stay within the 24x24 body zone.

Key principle: fewer frames with longer holds looks better in pixel art than
many frames played fast. Each frame should be a distinct, readable pose.

### Standard Enemies (16x16)

| State       | Frames | Anim FPS | Loop? | Notes                          |
| ----------- | ------ | -------- | ----- | ------------------------------ |
| Idle        | 2–4    | 3–4      | Yes   | Simpler than player            |
| Walk/patrol | 4      | 8        | Yes   | Fewer frames than player       |
| Attack      | 3–4    | 10       | No    | Telegraph clearly for fairness |
| Hurt        | 2      | 8        | No    | Brief flinch                   |
| Death       | 4      | 8        | No    | Satisfying pop / collapse      |

### Large Enemies / Mini-bosses (24x24)

Same states as standard enemies. The extra pixel real estate allows 1–2 more
frames per state. Attack telegraph must be very readable — use a bright wind-up
frame.

### Bosses (48x48 frame, 36x36 body — or 64x64 frame, 48x48 body)

| State      | Frames | Anim FPS | Loop? | Notes                            |
| ---------- | ------ | -------- | ----- | -------------------------------- |
| Idle       | 4–6    | 4        | Yes   | Menacing presence, personality   |
| Phase idle | 4–6    | 4        | Yes   | Distinct look per phase          |
| Attack A   | 5–6    | 12       | No    | Clear telegraph + payoff         |
| Attack B   | 5–6    | 12       | No    | Visually distinct from A         |
| Hurt       | 2–3    | 8        | No    | Brief stagger                    |
| Transition | 4–6    | 6        | No    | Phase change (dramatic)          |
| Death      | 6–8    | 6        | No    | Spectacular, slower than enemies |

Boss attack telegraphs should use the 6–8px padding zone for wind-up frames
(raised claws, charging beams). See the
[Aseprite Setup Guide](art-aseprite-guide.md) for boss-specific workflow.

### Projectiles and VFX

| Entity               | Size  | Frames | Anim FPS | Loop? | Notes                         |
| -------------------- | ----- | ------ | -------- | ----- | ----------------------------- |
| Player bullet        | 8x8   | 2–3    | 12       | Yes   | Spin or pulse while in flight |
| Enemy bullet (small) | 8x8   | 2      | 8        | Yes   | Subtle pulse                  |
| Enemy bullet (large) | 16x16 | 2–3    | 8        | Yes   | Menacing glow / pulse         |
| Hit spark            | 16x16 | 3–4    | 15       | No    | Fast, bright, satisfying      |
| Explosion            | 16x16 | 5–6    | 12       | No    | Enemy death VFX               |
| Muzzle flash         | 8x8   | 2      | 15       | No    | 1–2 frame burst at gun        |
| Pickup shimmer       | 8x8   | 4      | 6        | Yes   | Draws the eye to items        |

---

## 5. Making Animation Feel Alive

- **Idle must breathe.** Even 2–3px of vertical bobbing sells "alive."
- **Anticipation.** One wind-up frame before fast actions (attacks, dashes).
- **Follow-through.** 1–2 frames overshooting then settling after actions.
- **Squash and stretch.** ±1–2px height/width deformation reads clearly at
  24x24 body size. Use the frame padding for overshoot.
- **Overlapping action.** Hair, capes, and tails lag 1–2 frames behind the body.
- **Silhouette test.** Every character must be recognizable as a solid black
  fill at its body size (24x24 for player, 14x14 for grunts).

---

## 6. Facing Directions

- **Left and right only** for gameplay (Blazing Beaks style).
- Draw all gameplay sprites **facing right**.
- The engine mirrors sprites via `flip_x` for leftward movement — no extra art
  needed.
- No up/down/diagonal facing directions during gameplay. In a twin-stick
  shooter the aim direction is constantly sweeping, so additional facing
  directions create visible snapping at thresholds without adding clarity.

### Optional: front-facing portraits

If art resources allow, a **front-facing idle** (4 frames) per player class
is useful for non-gameplay contexts:

- Character select screen
- Pause menu portrait
- Death / game-over screen
- NPC dialogue

This is a single short animation, not a full directional set. Add it as an
extra row at the bottom of the player sprite sheet:

```
Row 0–5: Side-facing gameplay animations
Row 6:   Front idle  [4 frames] ████░░   ← UI only
```

The UI scene reads this row directly by `frame_y` index. No engine changes
or additional facing-direction logic required. This is a **stretch goal** —
the game is fully functional with side-facing sprites only.

---

## 7. Sprite Sheet Layout

- One row per animation state, frames left to right.
- Sheet width = longest animation × frame size.
- Short animations padded with transparent frames to fill the row.
- Consistent anchor point across all frames (feet at the same Y position).
- The engine addresses frames by `(column, row)` index, reading left to right,
  top to bottom.

Example player sheet (32x32 frames, 6 columns wide):

```
Row 0: Idle    [4 frames] ████░░
Row 1: Walk    [6 frames] ██████
Row 2: Attack  [4 frames] ████░░    ← weapon extends into padding
Row 3: Dodge   [3 frames] ███░░░    ← body leans into padding
Row 4: Hurt    [2 frames] ██░░░░
Row 5: Death   [5 frames] █████░
```

`█` = frame with content, `░` = transparent padding.

---

## 8. Pixel Art Rules for Nearest-Neighbor

- **Hard edges.** Fully opaque or fully transparent — no anti-aliased outlines.
- **1px line weight** consistently.
- **No gradients.** Flat color bands with 2–4 shading steps.
- **No rotation** for angled sprites — draw each angle by hand or rely on
  `flip_x`.
- Only draw rightward-facing; the engine mirrors for left.

---

## 9. Color and Palette

- **Limited palette** (16–32 colors) for visual cohesion.
- The player must be the most visually distinct entity on screen.
- Enemy projectiles need high contrast against any background.
- Background tiles use lower saturation and contrast than foreground sprites.
- Deliver a `.pal` or `.gpl` palette file alongside the art.

---

## 10. File Format and Delivery

- **PNG**, 32-bit RGBA, non-interlaced, no embedded ICC profile.
- Transparent background (alpha 0) — not a color key.
- Filenames: lowercase with underscores (`player.png`, `boss_raven.png`).
- Files go in `assets/sprites/`.
- Aseprite source files (`.ase` / `.aseprite`) with animation tags delivered
  alongside exported PNGs.
- **No padding** between frames in exported sheets.
- Communicate frame dimensions and row/column counts with each delivery.

---

## 11. Quick Reference Card

| Property             | Value                             |
| -------------------- | --------------------------------- |
| Virtual resolution   | 480x270                           |
| Primary scale target | 4x (1080p)                        |
| Player frame / body  | 32x32 frame, 24x24 body           |
| Grunt enemy size     | 16x16 frame, 14x14 body           |
| Mid enemy size       | 32x32 frame, 24x24 body           |
| Boss size            | 48x48 frame, 36x36 body           |
| Mega-boss size       | 64x64 frame, 48x48 body           |
| Projectile size      | 8x8                               |
| Tile size            | 16x16                             |
| Scaling filter       | Nearest-neighbor                  |
| Facing directions    | Left / right (flip_x)             |
| Animation timing     | `frame_duration` seconds           |
| File format          | PNG, 32-bit RGBA                  |
| Palette              | 16–32 colors + .pal file          |
| Asset directory      | `assets/sprites/`                 |
| Aseprite templates   | See [Aseprite Setup Guide](art-aseprite-guide.md) |

---

## 12. Delivery Checklist

Run through this list before every handoff:

- [ ] Frame size matches the spec table (32x32, 16x16, 48x48, etc.)
- [ ] Idle/walk art stays within the body zone (see Aseprite Setup Guide)
- [ ] Action frames use padding zone appropriately — no art clipped at frame edge
- [ ] No padding between frames in the exported sheet
- [ ] All rows are the same width (short animations padded with transparent
      frames)
- [ ] Anchor point consistent — feet (or center) at the same Y across all frames
- [ ] Sprite faces right
- [ ] Hard edges only — no anti-aliasing on outlines
- [ ] Transparent background (alpha 0), not a colored background
- [ ] PNG exported as 32-bit RGBA, non-interlaced, no ICC profile
- [ ] Filename is lowercase with underscores
- [ ] Aseprite source file included with animation tags
- [ ] Frame count and dimensions noted in the delivery message
- [ ] Palette file (`.pal` or `.gpl`) included or updated
