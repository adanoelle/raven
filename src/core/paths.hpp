#pragma once

#include <string>

namespace raven::paths {

/// @brief Resolve a path relative to the executable's directory.
///
/// Asset loads must not depend on the process working directory: Steam,
/// desktop shortcuts, and terminal launches all start the game with
/// arbitrary CWDs. Absolute inputs are returned unchanged. If SDL cannot
/// determine the base path, the input is returned as-is (CWD-relative
/// fallback) and a warning is logged once.
/// @param relative Path relative to the executable, e.g. "assets/x.png".
/// @return Absolute path to the file next to the executable.
[[nodiscard]] std::string asset(const std::string& relative);

/// @brief Directory for user-writable data (settings, saves).
///
/// Uses SDL_GetPrefPath, which creates the directory if needed
/// (e.g. ~/.local/share/adanoelle/raven/ on Linux, %APPDATA% on Windows).
/// @return Directory path ending in a separator, or empty on failure.
[[nodiscard]] std::string pref_dir();

} // namespace raven::paths
