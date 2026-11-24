#pragma once

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <span>
#include <string>
#include <string_view>

namespace raven {

/// @brief Report keys of a JSON object that aren't part of its format.
///
/// An unknown key is usually a typo, which would otherwise silently leave a
/// default in place. Each one is logged as a warning; nothing is rejected.
/// @param object The JSON value to check. Anything but an object has no keys.
/// @param known The keys the format reads.
/// @param where What is being read, for the warning ("Pattern 'nova'").
/// @return The number of unknown keys found.
inline int warn_unknown_keys(const nlohmann::json& object, std::span<const std::string_view> known,
                             const std::string& where) {
    int unknown = 0;
    if (!object.is_object()) {
        return unknown;
    }
    for (const auto& [key, value] : object.items()) {
        if (std::ranges::find(known, key) == known.end()) {
            spdlog::warn("{}: unknown key '{}'; is it a typo?", where, key);
            ++unknown;
        }
    }
    return unknown;
}

} // namespace raven
