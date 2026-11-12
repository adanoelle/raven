#!/usr/bin/env bash
# Choose which sources clang-tidy needs to check for a set of changed files.
#
# Reads changed paths on stdin, one per line, and prints the sources to lint,
# NUL-separated (for xargs -0). A change that can affect files other than
# itself (a header, the lint or build configuration) selects every source.
# Prints nothing when no source needs checking.
#
# usage: git diff --name-only main... | tools/lint_files.sh | xargs -0 clang-tidy -p build
set -euo pipefail
cd "$(dirname "$0")/.."

all=false
changed=()
while IFS= read -r path; do
    case "$path" in
    src/*.hpp | .clang-tidy | CMakeLists.txt | cmake/* | .github/workflows/ci.yml | tools/lint_files.sh)
        all=true
        ;;
    src/*.cpp)
        # A deleted or renamed-away file is in the list but no longer exists
        [[ -f "$path" ]] && changed+=("$path")
        ;;
    esac
done

if $all; then
    find src -name '*.cpp' -print0
elif [[ ${#changed[@]} -gt 0 ]]; then
    printf '%s\0' "${changed[@]}"
fi
