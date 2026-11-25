# A trial palette for the shader spike

Starter notes for whoever does this next, person or agent. Written 2026-11-24.

The spike resolves *(ramp, step)* values through a lookup table that is
written by hand in `make_lut()`. This task replaces that table with real
ramps from Pigment, to see how the idea looks with the palette tool's own
colours and to answer some of ADR-0028's questions about the look before the
ADR is decided.

**This is not Raven's palette.** It is a trial, on throwaway code, and it
commits to nothing. Raven's real palette comes after ADR-0028 is decided
(step 6 of the plan in the devlog).

## Read first

| File                                                             | Why                                                  |
| ---------------------------------------------------------------- | ---------------------------------------------------- |
| `docs/book/src/decisions/0028-shader-resolved-palette-lighting.md` | The proposal, its risks and open questions           |
| `docs/book/src/devlog/2026-11-24-shader-direction.md`              | The plan, and what would change for drawing          |
| `tools/shader_spike/README.md`                                     | What the spike does, how to run it, results so far   |
| `docs/book/src/art/tileset-palette.md`                             | What stone, vegetation, earth and the levels should look like |
| `docs/book/src/art/color-theory.md`                                | The visual layers; what must stay readable           |

`docs/book/src/art/pigment.md` is out of date on how Pigment works. Trust the
tool descriptions.

## Where to work

This worktree (`~/src/raven-shader-spike`, branch `spike/shader-palette`).
Everything under `tools/shader_spike/` is throwaway, so change it freely.
Nothing in `src/` should change. Commit only when asked; the repository's
conventions are in `CLAUDE.md`.

## What the spike expects today

From `main.cpp`:

- `RAMPS = 4`, in this order: stone, vegetation, earth, character (burgundy).
- `STEPS = 8` per ramp, "including headroom at both ends".
- `VARIANTS = 2`: 0 is open daylight, 1 is dark ruins.
- The table is an image `STEPS` wide and `RAMPS * VARIANTS` tall. The colour
  for a pixel is at column `step`, row `variant * RAMPS + ramp`.
- The placeholder art sits around **step 3** (tiles use steps 2 to 4). Steps
  0 to 1 and 5 to 7 are reached only by light and the ambient shift.
- The table is built in code. There is **no way to load one from a file**.
- An **ambient shift** exists as a shader uniform (`params.y`; the `--check`
  light case uses -2) but is not on a key in interactive mode.
- The hand-made dark ruins variant is "cooler, compressed, dimmer": it changes
  value as well as hue.

## The tools

Pigment's engine is available as an MCP server named `pigment`, registered for
every project on this machine. If its tools are not listed, start a new
session. Its reference is `~/src/pigment/book/src/reference/mcp-server.md`.

| To do this                               | Use                                |
| ---------------------------------------- | ---------------------------------- |
| Try one ramp                             | `generate_ramp`                    |
| Build a ramp around two colours you want | `fit_ramp`                         |
| Compare two colours (contrast, distance) | `color_info`                       |
| Build every ramp under every variant     | `build_palette`                    |
| Find problems, and worst-case contrast   | `check_palette`                    |
| Get a project file to save               | `project_text`                     |
| List a sprite's colours                  | `analyze_sprite`                   |

- **The server writes nothing.** It returns text; save it yourself.
- **`build_palette` returns what the table needs**: for each variant, the
  ramps in the order given, each as hex colours from dark to light.
- Colours are hex, hue is in degrees, lightness is 0 to 1 (OKLCH). Input uses
  the project file's names (`centerHue`, `hueCycle`, `total`).

A palette is described like this (placeholder values, not a proposal):

```json
{
  "ramps": [
    { "name": "Stone", "params": { "centerHue": 250, "total": 8, "maxSaturation": 0.12 } },
    { "name": "Vegetation", "params": { "material": "vegetation", "total": 8 } },
    { "name": "Earth", "params": { "material": "earth", "total": 8 } },
    { "name": "Character", "params": { "centerHue": 15, "total": 8 } }
  ],
  "variants": [
    { "name": "Open Daylight", "ambientLight": "#b8e8e0" },
    { "name": "Dark Ruins", "ambientLight": "#6a5acd" }
  ]
}
```

## The jobs, in order

### 1. Build the trial palette

Four ramps of eight steps, two variants, matching what the spike expects, so
it drops in without touching the art.

Design it for light, which is different from designing a ramp to shade a
sprite by hand:

- **The ramp is the path light travels.** A lit pixel moves up it and a
  shadowed one moves down, so the hue shift along the whole ramp is the
  colour of the light. Cool at the bottom, warm at the top.
- **Step 3 is the material's own colour**, since that is where the art sits.
  Steps 0 to 1 and 6 to 7 are headroom.
- **Even steps matter more than before.** A light is a shape one step
  brighter; an uneven ramp gives uneven rings. Read `lightness` for each
  swatch from `generate_ramp`.
