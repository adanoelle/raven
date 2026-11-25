#!/usr/bin/env python3
"""Build the lookup tables for the shader lighting prototype.

    python3 tools/shader_lighting/build_tables.py

Reads the game's sprite sheets and tileset as they are and writes two
images to assets/data/lighting/:

  ramps.png    One row per ramp, one column per step. Every ramp sits on the
               same lightness ladder, so one step of light is the same size
               on every material.
  reverse.png  Colour to (ramp, step): a 64x64x64 cube of colours laid out
               as 8x8 slices. R = ramp, G = step, A = 255 where the colour
               is known and 0 where it is not.

The game draws in ordinary colour. The shader looks each pixel up in
reverse.png, moves it along its ramp by the light at that pixel, and reads
the new colour from ramps.png. A colour that is not in the table is left as
drawn, which is how bullets and the HUD stay unlit.

How ramps are made, all of it a first guess and none of it an artist's
choice:

- Colours are grouped by sheet and then by hue (greys in one group). A
  group is a ramp. Keeping sheets apart stops a wall from borrowing a
  character's brighter reds as its lit colours. A colour used by two sheets
  belongs to the first in the list.
- Along a ramp, colour between two of the art's colours is interpolated;
  below the darkest it turns cooler and greyer, above the lightest warmer
  and paler.
- Near-black colours are left out so outlines stay black under light.
- ramps.png holds every ramp twice. The lower half is the ramp under the
  cool light: each colour moved most of the way to one blue and made a
  little lighter. The shader picks it where cool light falls.
- With no light and no ambient shift the shader passes the drawn colour
  through, so the art is unchanged where nothing acts on it.

Needs Pillow.
"""

import math
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
OUT_DIR = ROOT / "assets" / "data" / "lighting"

# Art that light acts on. Bullets are left out on purpose: they are light.
SHEETS = [
    "assets/tilesets/ruins.png",
    "assets/sprites/player.png",
    "assets/sprites/knight.png",
    "assets/sprites/enemies.png",
    "assets/sprites/enemies_mid.png",
    "assets/sprites/enemies_boss.png",
    "assets/sprites/props.png",
    "assets/sprites/pickups.png",
]

STEPS = 24            # columns of ramps.png
LADDER_FIRST = 0.04   # OKLCH lightness of step 0
LADDER_STEP = 0.04    # lightness added per step
OUTLINE_BELOW = 0.15  # darker colours are outlines: never lit
GREY_BELOW = 0.03     # chroma under this is a grey
HUE_GAP = 30.0        # degrees of empty hue that separate two groups
SNAP_DISTANCE = 0.035 # OKLab distance within which a blended pixel snaps to an art colour
COOL_HUE = 280.0      # shadows turn towards this
WARM_HUE = 85.0       # highlights turn towards this
MAX_TURN = 40.0       # degrees, at the far end of the headroom
TINT_HUE = 242.0      # the cool light's colour: an electric blue
TINT_AMOUNT = 0.7     # how far a colour moves to it, 0 to 1
TINT_CHROMA = 0.09    # the chroma it moves to
TINT_LIFT = 0.04      # lightness added: cool light reads as whiter


def to_linear(c):
    c /= 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def oklab(rgb):
    r, g, b = (to_linear(c) for c in rgb)
    l = (0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b) ** (1 / 3)
    m = (0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b) ** (1 / 3)
    s = (0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b) ** (1 / 3)
    return (
        0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s,
        1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s,
        0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s,
    )


def linear_rgb(lab):
    L, a, b = lab
    l = (L + 0.3963377774 * a + 0.2158037573 * b) ** 3
    m = (L - 0.1055613458 * a - 0.0638541728 * b) ** 3
    s = (L - 0.0894841775 * a - 1.2914855480 * b) ** 3
    return (
        4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
        -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
        -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s,
    )


