#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace raven {

/// @brief Lightweight interned string identifier. Trivially copyable.
///
/// Wraps a uint16_t index into a StringInterner. Index 0 is reserved as
/// the invalid/empty sentinel.
struct StringId {
    uint16_t value = 0; ///< Index into the interner (0 = invalid/empty).

    /// @brief Check if this ID refers to a valid interned string.
    /// @return True if the ID is not the empty sentinel.
    [[nodiscard]] bool valid() const { return value != 0; }

    bool operator==(const StringId&) const = default;
    bool operator!=(const StringId&) const = default;
};

/// @brief Bidirectional string-to-uint16_t mapping for string interning.
///
/// Index 0 is reserved as an empty sentinel. Intern returns a stable StringId
/// for a given string; resolve converts back to the original string.
class StringInterner {
  public:
    /// @brief Intern a string, returning its unique StringId.
    /// @param str The string to intern.
    /// @return The interned StringId (stable across calls for the same string).
    StringId intern(const std::string& str) {
        auto it = map_.find(str);
        if (it != map_.end()) {
            return it->second;
        }
        auto id = StringId{static_cast<uint16_t>(strings_.size())};
        strings_.push_back(str);
        map_.emplace(str, id);
        return id;
    }

    /// @brief Resolve a StringId back to its original string.
    /// @param id The StringId to resolve.
    /// @return Reference to the interned string, or an empty string for invalid IDs.
    [[nodiscard]] const std::string& resolve(StringId id) const {
        if (id.value >= strings_.size()) {
            return strings_[0]; // empty sentinel
        }
        return strings_[id.value];
    }

  private:
    std::vector<std::string> strings_ = {""}; ///< Index 0 = empty sentinel.
    std::unordered_map<std::string, StringId> map_;
};

} // namespace raven
