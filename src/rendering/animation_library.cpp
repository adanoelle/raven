#include "rendering/animation_library.hpp"

#include "core/fs.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <charconv>
#include <optional>
#include <string>
#include <utility>

namespace raven {

namespace {

/// @brief Read a frame tag's repeat count. Aseprite writes it as a string
/// and omits it when the tag repeats forever.
int read_repeat(const nlohmann::json& tag) {
    auto it = tag.find("repeat");
    if (it == tag.end()) {
        return 0;
    }
    if (it->is_number_integer()) {
        return std::max(0, it->get<int>());
    }
    if (it->is_string()) {
        const auto& text = it->get_ref<const std::string&>();
        int value = 0;
        auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
        if (ec == std::errc{} && end == text.data() + text.size()) {
            return std::max(0, value);
        }
    }
    return 0;
}

/// @brief Put a tag's frames in play order for its direction.
/// @param frames The tag's frames in sheet order.
/// @param direction Aseprite tag direction.
/// @param looping Whether the clip loops; a looping pingpong skips the
///        frame the loop returns to.
/// @return The frames in play order, or nullopt for an unknown direction.
std::optional<std::vector<AnimationFrame>>
apply_direction(std::vector<AnimationFrame> frames, const std::string& direction, bool looping) {
    const bool pingpong = direction == "pingpong" || direction == "pingpong_reverse";
    if (direction == "reverse" || direction == "pingpong_reverse") {
        std::ranges::reverse(frames);
    } else if (direction != "forward" && direction != "pingpong") {
        return std::nullopt;
    }

    if (pingpong && frames.size() > 1) {
        // Back again without repeating the turnaround frame
        const size_t stop = looping ? 1 : 0;
        for (size_t i = frames.size() - 1; i-- > stop;) {
            const AnimationFrame frame = frames[i];
            frames.push_back(frame);
        }
    }
    return frames;
}

} // namespace

bool AnimationLibrary::load_file(StringId sheet, const std::string& path, int frame_w, int frame_h,
                                 StringInterner& interner) {
    const auto text = fs::read_text(path);
    if (!text) {
        spdlog::error("Failed to open animation data '{}'", path);
        return false;
    }

    try {
        return load_json(sheet, nlohmann::json::parse(*text), frame_w, frame_h, interner, path);
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("Failed to parse animation data '{}': {}", path, e.what());
        return false;
    }
}

bool AnimationLibrary::load_json(StringId sheet, const nlohmann::json& j, int frame_w, int frame_h,
                                 StringInterner& interner, const std::string& source) {
    if (!sheet.valid() || frame_w <= 0 || frame_h <= 0) {
        spdlog::error("{}: invalid sheet or frame size {}x{}", source, frame_w, frame_h);
        return false;
    }

    try {
        const auto& frames_json = j.at("frames");
        if (!frames_json.is_array()) {
            spdlog::error("{}: 'frames' must be a list; export with --format json-array", source);
            return false;
        }

        // Resolve each frame to a grid cell; nullopt marks one that isn't one
        std::vector<std::optional<AnimationFrame>> frames;
        frames.reserve(frames_json.size());
        for (size_t i = 0; i < frames_json.size(); ++i) {
            const auto& rect = frames_json[i].at("frame");
            const int x = rect.at("x").get<int>();
            const int y = rect.at("y").get<int>();
            const int w = rect.at("w").get<int>();
            const int h = rect.at("h").get<int>();
            const int ms = frames_json[i].at("duration").get<int>();

            if (w != frame_w || h != frame_h || x % frame_w != 0 || y % frame_h != 0) {
                spdlog::error("{}: frame {} ({}x{} at {}, {}) is not a cell of the {}x{} grid; "
                              "export without borders or padding",
                              source, i, w, h, x, y, frame_w, frame_h);
                frames.emplace_back(std::nullopt);
            } else if (ms <= 0) {
                spdlog::error("{}: frame {} has a duration of {} ms", source, i, ms);
                frames.emplace_back(std::nullopt);
            } else {
                frames.emplace_back(
                    AnimationFrame{x / frame_w, y / frame_h, static_cast<float>(ms) / 1000.f});
            }
        }

        const auto meta = j.find("meta");
        const auto tags = meta != j.end() ? meta->find("frameTags") : j.end();
        if (meta == j.end() || tags == meta->end() || !tags->is_array() || tags->empty()) {
            spdlog::warn("{}: no frame tags, so no animations; tag each animation in Aseprite "
                         "and export with --list-tags",
                         source);
            return true;
        }

        int loaded = 0;
        for (const auto& tag : *tags) {
            const auto name = tag.at("name").get<std::string>();
            const int from = tag.at("from").get<int>();
            const int to = tag.at("to").get<int>();
            if (from < 0 || to < from || std::cmp_greater_equal(to, frames.size())) {
                spdlog::error("{}: tag '{}' covers frames {}-{}, but there are {} frames", source,
                              name, from, to, frames.size());
                continue;
            }

            AnimationClip clip;
            clip.repeat = read_repeat(tag);
            bool cells_ok = true;
            for (int i = from; i <= to; ++i) {
                const auto& frame = frames[static_cast<size_t>(i)];
                if (!frame) {
                    cells_ok = false;
                    break;
                }
                clip.frames.push_back(*frame);
            }
            if (!cells_ok) {
                spdlog::error("{}: tag '{}' uses a frame reported above; skipped", source, name);
                continue;
            }

            const auto direction = tag.value("direction", std::string{"forward"});
            auto ordered = apply_direction(std::move(clip.frames), direction, clip.repeat == 0);
            if (!ordered) {
                spdlog::error("{}: tag '{}' has unknown direction '{}'; skipped", source, name,
                              direction);
                continue;
            }
            clip.frames = std::move(*ordered);

            const StringId id = interner.intern(name);
            if (!id.valid()) {
                continue;
            }
            if (!clips_.emplace(key(sheet, id), std::move(clip)).second) {
                spdlog::warn("{}: duplicate tag '{}'; keeping the first", source, name);
                continue;
            }
            ++loaded;
        }

        if (loaded > 0) {
            sheets_.insert(sheet.value);
        }
        spdlog::debug("{}: loaded {} animation clips", source, loaded);
        return true;
    } catch (const nlohmann::json::exception& e) {
        spdlog::error("{}: malformed animation data: {}", source, e.what());
        return false;
    }
}

const AnimationClip* AnimationLibrary::get(StringId sheet, StringId clip) const {
    auto it = clips_.find(key(sheet, clip));
    return it != clips_.end() ? &it->second : nullptr;
}

bool AnimationLibrary::has_sheet(StringId sheet) const {
    return sheets_.contains(sheet.value);
}

} // namespace raven
