#pragma once

namespace raven {

/// @brief Optional Steamworks integration.
///
/// Compiled in only when RAVEN_ENABLE_STEAM is set (requires the
/// Steamworks SDK under vendor/steamworks/sdk — see cmake/Steamworks.cmake).
/// Without it, every method is a no-op and is_active() stays false, so
/// call sites never need #ifdefs. Failure to attach to Steam at runtime
/// (SDK present but Steam not running) is also non-fatal: the game runs
/// without Steam features.
class Steam {
  public:
    /// @brief Attach to Steam if built with Steamworks support.
    ///
    /// During development this requires a steam_appid.txt next to the
    /// binary (not shipped); under a Steam launch it attaches directly.
    /// @return True if the Steam API initialised.
    bool init();

    /// @brief Detach from Steam. Safe to call when inactive.
    void shutdown();

    /// @brief Pump Steam callbacks. Call once per frame; no-op when inactive.
    void run_callbacks();

    /// @brief Whether the Steam API is attached and callbacks are live.
    /// @return True between a successful init() and shutdown().
    [[nodiscard]] bool is_active() const { return active_; }

  private:
    bool active_ = false;
};

} // namespace raven
