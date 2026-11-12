#pragma once

#include "core/string_id.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace raven {

/// @brief One frame of an animation clip: a cell in the sheet's frame grid
/// and how long it stays on screen.
struct AnimationFrame {
    int frame_x = 0;       ///< Column in the sheet's frame grid.
    int frame_y = 0;       ///< Row in the sheet's frame grid.
    float duration = 0.1f; ///< Seconds the frame is shown.
};

/// @brief A named animation, built from one Aseprite frame tag.
struct AnimationClip {
    std::vector<AnimationFrame> frames; ///< Frames in play order, tag direction applied.
    int repeat = 0; ///< Passes to play before holding the last frame; 0 loops forever.
};

/// @brief Clip names the game asks for. These are the Aseprite tag names
/// artists use for each state.
namespace clips {
inline constexpr const char* IDLE = "idle";     ///< Standing still.
inline constexpr const char* WALK = "walk";     ///< Moving.
inline constexpr const char* ATTACK = "attack"; ///< Melee and ground slam.
inline constexpr const char* DASH = "dash";     ///< Dash / dodge.
} // namespace clips

/// @brief Animation clips per sprite sheet, read from Aseprite's JSON export.
///
/// Each frame tag in the export becomes a clip with the tag's name. Frame
/// durations come from Aseprite. The tag direction (forward, reverse,
/// pingpong, pingpong_reverse) is applied at load, and the tag's repeat
/// count makes a one-shot clip: repeat 1 plays once and holds the last
/// frame. Export with `just export-art`, which uses rows-per-tag sheets and
/// the json-array data format.
///
/// Lives in the registry context so systems can look clips up.
class AnimationLibrary {
  public:
    /// @brief Load every frame tag in an Aseprite JSON export as a clip.
    /// @param sheet Interned id of the sprite sheet the clips belong to.
    /// @param path Path to the JSON file.
    /// @param frame_w Frame width of the sheet's grid in pixels.
    /// @param frame_h Frame height of the sheet's grid in pixels.
    /// @param interner Interner for clip names.
    /// @return True if the file was read and parsed. Malformed frames and
    ///         tags are reported and skipped.
    bool load_file(StringId sheet, const std::string& path, int frame_w, int frame_h,
                   StringInterner& interner);

    /// @brief Load clips from an already-parsed Aseprite JSON export.
    /// @param sheet Interned id of the sprite sheet the clips belong to.
    /// @param j The parsed JSON.
    /// @param frame_w Frame width of the sheet's grid in pixels.
    /// @param frame_h Frame height of the sheet's grid in pixels.
    /// @param interner Interner for clip names.
    /// @param source Name used in log messages (usually the file path).
    /// @return True if the JSON had the expected shape.
    bool load_json(StringId sheet, const nlohmann::json& j, int frame_w, int frame_h,
                   StringInterner& interner, const std::string& source = "animation data");

    /// @brief Look up a clip.
    /// @param sheet Interned sprite sheet id.
    /// @param clip Interned clip name.
    /// @return The clip, or nullptr if the sheet has no clip by that name.
    [[nodiscard]] const AnimationClip* get(StringId sheet, StringId clip) const;

    /// @brief Whether any animation data was loaded for a sheet.
    /// @param sheet Interned sprite sheet id.
    /// @return True if the sheet has at least one clip.
    [[nodiscard]] bool has_sheet(StringId sheet) const;

  private:
    [[nodiscard]] static uint32_t key(StringId sheet, StringId clip) {
        return (static_cast<uint32_t>(sheet.value) << 16) | clip.value;
    }

    std::unordered_map<uint32_t, AnimationClip> clips_;
    std::unordered_set<uint16_t> sheets_; ///< Sheets with at least one clip.
};

} // namespace raven
