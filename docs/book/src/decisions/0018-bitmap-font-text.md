# 18. Bitmap Font Atlas for Text Rendering

Date: 2026-07-03 Status: Accepted

## Context

The game had no text rendering. Menus, prompts, and the score were coloured
rectangles — the game-over screen encoded score digits as *brightness levels*.
Every UI feature on the roadmap (pause menu, options screen, real HUD,
dialogue) was blocked on text.

Two realistic approaches for an SDL3 game:

1. **SDL_ttf / FreeType** — rasterise a TTF at runtime. Flexible sizes and
   full Unicode, but adds a dependency, and vector fonts rasterised small look
   mushy at a 480x270 virtual resolution unless a pixel-perfect TTF is chosen
   and hinted carefully. Glyph caching also needs managing.
2. **Bitmap font atlas** — one texture of pre-drawn glyphs, drawn like any
   other sprite sheet. Monospace grid means lookup is arithmetic; integer
   scaling keeps pixels crisp; the art pass can restyle text by repainting the
   atlas.

At 480x270 with a pixel-art aesthetic, text *is* pixel art. The engine already
has exactly the right machinery for it (textures, `SDL_SCALEMODE_PIXELART`,
colour modulation).

## Decision

**Text renders from a monospace bitmap font atlas** via `BitmapFont`
(`src/rendering/bitmap_font.hpp/.cpp`).

- The atlas covers printable ASCII 32–126 in a 16-column row-major grid of
  identical cells (currently 6x8: a 5x7 glyph + 1px spacing). Glyph lookup is
  `index = c - 32`; no per-glyph metrics.
- Glyphs are stored **white**; `draw()` tints per call with SDL colour/alpha
  modulation, so one texture serves all text colours.
- Scaling is integer-only for pixel crispness.
- `tools/gen_font.py` generates the atlas from hand-defined ASCII-art glyph
  shapes (an early attempt thresholding Pillow's built-in TTF produced lumpy,
  clipped glyphs). The 16-column/32-first-char grid is the only contract
  between asset and code — hand-drawn replacement art needs no code changes.
- Degradation matches the sprite philosophy: unknown characters draw `?`, an
  unloaded font draws nothing rather than crashing.

`Game` owns the font, loads it from the `font` section of `config.json`, and
exposes `game.font()` to scenes and the HUD.

## Consequences

**Positive:**

- No new third-party dependency; ~200 lines total
- Pixel-perfect at every integer scale; visually native to the art style
- Unblocked immediately: title/menus/game-over/HUD text, pause menu, future
  options screen
- Art pass can restyle all text by editing glyph shapes in `gen_font.py` or
  replacing the PNG

**Negative:**

- ASCII-only; localisation beyond Latin would need atlas pages or a switch to
  SDL_ttf (acceptable for now — all strings are English UI labels)
- Monospace only; proportional spacing would need a per-glyph width table
- One size per atlas — different sizes come from integer scaling, which is the
  pixel-art-correct behaviour anyway
