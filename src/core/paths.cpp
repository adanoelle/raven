#include "core/paths.hpp"

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <cctype>

namespace raven::paths {

namespace {

/// @brief Cached executable directory, resolved once. Empty if unavailable.
const std::string& base_dir() {
    static const std::string base = [] {
        const char* p = SDL_GetBasePath();
        if (!p) {
            spdlog::warn("SDL_GetBasePath failed ({}) — asset paths fall back "
                         "to the working directory",
                         SDL_GetError());
            return std::string{};
        }
        return std::string{p};
    }();
    return base;
}

/// @brief Check whether a path is already absolute (POSIX or Windows).
bool is_absolute(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    if (path[0] == '/' || path[0] == '\\') {
        return true;
    }
    // A "scheme:" prefix marks an already-resolved path: a Windows drive
    // letter ("C:\...") or a console mount such as "romfs:/" or "sdmc:/".
    // Only alphanumerics may precede the colon.
    const auto colon = path.find(':');
    if (colon == std::string::npos || colon == 0) {
        return false;
    }
    for (size_t i = 0; i < colon; ++i) {
        if (!std::isalnum(static_cast<unsigned char>(path[i]))) {
            return false;
        }
    }
    return true;
}

} // anonymous namespace

std::string asset(const std::string& relative) {
    if (is_absolute(relative)) {
        return relative;
    }
    return base_dir() + relative;
}

std::string pref_dir() {
    static const std::string pref = [] {
        char* p = SDL_GetPrefPath("adanoelle", "raven");
        if (!p) {
            spdlog::error("SDL_GetPrefPath failed ({}) — settings will not persist",
                          SDL_GetError());
            return std::string{};
        }
        std::string result{p};
        SDL_free(p);
        return result;
    }();
    return pref;
}

} // namespace raven::paths
