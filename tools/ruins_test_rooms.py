#!/usr/bin/env python3
"""Repaint the rooms of assets/maps/raven.ldtk with the ruins tileset.

    python3 tools/ruins_test_rooms.py

A test, so the artist's tiles can be seen in the game before anyone lays
out a room by hand. It keeps each room's shape, collision and entities, and
replaces only what is drawn:

- every walkable cell gets a stone floor tile, with a few 2x2 large slabs;
- a solid cell directly below a floor cell gets the cliff face;
- every other solid cell is left empty (the void);
- moss goes on a second tile layer, "Overlay", mostly along the edges.

The choice of tiles is random with a fixed seed, so a rerun gives the same
rooms. Rooms painted this way are not a level design.

The tile numbers below are for assets/tilesets/ruins.png as exported by
tools/export_tileset.sh: row 0 is stone, row 1 is moss.
"""

import copy
import json
import random
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAP = ROOT / "assets" / "maps" / "raven.ldtk"

TILE = 16
COLUMNS = 11  # tiles per row of ruins.png
ROWS = 2
TILESET_PATH = "../tilesets/ruins.png"

# Stone, row 0.
PLAIN, CLIFF, RUNE = 1, 2, 6
CRACKED = [3, 4, 5]
BIG_SLAB = [[8, 9], [10, 7]]  # a 2x2 slab: top row, bottom row
# Moss, row 1 (numbers within the row).
MOSS_EDGE = [1, 2, 3, 4, 5]
MOSS_INNER = [3, 4, 5]
MOSS_DRIP = 6


def placed(cell_x, cell_y, tile, width):
    return {
        "px": [cell_x * TILE, cell_y * TILE],
        "src": [(tile % COLUMNS) * TILE, (tile // COLUMNS) * TILE],
        "f": 0,
        "t": tile,
        "d": [cell_y * width + cell_x],
        "a": 1,
    }


def paint(level, rng):
    layers = {layer["__identifier"]: layer for layer in level["layerInstances"]}
    collision = layers["Collision"]
    width, height = collision["__cWid"], collision["__cHei"]
    solid = collision["intGridCsv"]

    def is_floor(x, y):
        return 0 <= x < width and 0 <= y < height and solid[y * width + x] == 0

    stone = {}
    # Large slabs first, where all four cells are free floor.
    for _ in range(width * height // 40):
        x, y = rng.randrange(width - 1), rng.randrange(height - 1)
        cells = [(x + dx, y + dy) for dy in range(2) for dx in range(2)]
        if all(is_floor(cx, cy) and (cx, cy) not in stone for cx, cy in cells):
            for cx, cy in cells:
                stone[(cx, cy)] = BIG_SLAB[cy - y][cx - x]
    for y in range(height):
        for x in range(width):
            if is_floor(x, y):
                if (x, y) not in stone:
                    roll = rng.random()
                    stone[(x, y)] = (
                        PLAIN if roll < 0.5 else RUNE if roll < 0.57 else rng.choice(CRACKED)
                    )
            elif is_floor(x, y - 1):
                stone[(x, y)] = CLIFF

    moss = {}
    for (x, y), tile in stone.items():
        if tile == CLIFF:
            if rng.random() < 0.2:
                moss[(x, y)] = MOSS_DRIP
            continue
        at_edge = not all(
            is_floor(x + dx, y + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))
        )
        if at_edge and rng.random() < 0.3:
            moss[(x, y)] = rng.choice(MOSS_EDGE)
        elif not at_edge and rng.random() < 0.03:
            moss[(x, y)] = rng.choice(MOSS_INNER)

    layers["Tiles"]["gridTiles"] = [placed(x, y, t, width) for (x, y), t in sorted(stone.items(), key=lambda c: (c[0][1], c[0][0]))]
    layers["Overlay"]["gridTiles"] = [
        placed(x, y, COLUMNS + t, width) for (x, y), t in sorted(moss.items(), key=lambda c: (c[0][1], c[0][0]))
    ]
    return len(stone), len(moss)


def main():
    project = json.loads(MAP.read_text(encoding="utf-8"))
    defs = project["defs"]

    tileset = defs["tilesets"][0]
    tileset.update(
        identifier="Ruins",
        relPath=TILESET_PATH,
        pxWid=COLUMNS * TILE,
        pxHei=ROWS * TILE,
        __cWid=COLUMNS,
        __cHei=ROWS,
    )

    # A second tile layer for moss, drawn above "Tiles".
    layer_defs = defs["layers"]
    tiles_def = next(d for d in layer_defs if d["identifier"] == "Tiles")
    overlay_def = next((d for d in layer_defs if d["identifier"] == "Overlay"), None)
    if overlay_def is None:
        overlay_def = copy.deepcopy(tiles_def)
        overlay_def["identifier"] = "Overlay"
        overlay_def["uid"] = project["nextUid"]
        project["nextUid"] += 1
        layer_defs.insert(layer_defs.index(tiles_def), overlay_def)

    rng = random.Random(28)
    for number, level in enumerate(project["levels"]):
        instances = level["layerInstances"]
        tiles = next(i for i in instances if i["__identifier"] == "Tiles")
        tiles["__tilesetRelPath"] = TILESET_PATH
        if not any(i["__identifier"] == "Overlay" for i in instances):
            overlay = copy.deepcopy(tiles)
            overlay["__identifier"] = "Overlay"
            overlay["layerDefUid"] = overlay_def["uid"]
            overlay["iid"] = f"c00000{number + 1:02d}-0000-0000-0000-0000000000a0"
            instances.insert(instances.index(tiles), overlay)
        stone, moss = paint(level, rng)
        print(f"{level['identifier']}: {stone} stone tiles, {moss} moss tiles")

    MAP.write_text(json.dumps(project, indent="\t") + "\n", encoding="utf-8")
    print(f"wrote {MAP}")


if __name__ == "__main__":
    main()
