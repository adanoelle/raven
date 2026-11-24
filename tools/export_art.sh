#!/usr/bin/env bash
# Export Aseprite sources to sprite sheets and animation data.
#
# Every art/**/<name>.aseprite becomes assets/sprites/<name>.png (one row
# per frame tag) and assets/sprites/<name>.json (frame rects, durations and
# tags, read by the game's AnimationLibrary). Templates and sketches are
# skipped, and a layer named "guides" is never exported.
#
# All sheets share one directory, so two sources with the same file name
# would write the same sheet: that is an error, and nothing is exported.
#
# assets/sprites/.exported lists the sheets this script made and the source
# of each. A sheet that is not on the list (a hand-made placeholder, say) is
# not replaced unless --force is given. A full export also reports listed
# sheets whose source is gone; --prune deletes them.
#
# usage: tools/export_art.sh [--force] [--prune] [file.aseprite ...]
#        (default: everything under art/)
set -euo pipefail
cd "$(dirname "$0")/.."

out_dir=assets/sprites
manifest=$out_dir/.exported

force=0
prune=0
sources=()
for arg in "$@"; do
    case "$arg" in
    --force) force=1 ;;
    --prune) prune=1 ;;
    -*)
        echo "unknown option: $arg" >&2
        exit 2
        ;;
    *) sources+=("$arg") ;;
    esac
done

if ! command -v aseprite >/dev/null; then
    echo "aseprite is not on PATH" >&2
    exit 1
fi

shopt -s globstar nullglob
full_export=0
if [[ ${#sources[@]} -eq 0 ]]; then
    full_export=1
    for src in art/**/*.aseprite; do
        case "$src" in
        art/templates/* | */sketches/*) continue ;;
        esac
        sources+=("$src")
    done
fi

# What earlier runs exported: sheet name -> source
declare -A exported=()
if [[ -f $manifest ]]; then
    while IFS=$'\t' read -r name src; do
        [[ -n $name ]] && exported[$name]=$src
    done <"$manifest"
fi

# Check everything before writing anything
declare -A claimed=()
problems=0
for src in "${sources[@]}"; do
    name=$(basename "$src" .aseprite)
    if [[ ! -f $src ]]; then
        echo "error: $src does not exist" >&2
        problems=1
        continue
    fi
    if [[ -n ${claimed[$name]:-} ]]; then
        echo "error: ${claimed[$name]} and $src would both export $out_dir/$name.png;" \
            "rename one of them" >&2
        problems=1
        continue
    fi
    claimed[$name]=$src
    if [[ -n ${exported[$name]:-} && ${exported[$name]} != "$src" && -f ${exported[$name]} ]]; then
        echo "error: $out_dir/$name.png is exported from ${exported[$name]}, and $src" \
            "would replace it; rename one of them" >&2
        problems=1
    elif [[ -z ${exported[$name]:-} && $force -eq 0 ]] &&
        [[ -e $out_dir/$name.png || -e $out_dir/$name.json ]]; then
        echo "error: $out_dir/$name.png or .json exists and was not made by this script;" \
            "$src would replace it. Pass --force if that is what you want" >&2
        problems=1
    fi
done
if [[ $problems -ne 0 ]]; then
    echo "nothing was exported" >&2
    exit 1
fi

for src in "${sources[@]}"; do
    name=$(basename "$src" .aseprite)
    aseprite -b "$src" \
        --ignore-layer guides \
        --sheet "$out_dir/$name.png" \
        --sheet-type rows \
        --split-tags \
        --list-tags \
        --format json-array \
        --data "$out_dir/$name.json"
    exported[$name]=$src
    echo "  $src -> $out_dir/$name.png, $name.json"
done

# Sheets whose source has gone. Only a full export looks: a run on named
# files says nothing about the rest.
if [[ $full_export -eq 1 ]]; then
    for name in "${!exported[@]}"; do
        if [[ ! -f ${exported[$name]} ]]; then
            if [[ $prune -eq 1 ]]; then
                rm -f "$out_dir/$name.png" "$out_dir/$name.json"
                echo "  removed $out_dir/$name.png, $name.json (${exported[$name]} is gone)"
                unset "exported[$name]"
            else
                echo "  stale: $out_dir/$name.png, $name.json (${exported[$name]} is gone);" \
                    "run with --prune to delete them"
            fi
        fi
    done
fi

{
    for name in "${!exported[@]}"; do
        printf '%s\t%s\n' "$name" "${exported[$name]}"
    done
} | sort >"$manifest"
