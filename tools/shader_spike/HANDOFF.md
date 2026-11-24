# Shader work: state as of 2026-11-24

Written so a new session can pick this up without the conversation that
produced it. If this page disagrees with a repository, the repository wins.

## In one paragraph

Raven is trying a shader-led art direction: draw the world as *(ramp, step)*
index values and resolve it to colour in one fragment shader pass through a
table from Pigment, with light moving pixels along their ramps
([ADR-0028](../../docs/book/src/decisions/0028-shader-resolved-palette-lighting.md),
status Proposed). A throwaway spike proves the mechanism on Linux. It has
not run on Windows or macOS. The ADR is decided after those runs.

## Decisions made

| Decision | Where it is recorded |
| -------- | -------------------- |
| Try for shaders; option C of ADR-0028 is the proposal | ADR-0028 |
| Do not wait for Nintendo developer access; keep a shader-free fallback | ADR-0028, "Going ahead before Nintendo developer access" |
| The Switch 2 is a possible target | Switch Port Plan, step 3 |
| Light falloff: fully dithered ("smooth") preferred over flat rings or a narrow seam | `README.md` here, "The light" |
| ADR-0028 stays Proposed until the spike has run on three platforms | ADR-0028, devlog |

Not decided: everything under "Open questions" below.

## Where things are

| Thing | Location |
| ----- | -------- |
| Raven main checkout (other sessions work here) | `~/src/raven` |
| Spike checkout, branch `spike/shader-palette`, pushed, no PR | `~/src/raven-shader-spike` |
| Spike code and results | `tools/shader_spike/` (this folder) |
| ADR, devlog, plan pages | Raven `main`: `docs/book/src/decisions/0028-…`, `devlog/2026-11-24-shader-direction.md` |
| Pigment | `~/src/pigment`; MCP server on `feat/mcp-tools`, ready to test |
| Pigment note on what the proposal changes for the palette | Pigment `main`: `book/src/design/core-crate-plan.md`, step 2.4 |

Build and run the spike:

```sh
cd ~/src/raven-shader-spike
nix develop -c just shader-spike --check     # report
nix develop -c just shader-spike             # live view
nix shell nixpkgs#sdl3-shadercross -c tools/shader_spike/shaders/build.sh   # after editing the .hlsl
```

After any shader change, also change `cpu_resolve()` in `main.cpp` to match;
`--check` compares the two pixel for pixel.

## Issues

| Issue | What | Waiting on |
| ----- | ---- | ---------- |
| raven #35 | What LDtk layer opacity means under the shader plan | A choice |
| raven #36 | Run the spike on Windows and macOS | The developer, on those machines |
| raven #37 | Record the results in ADR-0028 and decide it | #35, #36, #38 |
| raven #38 | How characters and enemies take light | A try in the spike, then a choice |
| raven #39 | Apply for Nintendo developer access | The developer |
| raven #40 | Inspiration board for the palette | The developer |
| raven #41 | Decay explosion damage number (not shader work) | A design choice |
| pigment #54 | Build Raven's palette (core crate plan, step 2.4) | MCP server, #40, #37 |
| pigment #37, #38 | Index-encoded sheets; lookup-table export | A Pigment decision record first |

## What the spike found

Full tables are in `README.md`. In short, on Linux (SDL 3.4.0, Vulkan):

- The `gpu` renderer takes a custom fragment shader with the table as a
  second sampler. The resolve matches the CPU reference on every pixel.
- **Index pass rule:** nearest scale mode, no colour or alpha modulation.
  Sub-pixel positions are safe with nearest. Pixel-art scale mode is safe
  only on whole pixels.
- A corrupted index is still a valid index, so checks must compare against
  a reference.
- On a non-`gpu` renderer `SDL_CreateGPURenderState` returns null.
- ImGui draws on the `gpu` renderer.
- A render state's extra samplers are fixed at creation in SDL 3.4.0.
- The live view works; the developer used it and sent screenshots.

## Open questions

