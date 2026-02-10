#pragma once

namespace raven::systems {

/// @brief Test whether two circles overlap (inclusive of touching).
/// @param x1 Centre X of first circle.
/// @param y1 Centre Y of first circle.
/// @param r1 Radius of first circle.
/// @param x2 Centre X of second circle.
/// @param y2 Centre Y of second circle.
/// @param r2 Radius of second circle.
/// @return true if the circles overlap or are exactly tangent.
[[nodiscard]] inline bool circles_overlap(float x1, float y1, float r1, float x2, float y2,
                                          float r2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dist_sq = dx * dx + dy * dy;
    float radii = r1 + r2;
    return dist_sq <= radii * radii;
}

} // namespace raven::systems
