# Color Theory and Palette Design

Design notes for Raven's color palette system. The goal is instant visual readability
in a fast-paced 4-player bullet hell at 480x270 resolution.

---

## Design Principles

1. **Readability first.** Every element must be identifiable at a glance during
   chaotic combat. Color is the fastest visual signal the brain processes.
2. **Layered contrast.** Background, characters, enemies, and projectiles each occupy
   a distinct saturation/value band so they never collapse into each other.
3. **Closed palette system.** Five fixed player characters, a finite set of enemy
   types, and a known set of level palettes. Every color combination can be tested
   and verified by hand.

---

## Visual Hierarchy

Four visual layers, separated by saturation and outline treatment:

| Layer             | Saturation | Value    | Outline        | Role                |
| ----------------- | ---------- | -------- | -------------- | ------------------- |
| Tilesets          | Low        | Mid      | None           | Receding background |
| Player characters | High (sig) | Mid-high | Black exterior | Primary focus       |
| Enemies           | High (acc) | Mid-dark | Black exterior | Threat recognition  |
| Projectiles / VFX | High       | High     | None / glow    | Immediate danger    |

- **Tilesets** use muted, desaturated tones so they sit behind everything else.
  Inspired by Zelda and Studio Ghibli ruins aesthetics.
- **Player characters** have one high-saturation **signature color** (hair) on an
  otherwise muted body. The signature is the identification anchor.
- **Enemies** use reserved **accent colors** that are never shared with player
  signatures. Each enemy type has a consistent accent.
- **Projectiles** are pure color with no black outline, visually separating them from
  outlined entities (characters and enemies).

---

## Outline Conventions

Outlines reinforce the visual hierarchy and separate sprites from backgrounds.

- **Exterior outline:** Solid black (or near-black) on all player characters and
  enemies. This guarantees separation against any background palette.
- **Interior detail lines:** Dark shades of adjacent colors rather than black. For
  example, dark amber for hair detail, dark burgundy for armor edges. This keeps
  characters feeling cohesive internally while maintaining the hard exterior shell.
- **Projectiles:** No black outline. Projectiles use glow, pulse, or raw saturated
  color. The absence of outline is itself a visual signal — it tells the player "this
  is not a character, this is energy."
- **Tilesets:** No outline. Muted edges that blend into each other. The background
  should feel like a continuous surface, not a grid of outlined boxes.

---

## Color Ramps

A ramp is a sequence of colors arranged by brightness, used to shade a single
material or surface. Ramps are the building blocks of the entire palette — every
material, character, and enemy is rendered using one or more ramps.

See [Tileset Palette Design](tileset-palette.md) for detailed per-material ramps and
level-specific palette variants.

### Hue Shifting

The most important technique for pixel art ramps. A "straight ramp" that only changes
brightness looks flat and lifeless. A good ramp **shifts the hue** as it transitions
from dark to light:

- **Shadows → cooler** (shift toward blue/purple), slightly desaturated.
- **Highlights → warmer** (shift toward yellow/amber).
- Start with **~20 degrees of hue shift per step** and adjust by feel.

This mimics natural light behavior: shadows pick up cool ambient (sky) light, lit
surfaces pick up warm direct (sun) light.

### Saturation Curve

- **Peaks in midtones.** The middle swatches of a ramp carry the most color.
- Darkest and lightest swatches are less saturated.
- At extremes, different ramps converge — deep shadows across stone, foliage, and
  earth all approach similar dark values, which creates natural cohesion.

### Brightness Progression

- Increases steadily from dark to light.
- Larger steps at the dark end, smaller steps approaching highlights.
- Avoid starting at absolute 0 (pure black) unless intentional.

### Ramp Size

Ramp size is dictated by pixel real estate — how many pixels of a given material
are actually visible at the sprite's resolution. A 16x16 enemy has ~256 total
pixels; subtract outline and transparency and a single material surface might be
30-80 pixels. You cannot meaningfully distinguish more than 3-4 shades on a
surface that small.

