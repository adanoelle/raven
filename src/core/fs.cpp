#include "core/fs.hpp"

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

namespace raven::fs {

std::optional<std::string> read_text(const std::string& path) {
    if (path.empty()) {
        return std::nullopt;
    }
    size_t size = 0;
    void* data = SDL_LoadFile(path.c_str(), &size);
    if (!data) {
        return std::nullopt;
    }
    std::string text(static_cast<const char*>(data), size);
    SDL_free(data);
    return text;
}

bool write_text(const std::string& path, std::string_view data) {
    if (path.empty()) {
        return false;
    }
    const std::string tmp = path + ".tmp";
    if (!SDL_SaveFile(tmp.c_str(), data.data(), data.size())) {
        spdlog::warn("Could not write '{}': {}", tmp, SDL_GetError());
        return false;
    }
    if (!SDL_RenamePath(tmp.c_str(), path.c_str())) {
        spdlog::warn("Could not move '{}' into place: {}", tmp, SDL_GetError());
        SDL_RemovePath(tmp.c_str());
        return false;
    }
    return true;
}

} // namespace raven::fs
