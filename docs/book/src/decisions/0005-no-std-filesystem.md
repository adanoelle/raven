# 5. No std::filesystem

Date: 2026-02-08
Status: Accepted

## Context

We intend to port Raven to Nintendo Switch in the future. Console toolchains (particularly Nintendo's) often ship incomplete or non-standard C++ standard library implementations. `std::filesystem` is one of the most commonly missing or broken components.

## Decision

Do not use `std::filesystem` anywhere in the codebase. Instead, use SDL's file I/O functions (`SDL_RWops`) for all file access. For path manipulation, use string operations directly.

This constraint is enforced by convention and code review. A clang-tidy check could be added in the future.

## Consequences

**Positive:**
- Eliminates a major portability obstacle for console toolchains
- SDL's file I/O is well-tested across platforms including Switch
- Forces simpler file access patterns (no recursive directory walking, no symlink resolution)

**Negative:**
- Slightly more verbose file operations compared to `std::filesystem`
- Must implement any needed path utilities manually (dirname, extension splitting)
- Cannot use `std::filesystem` even in tools/scripts that will never run on consoles (consistency trade-off)