| Sprite Size | Shades/Ramp | Total Colors/Sprite | Source                            |
| ----------- | ----------- | ------------------- | --------------------------------- |
| 8x8         | 1-2         | 2-4                 | Community consensus, saint11      |
| 16x16       | 2-3         | 4-16                | Shovel Knight (3-5), saint11 (4)  |
| 24x24       | 3-4         | 6-12                | Nuclear Throne, Blazing Beaks     |
| 32x32       | 3-4         | 8-24                | Hyper Light Drifter, saint11 (12) |

Recommended swatch counts for Raven's sprite sizes:

| Material / Use         | Swatches | Rationale                                     |
| ---------------------- | -------- | --------------------------------------------- |
| Stone tiles (16x16)    | 5        | Largest continuous surfaces in the game        |
| Vegetation, earth      | 4        | Secondary tile materials, less screen area     |
| Water                  | 3        | Small surfaces (puddles, streams)              |
| Character armor (24x24)| 4        | Largest material on a 24x24 body               |
| Character hair         | 3        | Only 4-6 pixels on top of the head             |
| Character skin         | 3        | A few pixels of face/hands                     |
| Enemy bodies (16x16)   | 3-4      | 3 for simple (Swarm), 4 for large (Juggernaut) |
| Enemy accents          | 2        | Eyes, energy cores — 2-4 pixel features        |
| Projectiles (8x8)     | 2        | Core + edge/glow, no room for more             |
| Items/pickups (8x8)   | 2-3      | Icon-scale, one dominant color + shade          |
| Runic glow             | 2-3      | Glow core + falloff + dim edge                 |

**Default in Pigment: 4 swatches.** This is the most versatile count — it covers
the majority of materials. Generate at 5 for stone, at 3 for hair/skin/accents.

Fewer is usually better — the constraint forces cohesion. As saint11 (Pedro
Medeiros, Celeste) puts it: "Do as much as I can with as little as possible."
Every new color introduced to a game creates pressure to use it everywhere.

### Gamut Masking

Pick 2-3 key hues, connect them on the color wheel to form a triangle. Constrain the
palette to colors inside that shape. Everything automatically feels cohesive.
Different levels can use different gamut masks while sharing overlapping materials
(stone, earth) for continuity.

---

## Tileset Palettes

The world is the ruins of an older civilization. Grey runic stones are the primary
construction material. Some stones glow with residual mystical energy.

See [Tileset Palette Design](tileset-palette.md) for full details on material ramps,
runic glow rules, and per-level palette variants.

---

## Player Character Palettes

### Constraints

- **5 fixed characters** (4 initial + 1 unlockable). Up to 4 on screen at once.
- Each character has a distinct **silhouette** (class-based body shape) and a
  distinct **signature color** (vibrant hair).
- Silhouette and color are redundant identification signals — either one alone is
  sufficient to identify the character.
- Signature colors must be mutually distinct across all five characters.
- Signature colors must not overlap with any enemy accent color.
- Signature colors must read against every level palette (dark, forest, daylight).

### Sprite Budget

Characters use a 24x24 body on a 32x32 canvas. The 4px margin on each side
accommodates overflow for hair, weapons, shoulder armor, and effects.

At 24x24, each character has roughly 6-10 colors:

| Element          | Colors | Purpose                                   |
| ---------------- | ------ | ----------------------------------------- |
| Hair (signature) | 2-3    | Base, highlight, shadow. Identity anchor. |
| Skin             | 2      | Base, shadow. Face is only a few pixels.  |
| Armor / clothing | 2-3    | Base, highlight, shadow. Class identity.  |
| Undersuit / fill | 1      | Dark neutral in armor gaps and joints.    |
| Exterior outline | 1      | Black. Non-negotiable.                    |

### Roster (WIP)

