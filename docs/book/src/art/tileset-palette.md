# Tileset Palette Design

Design notes for Raven's tileset color palettes. The world is the ruins of an ancient
runic civilization, now overgrown and repurposed as shelter by people hiding from a
new alien threat.

**Aesthetic targets:** Studio Ghibli backgrounds, Zelda overworld, "beautiful
aftermath" — not bleak post-apocalypse, but a world that has moved on. There is
sadness in the ruins but also beauty in how nature has embraced them.

---

## Camera and Lighting Context

The game uses a **top-down view with a slight angle** to show depth. This means:

- The primary light source comes from above and slightly toward the viewer.
- **Horizontal surfaces** (ground, tops of walls, tree canopy crowns) catch the
  warmest highlights.
- **Vertical faces** (wall sides, cliff edges, pillar fronts) sit in the cooler
  mid-to-shadow range.
- Very little sky is visible — possibly a sliver at screen edges on outdoor levels,
  or none at all. Sky color matters more as an **ambient light tint** that influences
  shadow colors than as a directly visible element.

---

## Color Ramps

A color ramp is a sequence of colors arranged by brightness, used to shade a single
material. Every material in the tileset (stone, vegetation, earth) gets its own ramp.

### Hue Shifting

A naive ramp just darkens and lightens the same hue ("straight ramp"). This looks
flat and lifeless. A good ramp **shifts the hue** as it moves from dark to light:

- **Shadows shift cooler** — toward blue/purple. Desaturate slightly.
- **Highlights shift warmer** — toward yellow/amber.
- A common starting point is **~20 degrees of hue shift per step**.

This mimics how real light behaves: shadows pick up ambient (cool sky) light, and lit
surfaces pick up direct (warm sun) light. The reference images for this project
demonstrate this — stone shadows lean blue-grey while highlights lean warm grey-green
where light catches the surface.

### Saturation Curve

Saturation is not uniform across a ramp:

- **Peaks in the midtones**, not at the extremes.
- Darkest and lightest swatches are less saturated.
- At the extremes, colors from different ramps converge — very dark shadows across
  stone and vegetation start to look similar, which actually helps cohesion.

### Brightness Progression

- Increases steadily from dark to light.
- Steps are larger at the dark end, smaller approaching the highlights.
- Never start at absolute 0 (pure black) unless that is intentional.

### Ramp Size

For 16x16 tiles at 480x270 resolution, ramps should be small:

- **3-4 swatches** for simple materials (earth, wood).
- **4-5 swatches** for primary materials (stone, vegetation).
- More swatches are rarely needed — the limited palette is a feature, not a
  constraint. It forces cohesion.

---

## Material Ramps

### Stone (Primary Construction Material)

The grey runic stone is the dominant material. It must feel ancient, weathered, and
slightly cool — but not dead.

| Swatch    | Role         | Hue direction     | Notes                        |
| --------- | ------------ | ----------------- | ---------------------------- |
| Shadow    | Recessed     | Blue-purple grey  | Deepest crevices, undercuts  |
| Mid-dark  | Core shadow  | Cool grey         | Vertical faces, side walls   |
| Midtone   | Base         | Neutral cool grey | The "local color" of stone   |
| Light     | Lit surface  | Warm grey-green   | Tops of walls, flat surfaces |
| Highlight | Direct light | Pale warm grey    | Edges catching sunlight      |

The shift from blue-purple in shadow to warm grey-green in light matches the
reference images — the shrine statues and ruined pillars show exactly this
transition.

### Vegetation (Overgrowth)

Nature is reclaiming the ruins. Vegetation reads as masses of value-shifted green,
not individual leaves. Vines draping over architecture read as shape.

| Swatch   | Role          | Hue direction     | Notes                      |
| -------- | ------------- | ----------------- | -------------------------- |
| Shadow   | Under canopy  | Dark desaturated  | Blue-green, almost teal    |
| Mid-dark | Dense foliage | Muted green       | The bulk of leaf masses    |
| Midtone  | Base green    | Natural mid-green | "Local color" of leaves    |
| Light    | Sunlit leaves | Warm yellow-green | Where canopy catches light |

### Earth / Ground

Paths, dirt, exposed ground between stone and vegetation.

| Swatch  | Role        | Hue direction | Notes                   |
| ------- | ----------- | ------------- | ----------------------- |
| Shadow  | Recessed    | Cool brown    | Under overhangs, cracks |
| Midtone | Base        | Warm ochre    | "Local color" of earth  |
| Light   | Lit surface | Pale warm tan | Sunlit paths            |

### Water / Damp Surfaces

Puddles in ruins, mossy wet stone, underground streams.

| Swatch  | Role       | Hue direction   | Notes                        |
| ------- | ---------- | --------------- | ---------------------------- |
| Shadow  | Deep water | Dark teal-blue  | Still pools, deep areas      |
| Midtone | Surface    | Cool blue-green | Reflective, translucent      |
| Light   | Reflection | Pale cyan       | Specular highlights on water |

