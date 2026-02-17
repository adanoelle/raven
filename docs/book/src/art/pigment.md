# Pigment — Palette Tool

Pigment is a standalone desktop application for generating pixel art color
palettes with hue-shifted ramps, ramp linking, and Aseprite-compatible export.

It was born from Raven's palette design work — the existing tooling landscape
has no dedicated tool that combines structured ramp generation, ramp linking,
and .gpl/.pal export in a focused UI.

**Repository:** github.com/adanoelle/pigment (private, separate from Raven)

---

## Why We're Building This

During Raven's art direction work, we evaluated every available palette tool:

| Tool | Issue |
|---|---|
| KPal | Abandoned (Dec 2023), crash bugs, Windows only |
| Lightcube | Palette features bundled inside a full editor ($15) |
| HSV Palette Generator | Too basic — single 5-color ramp, PNG only |
| CoMiGo's Palette Generator | Dormant since 2021, no .gpl export |
| FettePalette | Powerful engine, but a code library with no GUI |

No tool combines multi-ramp HSV control, hue shift curves, ramp linking, and
Aseprite export. Pigment fills that gap.

---

## How It Serves Raven

Pigment's primary use case is building Raven's palette system:

- **Material ramps:** Stone (4-5 swatches), vegetation (3-4), earth (3),
  water (3) — each with hue-shifted shadows and highlights.
- **Character ramps:** Knight hair (gold, 2-3 swatches), armor (burgundy,
  2-3), skin (2). Future characters follow the same structure.
- **Ramp linking:** Stone and vegetation share 1-2 dark shadow swatches for
  visual cohesion across materials.
- **Level variants:** Regenerate the same ramp structure with different ambient
  hue shifts for dark ruins, forest, daylight, and subway levels.
- **Background preview:** Load a Raven tileset screenshot and verify palette
  readability — does the knight's gold hair pop against the stone tiles?

The workflow: design palettes in Pigment, export .gpl, import into Aseprite,
apply to sprites and tiles.

---

## Tech Stack

| Layer | Choice | Rationale |
|---|---|---|
| App framework | Tauri v2 | Cross-platform, small binary, Rust backend |
| Frontend | React + TypeScript | Largest ecosystem, good canvas/SVG support |
| Color engine | FettePalette (MIT) | Curve-based HSV ramp generation, proven math |
| State | Zustand | Lightweight React state management |
| Styling | Tailwind + custom CSS | Layout + pixel art theme |
| Backend | Rust | File export (.gpl/.pal), project persistence |

### FettePalette

The core color engine. A zero-dependency JavaScript library that generates
hue-shifted color ramps using curves within the HSV color model.

Key parameters we expose:

| Parameter | What it controls |
|---|---|
| `centerHue` (0-360) | Base hue of the ramp |
| `hueCycle` (-1.25 to 1.5) | How much hue rotates across the ramp |
| `tintShadeHueShift` (0-1) | Cool shadows / warm highlights shift |
| `curveAccent` (-0.095 to 1) | Shape of sat/val distribution curve |
| `curveMethod` | Curve type (lame, arc, pow, easings) |
| `total` (3-35) | Number of swatches |

FettePalette runs entirely in the frontend. The Rust backend only handles
file I/O.

---

## UI Design

Aseprite-like aesthetic — grey beveled panels, Press Start 2P bitmap font,
pixel-perfect borders, no anti-aliasing. Feels like a companion app.

### Layout

```
┌──────────────────────────────────────────────────┐
│  Menu Bar                                        │
├───────────────┬──────────────────────────────────┤
│  RAMP LIST    │  RAMP EDITOR                     │
│               │                                  │
│  [+ Add Ramp] │  Swatch Strip: [■][■][■][■][■]  │
│               │                                  │
│  ▸ Stone      │  Curve Editor: (sat/val curves)  │
│  ▸ Vegetation │                                  │
│  ▸ Earth      │  HSV Sliders:                    │
│  ▸ Hair       │    Center Hue ═══●═══            │
│               │    Hue Cycle  ══●════            │
│  LINKS        │    Hue Shift  ═══●═══            │
│  Shadow: [■]  │    Curve Accent ════●            │
│  → Stone,     │    Total: [5]                    │
│    Vegetation │    Method: [lame ▾]              │
├───────────────┴──────────────────────────────────┤
│  PREVIEW: all ramps combined    [.gpl] [.pal]    │
│  BACKGROUND: (load PNG to test readability)      │
└──────────────────────────────────────────────────┘
```

---

## MVP Features

1. **Single ramp generation** — FettePalette with all parameters as sliders,
   real-time preview, swatch strip with hex/HSV on click
2. **Multi-ramp management** — add/remove/rename ramps, ramp list with
   mini previews, click to switch
3. **Ramp linking** — share dark-end swatches across ramps, visual
   indicators for link groups
4. **Background preview** — load PNG, overlay swatches, test readability
5. **Export** — .gpl and JASC .pal, combined or individual ramps
6. **Save/Load** — JSON project files storing all parameters and links

## Post-MVP

- Colorblind simulation
- Aseprite hot-reload (auto-reimport palette on export)
- Preset ramp templates (stone, vegetation, skin, metal)
- Gamut mask visualization
- Import palette from image
- Undo/redo, keyboard shortcuts
- Additional export formats (CSS vars, Tailwind config, Adobe .ase)

---

## Product Direction

Pigment is built for Raven first, with plans to release as a standalone
paid product on itch.io ($5-$15 range).

**Target audience:** Pixel artists using Aseprite who need structured,
hue-shifted color ramps.

**Positioning:** "The palette tool built for pixel artists."

**Market context:** ~300K-500K active pixel artists worldwide. No competing
paid tool in this specific niche. Aseprite itself (1-2M Steam owners, $20,
solo developer) proves the audience pays for focused pixel art tools.

---

## Implementation Phases

| Phase | Scope | Estimate |
|---|---|---|
| 1. Scaffold | Tauri + React + pixel theme + base components | 1-2 days |
| 2. Single ramp | FettePalette + sliders + swatch strip | 2-3 days |
| 3. Multi-ramp | Zustand store, ramp list, ramp switching | 2-3 days |
| 4. Ramp linking | Link manager, shadow merging, visual indicators | 2-3 days |
| 5. Export | Rust .gpl/.pal serializers, project save/load | 1-2 days |
| 6. Background preview | Canvas preview, PNG loading, toggle | 1-2 days |
| 7. Polish | Keyboard shortcuts, cross-platform testing, icon | 2-3 days |

**Total: ~2-3 weeks for MVP**

---

## References

- [FettePalette](https://github.com/meodai/fettepalette) — color engine (MIT)
- [FettePalette demo](https://meodai.github.io/fettepalette/) — live parameter playground
- [Slynyrd — Color Palettes](https://www.slynyrd.com/blog/2018/1/10/pixelblog-1-color-palettes) — ramp theory
- [Pixel Parmesan — Color Theory](https://pixelparmesan.com/blog/color-theory-for-pixel-artists-its-all-relative) — gamut masking, relativity
- [Lospec](https://lospec.com/palette-list) — palette reference library (4100+ palettes)
