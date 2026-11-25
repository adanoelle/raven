#!/usr/bin/env bash
# Export the tilesets inside an Aseprite file to assets/tilesets/<name>.png.
#
# The file must use Aseprite's tilemap layers. Each tilemap layer becomes one
# row of the sheet, bottom layer first; tile 0 of every row is the empty
# tile. The picture painted with the tiles is not exported, only the tiles.
#
# usage: tools/export_tileset.sh file.aseprite name
set -euo pipefail

if [[ $# -ne 2 ]]; then
    echo "usage: tools/export_tileset.sh file.aseprite name" >&2
    exit 2
fi
if ! command -v aseprite >/dev/null; then
    echo "aseprite is not on PATH" >&2
    exit 1
fi

source_file=$(realpath "$1")
cd "$(dirname "$0")/.."
aseprite -b "$source_file" \
    --script-param out="$PWD/assets/tilesets/$2.png" \
    --script tools/export_tileset.lua