| Character   | Class   | Silhouette        | Signature Color | Status  |
| ----------- | ------- | ----------------- | --------------- | ------- |
| Knight      | Knight  | Medium, pauldrons | Gold / blonde   | WIP     |
| (TBD)       | Rogue   | Small, compact    | TBD             | Planned |
| (TBD)       | Brawler | Large, heavy      | TBD             | Planned |
| (TBD)       | TBD     | TBD               | TBD             | Planned |
| Runic Golem | Golem   | Blocky, angular   | Runic glow      | Planned |

---

## Enemy Palettes

Enemies are an alien robotic force — visually distinct from the ancient runic
aesthetic of the world.

### Design Language

- **Silhouette:** Sleek, alien geometry. Contrasts with the organic forms of player
  characters and the angular-but-ancient forms of the runic environment.
- **Base:** Saturated blacks and dark metallics.
- **Accent colors:** Vibrant, saturated hues for eyes, energy cores, and laser
  sources. Each enemy type has a consistent accent that identifies it.
- **Accent colors are reserved.** No player signature color may duplicate an enemy
  accent.

### Bullet Color Rules

- Enemy projectiles carry their parent enemy's accent color. A red-accented enemy
  fires red bullets. This teaches players to associate colors with threat types.
- Player projectiles use a separate color space from enemy accents.

---

## Stolen Weapon Mechanic

Players can steal weapons from enemies. When a stolen weapon fires, the projectiles
must read as "friendly" despite originating from an enemy weapon design.

### Approaches Under Consideration

1. **Color shift on pickup:** Projectiles swap to the player's signature color. Clear
   identification, but loses the visual link to the enemy weapon's origin.
2. **Outline / trail recolor:** Keep the enemy's core projectile color, add the
   player's signature as an outline or particle trail. Preserves both identities.
3. **Player aura wrapper:** Projectiles keep enemy color but gain a universal
   "friendly" modifier (white core, consistent trail) shared by all player-fired
   projectiles.

The fixed character roster makes any of these approaches viable — all color
combinations are known at design time and can be verified.

The **Runic Golem** may handle stolen weapons differently from human characters,
potentially absorbing and reinterpreting weapons through the runic visual language.
(TBD)

---

## Industry Reference Data

How comparable pixel art games handle palettes. This data was compiled from
developer interviews, GDC talks, palette breakdowns, and artist tutorials.

### Shipped Games

| Game               | Resolution | Sprite Size    | Colors/Sprite | Palette Approach              |
| ------------------ | ---------- | -------------- | ------------- | ----------------------------- |
| Shovel Knight      | 256x240    | Multi-tile     | 3-5 + transp  | NES 54 + 5 cheater = 59 master|
| Celeste            | 320x180    | 8x8 tiles      | ~12           | Curated per level             |
| Nuclear Throne     | 320x240    | 24x24          | —             | Per-sprite, muted + neon      |
| Blazing Beaks      | ~320x240   | ~24x24         | —             | Per-sprite, clean reads       |
| Hyper Light Drifter| 480x270    | 32x32          | Broad         | Per-biome + gradient overlays |
| Enter the Gungeon  | 480x270    | ~22x28         | —             | Per-sprite                    |
| Dead Cells         | 640x360    | ~50px tall     | N/A           | Grayscale + gradient map      |

**Shovel Knight** is the best-documented reference. Nick Wozniak confirmed 3-5
colors per sprite from a ~59 color master palette. An early King Knight design
with 5 colors but too much detail "read as 16-bit" — they simplified shapes for
readability, proving that fewer colors with strong silhouettes beats more colors
with fussy detail.

**Hyper Light Drifter** shares Raven's exact resolution (480x270) and uses 32x32
characters, but takes a looser approach — broad per-biome palettes with gradient
overlays for atmospheric shifts.

**Dead Cells** uses a gradient map workflow (sprites drawn in grayscale, then
colorized per biome) — a fundamentally different pipeline, but the result is
consistent palette per environment, which is the same goal.

