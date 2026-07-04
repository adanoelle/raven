# 16. Executable-Relative Asset Paths

Date: 2026-07-03 Status: Accepted

## Context

Every asset load used a literal CWD-relative path (`"assets/data/config.json"`,
`"assets/maps/raven.ldtk"`, ...), including paths listed *inside* the pattern
and stage manifests. That works when launching from the repo root (`just run`)
and breaks everywhere else: Steam launches games with an arbitrary working
directory, as do desktop shortcuts and terminals.

Because every loader degrades gracefully (warn and continue), the failure mode
was silent and total: no sprites, no stages, no tilemap — the player spawns in
an empty void with no waves and no exits. A softlock instead of an error.

The install layout had the same disease from the other side: CMake installed
the binary to `bin/` and assets to `share/raven/assets`, a location the
runtime lookup would never find.

`std::filesystem` is banned for portability ([ADR-0005](0005-no-std-filesystem.md)),
so the fix needed to be plain string handling over an SDL primitive.

## Decision

**All asset paths resolve relative to the executable's directory via
`paths::asset()`** (`src/core/paths.hpp`), which prefixes relative paths with
a cached `SDL_GetBasePath()`. Absolute inputs pass through unchanged; if SDL
cannot determine the base path, the input is returned as-is (CWD fallback)
with a one-time warning.

This applies at every file-open site: `config.json`, sprite sheet paths from
config, both manifests, per-file paths listed inside the manifests, and the
LDtk project. Manifest entries stay written as `assets/...` — they are
install-dir-relative by convention, anchored at load time.

The install/package layout now places `assets/` next to the binary
(`install(TARGETS raven RUNTIME DESTINATION .)`), matching the runtime lookup.
The development build keeps working because the build already symlinks
`assets/` into `build/bin/`.

## Consequences

**Positive:**

- The game runs correctly from any working directory — Steam, shortcuts,
  terminals, double-click
- Packages produced by CPack are actually runnable
- One helper (`paths::asset`) is the single place path policy lives; future
  loaders (fonts, audio) inherit it for free

**Negative:**

- Loaders that open manifest-listed files must remember to anchor
  (`paths::asset(entry)`) — a convention, not a compiler-enforced rule
- Tests and tools that construct loaders directly must pass absolute or
  CWD-valid paths (they already did)
