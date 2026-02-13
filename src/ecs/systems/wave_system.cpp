#include "ecs/systems/wave_system.hpp"

#include "core/string_id.hpp"
#include "ecs/systems/hitbox_math.hpp"
#include "ecs/systems/player_utils.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>

namespace raven {

// ── Helper: map string to Enemy::Type ──────────────────────────────

namespace {

Enemy::Type parse_enemy_type(const std::string& str) {
    if (str == "mid")
        return Enemy::Type::Mid;
    if (str == "boss")
        return Enemy::Type::Boss;
    return Enemy::Type::Grunt;
}

AiBehavior::Archetype parse_ai_archetype(const std::string& str) {
    if (str == "drifter")
        return AiBehavior::Archetype::Drifter;
    if (str == "stalker")
        return AiBehavior::Archetype::Stalker;
    if (str == "coward")
        return AiBehavior::Archetype::Coward;
    return AiBehavior::Archetype::Chaser;
}

/// @brief Build default AiBehavior for a given archetype.
AiBehavior make_ai(AiBehavior::Archetype archetype) {
    AiBehavior ai{};
    ai.archetype = archetype;
    ai.phase = AiBehavior::Phase::Idle;

    switch (archetype) {
    case AiBehavior::Archetype::Chaser:
        ai.move_speed = 70.f;
        ai.activation_range = 160.f;
        ai.preferred_range = 0.f;
        ai.attack_range = 80.f;
        break;
    case AiBehavior::Archetype::Drifter:
        ai.move_speed = 40.f;
        ai.activation_range = 200.f;
        ai.preferred_range = 0.f;
        ai.attack_range = 100.f;
        break;
    case AiBehavior::Archetype::Stalker:
        ai.move_speed = 90.f;
        ai.activation_range = 160.f;
        ai.preferred_range = 90.f;
        ai.attack_range = 120.f;
        break;
    case AiBehavior::Archetype::Coward:
        ai.move_speed = 110.f;
        ai.activation_range = 200.f;
        ai.preferred_range = 0.f;
        ai.attack_range = 999.f;
        break;
    }

    return ai;
}

/// @brief Get sprite frame_y for an enemy type.
int enemy_frame(Enemy::Type type) {
    switch (type) {
    case Enemy::Type::Grunt:
        return 0;
    case Enemy::Type::Mid:
        return 0;
    case Enemy::Type::Boss:
        return 0;
    }
    return 0;
}

/// @brief Per-tier sprite sizing and hitbox configuration.
struct EnemyVisuals {
    const char* sheet;  ///< Sprite sheet id.
    int sprite_w;       ///< Rendered sprite width.
    int sprite_h;       ///< Rendered sprite height.
    float circle_r;     ///< Circle hitbox radius.
    float rect_w;       ///< Rect hitbox width.
    float rect_h;       ///< Rect hitbox height.
    float offset_y;     ///< Sprite render offset Y (shift up for feet alignment).
};

/// @brief Return visual configuration for an enemy tier.
EnemyVisuals enemy_visuals(Enemy::Type type) {
    switch (type) {
    case Enemy::Type::Grunt:
        //          sheet       sw  sh  cr    rw    rh   oy
        return {"enemies",      24, 24, 7.f, 12.f, 14.f, -3.f};
    case Enemy::Type::Mid:
        return {"enemies_mid",  32, 32, 9.f, 16.f, 18.f, -5.f};
    case Enemy::Type::Boss:
        return {"enemies_boss", 48, 48, 18.f, 32.f, 36.f, -6.f};
    }
    return {"enemies", 24, 24, 7.f, 12.f, 14.f, -3.f};
}

} // namespace

// ── StageLoader ────────────────────────────────────────────────────

bool StageLoader::load_manifest(const std::string& manifest_path) {
    std::ifstream file(manifest_path);
    if (!file.is_open()) {
        spdlog::warn("Stage manifest '{}' not found", manifest_path);
        return false;
    }

    try {
        auto j = nlohmann::json::parse(file);
        int loaded = 0;
        for (const auto& path : j.at("stages")) {
            if (load_file(path.get<std::string>())) {
                ++loaded;
            }
        }
        spdlog::info("Loaded {} stages from manifest '{}'", loaded, manifest_path);
        return loaded > 0;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("Failed to parse stage manifest '{}': {}", manifest_path, e.what());
        return false;
    }
}

bool StageLoader::load_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        spdlog::error("Failed to open stage file '{}'", file_path);
        return false;
    }

    try {
        auto j = nlohmann::json::parse(file);
        stages_.push_back(parse_stage(j));
        spdlog::debug("Loaded stage '{}'", stages_.back().name);
        return true;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("Failed to parse stage '{}': {}", file_path, e.what());
        return false;
    }
}

bool StageLoader::load_from_json(const nlohmann::json& j) {
    try {
        stages_.push_back(parse_stage(j));
        spdlog::debug("Loaded stage '{}' from JSON", stages_.back().name);
        return true;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("Failed to parse stage from JSON: {}", e.what());
        return false;
    }
}

const StageDef* StageLoader::get(int index) const {
    if (index < 0 || index >= static_cast<int>(stages_.size())) {
        return nullptr;
    }
    return &stages_[static_cast<size_t>(index)];
}

