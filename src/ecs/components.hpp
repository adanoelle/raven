#pragma once

#include "core/string_id.hpp"

#include <entt/entt.hpp>

#include <cstdint>
#include <vector>

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
    StringId sheet_id;   ///< Interned identifier of the SpriteSheet to draw from.
    int frame_x = 0;     ///< Frame column index in the sheet.
    int frame_y = 0;     ///< Frame row index in the sheet.
    int width = 16;      ///< Pixel width of one frame.
    int height = 16;     ///< Pixel height of one frame.
    int layer = 0;       ///< Render order (higher values draw on top).
    bool flip_x = false; ///< Flip the sprite horizontally when drawing.
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
        Idle,  ///< Standing still / idle animation.
        Walk,  ///< Moving / walk animation.
        Melee, ///< Melee attack animation.
        Dash   ///< Dash animation.
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

// ── Weapon ──────────────────────────────────────────────────────

/// @brief Weapon stats controlling bullet properties and fire behavior.
struct Weapon {
    /// @brief Weapon rarity tier affecting stabilization rules.
    enum class Tier : uint8_t {
        Common,   ///< Basic drops. Can be stabilized.
        Rare,     ///< Mid-tier drops. Can be stabilized.
        Legendary ///< Top-tier drops. Cannot be stabilized.
    };

    Tier tier = Tier::Common;    ///< This weapon's rarity tier.
    float bullet_speed = 300.f;  ///< Bullet travel speed in pixels/sec.
    float bullet_damage = 1.f;   ///< Damage dealt per bullet on contact.
    float bullet_lifetime = 3.f; ///< Bullet lifetime in seconds.
    float bullet_hitbox = 2.f;   ///< Bullet collision radius in pixels.
    float fire_rate = 0.2f;      ///< Minimum interval between shots (seconds).
    int bullet_count = 1;        ///< Bullets per shot.
    float spread_angle = 0.f;    ///< Arc width in degrees (0 = single line).
    StringId bullet_sheet;       ///< Interned sprite sheet ID for bullets.
    int bullet_frame_x = 1;      ///< Frame column in the sheet.
    int bullet_frame_y = 0;      ///< Frame row in the sheet.
    int bullet_width = 8;        ///< Pixel width of bullet frame.
    int bullet_height = 8;       ///< Pixel height of bullet frame.
    bool piercing = false;       ///< Whether bullets pass through targets.
};

// ── Emitter ─────────────────────────────────────────────────────

/// @brief Drives a bullet pattern from the pattern library on an entity.
struct BulletEmitter {
    StringId pattern_name;             ///< Interned name of the PatternDef to execute.
    std::vector<float> cooldowns;      ///< Per-emitter cooldown timers (seconds).
    std::vector<float> current_angles; ///< Per-emitter current rotation angles (degrees).
    bool active = true;                ///< Whether this emitter is currently firing.
};

// ── Pickup / Decay ──────────────────────────────────────────────

/// @brief Marks an entity as a weapon pickup that grants its weapon on collection.
struct WeaponPickup {
    Weapon weapon; ///< The weapon stats to give the player.
};

/// @brief Timer for a temporary stolen weapon. Reverts to default when expired.
struct WeaponDecay {
    float remaining = 10.f; ///< Seconds until stolen weapon expires.
};

/// @brief Stores the player's base weapon to revert to after WeaponDecay expires.
struct DefaultWeapon {
    Weapon weapon; ///< The player's original weapon.
};

// ── Enemy AI ────────────────────────────────────────────────────

/// @brief AI behavior configuration for enemy entities.
struct AiBehavior {
    /// @brief Enemy movement archetype.
    enum class Archetype : uint8_t { Chaser, Drifter, Stalker, Coward };

    /// @brief Current behavioral phase (simple state machine).
    enum class Phase : uint8_t { Idle, Advance, Attack, Retreat };

    Archetype archetype = Archetype::Chaser; ///< Movement archetype.
    Phase phase = Phase::Idle;               ///< Current phase.

    float move_speed = 60.f;        ///< Movement speed in pixels/s.
    float activation_range = 200.f; ///< Distance at which AI activates.
    float preferred_range = 0.f;    ///< Desired distance from player (Stalker).
    float attack_range = 80.f;      ///< Range at which emitter activates.
    float phase_timer = 0.f;        ///< Timer for phase transitions.
    float strafe_dir = 1.f;         ///< Strafe direction multiplier (+1/-1).
};

/// @brief Deals damage on spatial overlap (e.g. Chaser body damage).
struct ContactDamage {
    float damage = 1.f;    ///< Damage dealt per hit.
    float cooldown = 0.5f; ///< Minimum interval between hits (seconds).
    float timer = 0.f;     ///< Time until next hit allowed.
};

/// @brief Knockback impulse applied to an enemy when hit by a bullet.
struct Knockback {
    float dx = 0.f;        ///< Knockback velocity X in pixels/s.
    float dy = 0.f;        ///< Knockback velocity Y in pixels/s.
    float remaining = 0.f; ///< Duration remaining in seconds.
};

// ── Melee / Dash ────────────────────────────────────────────────

/// @brief Active melee attack arc hitbox.
struct MeleeAttack {
    float damage = 2.f;        ///< Per-enemy damage.
    float range = 30.f;        ///< Arc reach in pixels.
    float half_angle = 0.785f; ///< ~45 deg half-angle (90 deg total cone).
    float knockback = 250.f;   ///< Knockback speed on hit.
    float remaining = 0.1f;    ///< Active hitbox duration.
    float aim_x = 1.f;         ///< Aim direction snapshot X.
    float aim_y = 0.f;         ///< Aim direction snapshot Y.
    bool hit_checked = false;  ///< Ensures arc check runs exactly once.
};

/// @brief Cooldown timer between melee attacks.
struct MeleeCooldown {
    float remaining = 0.f; ///< Time until next melee allowed (seconds).
    float rate = 0.4f;     ///< Minimum interval between attacks (seconds).
};

/// @brief Active dash state with burst velocity override.
struct Dash {
    float speed = 400.f;     ///< Dash travel speed in pixels/s.
    float duration = 0.12f;  ///< Total dash duration.
    float remaining = 0.12f; ///< Time remaining in the dash.
    float dir_x = 1.f;       ///< Dash direction X.
    float dir_y = 0.f;       ///< Dash direction Y.
};

/// @brief Cooldown timer between dashes.
struct DashCooldown {
    float remaining = 0.f; ///< Time until next dash allowed (seconds).
    float rate = 0.6f;     ///< Minimum interval between dashes (seconds).
};

// ── Tags (empty structs for filtering) ───────────────────────────

/// @brief Tag: enemy lost its BulletEmitter via melee disarm.
struct Disarmed {};

/// @brief Tag: entity is removed when it leaves the play area.
struct OffScreenDespawn {};

/// @brief Tag: bullet passes through targets instead of being destroyed.
struct Piercing {};

/// @brief Tag: marks a decay stabilizer pickup entity.
struct StabilizerPickup {};

/// @brief Tag: marks a visual-only explosion effect entity.
struct ExplosionVfx {};

} // namespace raven