1. **Layer opacity (raven #35).** Raven's tilemap sets alpha modulation per
   tile for LDtk layer opacity (`tilemap_render_system.cpp`). That would
   corrupt the index pass.
2. **Actors and light (raven #38).** In the dark room, sprites outside the
   light nearly vanish. The spike's sprites carry a flag byte; bit 0 means
   unlit and is used for outlines. Trying "actors skip the ambient drop"
   is a few lines.
3. **Switch 2.** SDL's public pages say the port supports it and list the
   original Switch under Vulkan, but say nothing about the shader path on
   the Switch 2. Needs the NDA port.
4. **Oldest desktop hardware to support.** The `gpu` backend has no OpenGL
   path.
5. **Shared swatches and outlines.** How a swatch shared by two ramps names
   its ramp; whether outlines are a fixed index or drawn by the shader.
6. **The table format** (two channels of a sheet, or an indexed PNG plus a
   table). Belongs to pigment #38.

## Known rough edges in the spike

- The top overlay line looked clipped in the developer's screenshots. It
  may be the screenshot crop. Not confirmed, not fixed.
- The seam width in the narrow-seam mode is set in light level, not
  pixels, so seams are uneven. Moot while "smooth" is the choice.
- The table and the art are generated in `make_lut()`,
  `make_tile_sheet()` and `make_sprite_sheet()`. Do not judge colour from
  them.
- `F` imitates the shader-free look by resolving on the CPU each frame. It
  is not the real fallback.

## Next steps, in order

1. Decide raven #35.
2. Run the spike on Windows and macOS (raven #36).
3. Try the actor-lighting options in the spike and decide raven #38.
4. Update ADR-0028 with all of it and decide it (raven #37).
5. If accepted: a Pigment decision record for the table export, then the
   palette (pigment #54), then one room drawn in Aseprite and lit: tiles,
   the Knight, one enemy, bullets, in two environments.

In parallel: Nintendo access (raven #39), the inspiration board
(raven #40).

## A good first task with the Pigment MCP server

> Using the Pigment MCP server, generate eight-step ramps for the spike's
> four materials (stone, vegetation, earth, a burgundy character) in two
> lighting variants (open daylight, dark ruins). Replace `make_lut()` in
> `tools/shader_spike/main.cpp` with those values, keep `--check` passing,
> and write down anything awkward or missing in the server's tools.

The server is read-and-compute only, so the values are copied in by hand.
That is fine for the spike and is itself evidence for pigment #38.

## Things said in conversation and written nowhere else

### Drawing rules, if ADR-0028 is accepted

These are also in the devlog as "not yet rules":

- Indexed colour mode in Aseprite; every pixel an exact palette swatch.
- Draw material in a reference environment, not the mood.
- Shade form by hand; keep base art in the middle of each ramp.
- Do not draw light the shader will draw: the lamp, not its pool.
- Glowing parts on their own layer, exported separately.
- No translucent world pixels; shadows dithered or separate.
- New from the spike: textured tiles hide dither grain, large flat fills
  show it; longer ramps make the grain finer.
- In Indexed mode, loading another environment's palette file in Aseprite
  previews that mood in place (pigment #40 would automate it).

### Outline of an art direction chapter (not written)

1. The direction in one line: the palette is the light.
2. What the player sees, per environment.
3. Three layers of a frame: world, light, emissives.
4. Which assets belong in which layer, and whether light affects them.
5. Drawing for the resolve pass.
6. Palette structure: ramps, headroom, budget, variants.
7. Light as a design tool: what can and cannot be asked for.
8. Mechanics shown through the palette (weapon decay, Legendary shimmer,
   depth drift, hit flash). Provisional.
9. What we deliberately do not do.
10. Previewing and checking.
11. What the game looks like on the fallback.

Sections 1 to 4 and 9 can be drafted before the ADR is decided. The rest
wait for it.

### Rules that keep the fallback cheap

- Build the fallback early, with a debug toggle.
- Every room must read on the fallback. Nothing the player needs is carried
  only by shader lighting.

### Pigment issue review (2026-11-24)

- Pigment #35 (command-line tool) is the hinge: #36, #37/#38 and #40 need
  it. Recommended order after the MCP server: #35 with `check` (#36) built
  in, then settle #37 against #38, hold #39 until the release resumes,
  leave #41 and #42 until Raven's palette exists.
- **#39, web demo on itch.io:** itch hosts HTML5 uploads in an iframe and
  plain WebAssembly runs there. Unchecked: whether a paid download page can
  also carry a free browser embed. If not, a separate free page.
- **#40, Aseprite:** three shapes. Shell out to the command-line tool
  (enough for Raven; a sold extension would bundle a binary per platform).
  Port the engine to Lua (self-contained and sellable; the shared fixtures
  could keep it honest). Connect to the running app over Aseprite's
  WebSocket client (true hot reload; puts a local server in the app).
- Pigment's 0.2 backlog and the paused 0.1.8 release are tracked only in
  `book/src/start-here/status.md`, not in issues.

## Housekeeping left over

- Local branches safe to delete once `main` is pulled:
  `docs/adr-0028-shader-palette` in Raven, `docs/raven-shader-direction`
  in Pigment.
- `~/.cache/CPM` was created by the spike build and is safe to delete.
- `build-spike/` in this checkout is ignored by git.
- A live spike window may still be open from this session.
