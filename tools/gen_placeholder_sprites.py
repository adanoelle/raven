#!/usr/bin/env python3
"""Generate placeholder sprite sheets at the new 32x32 sizing.

Creates:
  - assets/sprites/player.png      (32x32 frames, 6 cols x 9 rows)
  - assets/sprites/enemies_mid.png  (32x32 frames, 3 cols x 2 rows)
  - assets/sprites/enemies_boss.png (48x48 frames, 3 cols x 2 rows)

Run: nix-shell -p python3Packages.pillow --run "python3 tools/gen_placeholder_sprites.py"
"""

from PIL import Image, ImageDraw
import os

ASSETS = os.path.join(os.path.dirname(__file__), "..", "assets", "sprites")


def draw_bird(draw, x, y, w, h, body_color, eye_color="white", beak_color="#e8a020"):
    """Draw a simple bird character silhouette centered in a wxh frame at (x, y)."""
    cx, cy = x + w // 2, y + h // 2

    # Body (oval, lower center)
    body_w, body_h = int(w * 0.55), int(h * 0.50)
    body_top = cy - int(body_h * 0.3)
    draw.ellipse(
        [cx - body_w // 2, body_top, cx + body_w // 2, body_top + body_h],
        fill=body_color,
        outline="#1a1a1a",
    )

    # Head (circle, overlapping body top)
    head_r = int(w * 0.22)
    head_cy = body_top - int(head_r * 0.3)
    draw.ellipse(
        [cx - head_r, head_cy - head_r, cx + head_r, head_cy + head_r],
        fill=body_color,
        outline="#1a1a1a",
    )

    # Eyes
    eye_r = max(1, int(w * 0.06))
    eye_spacing = int(w * 0.10)
    for ex in [cx - eye_spacing, cx + eye_spacing]:
        draw.ellipse(
            [ex - eye_r, head_cy - eye_r - 1, ex + eye_r, head_cy + eye_r - 1],
            fill=eye_color,
        )
        # Pupil
        pr = max(1, eye_r - 1)
        draw.ellipse(
            [ex - pr + 1, head_cy - pr, ex + pr + 1, head_cy + pr],
            fill="black",
        )

    # Beak
    beak_w = int(w * 0.12)
    beak_h = int(h * 0.08)
    beak_y = head_cy + int(head_r * 0.3)
    draw.polygon(
        [(cx - beak_w, beak_y), (cx + beak_w, beak_y), (cx, beak_y + beak_h)],
        fill=beak_color,
    )

    # Feet
    foot_y = body_top + body_h - 1
    foot_w = int(w * 0.08)
    for fx in [cx - int(w * 0.12), cx + int(w * 0.12)]:
        draw.rectangle([fx - foot_w, foot_y, fx + foot_w, foot_y + int(h * 0.08)],
                        fill=beak_color)


def draw_spiky_enemy(draw, x, y, w, h, body_color, spike_color=None):
    """Draw a spiky enemy silhouette centered in a wxh frame at (x, y)."""
    if spike_color is None:
        spike_color = body_color
    cx, cy = x + w // 2, y + h // 2

    # Main body (circle)
    body_r = int(w * 0.30)
    draw.ellipse(
        [cx - body_r, cy - body_r, cx + body_r, cy + body_r],
        fill=body_color,
        outline="#1a1a1a",
    )

    # Spikes (8 directions)
    import math
    spike_len = int(w * 0.18)
    spike_w = int(w * 0.08)
    for i in range(8):
        angle = i * math.pi / 4
        sx = cx + int(math.cos(angle) * body_r)
        sy = cy + int(math.sin(angle) * body_r)
        ex = cx + int(math.cos(angle) * (body_r + spike_len))
        ey = cy + int(math.sin(angle) * (body_r + spike_len))
        # Draw spike as a triangle
        perp_x = -math.sin(angle) * spike_w
        perp_y = math.cos(angle) * spike_w
        draw.polygon(
            [
                (sx + perp_x, sy + perp_y),
                (sx - perp_x, sy - perp_y),
                (ex, ey),
            ],
            fill=spike_color,
        )

    # Eyes (angry)
    eye_r = max(1, int(w * 0.06))
    eye_spacing = int(w * 0.12)
    for ex_pos in [cx - eye_spacing, cx + eye_spacing]:
        draw.ellipse(
            [ex_pos - eye_r, cy - eye_r - 2, ex_pos + eye_r, cy + eye_r - 2],
            fill="red",
        )


def draw_boss_enemy(draw, x, y, w, h, body_color):
    """Draw a boss enemy with horns and menacing eyes."""
    cx, cy = x + w // 2, y + h // 2

    # Main body (larger oval)
    body_w, body_h = int(w * 0.55), int(h * 0.50)
    body_top = cy - int(body_h * 0.2)
    draw.ellipse(
        [cx - body_w // 2, body_top, cx + body_w // 2, body_top + body_h],
        fill=body_color,
        outline="#1a1a1a",
    )

    # Head
    head_r = int(w * 0.25)
    head_cy = body_top - int(head_r * 0.2)
    draw.ellipse(
        [cx - head_r, head_cy - head_r, cx + head_r, head_cy + head_r],
        fill=body_color,
        outline="#1a1a1a",
    )

    # Horns
    horn_h = int(h * 0.20)
    horn_w = int(w * 0.06)
    for hx, direction in [(cx - int(w * 0.18), -1), (cx + int(w * 0.18), 1)]:
        hy = head_cy - head_r
        draw.polygon(
            [
                (hx - horn_w, hy),
                (hx + horn_w, hy),
                (hx + direction * int(w * 0.06), hy - horn_h),
            ],
            fill="#2a0a0a",
        )

    # Eyes (glowing)
    eye_r = max(2, int(w * 0.07))
    eye_spacing = int(w * 0.12)
    for ex_pos in [cx - eye_spacing, cx + eye_spacing]:
        draw.ellipse(
            [ex_pos - eye_r, head_cy - eye_r, ex_pos + eye_r, head_cy + eye_r],
            fill="#ff2020",
        )
        # Inner glow
        ir = max(1, eye_r - 2)
        draw.ellipse(
            [ex_pos - ir, head_cy - ir, ex_pos + ir, head_cy + ir],
            fill="#ffaa20",
        )


def generate_player_sheet():
    """Generate 32x32 player sprite sheet (6 cols x 9 rows)."""
    cols, rows = 6, 9
    fw, fh = 32, 32
    img = Image.new("RGBA", (cols * fw, rows * fh), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    colors = [
        "#40c040",  # row 0: idle (green)
        "#30b030",  # row 1: walk (slightly darker green)
        "#50d050",  # row 2
        "#40c040",  # row 3
        "#40c040",  # row 4
        "#40c040",  # row 5
        "#40c040",  # row 6
        "#40c040",  # row 7
        "#40c040",  # row 8
    ]

    for row in range(rows):
        # Determine how many frames are populated per row
        if row == 0:
            frame_count = 4  # idle: 4 frames
        elif row == 1:
            frame_count = 6  # walk: 6 frames
        else:
            frame_count = 4  # filler

        for col in range(frame_count):
            x, y = col * fw, row * fh
            # Slight vertical bounce for animation frames
            y_offset = (col % 2) * 2 if row == 1 else 0
            draw_bird(draw, x, y + y_offset, fw, fh, colors[row])

    path = os.path.join(ASSETS, "player.png")
    img.save(path)
    print(f"  player.png: {img.size[0]}x{img.size[1]} ({cols}x{rows} frames @ {fw}x{fh})")


def generate_enemies_mid_sheet():
    """Generate 32x32 mid-tier enemy sprite sheet (3 cols x 2 rows)."""
    cols, rows = 3, 2
    fw, fh = 32, 32
    img = Image.new("RGBA", (cols * fw, rows * fh), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    colors = ["#a030c0", "#8020a0"]  # purple variants per row

    for row in range(rows):
        for col in range(cols):
            x, y = col * fw, row * fh
            draw_spiky_enemy(draw, x, y, fw, fh, colors[row])

    path = os.path.join(ASSETS, "enemies_mid.png")
    img.save(path)
    print(f"  enemies_mid.png: {img.size[0]}x{img.size[1]} ({cols}x{rows} frames @ {fw}x{fh})")


def generate_enemies_boss_sheet():
    """Generate 48x48 boss enemy sprite sheet (3 cols x 2 rows)."""
    cols, rows = 3, 2
    fw, fh = 48, 48
    img = Image.new("RGBA", (cols * fw, rows * fh), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    colors = ["#c02020", "#a01010"]  # red variants per row

    for row in range(rows):
        for col in range(cols):
            x, y = col * fw, row * fh
            draw_boss_enemy(draw, x, y, fw, fh, colors[row])

    path = os.path.join(ASSETS, "enemies_boss.png")
    img.save(path)
    print(f"  enemies_boss.png: {img.size[0]}x{img.size[1]} ({cols}x{rows} frames @ {fw}x{fh})")


if __name__ == "__main__":
    os.makedirs(ASSETS, exist_ok=True)
    print("Generating placeholder sprites...")
    generate_player_sheet()
    generate_enemies_mid_sheet()
    generate_enemies_boss_sheet()
    print("Done.")
