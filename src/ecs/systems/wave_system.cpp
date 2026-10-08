#include "ecs/systems/wave_system.hpp"

#include "core/fs.hpp"
#include "core/paths.hpp"
#include "core/string_id.hpp"
#include "ecs/systems/hitbox_math.hpp"
#include "ecs/systems/player_utils.hpp"
#include "rendering/animation_library.hpp"
#include "rendering/sheet_ids.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <utility>

namespace raven {

// ── StageLoader ────────────────────────────────────────────────────

bool StageLoader::load_manifest(const std::string& manifest_path) {
    const auto text = fs::read_text(manifest_path);
    if (!text) {
        spdlog::warn("Stage manifest '{}' not found", manifest_path);
        return false;
    }

    try {
        auto j = nlohmann::json::parse(*text);
        int loaded = 0;
        for (const auto& path : j.at("stages")) {
            // Manifest entries are relative to the install dir, not the CWD
            if (load_file(paths::asset(path.get<std::string>()))) {
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
    const auto text = fs::read_text(file_path);
    if (!text) {
        spdlog::error("Failed to open stage file '{}'", file_path);
        return false;
    }

    try {
        auto j = nlohmann::json::parse(*text);
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
    if (index < 0 || std::cmp_greater_equal(index, stages_.size())) {
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
        stage.waves.push_back(parse_wave(wj, stage.name));
    }

    return stage;
}

WaveDef StageLoader::parse_wave(const nlohmann::json& j, const std::string& stage_name) const {
    WaveDef wave;
    for (const auto& ej : j.at("enemies")) {
        if (auto entry = parse_enemy(ej, stage_name)) {
            wave.enemies.push_back(std::move(*entry));
        }
    }
    return wave;
}

std::optional<WaveEnemyDef> StageLoader::parse_enemy(const nlohmann::json& j,
                                                     const std::string& stage_name) const {
    WaveEnemyDef entry;
    entry.spawn_index = j.value("spawn_index", 0);
    entry.enemy = j.value("enemy", std::string{});
    if (entry.enemy.empty()) {
        spdlog::error("Stage '{}': wave entry {} names no 'enemy'; enemies are defined in "
                      "assets/data/enemies.json and placed by name",
                      stage_name, j.dump());
        return std::nullopt;
    }
    for (const auto& [key, value] : j.items()) {
        if (key != "spawn_index" && key != "enemy") {
            spdlog::warn("Stage '{}': wave entry for '{}' has unknown key '{}'; set it in "
                         "enemies.json instead",
                         stage_name, entry.enemy, key);
        }
    }
    return entry;
}

// ── System functions ───────────────────────────────────────────────

namespace systems {

void spawn_wave(entt::registry& reg, const Tilemap& tilemap, const StageDef& stage, int wave_index,
                const PatternLibrary& patterns, const EnemyLibrary& enemies) {
    if (wave_index < 0 || std::cmp_greater_equal(wave_index, stage.waves.size())) {
        return;
    }

    auto& interner = reg.ctx().get<StringInterner>();
    auto spawn_points = tilemap.find_all_spawns("EnemySpawn");
    const auto& wave = stage.waves[static_cast<size_t>(wave_index)];

    if (spawn_points.empty() && tilemap.is_loaded()) {
        spdlog::warn("Stage '{}': level '{}' has no EnemySpawn entities, so wave {} spawns at "
                     "the room centre",
                     stage.name, stage.level, wave_index + 1);
    }

    for (const auto& entry : wave.enemies) {
        const EnemyDef* def = enemies.get(entry.enemy);
        if (!def) {
            spdlog::warn("Stage '{}': unknown enemy '{}'; skipped", stage.name, entry.enemy);
            continue;
        }

        // Pick spawn position from the spawn point list (clamp to bounds)
        float spawn_x = 240.f;
        float spawn_y = 135.f;
        if (tilemap.width_px() > 0 && tilemap.height_px() > 0) {
            spawn_x = static_cast<float>(tilemap.width_px()) / 2.f;
            spawn_y = static_cast<float>(tilemap.height_px()) / 2.f;
        }
        if (!spawn_points.empty()) {
            if (entry.spawn_index < 0 ||
                std::cmp_greater_equal(entry.spawn_index, spawn_points.size())) {
                spdlog::warn("Stage '{}': spawn_index {} is out of range for level '{}' ({} "
                             "EnemySpawn entities)",
                             stage.name, entry.spawn_index, stage.level, spawn_points.size());
            }
            size_t idx = static_cast<size_t>(
                std::clamp(entry.spawn_index, 0, static_cast<int>(spawn_points.size()) - 1));
            spawn_x = spawn_points[idx]->x;
            spawn_y = spawn_points[idx]->y;
        }

        auto enemy = reg.create();
        reg.emplace<Transform2D>(enemy, spawn_x, spawn_y);
        reg.emplace<PreviousTransform>(enemy, spawn_x, spawn_y);
        reg.emplace<Velocity>(enemy);
        reg.emplace<Enemy>(enemy, def->tier);
        reg.emplace<StabilizerDrop>(enemy, def->stabilizer_drop);
        reg.emplace<Health>(enemy, def->hp, def->hp);
        reg.emplace<CircleHitbox>(enemy, def->radius);
        reg.emplace<RectHitbox>(enemy, def->rect_w, def->rect_h, 0.f, 0.f);
        // A size of 0 draws the sheet's frame size
        reg.emplace<Sprite>(enemy, interner.intern(def->sheet), 0, 0, def->width, def->height, 10,
                            false, 0.f, def->offset_y);
        reg.emplace<ScoreValue>(enemy, def->score);
        // Animates once the enemy's sheet has an exported "idle" tag
        reg.emplace<Animation>(enemy, Animation{interner.intern(clips::IDLE)});

        // Set up bullet emitter if a pattern exists. An empty pattern name
        // means an enemy that deliberately doesn't shoot.
        if (patterns.get(def->pattern)) {
            reg.emplace<BulletEmitter>(enemy, BulletEmitter{interner.intern(def->pattern), {}, {}});
        } else if (!def->pattern.empty()) {
            spdlog::warn("Enemy '{}': pattern '{}' not found, so it won't fire", def->id,
                         def->pattern);
        }

        reg.emplace<AiBehavior>(enemy, def->ai);

        if (def->contact_damage > 0.f) {
            auto& contact = reg.emplace<ContactDamage>(enemy);
            contact.damage = def->contact_damage;
            contact.timer = SPAWN_CONTACT_GRACE;
        }
    }

    spdlog::debug("Spawned wave {}/{} ({} enemies)", wave_index + 1,
                  static_cast<int>(stage.waves.size()), wave.enemies.size());
}

void update_waves(entt::registry& reg, const Tilemap& tilemap, const StageDef& stage,
                  const PatternLibrary& patterns, const EnemyLibrary& enemies) {
    auto* state = reg.ctx().find<GameState>();
    if (!state || state->room_cleared || state->game_over) {
        return;
    }

    // Count remaining enemies
    auto enemy_view = reg.view<Enemy>();
    if (!enemy_view.empty()) {
        return; // Wave still in progress
    }

    // Current wave is clear — advance to next
    state->current_wave++;
    if (state->current_wave < state->total_waves) {
        spawn_wave(reg, tilemap, stage, state->current_wave, patterns, enemies);
    } else {
        // All waves exhausted — room cleared
        state->room_cleared = true;

        // Open all Exit entities
        auto exit_view = reg.view<Exit>();
        for (auto [entity, exit] : exit_view.each()) {
            exit.open = true;
            if (auto* sprite = reg.try_get<Sprite>(entity)) {
                sprite->frame_x = sheets::PROP_FRAME_EXIT_OPEN;
            }
        }

        spdlog::info("Room cleared!");
    }
}

const Exit* check_exit_overlap(entt::registry& reg) {
    float player_x = 0.f;
    float player_y = 0.f;
    if (!find_player_position(reg, player_x, player_y)) {
        return nullptr;
    }

    constexpr float exit_radius = 12.f;
    constexpr float player_radius = 6.f;

    auto exit_view = reg.view<Exit, Transform2D>();
    for (auto [entity, exit, tf] : exit_view.each()) {
        if (!exit.open) {
            continue;
        }
        if (circles_overlap(player_x, player_y, player_radius, tf.x, tf.y, exit_radius)) {
            return &exit;
        }
    }

    return nullptr;
}

} // namespace systems
} // namespace raven
