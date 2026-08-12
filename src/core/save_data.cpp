#include "core/save_data.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>

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
    std::ifstream f(file_path);
    if (!f.is_open()) {
        spdlog::info("No save file at '{}' — starting fresh", file_path);
        return SaveData{};
    }

    try {
        return from_json(nlohmann::json::parse(f));
    } catch (const nlohmann::json::exception& e) {
        spdlog::warn("Failed to parse save '{}': {} — starting fresh", file_path, e.what());
        return SaveData{};
    }
}

bool SaveData::save(const std::string& file_path) const {
    if (file_path.empty()) {
        return false;
    }
    std::ofstream f(file_path);
    if (!f.is_open()) {
        spdlog::warn("Could not write save to '{}'", file_path);
        return false;
    }
    f << to_json().dump(4) << '\n';
    return f.good();
}

} // namespace raven
