#include "patterns/pattern_library.hpp"

#include "core/paths.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>

namespace raven {

namespace {

/// @brief Clamp a parsed value to a safe range, warning when out of bounds.
float clamp_field(const std::string& pattern, const char* field, float value, float min_v,
                  float max_v) {
    if (value < min_v || value > max_v) {
        float clamped = std::clamp(value, min_v, max_v);
        spdlog::warn("Pattern '{}': {} = {} out of range [{}, {}], clamped to {}", pattern, field,
                     value, min_v, max_v, clamped);
        return clamped;
    }
    return value;
}

} // anonymous namespace

bool PatternLibrary::load_manifest(const std::string& manifest_path) {
    if (!interner_) {
        spdlog::error("PatternLibrary: set_interner() must be called before loading");
        return false;
    }
    std::ifstream f(manifest_path);
    if (!f.is_open()) {
        spdlog::warn("Pattern manifest '{}' not found", manifest_path);
        return false;
    }

    try {
        auto j = nlohmann::json::parse(f);
        int count = 0;
        for (const auto& path : j.at("patterns")) {
            // Manifest entries are relative to the install dir, not the CWD
            if (load_file(paths::asset(path.get<std::string>()))) {
                ++count;
            }
        }
        spdlog::info("Loaded {} bullet patterns from manifest '{}'", count, manifest_path);
        return count > 0;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("Failed to parse manifest '{}': {}", manifest_path, e.what());
        return false;
    }
}

bool PatternLibrary::load_file(const std::string& file_path) {
    if (!interner_) {
        spdlog::error("PatternLibrary: set_interner() must be called before loading");
        return false;
    }
    std::ifstream f(file_path);
    if (!f.is_open()) {
        spdlog::error("Failed to open pattern file '{}'", file_path);
        return false;
    }

    try {
        auto j = nlohmann::json::parse(f);
        auto pattern = parse_pattern(j);
        auto name = pattern.name;
        patterns_[name] = std::move(pattern);
        spdlog::debug("Loaded pattern '{}'", name);
        return true;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("Failed to parse pattern '{}': {}", file_path, e.what());
        return false;
    }
}

bool PatternLibrary::load_from_json(const nlohmann::json& j) {
    if (!interner_) {
        spdlog::error("PatternLibrary: set_interner() must be called before loading");
        return false;
    }
    try {
        auto pattern = parse_pattern(j);
        auto name = pattern.name;
        patterns_[name] = std::move(pattern);
        spdlog::debug("Loaded pattern '{}' from JSON", name);
        return true;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("Failed to parse pattern from JSON: {}", e.what());
        return false;
    }
}

const PatternDef* PatternLibrary::get(const std::string& name) const {
    auto it = patterns_.find(name);
    return it != patterns_.end() ? &it->second : nullptr;
}

std::vector<std::string> PatternLibrary::names() const {
    std::vector<std::string> result;
    result.reserve(patterns_.size());
    for (const auto& [name, _] : patterns_) {
        result.push_back(name);
    }
    return result;
}

PatternDef PatternLibrary::parse_pattern(const nlohmann::json& j) const {
    PatternDef def;
    def.name = j.at("name").get<std::string>();

    auto tier_str = j.value("tier", "common");
    if (tier_str == "rare")
        def.tier = Weapon::Tier::Rare;
    else if (tier_str == "legendary")
        def.tier = Weapon::Tier::Legendary;
    else
        def.tier = Weapon::Tier::Common;

    for (const auto& ej : j.at("emitters")) {
        def.emitters.push_back(parse_emitter(ej, def.name));
    }

    return def;
}

EmitterDef PatternLibrary::parse_emitter(const nlohmann::json& j,
                                         const std::string& pattern_name) const {
    EmitterDef def;

    auto type_str = j.value("type", "radial");
    if (type_str == "aimed")
        def.type = EmitterDef::Type::Aimed;
    else if (type_str == "linear")
        def.type = EmitterDef::Type::Linear;
    else
        def.type = EmitterDef::Type::Radial;

    // Clamp values that could destabilize the game if mistyped in data:
    // fire_rate <= 0 fires a burst every tick (runaway entity growth), and
    // huge counts spawn thousands of bullets per burst.
    def.count = static_cast<int>(
        clamp_field(pattern_name, "count", static_cast<float>(j.value("count", 1)), 1.f, 256.f));
    def.speed = j.value("speed", 100.f);
    def.angular_velocity = j.value("angular_velocity", 0.f);
    def.fire_rate = clamp_field(pattern_name, "fire_rate", j.value("fire_rate", 0.1f), 0.05f, 60.f);
    def.spread_angle = j.value("spread_angle", 360.f);
    def.start_angle = j.value("start_angle", 0.f);
    def.bullet_sheet = interner_->intern(j.value("bullet_sheet", "projectiles"));
    def.bullet_frame_x = j.value("bullet_frame_x", 0);
    def.bullet_frame_y = j.value("bullet_frame_y", 0);
    def.bullet_width = j.value("bullet_width", 8);
    def.bullet_height = j.value("bullet_height", 8);
    def.lifetime = clamp_field(pattern_name, "lifetime", j.value("lifetime", 5.f), 0.05f, 60.f);
    def.damage = j.value("damage", 1.f);
    def.hitbox_radius =
        clamp_field(pattern_name, "hitbox_radius", j.value("hitbox_radius", 3.f), 0.f, 64.f);

    return def;
}

} // namespace raven
