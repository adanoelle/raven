# ADR-0028 shader spike

Throwaway code for
[ADR-0028](../../docs/book/src/decisions/0028-shader-resolved-palette-lighting.md).
It is not part of the game and is off by default in the build. Nothing in
`src/` depends on it.

Resuming this work? Read [HANDOFF.md](HANDOFF.md) first: decisions, issues,
open questions and next steps.

It draws a hand-encoded scene as *(ramp, step)* values into a 480x270 index
target, resolves it to colour in one fragment shader pass through a small
lookup table, and checks the result against the same arithmetic on the CPU.

## Build and run

```sh
just shader-spike --check          # automated report, hidden window
just shader-spike --check --out /tmp/spike --show
just shader-spike                  # interactive
just shader-spike --driver vulkan  # force a GPU driver (vulkan, direct3d12, metal)
just shader-spike --lut pigment    # a table from Pigment; works with --check too
```

`--lut` takes `builtin` (the default, from `make_lut()`), `pigment`,
`pigment-fixed`, `pigment-5` or `pigment-12`. See
[Trial palette from Pigment](#trial-palette-from-pigment-2026-11-24).

Without `just`:

```sh
cmake -B build-spike -G Ninja -DRAVEN_BUILD_SHADER_SPIKE=ON -DRAVEN_ENABLE_TESTS=OFF
cmake --build build-spike --target raven_shader_spike
./build-spike/bin/raven_shader_spike --check
```

`--check` exits 0 when every `[PASS]` line passed. `[OK  ]` and `[BAD ]`
lines are findings, not failures: several are expected to be bad. With
`--out DIR` it writes numbered PNGs of each case.

Five of them from the Linux run are in `results/linux/`.

Interactive keys: `1` `2` pick a variant, `C` auto cross-fade, `L` light,
mouse moves the light, the wheel changes its radius, `[` `]` change its
number of zones, `D` cycles the zone edges through a fully dithered falloff,
flat rings and a narrow dithered seam, `-` `=` set the seam width,
`S` whole-pixel snapping, `P` pixel-art scale mode,
`M` colour modulation, `F` the shader-free CPU path, `E` emissives,
`,` `.` move the ambient shift down and up a step, `Esc` quits. `L`
switches only the light; the ambient shift stays where the keys put it. The
top line shows how many index values are invalid.

## The light

The light raises each pixel along its ramp by up to `zones` steps at the
centre, falling off as (1 - (d/r)²)². The level is worked out in whole
numbers, in sixteenths of a step. The whole part is the step shift. The
remainder decides the edge between two zones, in one of three ways:

| Dither band | Look | Still |
| ----------- | ---- | ----- |
| 0 | Flat rings with hard edges | `4b-light-banded.png` |
| 1 to 7 | Flat zones with a dithered seam where they meet | `4c-light-narrow-band.png` |
| 8 | Every zone fades into the next; grain across the whole light. The default | `4-light.png` |

The band is measured in light level, not in pixels, so a seam is wider
where the falloff is shallow (near the centre and the rim) and narrower
where it is steep. No in-between colour is produced in any mode.

**Preference, 2026-11-24:** after comparing the three live, the fully
dithered falloff (band 8) is the one to carry forward. Its cost is grain
across the lit area, which shows most on flat, plain surfaces, so textured
tiles suit it better than large flat fills.

Because it is all whole-number arithmetic, the shader and the CPU reference
agree on every pixel.

## Shaders

`shaders/resolve.frag.hlsl` is the source. `shaders/build.sh` compiles it to
SPIR-V, MSL and DXIL with SDL_shadercross and writes the three `.h` files,
which are committed so that building the spike needs no shader compiler:

```sh
nix shell nixpkgs#sdl3-shadercross -c tools/shader_spike/shaders/build.sh
```

## Results

### Linux, 2026-11-24 (SDL 3.4.0 from nixpkgs, GPU driver `vulkan`)

All must-pass checks passed.

| Question | Result |
| -------- | ------ |
| Does the `gpu` renderer take a custom fragment shader? | Yes, from SPIR-V, with the lookup table bound as a second sampler |
| Does the resolve match the CPU reference? | Exactly, 0 of 129,600 pixels differ, in all seven cases (two variants, dithered cross-fade, a four-zone light in three edge styles, cross-fade with light) |
| Does lighting stay on the palette? | Yes, 0 pixels off the table with cross-fade and light together |
| Do render targets and logical presentation work with it? | Yes |
| Does ordinary drawing work after the state is cleared? | Yes, additive emissives draw on top |
| Does ImGui (`imgui_impl_sdlrenderer3`) draw on the `gpu` renderer? | Yes |
| What happens on a renderer that is not `gpu`? | `SDL_CreateGPURenderState` returns null: "Renderer isn't a GPU renderer" (tried on `opengl`) |

Index integrity, counted as pixels that differ from the whole-pixel
reference:

| How sprites are drawn | Differ |
| --------------------- | -----: |
| Nearest, whole pixels, no modulation | 0 |
| Nearest, sub-pixel positions | 0 |
| Pixel-art scale mode, whole pixels | 0 |
| Pixel-art scale mode, sub-pixel positions | 1,236 |
| Linear scale mode, sub-pixel positions | 1,281 |
| Colour modulation | 1,736 |
| Alpha modulation | 1,802 |

What this says for the index pass:

- **No colour or alpha modulation.** Both rewrite the index.
- **Pixel-art and linear scale modes are only safe on whole pixels at 1:1.**
  Nearest is safe at any position, so the index pass should use nearest.
- **A blended index is usually still a valid index**, just the wrong one:
  every corrupted pixel above was in range. Counting out-of-range values
  finds nothing, so a check has to compare against a reference.

Other things learned:

- In SDL 3.4.0 a render state's extra samplers are fixed when the state is
  created. The table and a light mask can still change, by updating the
  same textures.
- Two resolves queued with different uniforms, with no read-back between,
  each kept their own values.
- ImGui is drawn in logical coordinates, so it is scaled with the 480x270
  presentation. That is the same as on the default renderer.

### Trial palette from Pigment, 2026-11-24

The task is [trial-palette.md](trial-palette.md). Built with the Pigment MCP
server (`pigment-mcp 0.1.0`, project file version 2). This is a trial and
not Raven's palette.

**What is here**

| File | What |
| ---- | ---- |
| `palette/trial.pigment` | The project: four ramps of eight steps, two variants |
| `palette/trial-build.json` | `build_palette` output for it |
| `palette/trial-build-no-light.json` | The same ramps with no ambient light, for rows that should not change with the environment |
| `palette/trial-build-5.json`, `-12.json` | The same ramps at 5 and 12 steps |
| `palette/tables.json` | Which builds become tables, and what the builds do not say (see below) |
| `lut_from_pigment.py` | Reads the above, writes `pigment_lut.h` (committed) |
| `palette/knight-match.txt` | The knight's colours matched against the palette |
| `results/trial-palette/` | Stills, one folder per table |

After changing anything in `palette/`:

```sh
python3 tools/shader_spike/lut_from_pigment.py
```

**The ramps.** All four run from lightness 0.15 to 0.90 (OKLCH) in even
steps of about 0.107, so one step of light is the same size on every
material. The art sits on step 3.

| Ramp | Centre hue, hue travel | Saturation (min to max) | Step 0, step 3, step 7 with no light |
| ---- | ---------------------- | ----------------------- | ------------------------------------ |
| Stone | 215, -0.45 | 0.03 to 0.08 | `#0c0816` `#4b5f68` `#dbe0d8` |
| Vegetation | 142, -0.22 | 0.12 to 0.20 | `#00130d` `#3e6643` `#e3e0be` |
| Earth | 62, +0.15 | 0.12 to 0.20 | `#220000` `#7a5130` `#eaddbd` |
| Character | 8, +0.12 | 0.15 to 0.35 | `#290015` `#923752` `#ffd1c9` |

Variants: Open Daylight with ambient light `#b8e8e0`, Dark Ruins with
`#6a5acd`. No gamut mask, no links.

Choices made while tuning, for the artist to overrule:

- Saturation is much lower than Pigment's defaults and material presets. At
  the defaults the three darkest swatches of vegetation, earth and burgundy
  fall outside sRGB and are clipped, which bends their hue and makes the
  dark steps uneven (burgundy at 0.75: lightness 0.24, 0.34, 0.43, 0.51).
  At the values above only the darkest one or two swatches clip.
- `minSaturation` is raised so the top step keeps some colour. At the
  default 0.05 the top step is near-white on every ramp and chroma drops by
  a factor of five in one step, which the centre of a strong light would
  show.
- One lightness range for every ramp. It gives even light, and it also
  means every ramp has a swatch at every value another ramp has. See
  Readability below.

**Checks.** `--check` passes with every table: the shader matches the CPU
reference on all 129,600 pixels in every case, and 0 pixels are off the palette. With `--lut builtin` the
stills are byte-identical to the ones in `results/linux/`.

**Seen in the stills.** By the agent, from the PNGs. The artist has not yet
judged any of this, and whether it looks good is not something the agent
can say.

| Question | What the stills show |
| -------- | -------------------- |
| Does the light carry colour? | Yes. Under the light, stone goes from blue-purple grey to a pale warm grey and earth from dark red-brown to tan. The built-in table shifts hue less along the way. (`pigment/4-light.png` against `linux/4-light.png`) |
| Do the two variants look different on their own? | Barely. `pigment/1-daylight.png` and `pigment/2-dark-ruins.png` are hard to tell apart; the built-in pair differs plainly. |
| Does the variant row plus an ambient shift separate them? | Dark Ruins at -2 (`pigment/12-ruins-down2.png`) is plainly a dark room next to daylight at 0 or +1. At -1 it is between the two. Whether that is enough is the artist's call (pigment issue 52). |
| Ramp length | The same light at 5, 8 and 12 steps (`pigment-5/`, `pigment/`, `pigment-12/`, files `4-light.png` and `4b-light-banded.png`). The dither grain is coarsest at 5 and finest at 12, and the flat rings are widest at 5. At 5 steps the placeholder art's own shading already uses steps 1 to 4 of 0 to 4, so there is almost no headroom. |

The art's own shading stays one step either side of the base at every ramp
length, so at 12 steps the tiles have less contrast than at 8. Real art
would use more steps of a longer ramp; the stills do not show that.

**Environments (job 5).** A Pigment variant changes hue only, and only on
the dark half of a ramp. With eight steps that is steps 0 to 3, so the top
four swatches of every ramp are identical in the two variants. Value has to
come from the shader's ambient shift. The stills above are the evidence;
the live view has the shift on `,` and `.`.

**Actors and the environment (decision 1, open).** Two tables differ only
in the Character rows:

- `pigment`: the Character ramp takes each variant's light like any other.
- `pigment-fixed`: the Character rows come from the no-light build and are
  the same in both variants.

In `pigment`, Open Daylight breaks the Character ramp between steps 2 and
3. The ambient hue (184) is almost opposite burgundy (355 to 5), and the
engine turns each swatch towards the light by the shorter way round: steps
0 to 2 turn 30 degrees towards purple (`#601d5f`), step 3 turns 26 degrees
towards orange (`#953a2d`), and step 4 is untouched (`#b55868`). Step 3 is
where the art sits. Steps 0 to 2 come out the same purple in both variants.

**Readability (job 6).**

- `check_palette` reports a lowest contrast of 1.00 to 1.03 between the
  Character ramp and each of Stone, Vegetation and Earth, in both variants.
  That is true of any two ramps that both run from near-black to
  near-white, so the number says little by itself.
- The narrower question, worked out with a few lines of Python from the
  saved build (WCAG contrast, agreeing with `color_info` to four places on
  the pairs tried): with actor and ground under the same light, the lowest
  contrast between the sprite's steps (2 to 5) and a tile's steps (2 to 4)
  is 1.00 to 1.10 at every shift from -3 to +4. This follows from giving
  every ramp the same lightness steps. Here an actor is told from the
  ground by hue, chroma and its unlit outline, not by value.
- A bullet core of `#ffd278` against each step, dark to light, on stone:
  13.8, 11.0, 7.4, 4.7, 3.0, 1.9, 1.3, 1.1. Vegetation and earth are
  within 0.6 of that, and Dark Ruins within 0.2. Drawn additively, as the
  spike does, the last four become 4.2, 2.8, 1.9, 1.3. So a bullet falls
  below 3:1 on any surface lit to step 5 or above, which is two steps of
  light on the base art. This is ADR-0028's readability risk in numbers.

**Encoding a real sprite (job 7).** `palette/knight-match.txt`: 15 of the
knight's 16 colours match nothing in the palette, covering 2,330 of 2,350
opaque pixels. Expected, since the knight is not drawn in this palette. One
of them is pure black for the outline, which no ramp holds: the outline
needs a fixed index or a ramp of its own. The match was made outside
Pigment; `analyze_sprite` lists colours but does not compare them with a
palette.

**What the script had to assume** (for pigment issue 38). All of it is in
`palette/tables.json` or the script's header comment:

- Ramp order and variant order. The spike addresses both by number;
  `build_palette` returns them in the order given. The script checks names
  and fails on a mismatch.
- Which step the art sits on. A ramp does not mark its local colour.
- How many steps the light and the ambient shift move, which depend on the
  ramp length.
- That every ramp has the same length. It fails on ragged ramps and does
  not pad.
- Rows that do not change with the variant need a second build with no
  light, merged by ramp name.
- Colours are taken as sRGB bytes; the JSON does not say.
- The server writes nothing, so each build was saved by hand from the tool
  result. That is the step an export command would replace.

**Pigment tools: missing or awkward.** First real use.

1. A variant cannot change value (issue 52). It also cannot be told to
   leave a ramp alone, which the actor question needs.
2. The ambient hue shift takes the shorter way round swatch by swatch, so
   a ramp whose hue is opposite the light is split in two (above). It
   also does not fade out towards the middle: on the Character ramp step
   2 moves 30 degrees, step 3 moves 26 and step 4 does not move. The tool
   text says "up to 30 degrees at the darkest swatch".
3. "The dark half" depends on the ramp length: steps 0 to 3 of 8, 0 to 5 of
   12, 0 to 1 of 5. For this design it would be better tied to the base
   step.
4. `centerHue` is described as the hue of the middle swatch. An even ramp
   has no middle swatch, and the art's base step (3 of 8) is not the
   middle either. There is no way to say "this step is this colour";
   `fit_ramp` fixes only the two ends.
5. Nothing reports that a swatch was clipped to sRGB. It shows only as a
   zero in the hex and as uneven lightness.
6. No summary of step evenness. Each swatch's lightness has to be read and
   subtracted by hand.
7. The saturation controls are hard to predict: chroma stays near its peak
   across the dark half and falls only at the top, and `minSaturation`
   changes the top step far more than the bottom one.
8. `check_palette` lists 44 value collisions for this palette, capped at
   25 shown. They are intended here. A way to say so, or a check of
   "contrast at the same light" between two ramps, would fit this design
   better than the lowest contrast over all swatches.
9. No call compares one colour with every swatch of a ramp (bullets), and
   none matches a sprite's colours to a palette (job 7). Both were done in
   Python.
10. `project_text` writes the first variant's light into every ramp's
    `params.ambientLight` and into the project's own `ambientLight`. It
    reads back correctly, but the file says the same thing in three places.
11. Material presets are far from the art pages: stone is warm (hue 40),
    vegetation peaks at chroma 0.22 (`#3bb800`).
12. No groups (decision 3). Not needed for four ramps.

### Windows

Not run yet. Run `--check` once as is and once with `--driver vulkan`.

### macOS

Not run yet. This is the only place the MSL shader is exercised.

## Not covered

- The Switch 2. That needs the NDA port of SDL3.
- Performance. One pass at 480x270 was not measured.
- A light mask texture, and more than one light. The one light here is a
  position, a radius and a zone count in uniforms.
- Real art. The art is generated in `make_tile_sheet()` and
  `make_sprite_sheet()`. The table can now come from Pigment (`--lut`), by
  way of a script and not a Pigment export.
- The real shader-free path. `F` resolves on the CPU every frame to show
  its look; the real one would recolour textures when a room loads.
