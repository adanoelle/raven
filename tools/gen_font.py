#!/usr/bin/env python3
"""Generate the bitmap font atlas for the in-game text renderer.

Renders printable ASCII (32-126) from the hand-defined 5x7 pixel glyphs
below into a fixed-cell grid PNG: 16 columns x 6 rows, 6x8 cells (5x7
glyph + 1px spacing), white on transparency so SDL color modulation can
tint text at draw time.

The atlas layout contract (consumed by src/rendering/bitmap_font.cpp):
  - first char = 32 (space), last char = 126 ('~')
  - 16 glyphs per row, row-major order
  - every glyph occupies an identical cell (monospace)

Glyphs are ASCII art: 'X' = opaque pixel, '.' = transparent. Edit shapes
here and re-run, or replace the PNG with hand-drawn art — as long as the
grid layout is kept, no code changes are needed.

Usage:
    python3 tools/gen_font.py [output.png]

Requires: Pillow (pip install pillow)
"""

import sys

from PIL import Image

GLYPH_W = 5
GLYPH_H = 7
CELL_W = GLYPH_W + 1
CELL_H = GLYPH_H + 1
COLUMNS = 16
FIRST_CHAR = 32
LAST_CHAR = 126
WHITE = (255, 255, 255, 255)

# fmt: off
FONT = {
    ' ': [".....", ".....", ".....", ".....", ".....", ".....", "....."],
    '!': ["..X..", "..X..", "..X..", "..X..", "..X..", ".....", "..X.."],
    '"': [".X.X.", ".X.X.", ".X.X.", ".....", ".....", ".....", "....."],
    '#': [".X.X.", ".X.X.", "XXXXX", ".X.X.", "XXXXX", ".X.X.", ".X.X."],
    '$': ["..X..", ".XXXX", "X.X..", ".XXX.", "..X.X", "XXXX.", "..X.."],
    '%': ["XX...", "XX..X", "...X.", "..X..", ".X...", "X..XX", "...XX"],
    '&': [".XX..", "X..X.", "X.X..", ".X...", "X.X.X", "X..X.", ".XX.X"],
    "'": ["..X..", "..X..", ".X...", ".....", ".....", ".....", "....."],
    '(': ["...X.", "..X..", ".X...", ".X...", ".X...", "..X..", "...X."],
    ')': [".X...", "..X..", "...X.", "...X.", "...X.", "..X..", ".X..."],
    '*': [".....", "..X..", "X.X.X", ".XXX.", "X.X.X", "..X..", "....."],
    '+': [".....", "..X..", "..X..", "XXXXX", "..X..", "..X..", "....."],
    ',': [".....", ".....", ".....", ".....", ".XX..", "..X..", ".X..."],
    '-': [".....", ".....", ".....", "XXXXX", ".....", ".....", "....."],
    '.': [".....", ".....", ".....", ".....", ".....", ".XX..", ".XX.."],
    '/': [".....", "....X", "...X.", "..X..", ".X...", "X....", "....."],
    '0': [".XXX.", "X...X", "X..XX", "X.X.X", "XX..X", "X...X", ".XXX."],
    '1': ["..X..", ".XX..", "..X..", "..X..", "..X..", "..X..", ".XXX."],
    '2': [".XXX.", "X...X", "....X", "...X.", "..X..", ".X...", "XXXXX"],
    '3': ["XXXXX", "...X.", "..X..", "...X.", "....X", "X...X", ".XXX."],
    '4': ["...X.", "..XX.", ".X.X.", "X..X.", "XXXXX", "...X.", "...X."],
    '5': ["XXXXX", "X....", "XXXX.", "....X", "....X", "X...X", ".XXX."],
    '6': ["..XX.", ".X...", "X....", "XXXX.", "X...X", "X...X", ".XXX."],
    '7': ["XXXXX", "....X", "...X.", "..X..", ".X...", ".X...", ".X..."],
    '8': [".XXX.", "X...X", "X...X", ".XXX.", "X...X", "X...X", ".XXX."],
    '9': [".XXX.", "X...X", "X...X", ".XXXX", "....X", "...X.", ".XX.."],
    ':': [".....", ".XX..", ".XX..", ".....", ".XX..", ".XX..", "....."],
    ';': [".....", ".XX..", ".XX..", ".....", ".XX..", "..X..", ".X..."],
    '<': ["...X.", "..X..", ".X...", "X....", ".X...", "..X..", "...X."],
    '=': [".....", ".....", "XXXXX", ".....", "XXXXX", ".....", "....."],
    '>': [".X...", "..X..", "...X.", "....X", "...X.", "..X..", ".X..."],
    '?': [".XXX.", "X...X", "....X", "...X.", "..X..", ".....", "..X.."],
    '@': [".XXX.", "X...X", "....X", ".XX.X", "X.X.X", "X.X.X", ".XXX."],
    'A': [".XXX.", "X...X", "X...X", "XXXXX", "X...X", "X...X", "X...X"],
    'B': ["XXXX.", "X...X", "X...X", "XXXX.", "X...X", "X...X", "XXXX."],
    'C': [".XXX.", "X...X", "X....", "X....", "X....", "X...X", ".XXX."],
    'D': ["XXX..", "X..X.", "X...X", "X...X", "X...X", "X..X.", "XXX.."],
    'E': ["XXXXX", "X....", "X....", "XXXX.", "X....", "X....", "XXXXX"],
    'F': ["XXXXX", "X....", "X....", "XXXX.", "X....", "X....", "X...."],
    'G': [".XXX.", "X...X", "X....", "X.XXX", "X...X", "X...X", ".XXXX"],
    'H': ["X...X", "X...X", "X...X", "XXXXX", "X...X", "X...X", "X...X"],
    'I': [".XXX.", "..X..", "..X..", "..X..", "..X..", "..X..", ".XXX."],
    'J': ["..XXX", "...X.", "...X.", "...X.", "...X.", "X..X.", ".XX.."],
    'K': ["X...X", "X..X.", "X.X..", "XX...", "X.X..", "X..X.", "X...X"],
    'L': ["X....", "X....", "X....", "X....", "X....", "X....", "XXXXX"],
    'M': ["X...X", "XX.XX", "X.X.X", "X.X.X", "X...X", "X...X", "X...X"],
    'N': ["X...X", "XX..X", "X.X.X", "X..XX", "X...X", "X...X", "X...X"],
    'O': [".XXX.", "X...X", "X...X", "X...X", "X...X", "X...X", ".XXX."],
    'P': ["XXXX.", "X...X", "X...X", "XXXX.", "X....", "X....", "X...."],
    'Q': [".XXX.", "X...X", "X...X", "X...X", "X.X.X", "X..X.", ".XX.X"],
    'R': ["XXXX.", "X...X", "X...X", "XXXX.", "X.X..", "X..X.", "X...X"],
    'S': [".XXXX", "X....", "X....", ".XXX.", "....X", "....X", "XXXX."],
    'T': ["XXXXX", "..X..", "..X..", "..X..", "..X..", "..X..", "..X.."],
    'U': ["X...X", "X...X", "X...X", "X...X", "X...X", "X...X", ".XXX."],
    'V': ["X...X", "X...X", "X...X", "X...X", "X...X", ".X.X.", "..X.."],
    'W': ["X...X", "X...X", "X...X", "X.X.X", "X.X.X", "XX.XX", "X...X"],
    'X': ["X...X", "X...X", ".X.X.", "..X..", ".X.X.", "X...X", "X...X"],
    'Y': ["X...X", "X...X", ".X.X.", "..X..", "..X..", "..X..", "..X.."],
    'Z': ["XXXXX", "....X", "...X.", "..X..", ".X...", "X....", "XXXXX"],
    '[': [".XXX.", ".X...", ".X...", ".X...", ".X...", ".X...", ".XXX."],
    '\\': [".....", "X....", ".X...", "..X..", "...X.", "....X", "....."],
    ']': [".XXX.", "...X.", "...X.", "...X.", "...X.", "...X.", ".XXX."],
    '^': ["..X..", ".X.X.", "X...X", ".....", ".....", ".....", "....."],
    '_': [".....", ".....", ".....", ".....", ".....", ".....", "XXXXX"],
    '`': [".X...", "..X..", "...X.", ".....", ".....", ".....", "....."],
    'a': [".....", ".....", ".XXX.", "....X", ".XXXX", "X...X", ".XXXX"],
    'b': ["X....", "X....", "XXXX.", "X...X", "X...X", "X...X", "XXXX."],
    'c': [".....", ".....", ".XXX.", "X....", "X....", "X...X", ".XXX."],
    'd': ["....X", "....X", ".XXXX", "X...X", "X...X", "X...X", ".XXXX"],
    'e': [".....", ".....", ".XXX.", "X...X", "XXXXX", "X....", ".XXX."],
    'f': ["..XX.", ".X..X", ".X...", "XXX..", ".X...", ".X...", ".X..."],
    'g': [".....", ".XXXX", "X...X", "X...X", ".XXXX", "....X", ".XXX."],
    'h': ["X....", "X....", "XXXX.", "X...X", "X...X", "X...X", "X...X"],
    'i': ["..X..", ".....", ".XX..", "..X..", "..X..", "..X..", ".XXX."],
    'j': ["...X.", ".....", "..XX.", "...X.", "...X.", "X..X.", ".XX.."],
    'k': ["X....", "X....", "X..X.", "X.X..", "XX...", "X.X..", "X..X."],
    'l': [".XX..", "..X..", "..X..", "..X..", "..X..", "..X..", ".XXX."],
    'm': [".....", ".....", "XX.X.", "X.X.X", "X.X.X", "X.X.X", "X...X"],
    'n': [".....", ".....", "XXXX.", "X...X", "X...X", "X...X", "X...X"],
    'o': [".....", ".....", ".XXX.", "X...X", "X...X", "X...X", ".XXX."],
    'p': [".....", "XXXX.", "X...X", "X...X", "XXXX.", "X....", "X...."],
    'q': [".....", ".XXXX", "X...X", "X...X", ".XXXX", "....X", "....X"],
    'r': [".....", ".....", "X.XX.", "XX..X", "X....", "X....", "X...."],
    's': [".....", ".....", ".XXXX", "X....", ".XXX.", "....X", "XXXX."],
    't': [".X...", ".X...", "XXX..", ".X...", ".X...", ".X..X", "..XX."],
    'u': [".....", ".....", "X...X", "X...X", "X...X", "X..XX", ".XX.X"],
    'v': [".....", ".....", "X...X", "X...X", "X...X", ".X.X.", "..X.."],
    'w': [".....", ".....", "X...X", "X...X", "X.X.X", "X.X.X", ".X.X."],
    'x': [".....", ".....", "X...X", ".X.X.", "..X..", ".X.X.", "X...X"],
    'y': [".....", "X...X", "X...X", "X...X", ".XXXX", "....X", ".XXX."],
    'z': [".....", ".....", "XXXXX", "...X.", "..X..", ".X...", "XXXXX"],
    '{': ["...X.", "..X..", "..X..", ".X...", "..X..", "..X..", "...X."],
    '|': ["..X..", "..X..", "..X..", "..X..", "..X..", "..X..", "..X.."],
    '}': [".X...", "..X..", "..X..", "...X.", "..X..", "..X..", ".X..."],
    '~': [".....", ".....", ".X...", "X.X.X", "...X.", ".....", "....."],
}
# fmt: on


def main() -> None:
    out_path = sys.argv[1] if len(sys.argv) > 1 else "assets/fonts/font.png"

    chars = [chr(c) for c in range(FIRST_CHAR, LAST_CHAR + 1)]
    missing = [c for c in chars if c not in FONT]
    if missing:
        sys.exit(f"Missing glyphs: {missing}")

    rows = (len(chars) + COLUMNS - 1) // COLUMNS
    atlas = Image.new("RGBA", (COLUMNS * CELL_W, rows * CELL_H), (0, 0, 0, 0))

    for i, ch in enumerate(chars):
        art = FONT[ch]
        assert len(art) == GLYPH_H and all(len(row) == GLYPH_W for row in art), repr(ch)
        cx = (i % COLUMNS) * CELL_W
        cy = (i // COLUMNS) * CELL_H
        for y, row in enumerate(art):
            for x, pixel in enumerate(row):
                if pixel == "X":
                    atlas.putpixel((cx + x, cy + y), WHITE)

    atlas.save(out_path)
    print(f"Wrote {out_path}: {atlas.width}x{atlas.height}, "
          f"glyph cell {CELL_W}x{CELL_H}, {len(chars)} glyphs")


if __name__ == "__main__":
    main()
