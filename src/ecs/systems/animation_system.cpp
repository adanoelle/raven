#include "ecs/systems/animation_system.hpp"

#include "ecs/components.hpp"

namespace raven::systems {

void update_animation(entt::registry& reg, float dt) {
    auto view = reg.view<Animation, Sprite>();
    for (auto [entity, anim, sprite] : view.each()) {
        anim.elapsed += dt;

        while (anim.elapsed >= anim.frame_duration) {
            anim.elapsed -= anim.frame_duration;

            if (anim.current_frame < anim.end_frame) {
                ++anim.current_frame;
            } else if (anim.looping) {
                anim.current_frame = anim.start_frame;
            }
            // One-shot finished: clamp at end_frame (no-op)
        }

        sprite.frame_x = anim.current_frame;
    }
}

} // namespace raven::systems
