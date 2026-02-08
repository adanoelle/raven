#pragma once

#include "core/clock.hpp"
#include "core/input.hpp"
#include "rendering/renderer.hpp"
#include "rendering/sprite_sheet.hpp"
#include "scenes/scene.hpp"

#ifdef RAVEN_ENABLE_IMGUI
#include "debug/debug_overlay.hpp"
#endif

#include <entt/entt.hpp>

namespace raven {

/// Top-level game state. Owns all subsystems.
class Game {
public:
    Game();
    ~Game();

    bool init();
    void run();
    void shutdown();

    // Public access to subsystems (scenes and systems need these)
    entt::registry& registry() { return registry_; }
    Renderer& renderer() { return renderer_; }
    SpriteSheetManager& sprites() { return sprites_; }
    Input& input() { return input_; }
    SceneManager& scenes() { return scenes_; }

    void request_quit() { running_ = false; }

private:
    bool running_ = false;

    // Subsystems
    Renderer renderer_;
    Input input_;
    Clock clock_;
    SceneManager scenes_;
    SpriteSheetManager sprites_;

#ifdef RAVEN_ENABLE_IMGUI
    DebugOverlay debug_overlay_;
#endif

    // ECS
    entt::registry registry_;

    void fixed_update(float dt);
    void render();
    bool load_assets();
};

} // namespace raven
