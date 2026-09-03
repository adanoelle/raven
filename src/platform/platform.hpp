#pragma once

/// @brief Compile-time platform capabilities.
///
/// Desktop is the default. Configure with -DRAVEN_PLATFORM_CONSOLE=ON for
/// fixed-display targets (Switch), which have no window to scale, no
/// fullscreen toggle, and always vsync.
namespace raven::platform {

#ifdef RAVEN_PLATFORM_CONSOLE
/// @brief Whether the options menu should expose window-related settings.
inline constexpr bool HAS_WINDOW_SETTINGS = false;
#else
/// @brief Whether the options menu should expose window-related settings.
inline constexpr bool HAS_WINDOW_SETTINGS = true;
#endif

} // namespace raven::platform
