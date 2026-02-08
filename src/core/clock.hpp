#pragma once

#include <cstdint>

namespace raven {

/// Fixed timestep game clock with interpolation support.
/// Tick rate of 120Hz for precise bullet movement.
struct Clock {
    static constexpr float TICK_RATE = 1.f / 120.f;
    static constexpr int MAX_STEPS_PER_FRAME = 4; // prevent spiral of death

    float accumulator = 0.f;
    float interpolation_alpha = 0.f;
    uint64_t tick_count = 0;

    /// Feed raw frame delta. Returns number of fixed steps to execute.
    int advance(float frame_delta_seconds) {
        // Clamp to prevent huge deltas (e.g., after breakpoint)
        if (frame_delta_seconds > 0.25f) {
            frame_delta_seconds = 0.25f;
        }

        accumulator += frame_delta_seconds;

        int steps = 0;
        while (accumulator >= TICK_RATE && steps < MAX_STEPS_PER_FRAME) {
            accumulator -= TICK_RATE;
            ++steps;
            ++tick_count;
        }

        // Compute interpolation alpha for rendering between ticks
        interpolation_alpha = accumulator / TICK_RATE;

        return steps;
    }
};

} // namespace raven