int StageLoader::count() const {
    return static_cast<int>(stages_.size());
}

StageDef StageLoader::parse_stage(const nlohmann::json& j) const {
    StageDef stage;
    stage.name = j.at("name").get<std::string>();
    stage.level = j.at("level").get<std::string>();

    for (const auto& wj : j.at("waves")) {
        stage.waves.push_back(parse_wave(wj));
    }

    return stage;
}

WaveDef StageLoader::parse_wave(const nlohmann::json& j) const {
    WaveDef wave;
    for (const auto& ej : j.at("enemies")) {
        wave.enemies.push_back(parse_enemy(ej));
    }
    return wave;
}

WaveEnemyDef StageLoader::parse_enemy(const nlohmann::json& j) const {
    WaveEnemyDef def;
    def.spawn_index = j.value("spawn_index", 0);
    def.type = parse_enemy_type(j.value("type", "grunt"));
    def.pattern = j.value("pattern", "spiral_3way");
    def.hp = j.value("hp", 1.f);
    def.score = j.value("score", 100);
    def.ai = parse_ai_archetype(j.value("ai", "chaser"));
    def.contact_damage = j.value("contact_damage", false);
    return def;
}

// ── System functions ───────────────────────────────────────────────

namespace systems {

void spawn_wave(entt::registry& reg, const Tilemap& tilemap, const StageDef& stage, int wave_index,
                const PatternLibrary& patterns) {
    if (wave_index < 0 || wave_index >= static_cast<int>(stage.waves.size())) {
        return;
    }

    auto& interner = reg.ctx().get<StringInterner>();
    auto spawn_points = tilemap.find_all_spawns("EnemySpawn");
    const auto& wave = stage.waves[static_cast<size_t>(wave_index)];

    for (const auto& def : wave.enemies) {
        // Pick spawn position from the spawn point list (clamp to bounds)
        float spawn_x = 240.f;
        float spawn_y = 135.f;
        if (!spawn_points.empty()) {
            size_t idx = static_cast<size_t>(
                std::clamp(def.spawn_index, 0, static_cast<int>(spawn_points.size()) - 1));
            spawn_x = spawn_points[idx]->x;
            spawn_y = spawn_points[idx]->y;
        }

        auto enemy = reg.create();
        reg.emplace<Transform2D>(enemy, spawn_x, spawn_y);
        reg.emplace<PreviousTransform>(enemy, spawn_x, spawn_y);
        reg.emplace<Velocity>(enemy);
        reg.emplace<Enemy>(enemy, def.type);
        reg.emplace<Health>(enemy, def.hp, def.hp);

        const auto vis = enemy_visuals(def.type);
        reg.emplace<CircleHitbox>(enemy, vis.circle_r);
        reg.emplace<RectHitbox>(enemy, vis.rect_w, vis.rect_h, 0.f, 0.f);
        reg.emplace<Sprite>(enemy, interner.intern(vis.sheet), enemy_frame(def.type), 0,
                            vis.sprite_w, vis.sprite_h, 10, false, 0.f, vis.offset_y);
        reg.emplace<ScoreValue>(enemy, def.score);

        // Set up bullet emitter if a pattern exists
        if (patterns.get(def.pattern)) {
            reg.emplace<BulletEmitter>(enemy, BulletEmitter{interner.intern(def.pattern), {}, {}});
        }

        reg.emplace<AiBehavior>(enemy, make_ai(def.ai));

        if (def.contact_damage) {
            reg.emplace<ContactDamage>(enemy);
        }
    }

    spdlog::debug("Spawned wave {}/{} ({} enemies)", wave_index + 1,
                  static_cast<int>(stage.waves.size()), wave.enemies.size());
}

void update_waves(entt::registry& reg, const Tilemap& tilemap, const StageDef& stage,
                  const PatternLibrary& patterns) {
    auto* state = reg.ctx().find<GameState>();
    if (!state || state->room_cleared || state->game_over) {
        return;
    }

    // Count remaining enemies
    auto enemy_view = reg.view<Enemy>();
    if (enemy_view.size() > 0) {
        return; // Wave still in progress
    }

    // Current wave is clear — advance to next
    state->current_wave++;
    if (state->current_wave < state->total_waves) {
        spawn_wave(reg, tilemap, stage, state->current_wave, patterns);
    } else {
        // All waves exhausted — room cleared
        state->room_cleared = true;

        // Open all Exit entities
        auto exit_view = reg.view<Exit>();
        for (auto [entity, exit] : exit_view.each()) {
            exit.open = true;
        }

        spdlog::info("Room cleared!");
    }
}

std::string check_exit_overlap(entt::registry& reg) {
    float player_x = 0.f;
    float player_y = 0.f;
    if (!find_player_position(reg, player_x, player_y)) {
        return {};
    }

    constexpr float exit_radius = 12.f;
    constexpr float player_radius = 6.f;

    auto exit_view = reg.view<Exit, Transform2D>();
    for (auto [entity, exit, tf] : exit_view.each()) {
        if (!exit.open) {
            continue;
        }
        if (circles_overlap(player_x, player_y, player_radius, tf.x, tf.y, exit_radius)) {
            return exit.target_level;
        }
    }

    return {};
}

} // namespace systems
} // namespace raven