- Stone is a cool grey that must stay grey. Vegetation, earth and the
  burgundy character ramp follow the art pages.

Save the project text and the `build_palette` output under
`tools/shader_spike/palette/`.

### 2. Get it into the spike

The spike needs a way to use a table it did not generate. Suggested, but
choose what is simplest:

- A small script, `tools/shader_spike/lut_from_pigment.py`, that reads the
  saved `build_palette` JSON and writes a C++ header holding the table.
- A switch, for example `--lut pigment` against the default built-in one, so
  the two can be compared and the existing results stay reproducible.
- `--check` must still pass. The CPU reference reads the same table as the
  shader, so it should, and a failure is a finding.

This script is a first sketch of the table export Pigment would need
(pigment issue 38). Note what it had to assume.

### 3. Look at it

Run `just shader-spike` and compare the Pigment table with the hand-made one:
the two variants, the cross-fade, and the light in the fully dithered falloff
(the preferred style, see the README).

The artist judges this, not the agent. Save stills with
`just shader-spike --check --out DIR` and say plainly what you cannot judge.

### 4. Ramp length

ADR-0028 worries that stepped light looks harsh on short ramps. Try the same
materials at 5, 8 and 12 steps (`STEPS`, and where the art sits, change with
it) and save stills of the light for each. This is the cheapest way to settle
how long Raven's ramps should be.

### 5. Environments: hue from Pigment, value from the shader

A Pigment lighting variant changes **only the hue of the dark half** of each
ramp (at most 30 degrees at the darkest swatch). It cannot make dark ruins
dimmer or compressed, which the hand-made variant does.

The shader's ambient shift can: it moves every pixel along its ramp. So try
dark ruins as "the Dark Ruins row, shifted down one or two steps", and open
daylight as "shifted up". Put the ambient shift on a key to try it live.

The question to answer: is variant row plus ambient shift enough to tell the
environments apart, or does Pigment need variants that change value
(pigment issue 52)?

### 6. Readability in the worst case

Under this design a surface can show any colour on its ramp, depending on the
light. `check_palette` with `contrastPairs` reports the lowest contrast
between any swatch of one ramp and any swatch of another, which is the worst
case under any light. Run it for the character ramp against each environment
ramp, in each variant.

Bullets are drawn after the resolve, outside the palette. Check a bullet
colour against every step of each environment ramp with `color_info`.

### 7. Optional: encode a real sprite

`analyze_sprite` on `assets/sprites/knight.png` (192x64, 16 colours) lists its
colours. Matching each to a *(ramp, step)* and reporting the ones that match
nothing is ADR-0028's open question 4 in miniature. The knight is drawn in
its own colours, not this trial palette, so expect most to match nothing;
the point is what the report should look like.

## Decisions for the artist

Ask; do not pick silently.

1. **Do actors change with the environment?** A Pigment variant shifts the
   shadows of every ramp, the character ramp included. ADR-0028 says actors
   keep their signature colours. Either accept the tint, or give the
   character ramp the same row in every variant (build it in a second palette
   with no variants and merge the two in the script). Try both in the spike;
   it is one row of the table.
2. **A common step count.** The table is rectangular. Ramps of different
   lengths need padding by repeating their end colours, or every ramp gets
   the same length.
3. **Groups.** The devlog plans "three groups" of ramps in one project.
   Pigment has no groups; for now a group can only be a naming convention.

## Things to know about Pigment's engine

- **Hue shift direction.** `hueCycle` turns the hue from the dark end to the
  light end. Which sign gives cool shadows and warm highlights depends on the
  centre hue. Generate the ramp and read the `hue` of each swatch.
- **The lightest swatch** of a ramp with default settings is a near-white with
  little colour. As headroom that only strong light reaches, that may be
  right; if not, lower `maxLightness`.
- **A gamut mask pushes greys into colour** unless its shapes reach the centre
  of the wheel (pigment issue 53). Stone must stay grey: start with no mask.
- **Materials are starting points.** The stone preset is warm (hue 40); the
  art pages want cool.
- **Links replace swatches**: followers take the source's darkest swatches.
  In the table two ramps can share a colour without trouble, because each
  ramp has its own row. It matters when art is encoded by matching colours:
  a shared colour does not say which ramp it belongs to. That is one of the
  two choices the devlog says to make before drawing much.

## What to hand back

- The palette files, the script, and stills for jobs 3 to 5.
- A new section in `README.md` under Results, in the style of the Linux one.
- A list of what was missing or awkward in the Pigment tools. This is their
  first real use; the list feeds pigment issues 38, 52 and 53 and the plan in
  `~/src/pigment/book/src/design/core-crate-plan.md`, step 2.4.
- Anything that bears on ADR-0028's risks "About the look", stated as what
  was seen and by whom.
