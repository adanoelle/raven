# 20. Bundled Dependency Fallback for Windows Builds

Date: 2026-07-06 Status: Accepted

## Context

Dependency discovery was Linux-shaped: `find_package(SDL3)` against system
packages and `pkg_check_modules` for SDL3_image, with `PkgConfig` marked
`REQUIRED`. On Windows/MSVC none of that exists — no SDL3 distro packages,
usually no pkg-config — so the project simply could not configure on the
platform where most Steam sales happen.

The other dependencies (EnTT, nlohmann_json, spdlog, LDtkLoader, Catch2,
ImGui) already build from source via CPM on every platform; SDL was the only
system-package holdout, kept that way because the Nix dev shell and the
cached CI prefix provide it faster than a source build.

## Decision

**System packages first, CPM source build as fallback.** `find_package(SDL3
3.4 QUIET)` and an optional pkg-config probe run first; if the targets don't
materialise, `CPMAddPackage` builds SDL 3.4.12 / SDL3_image 3.2.4 from
source, pinned to the same versions CI compiles on Linux.
`-DRAVEN_BUNDLED_DEPS=ON` forces the source path even when system packages
exist (used by Windows CI, and useful for reproducing Windows-shaped builds
on Linux).

Windows-specific build fixes ride along:

- Assets are **copied** next to the binary instead of symlinked (symlinks
  need developer mode on Windows), and `$<TARGET_RUNTIME_DLLS>` post-build
  steps place SDL3.dll et al. beside both the game and test executables.
- MSVC gets real warning flags (`/W4 /permissive- /Zc:__cplusplus`) and
  `/utf-8` — sources contain UTF-8 punctuation that MSVC would otherwise
  misread under a system codepage.
- CI gains a `windows-latest` job: MSVC + bundled deps + full test suite.

## Consequences

**Positive:**

- `cmake -B build` works on a bare Windows machine with only Visual Studio
  installed; no vcpkg, MSYS2, or manual SDK downloads
- Linux dev-shell and CI paths are unchanged (system packages still win)
- One pinned SDL version everywhere; version skew between platforms is
  impossible to miss

**Negative:**

- Cold Windows CI builds compile SDL from source (~minutes); CPM source
  caching softens repeats but object files rebuild per run
- The fallback is a second configure path to keep working — exercised
  continuously by the Windows CI job, and locally via `RAVEN_BUNDLED_DEPS=ON`