def srgb_bytes(lab):
    """The sRGB colour for an OKLab one, with chroma reduced until it fits."""
    L, a, b = lab
    L = min(max(L, 0.0), 1.0)
    for _ in range(40):
        rgb = linear_rgb((L, a, b))
        if all(-1e-4 <= c <= 1.0001 for c in rgb):
            break
        a *= 0.92
        b *= 0.92
    out = []
    for c in linear_rgb((L, a, b)):
        c = min(max(c, 0.0), 1.0)
        out.append(round(255 * (12.92 * c if c <= 0.0031308 else 1.055 * c ** (1 / 2.4) - 0.055)))
    return tuple(out)


def cool_tint(rgb):
    """The colour as it looks under the cool light (see the lower half of ramps.png)."""
    L, a, b = oklab(rgb)
    hue = math.radians(TINT_HUE)
    a = a + (TINT_CHROMA * math.cos(hue) - a) * TINT_AMOUNT
    b = b + (TINT_CHROMA * math.sin(hue) - b) * TINT_AMOUNT
    return srgb_bytes((L + TINT_LIFT * (1.0 - L), a, b))


def step_of(lightness):
    return min(max(round((lightness - LADDER_FIRST) / LADDER_STEP), 0), STEPS - 1)


def turn_towards(hue, target, amount):
    delta = (target - hue + 180.0) % 360.0 - 180.0
    return hue + max(-amount, min(amount, delta))


def collect_colours():
    """Per sheet, the opaque colours no earlier sheet has claimed, with pixel counts."""
    claimed = set()
    per_sheet = []
    for sheet in SHEETS:
        counts = {}
        image = Image.open(ROOT / sheet).convert("RGBA")
        for r, g, b, a in image.getdata():
            if a == 255 and (r, g, b) not in claimed:
                counts[(r, g, b)] = counts.get((r, g, b), 0) + 1
        claimed |= set(counts)
        per_sheet.append((sheet, counts))
    return per_sheet


def group_by_hue(colours):
    """Lists of (rgb, lab), one list per ramp: greys first, then by hue."""
    greys, chromatic = [], []
    for rgb in colours:
        lab = oklab(rgb)
        if lab[0] < OUTLINE_BELOW:
            continue
        chroma = math.hypot(lab[1], lab[2])
        (greys if chroma < GREY_BELOW else chromatic).append((rgb, lab))
    chromatic.sort(key=lambda c: math.degrees(math.atan2(c[1][2], c[1][1])) % 360.0)
    groups = []
    hues = [math.degrees(math.atan2(lab[2], lab[1])) % 360.0 for _, lab in chromatic]
    current = []
    for index, colour in enumerate(chromatic):
        if current and hues[index] - hues[index - 1] > HUE_GAP:
            groups.append(current)
            current = []
        current.append(colour)
    if current:
        # The hue circle wraps: join the last group to the first when they touch.
        if groups and hues[0] + 360.0 - hues[-1] <= HUE_GAP:
            groups[0] = current + groups[0]
        else:
            groups.append(current)
    return ([greys] if greys else []) + groups


def ramp_colours(group, counts):
    """The ramp for one group: a colour for every step of the ladder."""
    # One knot per ladder step the art uses, weighted by how many pixels use it.
    by_step = {}
    for rgb, lab in group:
        by_step.setdefault(step_of(lab[0]), []).append((counts[rgb], lab))
    knots = []
    for step in sorted(by_step):
        total = sum(n for n, _ in by_step[step])
        a = sum(n * lab[1] for n, lab in by_step[step]) / total
        b = sum(n * lab[2] for n, lab in by_step[step]) / total
        knots.append((step, a, b))

    first, last = knots[0], knots[-1]
    colours = []
    for step in range(STEPS):
        lightness = LADDER_FIRST + LADDER_STEP * step
        if step < first[0]:
            t = (first[0] - step) / max(first[0], 1)
            chroma = math.hypot(first[1], first[2]) * (1.0 - 0.6 * t)
            hue = math.degrees(math.atan2(first[2], first[1])) % 360.0
            hue = turn_towards(hue, COOL_HUE, MAX_TURN * t)
            a, b = chroma * math.cos(math.radians(hue)), chroma * math.sin(math.radians(hue))
        elif step > last[0]:
            t = (step - last[0]) / max(STEPS - 1 - last[0], 1)
            chroma = math.hypot(last[1], last[2]) * (1.0 - 0.7 * t)
            hue = math.degrees(math.atan2(last[2], last[1])) % 360.0
            hue = turn_towards(hue, WARM_HUE, MAX_TURN * t)
            a, b = chroma * math.cos(math.radians(hue)), chroma * math.sin(math.radians(hue))
        else:
            lower = max(k for k in knots if k[0] <= step)
            upper = min(k for k in knots if k[0] >= step)
            t = 0.0 if upper[0] == lower[0] else (step - lower[0]) / (upper[0] - lower[0])
            a = lower[1] + (upper[1] - lower[1]) * t
            b = lower[2] + (upper[2] - lower[2]) * t
        colours.append(srgb_bytes((lightness, a, b)))
    return colours


