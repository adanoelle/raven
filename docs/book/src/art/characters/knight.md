# Knight

The knight is the first playable character. A nimble, aggressive fighter in ornate
armor — more Saber (Fate/Stay Night) than slow medieval tank.

---

## Character Overview

| Property          | Value                                      |
| ----------------- | ------------------------------------------ |
| Class             | Knight                                     |
| Playstyle         | Nimble, aggressive melee/ranged hybrid     |
| Default weapon    | Broadsword                                 |
| Sprite body       | 24x24 on 32x32 canvas                      |
| Silhouette traits | Medium build, prominent shoulder pauldrons |
| Signature color   | Gold / blonde (hair)                       |

---

## Design References

- **Final Fantasy Tactics knight** — ornate dark armor, exposed blonde hair,
  decorated pauldrons. The armor is visually heavy but the character reads as capable
  and agile.
- **Saber (Fate/Stay Night)** — the combat feel. Fast, aggressive, precise. Armored
  but never sluggish. Attack animations should convey explosive speed despite the
  plate armor.

---

## Silhouette

At 24x24, the knight's distinguishing shape features are:

- **Shoulder pauldrons:** 2-3 pixels wide on each side, pushing slightly into the
  32x32 overflow margin. These are the strongest shape differentiator from lighter
  classes (rogue, etc.).
- **Medium build:** Not as wide as the brawler, not as compact as the rogue. The
  "middle" body type that establishes the baseline.
- **Short hair:** Compact on top of the head. Does not extend far into the overflow
  margin (unlike characters with long or wild hair).
- **Broadsword:** Approximately 2px wide, 8-10px long. Slightly exaggerated width
  compared to a realistic sword so it reads as "heavy blade" rather than a generic
  line at this resolution.

---

## Color Palette (Working Draft)

Target: 6-10 colors total for the sprite.

### Hair (Signature) — Gold / Blonde

The hair is the identity anchor. At 24x24, it occupies roughly 4-6 pixels on top of
the head. It must be the single brightest, most saturated element on the upper
portion of the sprite.

| Role      | Description                                    |
| --------- | ---------------------------------------------- |
| Base      | Warm gold — the primary read color             |
| Highlight | Lighter yellow — catches light, adds volume    |
| Shadow    | Amber — defines hair shape without going muddy |

Gold was chosen because:

- It is naturally warm and high-value, popping against both dark ruins and green
  forest backgrounds.
- It pairs well with the dark warm armor tones (burgundy/bronze) without competing.
- It sits far from the cool blues, hot reds, and cyans that enemy accents will likely
  occupy.
- It is a classic knight archetype color, reinforcing the class identity.

### Skin — Warm Tones

The face is only a few pixels. Keep it simple.

| Role   | Description                             |
| ------ | --------------------------------------- |
| Base   | Warm skin tone                          |
| Shadow | Slightly darker and cooler, minimal use |

### Armor — Dark Burgundy / Bronze Family

Inspired by the FFT knight reference. The armor should be warm but desaturated so it
does not compete with the gold hair for attention.

| Role      | Description                                           |
| --------- | ----------------------------------------------------- |
| Base      | Dark burgundy or bronze — the dominant body color     |
| Highlight | Bright metallic edge — sells the "metal" read, used   |
|           | sparingly on pauldron edges and armor contours        |
| Shadow    | Deeper burgundy, nearly black — recessed armor plates |

The warm armor + warm hair creates a unified temperature for the character. Value
contrast (dark armor vs bright hair) does the separation work, not temperature
contrast.

### Undersuit / Joints

| Role | Description                                        |
| ---- | -------------------------------------------------- |
| Fill | Dark neutral (near-black, slightly warm) — visible |
|      | at joints, gaps, and where armor plates separate   |

### Outline

| Role     | Description                                           |
| -------- | ----------------------------------------------------- |
| Exterior | Black — hard shell separating sprite from background  |
| Interior | Dark versions of adjacent colors (dark amber for hair |
|          | details, dark burgundy for armor edges)               |

---

## Readability Tests

Before finalizing the palette, verify these at 1x zoom in Aseprite:

- [ ] **Squint test:** Zoom out to actual game size. The gold hair should be the
      first thing the eye finds — a bright warm dot on a dark shape.
- [ ] **Dark level test:** Place sprite against the darkest planned background (deep
      blue-grey ruins). Black outline should hold. Armor must not disappear into dark
      tiles — if it does, bump the armor base value or lean harder on the metallic
      highlight.
- [ ] **Bright level test:** Place sprite against the brightest planned background
      (daylit overgrown ruins). Gold hair must not wash out.
- [ ] **Multiplayer test:** Place all four characters side by side at actual size.
      The knight must be instantly distinguishable by color alone (even if
      silhouettes were identical).
- [ ] **Enemy adjacency test:** Place the knight next to enemy sprites. The gold
      signature must not be confused with any enemy accent color.
- [ ] **Stolen weapon test:** Render a stolen enemy weapon's projectiles in the
      knight's context. They must read as "friendly."

---

## Animation Notes

The knight's animation should sell "fast and armored" — the apparent contradiction is
the character's personality.

- **Idle:** Subtle breathing. Sword held ready, not resting. Confidence.
- **Attack:** Explosive wind-up, fast swing. The broadsword should extend fully into
  the 32x32 overflow margin during the swing arc.
- **Dash / dodge:** Quick and clean. A cloth element (cape, sash, tabard) trailing
  1-2 frames behind the body conveys speed despite the armor's visual weight.
- **Movement:** Agile stride, not a heavy march. Armor clinks but doesn't slow the
  character down.

---

## Open Questions

- Exact hex values for the palette (pending Aseprite iteration)
- Whether the knight has a secondary color element (cape, sash, emblem) or if it's
  strictly hair + armor
- Broadsword color — metallic silver/grey, or tinted to match the armor?
- How the knight visually handles stolen enemy weapons (color shift approach)
