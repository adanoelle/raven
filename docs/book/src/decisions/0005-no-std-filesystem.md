# 5. No std::filesystem

Date: 2026-02-08 Status: Accepted

## Context

We intend to port Raven to Nintendo Switch in the future. Console toolchains
(particularly Nintendo's) often ship incomplete or non-standard C++ standard
library implementations. `std::filesystem` is one of the most commonly missing
or broken components.

## Decision

Do not use `std::filesystem` anywhere in the codebase. Instead, use SDL's file
I/O functions (`SDL_IOStream`, `SDL_LoadFile`, `SDL_SaveFile`) for all file
access. For path manipulation, use string operations directly.

This constraint is enforced by convention and code review. A clang-tidy check
could be added in the future.

### Amendment 2026-09-02

The Switch-readiness review found that the rule had been honoured in letter
(no `std::filesystem`) but not in spirit: settings, save data, `config.json`,
stage and pattern manifests, and the LDtk loader all opened files with
`std::ifstream` / `std::ofstream` directly. Six call sites plus one library is
exactly the surface a port would have to chase.

The decision is therefore tightened:

- **All file reads and writes go through `raven::fs`** (`src/core/fs.hpp`):
  `fs::read_text` and `fs::write_text`, built on `SDL_LoadFile` and
  `SDL_SaveFile`.
- **Direct `<fstream>` use in `src/` is not allowed.** Tests may still use it
  to set up fixtures.
- **Writes are atomic**: `write_text` writes to a temporary sibling and
  `SDL_RenamePath`s it over the target, so a crash mid-write leaves the old
  file, not a truncated one.
- **LDtk projects load through the library's `FileLoader` hook**, so tilemap
  loading also goes through the seam rather than the library's own
  `std::ifstream`.

A Switch port then swaps only `fs.cpp`: asset reads come from the ROM
filesystem, and settings/save writes route through the platform save-data
API. Nothing above the seam changes.

## Consequences

**Positive:**

- Eliminates a major portability obstacle for console toolchains
- SDL's file I/O is well-tested across platforms including Switch
- Forces simpler file access patterns (no recursive directory walking, no
  symlink resolution)

**Negative:**

- Slightly more verbose file operations compared to `std::filesystem`
- Must implement any needed path utilities manually (dirname, extension
  splitting)
- Cannot use `std::filesystem` even in tools/scripts that will never run on
  consoles (consistency trade-off)
