# Shader Direction: Proposal, Research and Plan

Date: 2026-11-24 Tags: art, rendering, shaders, pigment, switch

## Why

Pigment's core library was split out this month, and its open issues asked
whether Raven wants recoloured sheets per environment or a palette swap at
run time. Answering that meant asking a larger question: what would Raven
look like if it used shaders at all?

## What was decided

- **Try for shaders.** The proposal is
  [ADR-0028](../decisions/0028-shader-resolved-palette-lighting.md): draw
  the world as *(ramp, step)* values and resolve it to colour in one shader
  pass through a table from Pigment. Light moves pixels along their ramps.
  Only projectiles and runes glow.
- **Do not wait for Nintendo developer access.** The fallback uses the same
  art and the same export, so a "no" on the Switch 2 costs atmosphere on
  that platform and not rework. The three rules that keep it that way are
  in the ADR.
- **The ADR stays Proposed** until a spike has run on Linux, Windows and
  the Mac.

## What the research found

From SDL's public documentation; nothing was run.

- SDL 3.4, which Raven already requires, has custom fragment shaders for
  the 2D renderer and palettised textures. Both are in the 3.4.0 headers.
- The shader path runs on SDL's `gpu` backend: Direct3D 12 or Vulkan on
  Windows 10+, Vulkan on Linux and the Steam Deck, Metal on macOS 10.14+.
  There is no OpenGL path.
- SDL3 supports the Switch and the Switch 2 through a port given to
  developers under Nintendo's NDA. SDL lists the Switch under its Vulkan
  backend. It does not say so for the Switch 2. That is the one question
  the research could not close.

## What was not checked

- Whether index values survive Raven's current drawing: the pixel-art scale
  mode, sub-pixel positions, and colour modulation all alter pixel values.
  This is the first thing the spike tests.
- How the render-state calls behave with render targets and logical
  presentation, and what they do on a renderer that is not `gpu`.
- Whether the ImGui overlay draws correctly on the `gpu` renderer.
- The book build, after these pages were added.

## The plan

In order. Steps 1 and 2 can start today.

1. Apply for Nintendo developer access.
2. Build the spike from ADR-0028's First step, on a throwaway branch, on
   placeholder art, with a hand-written table. It needs nothing from
   Pigment.
3. Run it on Linux, Windows and the Mac.
4. Decide ADR-0028: accept option C, or fall back to option B.

Alongside, none of it waiting on the spike:

- Pigment's MCP server (in progress).
- An inspiration board, organised by role (enemies, playable characters,
  backgrounds) and by the four environments.
- Two answers: must light change within a room, or only per room? What is
  the oldest desktop hardware Raven should run on?

After the decision:

5. Settle the table format in Pigment (pigment #38) and build the export.
6. Build the palette: one Pigment project, three groups, four lighting
   variants, headroom at both ends of each ramp, and a new colour budget to
   replace 16–32.
7. Draw one slice in Aseprite and light it: one room's tiles, the Knight,
   one enemy, bullets, in two environments. Revise the drawing rules from
   what it shows, then widen.

## What changes for drawing, if option C is accepted

Not yet rules. They become rules, in the art pages, after the slice.

- Work in Indexed colour mode; every pixel is an exact palette swatch.
- Draw the material in a reference environment, not the mood.
- Shade form by hand as now; leave the top and bottom ramp steps mostly
  unused so light has room to move.
- Do not draw light the shader will draw: the lamp, not its pool.
- Glowing parts go on their own layer and are exported separately.
- No translucent pixels in the world; shadows are dithered or separate.

Two choices to make before drawing much: how a swatch shared between two
ramps says which ramp it belongs to, and whether outlines are a fixed index
or drawn by the shader.

## Pages touched

- ADR-0028: risks, platform support, the wider spike, and the decision to
  go ahead before developer access.
- [Production Plan](../production-plan.md): one row under Decisions Needed
  Now.
- [Switch Port Plan](../guide/switch-port-plan.md): the Switch 2 and the
  `gpu` backend question, in step 3.
- [Rendering Pipeline](../architecture/rendering.md),
  [Art Specification](../art/art-spec.md) and
  [Tileset Palette](../art/tileset-palette.md): a note pointing at the
  proposal. Their rules are unchanged until the ADR is accepted.