### Educator Recommendations

**Slynyrd** (Raymond Schlitter) — the most concrete numbers available:
- 9 swatches per ramp, 20 degrees hue shift per step (his stated maximum)
- 8 ramps at 45-degree intervals to cover the full hue wheel
- Result: ~72-128 color master palette; individual sprites pull small subsets
- "A little color goes a long way with pixel art — I never use a lot of colors
  for any one image."

**saint11** (Pedro Medeiros, Celeste artist):
- Recommends 4 total colors for 16x16 sprites, 12 total colors for 32x32
- "Do as much as I can with as little as possible"
- Hue shift is essential: cold shadows, warm highlights — "not a hard rule, but
  a good default"
- On consistency: "Maintaining color count consistency is crucial in pixel art
  and is often underestimated."

**AdamCYounis** (Apollo palette, 46 colors):
- ~4-7 shades per hue group
- Start saturation at 50-60%, shift warm when brightening, cool when darkening
- "Larger jump between brighter colors and much smaller jumps between darker
  colors looked pretty nice"
- Start with more colors than needed, remove excess later

**2D Will Never Die** (Capcom-style sprite reference):
- 15 colors + 1 transparent per sprite maximum (16-bit era standard)
- Key technique: **share colors between materials** — skin highlight doubles as
  hair base, shirt highlight matches jeans lightest blue
- "Stealing palettes" from existing games is recommended for learning contrast
  and shading relationships

### Color Sharing

Multiple sources emphasize sharing swatches across materials to reduce total
palette size while maintaining cohesion:

- Skin highlight can double as a light armor accent
- Deep shadows across stone and vegetation converge to similar dark values
- The undersuit/joint fill color can be shared across all characters
- Enemy dark metallics can share shadow swatches with environment stone

This is exactly what Pigment's ramp linking feature enables — link the dark ends
of material ramps so they share shadow swatches, reducing total count while
creating natural visual cohesion at the extremes.

### Master Palette Budget

Adding up all material and entity ramps:

| Category              | Ramps | Avg Swatches | Subtotal |
| --------------------- | ----- | ------------ | -------- |
| Tileset materials     | 4-5   | 4            | ~18      |
| Character (per class) | 4     | 3            | ~11      |
| Enemy accents         | 6     | 2            | ~12      |
| Projectiles           | 3-4   | 2            | ~7       |

After ramp linking and color sharing, the master palette should land in the
**32-48 color range** — consistent with the art spec's 16-32 target (which was
conservative) and well within the norms for games at this resolution.

---

## Palette Verification

Because the palette system is closed (5 characters, N enemies, L levels), every
combination can be checked:

- [ ] Each character signature reads against every level palette
- [ ] Each character signature is distinct from every other character
- [ ] Each character signature is distinct from every enemy accent
- [ ] Enemy accents are distinct from environmental runic glow
- [ ] Stolen weapon projectiles are distinguishable from enemy projectiles
- [ ] All 4-player combinations maintain readability at 480x270

---

## Tools for Palette Experimentation

### Aseprite (Primary — Already in Use)

Aseprite is the primary sprite editor and has built-in palette tools:

- **Sort & Gradients → Gradient by Hue** generates smooth hue-shifted ramps between
  two selected colors.
- **Shift+scroll** over the palette adjusts the hue of the selected color without
  changing brightness or saturation — useful for quick hue exploration.
- Community extensions for deeper ramp control:
  - **Rampart** — generates ramps between two colors directly in the palette panel
    (sudo-whoami.itch.io/rampart).
  - **Super Color Picker** — generates harmonized ramps from a selected color
    (fbepyon.itch.io/super-color-picker-for-aseprite).
  - **Color Ramp Sort** — sorts existing palettes into ramp order
    (github.com/matsagad/color-ramp-sort).

### Pigment (Primary — Built for This Project)

