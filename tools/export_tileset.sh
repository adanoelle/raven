#!/usr/bin/env bash
# Export the tilesets inside Aseprite files to assets/tilesets/<name>.png.
#
# A source must use Aseprite's tilemap layers. Each tilemap layer becomes
# one row of the sheet, bottom layer first; tile 0 of every row is the empty
# tile. The picture painted with the tiles is not exported, only the tiles.
#
# usage: tools/export_tileset.sh [file.aseprite ...]
#        (default: every art/tiles/*.aseprite)
#
# The sheet takes the source's file name: art/tiles/ruins.aseprite becomes
# assets/tilesets/ruins.png.
set -euo pipefail

if ! command -v aseprite >/dev/null; then
    echo "aseprite is not on PATH" >&2
    exit 1
fi

sources=()
for arg in "$@"; do
    sources+=("$(realpath "$arg")")
done
cd "$(dirname "$0")/.."
if [[ ${#sources[@]} -eq 0 ]]; then
    shopt -s nullglob
    for src in art/tiles/*.aseprite; do
        sources+=("$PWD/$src")
    done
fi
if [[ ${#sources[@]} -eq 0 ]]; then
    echo "no tileset sources in art/tiles/" >&2
    exit 1
fi

for src in "${sources[@]}"; do
    name=$(basename "$src" .aseprite)
    aseprite -b "$src" \
        --script-param out="$PWD/assets/tilesets/$name.png" \
        --script tools/export_tileset.lua
    echo "exported assets/tilesets/$name.png"
done
