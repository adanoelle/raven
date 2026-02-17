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

- **3-4 swatches** for simple materials (earth, wood, skin).
- **4-5 swatches** for primary materials (stone, vegetation, armor).
- Fewer is usually better — the constraint forces cohesion.

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

### Dedicated Palette Generators

For rapid experimentation before bringing colors into Aseprite:

- **Lospec Palette List** (lospec.com/palette-list) — database of 2500+ pixel art
  palettes, filterable by color count and tag. Good for studying how other artists
  structure ramps. Also has a palette analyzer that shows color distribution.
- **HSV Palette Generator** (njine.itch.io/hsv-palette-generator) — generates 5-color
  palettes with direct HSV slider control and automatic hue-shifted lighter/darker
  variants. Good for quickly testing ramp ideas.
- **KPal** (github.com/krush62/KPal) — advanced palette generator where ramps are
  highly customizable and can be linked. Best for building complete multi-ramp
  palettes with controlled hue shift curves.
- **CoMiGo's Palette Generator** (comigo.itch.io/palettes) — creates complete color
  grids and exports directly to Aseprite (.pal) format. Available as both an online
  tool and a desktop app.

### Workflow

1. **Explore** in a palette generator (KPal or HSV Palette Generator) to find ramp
   directions — experiment with hue shift angles and saturation curves.
2. **Export** promising palettes as .pal or .gpl files.
3. **Import** into Aseprite and test against actual sprites and tile mockups.
4. **Iterate** in Aseprite using Shift+scroll and the gradient tools to refine.
5. **Verify** using the palette checklist above.

---

## References

- [Slynyrd — Pixelblog 1: Color Palettes](https://www.slynyrd.com/blog/2018/1/10/pixelblog-1-color-palettes)
  — foundational tutorial on ramps, hue shifting, and HSB-based palette construction.
  Covers the "Mondo" palette structure (9 swatches per ramp, 8 ramps at 45-degree
  intervals).
- [Pixel Parmesan — Color Theory for Pixel Artists](https://pixelparmesan.com/blog/color-theory-for-pixel-artists-its-all-relative)
  — deeper theory on color relativity, gamut masking, and why rigid formulas are less
  important than observation and intentionality.
- [Lospec — Hue Shifting Tutorials](https://lospec.com/pixel-art-tutorials/tags/hueshifting)
  — community tutorials on hue shifting techniques.
