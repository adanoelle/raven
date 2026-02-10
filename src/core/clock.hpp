#pragma once

#include <cstdint>

namespace raven {

/// @brief Fixed timestep game clock with interpolation support.
///
/// Uses a 120 Hz tick rate for precise physics and bullet movement.
/// An accumulator pattern prevents the spiral-of-death by capping the
/// maximum number of steps per frame.
struct Clock {
    static constexpr float TICK_RATE = 1.f / 120.f; ///< Seconds per fixed tick (1/120).
    static constexpr int MAX_STEPS_PER_FRAME = 4;   ///< Cap to prevent spiral of death.

    float accumulator = 0.f;         ///< Unprocessed time carried across frames.
    float interpolation_alpha = 0.f; ///< Blend factor [0,1] for rendering between ticks.
    uint64_t tick_count = 0;         ///< Total fixed ticks since start.

    /// @brief Feed a raw frame delta and compute how many fixed steps to run.
    /// @param frame_delta_seconds Wall-clock time since the last frame, in seconds.
    /// @return Number of fixed-timestep updates to execute this frame.
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

        // If we hit the step cap, drain excess accumulator to prevent
        // interpolation_alpha from exceeding 1.0 (which turns interpolation
        // into extrapolation, flinging rendered positions off-screen).
        if (steps >= MAX_STEPS_PER_FRAME && accumulator > TICK_RATE) {
            accumulator = 0.f;
        }

        // Compute interpolation alpha for rendering between ticks
        interpolation_alpha = accumulator / TICK_RATE;

        return steps;
    }
};

} // namespace raven
