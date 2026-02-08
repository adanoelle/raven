#include "ecs/systems/render_system.hpp"
#include "ecs/components.hpp"

#include <algorithm>
#include <vector>

namespace raven::systems {

void render_sprites(entt::registry& reg, SDL_Renderer* renderer,
                    const SpriteSheetManager& sprites,
                    float interpolation_alpha) {
    // Collect all renderable entities and sort by layer
    struct RenderEntry {
        float x, y;
        int frame_x, frame_y;
        int width, height;
        int layer;
        bool flip_x;
        const SpriteSheet* sheet;
    };

    std::vector<RenderEntry> entries;

    auto view = reg.view<Transform2D, Sprite>();
    for (auto [entity, tf, sprite] : view.each()) {
        // Interpolate position if previous transform is available
        float render_x = tf.x;
        float render_y = tf.y;
        if (auto* prev = reg.try_get<PreviousTransform>(entity)) {
            render_x = prev->x + (tf.x - prev->x) * interpolation_alpha;
            render_y = prev->y + (tf.y - prev->y) * interpolation_alpha;
        }

        const auto* sheet = sprites.get(sprite.sheet_id);
        if (!sheet) {
            // No sprite sheet loaded — draw a placeholder colored rect
            SDL_Rect rect{
                static_cast<int>(render_x - static_cast<float>(sprite.width) / 2.f),
                static_cast<int>(render_y - static_cast<float>(sprite.height) / 2.f),
                sprite.width,
                sprite.height
            };

            // Color by entity type for debugging
            if (reg.any_of<Player>(entity)) {
                SDL_SetRenderDrawColor(renderer, 0, 200, 255, 255);
            } else if (reg.any_of<Bullet>(entity)) {
                SDL_SetRenderDrawColor(renderer, 255, 80, 80, 255);
            } else if (reg.any_of<Enemy>(entity)) {
                SDL_SetRenderDrawColor(renderer, 200, 50, 200, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
            }

            SDL_RenderFillRect(renderer, &rect);
            continue;
        }

        entries.push_back({
            render_x, render_y,
            sprite.frame_x, sprite.frame_y,
            sprite.width, sprite.height,
            sprite.layer,
            sprite.flip_x,
            sheet
        });
    }

    // Sort by layer (lower layers drawn first)
    std::sort(entries.begin(), entries.end(),
              [](const RenderEntry& a, const RenderEntry& b) {
                  return a.layer < b.layer;
              });

    // Draw
    for (const auto& e : entries) {
        e.sheet->draw(
            renderer,
            e.frame_x, e.frame_y,
            static_cast<int>(e.x - static_cast<float>(e.width) / 2.f),
            static_cast<int>(e.y - static_cast<float>(e.height) / 2.f),
            e.flip_x
        );
    }
}

} // namespace raven::systems