---

## Runic Glow

Some stones glow with residual mystical energy from the ancient civilization. This is
the **one high-saturation element** in the environment.

### Design Rules

- Runic glow must use a **distinct hue** from enemy projectile colors to avoid false
  threat reads. If enemies use reds, magentas, and cyans, the environmental glow
  should sit elsewhere — warm amber or teal are strong candidates.
- Glow should feel like it comes from within the stone — not a light source projected
  onto the surface, but luminescence from cracks and carved runes.
- Use sparingly. If every stone glows, nothing glows. The glow should draw the eye to
  specific architectural features — doorways, carved panels, intact rune sequences.

### Interactive vs Decorative

If some runic elements are interactive (power sources, traps, lore objects), use a
**secondary glow color** to distinguish "scenery glow" from "interact with me."
Subtle pulse animation on interactive runes can reinforce this.

---

## Level Palette Variants

All material ramps shift based on the ambient light of each level type. The
hue-shifting direction stays the same (cool shadows → warm highlights), but the
overall color temperature and value range changes.

### Dark Ruins (Underground / Night)

- **Ambient light:** Cool blue-purple. Minimal direct light.
- **Stone:** Shifts cooler overall. Shadow and midtone compress together. Highlights
  are muted — only runic glow and occasional torchlight provide warm accents.
- **Vegetation:** Sparse. What exists is dark and desaturated — moss, lichen, not
  lush greenery.
- **Key contrast:** The runic glow becomes the dominant light source. Torchlight
  provides warm orange pools against the cool ambient.
- **Value range:** Compressed and dark. Characters and enemies must read against
  low-value backgrounds — their black outlines still hold, but armor and clothing
  values need to clear the background.

### Overgrown Ruins (Forest / Daytime)

- **Ambient light:** Warm green, dappled. Sunlight filtered through canopy.
- **Stone:** Warmer overall. Mossy grey-greens. The warm highlights are more
  pronounced. Stone feels like it's being absorbed by nature.
- **Vegetation:** Dominant. Lush, multi-layered greens. The vegetation ramp gets its
  full range here.
- **Key contrast:** Warm dappled light vs cool shadows under trees and inside ruin
  interiors. The transition from bright exterior to dark ruin doorway is a strong
  compositional tool.
- **Value range:** Wide. Bright sunlit areas and deep shadows coexist.

### Open Daylight (Exposed Ruins / Cliffside)

- **Ambient light:** Pale mint/cyan sky tint. Strong direct overhead light.
- **Stone:** Highest value range. Highlights are nearly white. Shadows are still cool
  but lighter than underground. The stone reads lighter and more weathered.
- **Vegetation:** Less dominant than forest. Grass, scrub, wildflowers rather than
  dense canopy.
- **Key contrast:** Strong light/shadow from overhead sun. Cast shadows are crisp and
  cool-tinted.
- **Value range:** Widest. Bright highlights, but characters still need to read —
  their black outlines and saturated signature colors become even more important
  against high-value backgrounds.

### Subway / Underground Shelter

- **Ambient light:** Warm artificial (fluorescent or makeshift). Mixed with cool
  darkness.
- **Stone:** Blends ancient runic architecture with modern infrastructure. Concrete
  greys alongside carved stone. The palette mixes the ruin aesthetic with utilitarian
  tones.
- **Vegetation:** Absent or minimal — maybe bioluminescent moss where runes meet damp
  surfaces.
- **Key contrast:** Pools of warm artificial light surrounded by deep shadow. The
  "lived-in" areas feel warm; the tunnels beyond feel cold and ancient.

---

## Gamut Masking

To keep all level palettes cohesive, use **gamut masking**: pick 2-3 key hues for
each level type, connect them on the color wheel to form a triangle or quadrilateral,
and constrain the palette to colors inside that shape.

Example gamut masks:

- **Dark ruins:** Blue-purple → cool grey → amber (narrow, cool-dominated)
- **Forest:** Yellow-green → blue-green → warm brown (green-dominated)
- **Daylight:** Cyan → yellow → warm grey (wide, bright)
- **Subway:** Orange → cool grey → blue-purple (warm/cool split)

Colors within each gamut automatically feel cohesive. Colors shared between gamuts
(the cool greys, the stone tones) provide continuity across levels.

---

## Palette Verification

- [ ] Each material ramp hue-shifts from cool shadow to warm highlight
- [ ] Saturation peaks in midtones, not at extremes
- [ ] Stone ramp reads as "ancient grey stone" across all level variants
- [ ] Vegetation ramp is clearly distinct from stone at every value level
- [ ] Runic glow hue is distinct from all enemy accent colors
- [ ] Dark ruins palette does not cause player characters to disappear
- [ ] Bright daylight palette does not wash out player signature colors
- [ ] Underground shelter palette reads as "inhabited" vs "ancient"
- [ ] Cross-level materials (stone, earth) maintain recognizable identity despite
      ambient color shifts
