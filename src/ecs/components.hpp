#pragma once

#include <entt/entt.hpp>

#include <cstdint>
#include <string>

namespace raven {

// ── Spatial ──────────────────────────────────────────────────────

/// @brief 2D position and rotation in world space.
struct Transform2D {
    float x = 0.f;        ///< World X position in pixels.
    float y = 0.f;        ///< World Y position in pixels.
    float rotation = 0.f; ///< Rotation in radians.
};

/// @brief Linear velocity vector in pixels per second.
struct Velocity {
    float dx = 0.f; ///< Horizontal speed in pixels/s.
    float dy = 0.f; ///< Vertical speed in pixels/s.
};

// ── Interpolation ───────────────────────────────────────────────

/// @brief Stores the previous tick's position for render interpolation.
struct PreviousTransform {
    float x = 0.f; ///< Previous X position in pixels.
    float y = 0.f; ///< Previous Y position in pixels.
};

// ── Rendering ────────────────────────────────────────────────────

/// @brief Sprite rendering data referencing a frame within a sprite sheet.
struct Sprite {
    std::string sheet_id; ///< Identifier of the SpriteSheet to draw from.
    int frame_x = 0;      ///< Frame column index in the sheet.
    int frame_y = 0;      ///< Frame row index in the sheet.
    int width = 16;       ///< Pixel width of one frame.
    int height = 16;      ///< Pixel height of one frame.
    int layer = 0;        ///< Render order (higher values draw on top).
    bool flip_x = false;  ///< Flip the sprite horizontally when drawing.
};

/// @brief Frame-based animation state for cycling through sprite frames.
struct Animation {
    int start_frame = 0;         ///< First frame index in the animation.
    int end_frame = 0;           ///< Last frame index in the animation.
    float frame_duration = 0.1f; ///< Seconds per frame.
    float elapsed = 0.f;         ///< Time elapsed in the current frame.
    int current_frame = 0;       ///< Currently displayed frame index.
    bool looping = true;         ///< Whether the animation loops or stops at end.
};

// ── Collision ────────────────────────────────────────────────────

/// @brief Circular hitbox for collision detection (circle-circle checks).
struct CircleHitbox {
    float radius = 6.f;   ///< Hitbox radius in pixels.
    float offset_x = 0.f; ///< X offset from the entity's Transform2D position.
    float offset_y = 0.f; ///< Y offset from the entity's Transform2D position.
};

/// @brief Axis-aligned rectangular hitbox for collision detection.
struct RectHitbox {
    float width = 16.f;   ///< Hitbox width in pixels.
    float height = 16.f;  ///< Hitbox height in pixels.
    float offset_x = 0.f; ///< X offset from the entity's Transform2D position.
    float offset_y = 0.f; ///< Y offset from the entity's Transform2D position.
};

// ── Gameplay ─────────────────────────────────────────────────────

/// @brief Marks an entity as the player and stores player-specific stats.
struct Player {
    float speed = 200.f; ///< Movement speed in pixels per second.
    int lives = 3;       ///< Remaining lives.
};

/// @brief Marks an entity as an enemy with a type classification.
struct Enemy {
    /// @brief Enemy tier affecting behaviour and scoring.
    enum class Type : uint8_t {
        Grunt, ///< Basic enemy.
        Mid,   ///< Mid-tier enemy.
        Boss   ///< Boss enemy.
    };
    Type type = Type::Grunt; ///< This enemy's type.
};

/// @brief Hit points for damageable entities.
struct Health {
    float current = 1.f; ///< Current hit points.
    float max = 1.f;     ///< Maximum hit points.
};

/// @brief Marks an entity as a projectile with ownership tracking.
struct Bullet {
    /// @brief Which side fired this bullet (determines collision rules).
    enum class Owner : uint8_t {
        Player, ///< Fired by the player.
        Enemy   ///< Fired by an enemy.
    };
    Owner owner = Owner::Enemy; ///< Who fired this bullet.
};

/// @brief Remaining lifetime before automatic despawn.
struct Lifetime {
    float remaining = 5.f; ///< Seconds until the entity is destroyed.
};

/// @brief Deals damage on collision with an applicable target.
struct DamageOnContact {
    float damage = 1.f; ///< Amount of damage dealt on contact.
};

/// @brief Temporary invulnerability (e.g. after taking a hit).
struct Invulnerable {
    float remaining = 0.f; ///< Seconds of invulnerability left.
};

/// @brief Score value awarded when this entity is destroyed.
struct ScoreValue {
    int points = 100; ///< Points awarded to the player on kill.
};

// ── Animation State ─────────────────────────────────────────────

/// @brief Tracks the current animation state to avoid redundant transitions.
struct AnimationState {
    /// @brief Animation state for state-switching logic.
    enum class State : uint8_t {
        Idle, ///< Standing still / idle animation.
        Walk  ///< Moving / walk animation.
    };
    State current = State::Idle; ///< The active animation state.
};

// ── Aiming / Shooting ───────────────────────────────────────────

/// @brief Resolved aim direction for shooting. Updated each tick from input.
struct AimDirection {
    float x = 1.f; ///< Aim X component (unit vector).
    float y = 0.f; ///< Aim Y component (unit vector).
};

/// @brief Cooldown timer for shooting. Prevents firing every tick.
struct ShootCooldown {
    float remaining = 0.f; ///< Time until next shot allowed (seconds).
    float rate = 0.2f;     ///< Minimum interval between shots (seconds).
};

// ── Tags (empty structs for filtering) ───────────────────────────

/// @brief Tag: entity is removed when it leaves the play area.
struct OffScreenDespawn {};

} // namespace raven
