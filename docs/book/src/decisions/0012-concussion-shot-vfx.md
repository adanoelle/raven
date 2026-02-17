# 12. Concussion Shot VFX: Expanding Ring with Additive Glow

Date: 2026-02-12 Status: Accepted

## Context

The Sharpshooter's concussion shot ability (45px radius AOE, 0.1s duration, 400
units/s knockback) currently has no visual feedback. Players cannot see the area
of effect or confirm the ability fired. A visual effect is needed that:

1. Communicates the circular AOE radius clearly.
2. Feels impactful despite the short 0.1s active window.
3. Fits the pixel art style at 480x270 virtual resolution.
4. Is achievable with SDL2's `SDL_Renderer` (no custom shaders).

Three approaches were considered:

1. **Radial particle burst** — 8-12 small particles flying outward from the
   player. Visually interesting but harder to read as a precise AOE indicator at
   low resolution. Requires a particle system that doesn't yet exist.

2. **Screen shake + player flash** — Cheapest option (no new art), gives impact
   feel, but doesn't communicate the ability's radius. Players can't learn the
   range through visual feedback alone.

3. **Expanding ring + screen shake** — A sprite-sheet ring animation that
   expands from the player's center to the 45px radius, combined with a brief
   screen shake. The ring teaches the player the AOE range, the shake adds
   weight. Both are straightforward with SDL2.

## Decision

**Expanding ring sprite sheet with additive blending and minor screen shake.**

### Art pipeline (Aseprite)

- Canvas: 96x96 pixels (accommodates the 45px radius plus glow fringe).
- 4 frames, horizontal strip PNG:
  - Frame 1: small bright circle (~10px radius), full opacity core, soft alpha
    edges.
  - Frame 2: expanding hollow ring (~25px radius), alpha falloff on outer edge.
  - Frame 3: near-full ring (~40px radius), more transparent.
  - Frame 4: full radius (45px), mostly transparent — the dissipation frame.
- Colour: bright core (white/near-white) with a saturated tint on the outer
  pixels (e.g. cyan, yellow). The exact palette is an art decision left to the
  artist.

### Rendering

- Load the strip as an `SDL_Texture` with per-pixel alpha (PNG).
- Draw with `SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD)`. Additive
  blending makes bright pixels brighten the background, producing a glow effect
  without shaders.
- Optionally draw the ring twice per frame: once at normal blend mode for the
  solid core, once slightly scaled up and more transparent with additive
  blending for a softer outer glow. Two draw calls, no shader needed.
- Advance frames over the `ConcussionShot::remaining` timer (0.1s / 4 frames =
  ~25ms per frame at 120Hz, roughly 3 simulation ticks per frame).

### Screen shake

- 1-2px random offset applied to the camera for 2-3 render frames (~16-25ms).
- Triggered when the `ConcussionShot` component is first emplaced.
- Camera offset is reset to zero when the shake duration expires.

## Consequences

**Positive:**

- The expanding ring directly communicates the 45px AOE radius, helping players
  learn the ability's range through play.
- Additive blending produces a convincing glow on dark backgrounds without
  custom shaders or render-to-texture passes.
- The 4-frame sprite sheet is minimal art effort and small in memory.
- Screen shake adds impact feel with zero art assets.
- The layered draw approach (normal + additive) is optional and can be added
  later if the single-pass version looks flat.

**Negative:**

- Additive blending looks washed out against bright backgrounds. At 480x270 with
  a dark pixel art palette this is unlikely to be a problem, but light-coloured
  rooms may need the tint colour adjusted.
- The 96x96 canvas is larger than most entity sprites; the texture atlas packer
  (if introduced later) needs to accommodate it.
- Screen shake affects the entire camera. If other abilities also add shake, a
  shake-accumulation system will be needed to prevent jarring stacking.
- The 0.1s ability window is only ~12 simulation ticks at 120Hz. With 4 frames,
  each frame shows for ~3 ticks. This is fast but readable at pixel-art scale
  where players expect snappy animations.
