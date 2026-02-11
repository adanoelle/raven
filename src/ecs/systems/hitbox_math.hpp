#pragma once

#include <cmath>

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

/// @brief Test whether a target point lies inside a directional cone.
/// @param origin_x Centre X of the cone origin.
/// @param origin_y Centre Y of the cone origin.
/// @param aim_x Aim direction X (does not need to be normalised).
/// @param aim_y Aim direction Y (does not need to be normalised).
/// @param target_x Target point X.
/// @param target_y Target point Y.
/// @param range Maximum reach of the cone in pixels.
/// @param half_angle Half-angle of the cone in radians.
/// @return true if the target is within range and inside the cone angle.
[[nodiscard]] inline bool point_in_cone(float origin_x, float origin_y, float aim_x, float aim_y,
                                        float target_x, float target_y, float range,
                                        float half_angle) {
    float dx = target_x - origin_x;
    float dy = target_y - origin_y;
    float dist_sq = dx * dx + dy * dy;

    if (dist_sq > range * range) {
        return false;
    }
    if (dist_sq < 0.0001f) {
        return true; // target at origin is always inside
    }

    // Normalise aim direction
    float aim_len_sq = aim_x * aim_x + aim_y * aim_y;
    if (aim_len_sq < 0.0001f) {
        return false; // degenerate aim
    }
    float inv_aim_len = 1.f / std::sqrt(aim_len_sq);
    float aim_nx = aim_x * inv_aim_len;
    float aim_ny = aim_y * inv_aim_len;

    // Normalise direction to target
    float inv_dist = 1.f / std::sqrt(dist_sq);
    float dir_x = dx * inv_dist;
    float dir_y = dy * inv_dist;

    float dot = aim_nx * dir_x + aim_ny * dir_y;
    return dot >= std::cos(half_angle);
}

} // namespace raven::systems
