#!/usr/bin/env bash
# Export Aseprite sources to sprite sheets and animation data.
#
# Every art/**/<name>.aseprite becomes assets/sprites/<name>.png (one row
# per frame tag) and assets/sprites/<name>.json (frame rects, durations and
# tags, read by the game's AnimationLibrary). Templates and sketches are
# skipped, and a layer named "guides" is never exported.
#
# usage: tools/export_art.sh [file.aseprite ...]   (default: everything under art/)
set -euo pipefail
cd "$(dirname "$0")/.."

if ! command -v aseprite >/dev/null; then
    echo "aseprite is not on PATH" >&2
    exit 1
fi

shopt -s globstar nullglob
sources=("$@")
if [[ ${#sources[@]} -eq 0 ]]; then
    for src in art/**/*.aseprite; do
        case "$src" in
        art/templates/* | */sketches/*) continue ;;
        esac
        sources+=("$src")
    done
fi

for src in "${sources[@]}"; do
    name=$(basename "$src" .aseprite)
    aseprite -b "$src" \
        --ignore-layer guides \
        --sheet "assets/sprites/$name.png" \
        --sheet-type rows \
        --split-tags \
        --list-tags \
        --format json-array \
        --data "assets/sprites/$name.json"
    echo "  $src -> assets/sprites/$name.png, $name.json"
done
