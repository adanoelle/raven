#pragma once

#include <cstdint>
#include <string>

#include <entt/entt.hpp>

namespace raven {

// ── Spatial ──────────────────────────────────────────────────────

struct Transform2D {
    float x = 0.f;
    float y = 0.f;
    float rotation = 0.f; // radians
};

struct Velocity {
    float dx = 0.f;
    float dy = 0.f;
};

// ── Rendering ────────────────────────────────────────────────────

struct Sprite {
    std::string sheet_id;  // which sprite sheet
    int frame_x = 0;      // frame column in sheet
    int frame_y = 0;      // frame row in sheet
    int width = 16;        // pixel width of one frame
    int height = 16;       // pixel height of one frame
    int layer = 0;         // render order (higher = on top)
    bool flip_x = false;
};

struct Animation {
    int start_frame = 0;
    int end_frame = 0;
    float frame_duration = 0.1f; // seconds per frame
    float elapsed = 0.f;
    int current_frame = 0;
    bool looping = true;
};

// ── Collision ────────────────────────────────────────────────────

struct CircleHitbox {
    float radius = 6.f;
    float offset_x = 0.f;
    float offset_y = 0.f;
};

struct RectHitbox {
    float width = 16.f;
    float height = 16.f;
    float offset_x = 0.f;
    float offset_y = 0.f;
};

// ── Gameplay ─────────────────────────────────────────────────────

struct Player {
    float speed = 200.f;
    int lives = 3;
};

struct Enemy {
    enum class Type : uint8_t {
        Grunt,
        Mid,
        Boss
    };
    Type type = Type::Grunt;
};

struct Health {
    float current = 1.f;
    float max = 1.f;
};

struct Bullet {
    enum class Owner : uint8_t {
        Player,
        Enemy
    };
    Owner owner = Owner::Enemy;
};

struct Lifetime {
    float remaining = 5.f;     // seconds until auto-despawn
};

struct DamageOnContact {
    float damage = 1.f;
};

struct Invulnerable {
    float remaining = 0.f;     // seconds of invulnerability left
};

struct ScoreValue {
    int points = 100;
};

// ── Tags (empty structs for filtering) ───────────────────────────

struct OffScreenDespawn {}; // remove when leaving play area

} // namespace raven
