#include "platform/steam.hpp"

#include <spdlog/spdlog.h>

#ifdef RAVEN_ENABLE_STEAM
#include <steam/steam_api.h>
#endif

namespace raven {

bool Steam::init() {
#ifdef RAVEN_ENABLE_STEAM
    if (!SteamAPI_Init()) {
        spdlog::warn("SteamAPI_Init failed (is Steam running, and is "
                     "steam_appid.txt present for dev builds?) — running without Steam");
        return false;
    }
    active_ = true;
    spdlog::info("Steamworks initialised");
    return true;
#else
    spdlog::debug("Built without Steamworks support");
    return false;
#endif
}

void Steam::shutdown() {
#ifdef RAVEN_ENABLE_STEAM
    if (active_) {
        SteamAPI_Shutdown();
        active_ = false;
        spdlog::info("Steamworks shut down");
    }
#endif
}

void Steam::run_callbacks() {
#ifdef RAVEN_ENABLE_STEAM
    if (active_) {
        SteamAPI_RunCallbacks();
    }
#endif
}

} // namespace raven
