# ADR-0028 shader spike

Throwaway code for
[ADR-0028](../../docs/book/src/decisions/0028-shader-resolved-palette-lighting.md).
It is not part of the game and is off by default in the build. Nothing in
`src/` depends on it.

It draws a hand-encoded scene as *(ramp, step)* values into a 480x270 index
target, resolves it to colour in one fragment shader pass through a small
lookup table, and checks the result against the same arithmetic on the CPU.

## Build and run

```sh
just shader-spike --check          # automated report, hidden window
just shader-spike --check --out /tmp/spike --show
just shader-spike                  # interactive
just shader-spike --driver vulkan  # force a GPU driver (vulkan, direct3d12, metal)
```

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
`M` colour modulation, `F` the shader-free CPU path, `E` emissives, `Esc`
quits. The top line shows how many index values are invalid.

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

### Windows

Not run yet. Run `--check` once as is and once with `--driver vulkan`.

### macOS

Not run yet. This is the only place the MSL shader is exercised.

## Not covered

- The Switch 2. That needs the NDA port of SDL3.
- Performance. One pass at 480x270 was not measured.
- A light mask texture, and more than one light. The one light here is a
  position, a radius and a zone count in uniforms.
- Real art, and a table exported by Pigment. The table is generated in
  `make_lut()` and the art in `make_tile_sheet()` and `make_sprite_sheet()`.
- The real shader-free path. `F` resolves on the CPU every frame to show
  its look; the real one would recolour textures when a room loads.
