#include "ecs/enemy_library.hpp"

#include "core/fs.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <string>
#include <utility>

namespace raven {

namespace {

constexpr std::array<std::string_view, 11> DEF_KEYS = {
    "tier",  "sheet", "size",    "offset_y",       "hitbox",         "hp",
    "score", "ai",    "pattern", "contact_damage", "stabilizer_drop"};
constexpr std::array<std::string_view, 2> HITBOX_KEYS = {"radius", "rect"};
constexpr std::array<std::string_view, 5> AI_KEYS = {"archetype", "move_speed", "activation_range",
                                                     "preferred_range", "attack_range"};

/// @brief Report keys that aren't part of the format; usually a typo that
/// would otherwise silently leave a default in place.
template <size_t N>
void warn_unknown_keys(const nlohmann::json& object, const std::array<std::string_view, N>& known,
                       const std::string& where) {
    for (const auto& [key, value] : object.items()) {
        if (std::ranges::find(known, key) == known.end()) {
            spdlog::warn("{}: unknown key '{}'; is it a typo?", where, key);
        }
    }
}

/// @brief Read a two-number array such as "size": [w, h].
template <typename T>
bool read_pair(const nlohmann::json& value, T& first, T& second) {
    if (!value.is_array() || value.size() != 2) {
        return false;
    }
    first = value[0].get<T>();
    second = value[1].get<T>();
    return true;
}

/// @brief Parse one enemy definition, reporting why it can't be used.
std::optional<EnemyDef> parse_def(const std::string& id, const nlohmann::json& j,
                                  const std::string& source) {
    const std::string where = source + ": enemy '" + id + "'";
    if (!j.is_object()) {
        spdlog::error("{} must be an object", where);
        return std::nullopt;
    }
    warn_unknown_keys(j, DEF_KEYS, where);

    EnemyDef def;
    def.id = id;

    const auto tier_str = j.value("tier", std::string{});
    const auto tier = parse_enemy_type(tier_str);
    if (!tier) {
        spdlog::error("{}: tier '{}' must be grunt, mid or boss", where, tier_str);
        return std::nullopt;
    }
    def.tier = *tier;

    def.sheet = j.value("sheet", std::string{});
    if (def.sheet.empty()) {
        spdlog::error("{}: needs a 'sheet'", where);
        return std::nullopt;
    }

    if (auto size = j.find("size"); size != j.end()) {
        if (!read_pair(*size, def.width, def.height) || def.width <= 0 || def.height <= 0) {
            spdlog::error("{}: 'size' must be [width, height] in pixels", where);
            return std::nullopt;
        }
    }
    def.offset_y = j.value("offset_y", 0.f);

    if (auto hitbox = j.find("hitbox"); hitbox != j.end()) {
        warn_unknown_keys(*hitbox, HITBOX_KEYS, where + " hitbox");
        def.radius = hitbox->value("radius", def.radius);
        if (auto rect = hitbox->find("rect"); rect != hitbox->end()) {
            if (!read_pair(*rect, def.rect_w, def.rect_h)) {
                spdlog::error("{}: hitbox 'rect' must be [width, height]", where);
                return std::nullopt;
            }
        }
        if (def.radius <= 0.f || def.rect_w <= 0.f || def.rect_h <= 0.f) {
            spdlog::error("{}: hitbox sizes must be positive", where);
            return std::nullopt;
        }
    }

    def.hp = j.value("hp", def.hp);
    def.score = j.value("score", def.score);
    if (def.hp <= 0.f || def.score < 0) {
        spdlog::error("{}: 'hp' must be positive and 'score' not negative", where);
        return std::nullopt;
    }

    const auto ai = j.find("ai");
    if (ai == j.end() || !ai->is_object()) {
        spdlog::error("{}: needs an 'ai' object with an 'archetype'", where);
        return std::nullopt;
    }
    warn_unknown_keys(*ai, AI_KEYS, where + " ai");
    const auto archetype_str = ai->value("archetype", std::string{});
    const auto archetype = parse_ai_archetype(archetype_str);
    if (!archetype) {
        spdlog::error("{}: archetype '{}' must be chaser, drifter, stalker or coward", where,
                      archetype_str);
        return std::nullopt;
    }
    def.ai = default_ai(*archetype);
    def.ai.move_speed = ai->value("move_speed", def.ai.move_speed);
    def.ai.activation_range = ai->value("activation_range", def.ai.activation_range);
    def.ai.preferred_range = ai->value("preferred_range", def.ai.preferred_range);
    def.ai.attack_range = ai->value("attack_range", def.ai.attack_range);
    if (def.ai.move_speed < 0.f || def.ai.activation_range < 0.f || def.ai.preferred_range < 0.f ||
        def.ai.attack_range < 0.f) {
        spdlog::error("{}: ai speeds and ranges must not be negative", where);
        return std::nullopt;
    }

    def.pattern = j.value("pattern", std::string{});

    if (auto contact = j.find("contact_damage"); contact != j.end() && contact->is_boolean()) {
        spdlog::error("{}: 'contact_damage' is the damage per hit (e.g. 15), not true/false",
                      where);
        return std::nullopt;
    }
    def.contact_damage = j.value("contact_damage", 0.f);
    def.stabilizer_drop = j.value("stabilizer_drop", default_stabilizer_drop(def.tier));
    if (def.contact_damage < 0.f || def.stabilizer_drop < 0.f || def.stabilizer_drop > 1.f) {
        spdlog::error("{}: 'contact_damage' must not be negative and 'stabilizer_drop' must "
                      "be between 0 and 1",
                      where);
        return std::nullopt;
    }

    return def;
}

} // namespace

std::optional<Enemy::Type> parse_enemy_type(std::string_view str) {
    if (str == "grunt")
        return Enemy::Type::Grunt;
    if (str == "mid")
        return Enemy::Type::Mid;
    if (str == "boss")
        return Enemy::Type::Boss;
    return std::nullopt;
}

std::optional<AiBehavior::Archetype> parse_ai_archetype(std::string_view str) {
    if (str == "chaser")
        return AiBehavior::Archetype::Chaser;
    if (str == "drifter")
        return AiBehavior::Archetype::Drifter;
    if (str == "stalker")
        return AiBehavior::Archetype::Stalker;
    if (str == "coward")
        return AiBehavior::Archetype::Coward;
    return std::nullopt;
}

AiBehavior default_ai(AiBehavior::Archetype archetype) {
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

bool EnemyLibrary::load_file(const std::string& path) {
    const auto text = fs::read_text(path);
    if (!text) {
        spdlog::error("Failed to open enemy definitions '{}'", path);
        return false;
    }

    try {
        return load_json(nlohmann::json::parse(*text), path);
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("Failed to parse enemy definitions '{}': {}", path, e.what());
        return false;
    }
}

bool EnemyLibrary::load_json(const nlohmann::json& j, const std::string& source) {
    const auto enemies = j.is_object() ? j.find("enemies") : j.end();
    if (enemies == j.end() || !enemies->is_object()) {
        spdlog::error("{}: expected an 'enemies' object keyed by enemy name", source);
        return false;
    }

    int loaded = 0;
    for (const auto& [id, def_json] : enemies->items()) {
        try {
            if (auto def = parse_def(id, def_json, source)) {
                defs_.insert_or_assign(id, std::move(*def));
                ++loaded;
            }
        } catch (const nlohmann::json::exception& e) {
            spdlog::error("{}: enemy '{}' is malformed: {}", source, id, e.what());
        }
    }

    spdlog::info("Loaded {} enemy definitions from '{}'", loaded, source);
    return true;
}

const EnemyDef* EnemyLibrary::get(const std::string& id) const {
    auto it = defs_.find(id);
    return it != defs_.end() ? &it->second : nullptr;
}

std::vector<std::string> EnemyLibrary::names() const {
    std::vector<std::string> result;
    result.reserve(defs_.size());
    for (const auto& [id, def] : defs_) {
        result.push_back(id);
    }
    return result;
}

} // namespace raven
