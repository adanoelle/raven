#!/usr/bin/env python3
"""Generate placeholder sprite PNGs for raven.

Requires Pillow: pip install Pillow
Run from project root: python tools/scripts/generate_placeholders.py
"""

from pathlib import Path
from PIL import Image, ImageDraw

ASSETS = Path("assets/sprites")


def generate_player():
    """48x16 sheet: 3 frames (idle, walk1, walk2), each 16x16. Top-down view."""
    img = Image.new("RGBA", (48, 16), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Frame 0: idle — simple character shape
    draw.rectangle([(4, 2), (12, 14)], fill=(0, 200, 120, 255), outline=(255, 255, 255, 255))
    draw.rectangle([(6, 4), (10, 8)], fill=(40, 255, 180, 255))  # face

    # Frame 1: walk1 — offset legs
    draw.rectangle([(16 + 4, 2), (16 + 12, 14)], fill=(0, 200, 120, 255), outline=(255, 255, 255, 255))
    draw.rectangle([(16 + 3, 10), (16 + 6, 15)], fill=(0, 160, 100, 255))  # left leg
    draw.rectangle([(16 + 10, 10), (16 + 13, 15)], fill=(0, 160, 100, 255))  # right leg

    # Frame 2: walk2 — opposite legs
    draw.rectangle([(32 + 4, 2), (32 + 12, 14)], fill=(0, 200, 120, 255), outline=(255, 255, 255, 255))
    draw.rectangle([(32 + 5, 10), (32 + 8, 15)], fill=(0, 160, 100, 255))
    draw.rectangle([(32 + 8, 10), (32 + 11, 15)], fill=(0, 160, 100, 255))

    img.save(ASSETS / "player.png")
    print(f"  Created {ASSETS / 'player.png'} (48x16)")


def generate_projectiles():
    """64x16 sheet: 8 cols x 2 rows, each 8x8.
    Row 0: small projectiles, Row 1: larger projectiles."""
    img = Image.new("RGBA", (64, 16), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    colors = [
        (255, 60, 60),    # red
        (60, 100, 255),   # blue
        (60, 255, 60),    # green
        (255, 255, 60),   # yellow
        (255, 255, 255),  # white
        (200, 60, 255),   # purple
        (255, 160, 40),   # orange
        (0, 220, 255),    # cyan
    ]

    for i, (r, g, b) in enumerate(colors):
        # Row 0: small (3px radius circle centered in 8x8)
        cx, cy = i * 8 + 4, 4
        draw.ellipse((cx - 2, cy - 2, cx + 2, cy + 2), fill=(r, g, b, 255))

        # Row 1: large (3.5px radius)
        cy2 = 12
        draw.ellipse((cx - 3, cy2 - 3, cx + 3, cy2 + 3), fill=(r, g, b, 255), outline=(255, 255, 255, 200))

    img.save(ASSETS / "projectiles.png")
    print(f"  Created {ASSETS / 'projectiles.png'} (64x16)")


def generate_enemies():
    """48x32 sheet: 3 cols x 2 rows, each 16x16.
    Row 0: grunt walk frames. Row 1: brute walk frames."""
    img = Image.new("RGBA", (48, 32), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Row 0: grunt frames (red-ish characters)
    for i in range(3):
        cx = i * 16 + 8
        cy = 8
        offset = i  # slight animation variation
        draw.rectangle(
            [(cx - 5, cy - 6 + offset), (cx + 5, cy + 6 - offset)],
            fill=(200, 50, 50, 255),
            outline=(255, 120, 120, 255),
        )

    # Row 1: brute frames (larger purple characters)
    for i in range(3):
        cx = i * 16 + 8
        cy = 24
        offset = i
        draw.rectangle(
            [(cx - 6, cy - 7 + offset), (cx + 6, cy + 7 - offset)],
            fill=(140, 30, 160, 255),
            outline=(200, 100, 240, 255),
        )

    img.save(ASSETS / "enemies.png")
    print(f"  Created {ASSETS / 'enemies.png'} (48x32)")


def main():
    ASSETS.mkdir(parents=True, exist_ok=True)
    print("Generating placeholder sprites...")
    generate_player()
    generate_projectiles()
    generate_enemies()
    print("Done!")


if __name__ == "__main__":
    main()
