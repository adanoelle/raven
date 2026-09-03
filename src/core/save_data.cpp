#include "core/save_data.hpp"

#include "core/fs.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>

namespace raven {

SaveData SaveData::from_json(const nlohmann::json& j) {
    SaveData d;
    d.best_score = std::max(0, j.value("best_score", d.best_score));
    return d;
}

nlohmann::json SaveData::to_json() const {
    return {
        {"best_score", best_score},
    };
}

SaveData SaveData::load(const std::string& file_path) {
    const auto text = fs::read_text(file_path);
    if (!text) {
        spdlog::info("No save file at '{}' — starting fresh", file_path);
        return SaveData{};
    }

    try {
        return from_json(nlohmann::json::parse(*text));
    } catch (const nlohmann::json::exception& e) {
        spdlog::warn("Failed to parse save '{}': {} — starting fresh", file_path, e.what());
        return SaveData{};
    }
}

bool SaveData::save(const std::string& file_path) const {
    if (!fs::write_text(file_path, to_json().dump(4) + '\n')) {
        spdlog::warn("Could not write save to '{}'", file_path);
        return false;
    }
    return true;
}

} // namespace raven
