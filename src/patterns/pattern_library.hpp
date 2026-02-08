#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace raven {

/// Describes one emitter within a bullet pattern.
struct EmitterDef {
    enum class Type { Radial, Aimed, Linear };

    Type type = Type::Radial;
    int count = 1;               // bullets per burst
    float speed = 100.f;         // pixels/sec
    float angular_velocity = 0.f; // degrees/sec rotation
    float fire_rate = 0.1f;      // seconds between bursts
    float spread_angle = 360.f;  // degrees
    float start_angle = 0.f;     // degrees offset
    std::string bullet_sprite = "bullet_small_red";
    float lifetime = 5.f;
    float damage = 1.f;
    float hitbox_radius = 3.f;
};

/// A complete bullet pattern (may contain multiple emitters).
struct PatternDef {
    std::string name;
    std::vector<EmitterDef> emitters;
};

/// Loads and stores bullet pattern definitions from JSON files.
class PatternLibrary {
public:
    bool load_manifest(const std::string& manifest_path);
    bool load_file(const std::string& file_path);
    bool load_from_json(const nlohmann::json& j);

    [[nodiscard]] const PatternDef* get(const std::string& name) const;
    [[nodiscard]] std::vector<std::string> names() const;

private:
    std::unordered_map<std::string, PatternDef> patterns_;

    static PatternDef parse_pattern(const nlohmann::json& j);
    static EmitterDef parse_emitter(const nlohmann::json& j);
};

} // namespace raven
