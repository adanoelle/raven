#pragma once

#ifdef RAVEN_ENABLE_SHADER_LIGHTING

#include <string>

namespace raven {

class Renderer;

/// @brief Check that the lighting pass works on this machine's GPU driver.
///
/// Draws one room three ways (as drawn, with ambient light only, and with
/// a plain light and a cool light) and reads the frames back. The pass
/// works when the ambient shift changed the room, the plain light made
/// its surroundings lighter, the cool light made its surroundings bluer,
/// and pixels out of reach of both lights were left alone. Those hold
/// only if the shader loaded and all three of its tables and the light
/// buffer reached it.
///
/// Saves the lit frame as a PNG and prints a checksum of it, to compare
/// between platforms. Meant for CI on machines nobody looks at.
/// @param renderer The game's renderer, already initialised.
/// @param png_path Where to save the lit frame.
/// @return 0 when every check holds, 1 otherwise.
int run_lighting_check(Renderer& renderer, const std::string& png_path);

} // namespace raven

#endif // RAVEN_ENABLE_SHADER_LIGHTING
