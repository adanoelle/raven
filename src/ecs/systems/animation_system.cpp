#include "ecs/systems/animation_system.hpp"

#include "rendering/animation_library.hpp"

#include <spdlog/spdlog.h>

#include <initializer_list>
#include <string>
#include <unordered_set>

namespace raven::systems {

namespace {

/// @brief (sheet, clip) pairs already reported missing, so each is logged once.
struct MissingClipWarnings {
    std::unordered_set<uint32_t> keys;
};

/// @brief Look up a clip, logging once when a sheet that has animation data
/// lacks it. Sheets with no animation data at all stay quiet: they are
/// simply not animated yet.
const AnimationClip* find_clip(entt::registry& reg, const AnimationLibrary& library, StringId sheet,
                               StringId clip) {
    if (const auto* found = library.get(sheet, clip)) {
        return found;
    }
    if (clip.valid() && library.has_sheet(sheet)) {
        auto& warned = reg.ctx().emplace<MissingClipWarnings>();
        const uint32_t key = (static_cast<uint32_t>(sheet.value) << 16) | clip.value;
        if (warned.keys.insert(key).second) {
            const auto* interner = reg.ctx().find<StringInterner>();
            spdlog::warn("Sprite sheet '{}' has no '{}' animation; add the tag in Aseprite and "
                         "run 'just export-art'",
                         interner ? interner->resolve(sheet) : std::string{"?"},
                         interner ? interner->resolve(clip) : std::string{"?"});
        }
    }
    return nullptr;
}

} // namespace

void play_clip(Animation& anim, StringId clip) {
    if (anim.clip != clip) {
        anim = Animation{clip};
    }
}

void update_animation(entt::registry& reg, float dt) {
    const auto* library = reg.ctx().find<AnimationLibrary>();
    if (!library) {
        return;
    }

    auto view = reg.view<Animation, Sprite>();
    for (auto [entity, anim, sprite] : view.each()) {
        const auto* clip = find_clip(reg, *library, sprite.sheet_id, anim.clip);
        if (!clip || clip->frames.empty()) {
            continue;
        }

        const int count = static_cast<int>(clip->frames.size());
        if (anim.frame < 0 || anim.frame >= count) {
            anim.frame = 0;
            anim.elapsed = 0.f;
        }

        if (!anim.finished) {
            anim.elapsed += dt;
            while (!anim.finished) {
                // Durations are checked at load; the guard keeps a bad one
                // from spinning this loop forever
                const float duration = clip->frames[static_cast<size_t>(anim.frame)].duration;
                if (duration <= 0.f || anim.elapsed < duration) {
                    break;
                }
                anim.elapsed -= duration;

                if (anim.frame + 1 < count) {
                    ++anim.frame;
                } else if (clip->repeat == 0 || ++anim.passes < clip->repeat) {
                    anim.frame = 0;
                } else {
                    // The last frame has shown for its full duration
                    anim.finished = true;
                    anim.elapsed = 0.f;
                }
            }
        }

        const auto& frame = clip->frames[static_cast<size_t>(anim.frame)];
        sprite.frame_x = frame.frame_x;
        sprite.frame_y = frame.frame_y;
    }
}

void update_player_animation(entt::registry& reg) {
    auto* interner = reg.ctx().find<StringInterner>();
    const auto* library = reg.ctx().find<AnimationLibrary>();
    if (!interner || !library) {
        return;
    }

    const StringId idle = interner->intern(clips::IDLE);
    const StringId walk = interner->intern(clips::WALK);
    const StringId attack = interner->intern(clips::ATTACK);
    const StringId dash = interner->intern(clips::DASH);

    auto view = reg.view<Player, Velocity, Animation, Sprite>();
    for (auto [entity, player, vel, anim, sprite] : view.each()) {
        StringId wanted = idle;
        bool action = false;
        if (reg.any_of<MeleeAttack, GroundSlam>(entity)) {
            wanted = attack;
            action = true;
        } else if (reg.any_of<Dash>(entity)) {
            wanted = dash;
            action = true;
        } else if ((vel.dx * vel.dx + vel.dy * vel.dy) > 1.f) {
            wanted = walk;
        }

        // A one-shot clip plays to its end before walk or idle replaces it
        const auto* current = library->get(sprite.sheet_id, anim.clip);
        const bool holding = current && current->repeat > 0 && !anim.finished;
        if (action || !holding) {
            StringId chosen = idle;
            for (StringId candidate : {wanted, walk, idle}) {
                if (find_clip(reg, *library, sprite.sheet_id, candidate)) {
                    chosen = candidate;
                    break;
                }
            }
            play_clip(anim, chosen);
        }

        // Face the aim direction
        if (const auto* aim = reg.try_get<AimDirection>(entity)) {
            if (aim->x > 0.f) {
                sprite.flip_x = false;
            } else if (aim->x < 0.f) {
                sprite.flip_x = true;
            }
        }
    }
}

} // namespace raven::systems
