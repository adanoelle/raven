# Tools and Generators

The small tools that exist around the game: what each one makes, and what
it means for art and music work. Consistent with
[Getting Work Into the Game](getting-work-into-the-game.md), none of this
is *required* for drawing or composing — but knowing what's here answers
two questions that come up constantly: *"is this asset final or
placeholder?"* and *"is there already a tool for that?"*

---

## The Generators in `tools/`

The repo carries a few scripts that generate assets. They fall into two
groups with opposite relationships to art work.

### Generators whose output gets replaced

These produced the placeholder assets currently in the game. They are, in
effect, **the manifest of what's not final**: anything they generate is
explicitly waiting to be superseded by real work under the same file name.

| Script | Generates | What replaces it |
| --- | --- | --- |
| `tools/gen_placeholder_sprites.py` | The placeholder bird sheets — `player.png` (32x32 frames), `enemies_mid.png`, `enemies_boss.png` | Real character art, row by row ([Character Animation Process](art/character-animation-process.md)) |
| `tools/gen_sfx.py` | All the synthesized retro SFX in `assets/audio/sfx/` (square waves, noise bursts — deterministic, seeded) | Designed sounds per the [Audio Specification](audio-spec.md), same file names |

No one needs to run these — their output is committed. If a placeholder
gets accidentally deleted or a new placeholder slot is needed, they
regenerate everything (`python3 tools/gen_sfx.py`; the sprite script needs
Pillow: `nix-shell -p python3Packages.pillow --run "python3
tools/gen_placeholder_sprites.py"`).

### Generators whose output gets used

| Script | Generates | Notes |
| --- | --- | --- |
| `tools/gen_templates.lua` | The four Aseprite tier templates in `art/templates/` (guide layers: body zone, proportion lines, feet lock, center cross) | Run via `aseprite -b --script tools/gen_templates.lua` — see below |
| `tools/gen_font.py` | `assets/fonts/font.png` — the in-game bitmap font atlas | See "the font is claimable" below |

The templates are committed, so normally they're just *opened*, not
generated ([Aseprite Setup Guide](art/art-aseprite-guide.md#2-template-setup)).
Regeneration only matters if the guide positions in the spec ever change —
and it requires **real Aseprite**, not LibreSprite: the script uses
Aseprite's Lua scripting API, which the free fork doesn't have.

### The font is a claimable piece of art

`assets/fonts/font.png` — every piece of text in the game — is currently
**procedurally generated** 5x7 glyphs, defined as ASCII art inside
`gen_font.py`. The atlas contract is simple: 16 columns x 6 rows of
identical 6x8 cells (5x7 glyph + 1px spacing), ASCII 32–126 in row-major
order, white on transparency so the engine can tint text any color at draw
time. A hand-drawn font that keeps that grid drops in with zero code
changes — a small, self-contained project for whoever wants the game's
typography to have a voice.

---

## Pigment (palette design)

[Pigment](art/pigment.md) is the studio's palette tool — OKLCH hue-shifted
ramps, gamut masking, ramp linking — grown out of Raven's own palette
work. It lives in its own repository, and **builds are on
[itch.io](https://adanoelle.itch.io/pigment)** for Windows, macOS, and
Linux, so using it never requires touching its source.

The workflow it feeds: design ramps in Pigment, export **`.gpl`**, and the
file lands at `art/palettes/raven.gpl` — the source of truth for the
game's hex values ([Art Files and Organization](art/art-organization.md)).
Aseprite loads `.gpl` directly.

---

## In the Dev Environment

For anyone who does work inside the repo, `nix develop` (or direnv)
provides — among the compilers — a set of tools relevant to art and
audio, no separate installs needed:

| Tool | What it's for |
| --- | --- |
| **LibreSprite** | Free Aseprite fork — a fallback pixel editor if a machine doesn't have an Aseprite license (templates open fine; only the generator script needs real Aseprite) |
| **LDtk** | The level editor — `assets/maps/raven.ldtk` is the game's map source ([Tilemaps and LDtk](architecture/tilemaps.md)) |
| **Audacity** | Audio editing — trimming, loudness checks against the [audio spec](audio-spec.md) targets |
| **ffmpeg** | Format conversion — e.g. rendering a DAW export down to OGG Vorbis for the [music tracks](music-track-list.md) |

Aseprite itself is the one tool that's a personal install (it's paid
software; compiling it from source is also licensed for personal use).

---

## Not Tools (Yet)

`tools/pattern_editor/` is an empty placeholder for a future bullet-pattern
editor; today, bullet patterns are authored directly as JSON in
`assets/data/patterns/`.