[Pigment](pigment.md) is our dedicated palette tool, built specifically for
Raven's palette workflow. It wraps the FettePalette engine in a focused GUI with
multi-ramp management, ramp linking, and .gpl/.pal export. See the
[Pigment page](pigment.md) for full details.

### Supplementary Tools

- **Lospec Palette List** (lospec.com/palette-list) — database of 4100+ pixel art
  palettes, filterable by color count and tag. Essential for studying how other
  artists structure ramps. Downloads in .gpl and .pal formats.
- **Lightcube** (lightcube.art, Steam) — pixel art editor with graph-based
  hue/saturation/brightness curve editors. Useful as a second opinion on ramp
  shapes. Paid app, Windows only.

### Workflow

1. **Study** existing palettes on Lospec for reference and inspiration.
2. **Build ramps** in Pigment — set up material ramps (stone, vegetation, earth,
   water), character ramps (hair, armor, skin), and enemy accent ramps.
3. **Link ramps** in Pigment — share dark-end swatches across materials for cohesion.
4. **Export** as .gpl and import into Aseprite.
5. **Test** against actual sprites and tile mockups in Aseprite. Use Pigment's
   background preview to verify readability against level screenshots.
6. **Iterate** in Aseprite using Shift+scroll and Rampart for fine adjustments.
7. **Verify** using the palette checklist below.

---

## References

### Artist Tutorials
- [Slynyrd — Pixelblog 1: Color Palettes](https://www.slynyrd.com/blog/2018/1/10/pixelblog-1-color-palettes)
  — foundational tutorial on ramps, hue shifting, and HSB-based palette construction.
  Covers the "Mondo" palette structure (9 swatches per ramp, 8 ramps at 45-degree
  intervals).
- [saint11 — Color Theory (Pixel Grimoire #6)](https://saint11.art/pixel_art_articles/article6/)
  — hue shifting essentials, cold shadows / warm highlights as default.
- [saint11 — Consistency](https://saint11.art/blog/consistency/) — why maintaining
  color count consistency matters more than strict palette limits.
- [AdamCYounis — Palette Design (summary)](https://stevelilleyschool.blogspot.com/2021/02/designing-pixel-art-color-palettes-in.html)
  — saturation starting points, hue shifting workflow, color convergence at extremes.
- [2D Will Never Die — Picking the Best Colors](https://2dwillneverdie.com/tutorial/picking-the-best-colors-for-your-sprite/)
  — 16-bit sprite palette technique, color sharing across materials.
- [Pixel Parmesan — Color Theory for Pixel Artists](https://pixelparmesan.com/blog/color-theory-for-pixel-artists-its-all-relative)
  — deeper theory on color relativity, gamut masking, and observation over formulas.
- [Lospec — Hue Shifting Tutorials](https://lospec.com/pixel-art-tutorials/tags/hueshifting)
  — community tutorials on hue shifting techniques.

### Game Art Breakdowns
- [Breaking the NES for Shovel Knight (Game Developer)](https://www.gamedeveloper.com/design/breaking-the-nes-for-shovel-knight)
  — Nick Wozniak on the 54+5 color palette, 3-5 colors per sprite, and readability.
- [Celeste Pixel Art (Extremely OK Games)](https://exok.com/posts/2019-12-10-celeste-pixel-art/)
  — Pedro Medeiros on Celeste's art direction and consistency.
- [Celeste Tilesets Step-by-Step (Aran P. Ink)](https://aran.ink/posts/celeste-tilesets)
  — tileset construction at 320x180.
- [Dead Cells Art Design Deep Dive (Game Developer)](https://www.gamedeveloper.com/production/art-design-deep-dive-giving-back-colors-to-cryptic-worlds-in-i-dead-cells-i-)
  — gradient map workflow, saturated palette as art direction pillar.
- [Hyper Light Drifter Art Direction Analysis](http://idrawwearinghats.blogspot.com/2014/04/art-direction-analysis-of-hyper-light.html)
  — split complementary color scheme, flat color + gradient technique.
