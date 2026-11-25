#pragma once

#ifdef RAVEN_ENABLE_SHADER_LIGHTING

#include <string>

namespace raven {

class Renderer;

/// @brief Measure what the shader lighting prototype costs on this machine.
///
/// Draws one room with a fixed set of moving lights, with vsync off, and
/// times whole frames including the GPU's work. Each case is run with the
/// lighting pass once and many times per frame; the difference divided by
/// the extra passes is the cost of one pass, free of everything else in
/// the frame. Prints a table and writes the same numbers as CSV.
/// @param renderer The game's renderer, already initialised.
/// @param csv_path Where to write the results.
/// @return 0 on success, 1 if the lighting pass or the room is unavailable.
int run_lighting_benchmark(Renderer& renderer, const std::string& csv_path);

} // namespace raven

#endif // RAVEN_ENABLE_SHADER_LIGHTING
