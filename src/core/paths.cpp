#include "core/paths.hpp"

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

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
    // Windows drive letter, e.g. "C:\..." or "C:/..."
    return path.size() >= 2 && path[1] == ':';
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
