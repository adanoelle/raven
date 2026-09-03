# First Character: A Walkthrough

The concrete route from blank template to a playable character in the
game — every file path, tool, and command in order. The
[Character Animation Process](character-animation-process.md) explains
*why* the steps are ordered this way; this page is the route itself, with
the [Knight](characters/knight.md) as the running example. Every later
character follows the same path with different names.

One thing worth noticing up front: none of the generator scripts in
`tools/` appear below. The templates are already committed, the
placeholders die by being overwritten, and Pigment is its own app —
that's by design.

---

## 1. Silhouettes — template only, no palette yet

The silhouette phase needs exactly one color (black), so palette work
waits. It's easier to design ramps once the winning shape shows how many
pixels the hair actually gets and how much armor surface there is to
shade.

- Open `art/templates/medium_32x32.aseprite`, then **File > Save As** →
  `art/characters/knight/sketches/YYYY-MM-DD-silhouettes.aseprite`
- The bottom layer is the locked guide layer (cyan body box, orange
  proportion lines, red feet line); drawing happens on the `art` layer
  above it
- Draw **5–10 solid black silhouettes**, one per frame — vary pauldron
  width, hair mass, sword at rest vs. ready. Feet on the red line, head
  top near the first orange line
- Pick the winner by squint at 1x and 2x zoom
- Settle the `flip_x` question here: the sprite mirrors for leftward
  movement ([spec §6](art-spec.md#6-facing-directions)), so the
  broadsword switches hands — asymmetric designs are chosen knowing that

## 2. Bank the winner (optional, worth it)

Save the winning head/torso/leg masses as
`art/templates/chibi_mannequin_32x32.aseprite` and as locked
[custom brushes](art-aseprite-guide.md#4-silhouette-blocking). Twenty
minutes now, and every future character and frame starts from consistent
volumes.

## 3. Palette — Pigment, with the silhouette in view

The character's page makes the color *decisions* (for the knight:
gold hair ~3 shades, burgundy/bronze armor ~3, skin 2, near-black
undersuit — see the
[working draft](characters/knight.md#color-palette-working-draft));
[Pigment](pigment.md) makes those ramps *good* — the metal preset for
armor, a hue-shifted ramp for the gold, ramp linking so both share a
dark end.

- Export **`.gpl`** → save as `art/palettes/raven.gpl`. The
  [shared game palette](art-organization.md) has to start somewhere; the
  first character's colors are its founding entries
- Fallback if Pigment fights the work: pick hexes directly in Aseprite
  and save the palette from there (Palette options > **Save Palette** —
  the round trip works in both directions)

## 4. Load palette + render the key frame

- Duplicate the template again →
  `art/characters/knight/knight.aseprite`. This is the **canonical
  working file** from here on; the sketches stay behind as design
  history
- Load the palette: palette options button (top-left of the palette
  panel) → **Load Palette** → `art/palettes/raven.gpl`
- Copy the winning silhouette in, then render frame 1 fully: outline,
  hair, armor, sword
- Run the [context checks](character-animation-process.md#3-context-checks-before-animating-anything)
  *before animating*: 1x/2x, black-fill test, against the darkest and
  brightest environment swatches, next to a grunt. This is where Pigment
  round-trips — if the gold washes out on daylight tiles, fix the ramp
  there, re-export, reload

## 5. Animate idle, then walk

- Idle: 4 frames, tagged `idle`. Walk: 6 frames, tagged `walk`
  (timings in the [spec's player table](art-spec.md#4-animation-frame-counts-and-timing))
- Onion skinning on, feet welded to the red line
- Update the hex values and any resolved Open Questions on the
  character's book page while they're fresh

## 6. Export — alongside the placeholder

**File > Export Sprite Sheet**: By Rows, split tags, no border padding →
`assets/sprites/knight.png`. Or headless:

```bash
aseprite -b art/characters/knight/knight.aseprite \
  --sheet assets/sprites/knight.png --sheet-type rows --split-tags
```

This does **not** overwrite the placeholder bird. The knight is its own
sheet: register it in `assets/data/config.json` under a new id
(`knight`) and point the class recipe at it (`apply_knight` sets the
`Sprite` sheet id — see [Player Classes](../architecture/player-classes.md)).
`player.png` stays as the sheet for classes without final art. A
two-row sheet (192x64) is fine — the engine reads idle from row 0 and
walk from row 1, and melee and dash temporarily reuse the walk row, so
nothing breaks while only two rows exist.

## 7. `just run`

The character is in the game. Ten minutes in real rooms answers what no
preview can: does the idle read under bullets, does the walk carry the
intended personality, does the signature color survive the darkest
stage? The edit → export → `just run` loop is seconds long — iterate on
these two rows until the character *feels* right before investing in the
expensive frames.

## 8. Action rows, one at a time

Attack (4f @ 12 fps, weapon breaking into the padding), dodge
(3f @ 15 fps), hurt, death — each tagged, exported, and checked in-game
as it's finished. When the attack and dodge rows land, the code switches
off the walk-row reuse and plays them for real (a small, known change —
flag it when the rows are in).

## 9. Commit as it goes

Source and export travel together, per
[Art Files and Organization](art-organization.md): `knight.aseprite` +
`knight.png` in the same commit, one line of *why* in the message. The
first finished sheet earns a [devlog](../devlog/2026-02-08-project-setup.md)
entry.
