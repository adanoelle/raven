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
- The Switch port plan assumes plain `SDL_Renderer`, so it needs revising
  (see Platform support below).
- Raven's build would depend on a Pigment binary, which Pigment has not
  decided to ship (pigment #35, #42).

## Platform support

Researched 2026-11-24 from SDL's public documentation. Nothing here has
been run.

| Platform           | `gpu` backend        | Requirement, as documented                         |
| ------------------ | -------------------- | -------------------------------------------------- |
| Windows            | Direct3D 12, Vulkan  | Windows 10+; D3D12 Feature Level 11_0, or Vulkan 1.0 |
| Linux, Steam Deck  | Vulkan               | Vulkan 1.0 plus a short list of common features    |
| macOS              | Metal                | macOS 10.14+; Apple Silicon or a "Mac2 family" Intel GPU |
| Nintendo Switch    | Vulkan               | Listed as supported in SDL's GPU documentation     |
| Nintendo Switch 2  | Not stated           | See below                                          |

- **There is no OpenGL path.** A machine with neither Vulkan nor Direct3D 12
  cannot run the `gpu` backend. In practice that is hardware from before
  about 2013, and some virtual machines.
- **Switch and Switch 2.** SDL's Switch page says both consoles are
  supported and that commercial games ship on the port. The port is a
  separate repository, free under the zlib licence to developers under
  Nintendo's NDA, and is obtained from its maintainer. SDL's GPU page names
  "Nintendo Switch" under the Vulkan backend. It does not name the
  Switch 2, and the Switch page says nothing about rendering. So the
  Switch 2 answer is likely yes, but it is not written down anywhere
  public. It can only be confirmed by asking the port's maintainer once
  Raven is under NDA.
- **The shader calls and the `gpu` backend.** SDL's documentation says
  `SDL_SetGPURenderState` "allows using custom shaders with the GPU
  renderer". It does not say what the call does on another backend. The
  spike should check the return value on the default renderer too.

Sources: [SDL3 GPU category](https://wiki.libsdl.org/SDL3/CategoryGPU),
[SDL3 on Nintendo Switch](https://wiki.libsdl.org/SDL3/README-switch),
[SDL_SetGPURenderState](https://wiki.libsdl.org/SDL3/SDL_SetGPURenderState).

## Risks

### Could stop the proposal

- **Platform support.** As above: no OpenGL path, and the Switch 2 is
  unconfirmed. The fallback (option B) covers a platform without the `gpu`
  backend, but then there are two render paths to maintain, and a room
  designed around moving light will not read the same on the fallback.
- **Index values corrupted before the resolve.** The resolve pass needs
  every pixel in the target to be an exact *(ramp, step)* value. Raven does
  three things today that blend or alter pixel values:
  `SDL_SCALEMODE_PIXELART` on sprite textures, sub-pixel sprite positions
  from interpolation, and texture colour and alpha modulation. Any of them
  can turn an index into a different, wrong index. The index pass will
  need nearest sampling, whole-pixel positions and no modulation, and that
  has to be shown to work.
- **A new API.** The render-state calls arrived in SDL 3.4.0 and are thinly
  documented. How they behave with render targets and logical presentation
  is not known.

### Cost time

- **Dependence on unbuilt Pigment work.** The table export, the
  index-encoded sheets and a way to run them from `just export-art` do not
  exist, and Pigment's own release is paused. Two solo projects become
  coupled.
- **Shader toolchain.** Compiling for three backends needs extra tools in
  the nix shell, on Windows and in CI. CI machines have no GPU, so anything
  that renders needs a software Vulkan driver or stays untested.
- **Scope.** Shaders make many effects possible while the game is still
  pre-content. The effects listed under Decision are candidates, and each
  is its own piece of work.

### About the look

- **Stepped light may look harsh.** With ramps of three to five steps, a
  light is a hard-edged shape one step brighter. It will probably need
  dithered edges and longer ramps, and may still disappoint.
- **Readability.** Light that changes the environment's value changes its
  contrast with bullets and characters. A dark corner or a bright pool
  could hide a projectile, which works against Readable Chaos.
- **Stricter drawing.** Indexed colour mode, headroom at both ends of each
  ramp, no translucent pixels, and final colours visible only after
  export. A real cost if another artist joins.

## Open questions

1. Does the `gpu` backend work on the Switch 2 port of SDL3? (Ask the
   port's maintainer under NDA.) What is the oldest desktop hardware Raven
   should support, given there is no OpenGL path?
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

A spike on placeholder art, with no dependence on Pigment:

1. Create the renderer on the `gpu` backend and compile one fragment
   shader for the backends in use.
2. Encode one placeholder sheet as *(ramp, step)* values by hand, with a
   small hand-written lookup table of two variants.
3. Draw it moving at sub-pixel positions into an index target, with the
   pixel-art scale mode and colour modulation that Raven uses today, and
   find out which of them must be turned off.
4. Resolve through the table in one full-screen pass, and cross-fade
   between the two variants.
5. Add one hard-coded light that shifts ramp steps.
6. Draw the ImGui overlay on top.

Run it on Linux, Windows and the Mac. It answers the three risks that
could stop the proposal (except the Switch 2, which needs the NDA port),
answers question 5, and gives a first look at stepped light. A simpler
spike that only swaps colour to colour would not exercise the index
problem. If the spike fails, options A and B are untouched.
