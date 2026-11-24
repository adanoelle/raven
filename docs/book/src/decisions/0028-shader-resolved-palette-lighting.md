# 28. Shader-Resolved Palette and Lighting

Date: 2026-11-24 Status: Proposed (draft for discussion)

## Context

Raven draws with plain `SDL_Renderer`: one `SDL_RenderTexture` per sprite
into a 480x270 target, then one blit of that target to the window
([Rendering Pipeline](../architecture/rendering.md)). Custom shaders were
not available when the engine was written, and
[ADR-0012](0012-concussion-shot-vfx.md) chose its effect partly because it
needed none.

Three things have changed since:

- Raven requires SDL 3.4 ([ADR-0020](0020-bundled-dependency-fallback.md)
  pins 3.4.12). SDL 3.4 lets the 2D renderer run a custom fragment shader
  through `SDL_CreateGPURenderState` and `SDL_SetGPURenderState`, and adds
  palettised textures (`SDL_SetTexturePalette`). Both are in the 3.4.0
  headers.
- Pigment, the palette tool, now has a core library that can be used
  without the app. It already builds the colour-to-colour table for each
  lighting variant, and it knows which swatches belong to which ramp and in
  what order. Its open issues include a lookup-table export
  (pigment #38) and batch-recoloured sprite sheets (pigment #37), which
  solve the same problem in two ways.
- The art is still mostly placeholder. `art/palettes/` and `art/tiles/` are
  empty and only the Knight has source art, so a change of direction costs
  almost no rework now and a lot later.

The art documents already ask for things that are awkward without shaders:

- sky colour as "an ambient light tint" on shadows, and runic glow that
  comes "from within the stone" ([Tileset Palette](../art/tileset-palette.md));
- four environment moods from one set of sprites: Dark Ruins, Overgrown
  Ruins, Open Daylight and Underground Shelter
  ([Creative Vision](../creative-vision.md));
- "muted environments, saturated actors, glowing projectiles", the colour
  hierarchy behind the Readable Chaos pillar
  ([Color Theory](../art/color-theory.md)).

The question is whether Raven embraces shaders, and if so what art
direction they should serve.

## Options

### A. No shaders: baked sheets per environment

Pigment recolours every sprite sheet for each environment at build time
(pigment #37). The renderer does not change.

- Works on every SDL backend, the Switch included.
- Four copies of every sheet on disk and in memory.
- An environment is fixed per file. No cross-fades, no light that changes
  within a room, no per-entity swaps.

### B. No shaders: swap at load time or with palettised textures

Pigment exports a lookup table. Raven recolours textures on the CPU when a
room loads, or uses indexed textures and swaps their `SDL_Palette`.

- Works on every backend. One set of sheets on disk.
- Per-entity swaps (hit flash, tints) and palette cycling become possible.
- Still one palette per texture at a time: no light that varies across the
  screen, and a cross-fade means rebuilding palettes every frame.

### C. One shader pass that resolves the palette (proposed)

Sprites and tiles are drawn into the 480x270 target as *(ramp, step)*
values, not colours. One full-screen fragment shader then turns each pixel
into a colour by sampling a lookup table exported by Pigment.

- **Environments** are rows of the table. Changing the row changes the
  mood; blending two rows cross-fades between them.
- **Light** is a second low-resolution texture that moves a pixel up or
  down its ramp. Because the ramps are hue-shifted, shadows turn cool and
  highlights turn warm with no extra work, and every pixel stays on the
  palette. Rune glow that lights nearby stone, canopy dapple that scrolls,
  and pools of lamp warmth are all drawn into this light texture.
- **Emissive things** (bullets, runes, VFX) are drawn after the resolve, in
  ordinary colour, with additive blending. Only they may glow.

### D. Shaders per sprite and soft light at window resolution

Each sprite draws with its own shader state, and glow and gradients are
drawn at window resolution over the pixel art, in the manner of Hyper
Light Drifter.

- The most atmospheric option and the least strict: colours leave the
  palette, and the 480x270 grid is no longer the whole picture.
- A shader state change per sprite, where C needs one pass per frame.

## Decision

**Proposed: option C. The world is resolved through the palette at
480x270; soft glow is allowed only on projectiles and runes.**

The rule in one line: *the palette is the light*. The environment is never
tinted or multiplied; it moves along ramps that Pigment generated. Actors
keep their signature colours. Projectiles and runic glow are the only
things drawn outside the palette.

Things this makes possible, to be designed one at a time and not all
adopted here:

- A stolen weapon whose palette drains or dithers out as its decay timer
  runs, and a Legendary weapon that shimmers by palette cycling (pillar 1,
  Risk is the Economy).
- A palette that drifts with depth in the run (pillar 4).
- A distortion ring for the Concussion Shot in place of the sprite strip of
  ADR-0012, water ripple and heat shimmer.
- Outlines drawn by the shader in each character's signature colour.

What the renderer would need:

- Create the renderer on SDL's `gpu` backend, which is the backend the
  render-state API belongs to, and keep the present path working when that
  backend is missing (see the fallback below).
- A build step that compiles each shader for Vulkan (SPIR-V), Direct3D 12
  (DXIL) and Metal (MSL).
- A second render target, so the frame is: index pass, resolve, emissive
  pass, HUD.
- Sprite sheets exported as *(ramp, step)* data. `just export-art` would
  call Pigment for this.

What Pigment would need (to be recorded there, not here):

- pigment #38 becomes an index-to-colour table with one row per lighting
  variant, carrying ramp order, instead of a colour-to-colour table.
- pigment #37 becomes "export index-encoded sheets" instead of one
  recoloured sheet per variant.
- pigment #36 can then check the colour rules in every variant, for
  example that runic glow never shares a hue with enemy fire.

**Fallback.** Option B uses the same exported table. If the `gpu` backend
is unavailable on a platform, Raven can resolve the palette on the CPU at
room load and lose only the screen-varying light and the cross-fades. The
art and the Pigment export are the same either way, which is why C is safe
to draw for.

## Consequences

**Positive:**

- Every sprite is drawn once and serves all four environments.
- Lighting cannot produce an off-palette colour, so the "limited palette is
  a feature" rule is kept by the renderer and not by discipline.
- The colour hierarchy is structural: the environment cannot glow, because
  it is resolved before the emissive pass.
- One shader pass per frame at 480x270 is cheap, and the per-sprite draw
  path does not change.

**Negative:**

- Alpha and additive blending do not work on *(ramp, step)* values.
  Anything translucent has to be drawn in the emissive pass, or dithered.
- Artists see final colours only in Pigment's preview or in the game, not
  in the exported sheets. Aseprite still shows one variant.
- A shader toolchain enters the build, with three output formats to keep
  working in CI.
- The Switch port plan assumes plain `SDL_Renderer`. Whether the `gpu`
  backend exists in the Switch port of SDL3 is unknown. The fallback covers
  it, at the cost of the lighting.
- Raven's build would depend on a Pigment binary, which Pigment has not
  decided to ship (pigment #35, #42).

## Open questions

1. Does the `gpu` backend work on the Switch port of SDL3, and on the
   oldest desktop hardware Raven should support?
2. Must the palette change within a room (light pools, dapple), or is a
   change per room enough? If per room is enough, option B does the job
   without shaders.
3. How are *(ramp, step)* values encoded: two channels of an RGBA sheet, or
   an indexed PNG plus a table?
4. What happens to a sprite pixel that is not in the palette? Pigment's
   sprite analysis can report these; the export should fail and not guess.
5. Does the ImGui overlay's SDL renderer backend still work on the `gpu`
   renderer?

## First step

A spike that needs no new art: create the renderer on the `gpu` backend and
add one full-screen pass over the existing target that maps colour to
colour between two variants with a cross-fade. It answers questions 1 and
5, proves the shader build step, and shows the look on the placeholder
sprites. If it fails, options A and B are untouched.
