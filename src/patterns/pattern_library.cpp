#include "patterns/pattern_library.hpp"

#include <spdlog/spdlog.h>

#include <fstream>

namespace raven {

bool PatternLibrary::load_manifest(const std::string& manifest_path) {
    std::ifstream f(manifest_path);
    if (!f.is_open()) {
        spdlog::warn("Pattern manifest '{}' not found", manifest_path);
        return false;
    }

    try {
        auto j = nlohmann::json::parse(f);
        int count = 0;
        for (const auto& path : j.at("patterns")) {
            if (load_file(path.get<std::string>())) {
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

PatternDef PatternLibrary::parse_pattern(const nlohmann::json& j) {
    PatternDef def;
    def.name = j.at("name").get<std::string>();

    for (const auto& ej : j.at("emitters")) {
        def.emitters.push_back(parse_emitter(ej));
    }

    return def;
}

EmitterDef PatternLibrary::parse_emitter(const nlohmann::json& j) {
    EmitterDef def;

    auto type_str = j.value("type", "radial");
    if (type_str == "aimed") def.type = EmitterDef::Type::Aimed;
    else if (type_str == "linear") def.type = EmitterDef::Type::Linear;
    else def.type = EmitterDef::Type::Radial;

    def.count = j.value("count", 1);
    def.speed = j.value("speed", 100.f);
    def.angular_velocity = j.value("angular_velocity", 0.f);
    def.fire_rate = j.value("fire_rate", 0.1f);
    def.spread_angle = j.value("spread_angle", 360.f);
    def.start_angle = j.value("start_angle", 0.f);
    def.bullet_sprite = j.value("bullet_sprite", "bullet_small_red");
    def.lifetime = j.value("lifetime", 5.f);
    def.damage = j.value("damage", 1.f);
    def.hitbox_radius = j.value("hitbox_radius", 3.f);

    return def;
}

} // namespace raven