def main():
    counts = {}
    groups = []
    names = []
    for sheet, sheet_counts in collect_colours():
        counts.update(sheet_counts)
        for group in group_by_hue(sheet_counts):
            if group:
                groups.append(group)
                names.append(Path(sheet).stem)
    if len(groups) > 255:
        sys.exit("build_tables: more than 255 ramps")

    # Two layers: the ramps, then the same ramps under the cool light
    ramps = Image.new("RGBA", (STEPS, 2 * len(groups)))
    entries = []  # (rgb, lab, ramp, step)
    for row, group in enumerate(groups):
        for step, colour in enumerate(ramp_colours(group, counts)):
            ramps.putpixel((step, row), colour + (255,))
            ramps.putpixel((step, len(groups) + row), cool_tint(colour) + (255,))
        for rgb, lab in group:
            entries.append((rgb, lab, row, step_of(lab[0])))
        hue = "grey"
        if math.hypot(group[0][1][1], group[0][1][2]) >= GREY_BELOW:
            mean = sum(math.degrees(math.atan2(lab[2], lab[1])) % 360.0 for _, lab in group)
            hue = f"hue {mean / len(group):5.1f}"
        used = sorted({step_of(lab[0]) for _, lab in group})
        print(f"ramp {row:2d}: {names[row]:13s} {len(group):2d} colours, {hue}, art on steps {used}")

    # Colour to (ramp, step). Exact art colours first, then anything close
    # to one, so a blended or tinted pixel still finds its ramp.
    reverse = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    exact = {}
    for rgb, lab, row, step in sorted(entries, key=lambda e: counts[e[0]]):
        exact[(rgb[0] >> 2, rgb[1] >> 2, rgb[2] >> 2)] = (row, step)
    labs = [(lab, row, step) for _, lab, row, step in entries]
    known = 0
    for b6 in range(64):
        for g6 in range(64):
            for r6 in range(64):
                found = exact.get((r6, g6, b6))
                if found is None:
                    L, a, b = oklab((r6 * 4 + 2, g6 * 4 + 2, b6 * 4 + 2))
                    best = SNAP_DISTANCE * SNAP_DISTANCE
                    if L >= OUTLINE_BELOW:
                        for (l2, a2, b2), row, step in labs:
                            d = (L - l2) ** 2 + (a - a2) ** 2 + (b - b2) ** 2
                            if d < best:
                                best = d
                                # The pixel's own lightness places it on the ladder.
                                found = (row, step_of(L))
                if found is not None:
                    known += 1
                    reverse.putpixel((r6 + (b6 & 7) * 64, g6 + (b6 >> 3) * 64),
                                     (found[0], found[1], 0, 255))

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    ramps.save(OUT_DIR / "ramps.png")
    reverse.save(OUT_DIR / "reverse.png")
    skipped = sum(1 for rgb in counts if oklab(rgb)[0] < OUTLINE_BELOW)
    print(f"{len(entries)} art colours in {len(groups)} ramps of {STEPS} steps; "
          f"{skipped} near-black colours left unlit")
    print(f"{known} of 262144 colour cells map to a ramp")
    print(f"wrote {OUT_DIR / 'ramps.png'} and {OUT_DIR / 'reverse.png'}")


if __name__ == "__main__":
    main()
