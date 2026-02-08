#pragma once

#ifdef RAVEN_ENABLE_IMGUI

#include <SDL2/SDL.h>
#include <entt/entt.hpp>

#include <array>
#include <string>
#include <vector>

namespace raven {

class DebugOverlay {
public:
    void init(SDL_Window* window, SDL_Renderer* renderer);
    void shutdown();

    /// Feed an SDL event to ImGui. Returns true if ImGui consumed it.
    bool process_event(const SDL_Event& event);

    void begin_frame();
    void render(SDL_Renderer* renderer, entt::registry& reg);

    void toggle() { visible_ = !visible_; }
    [[nodiscard]] bool visible() const { return visible_; }

private:
    bool visible_ = false;

    // FPS tracking
    static constexpr int FRAME_HISTORY_SIZE = 120;
    std::array<float, FRAME_HISTORY_SIZE> frame_times_{};
    int frame_index_ = 0;
    float fps_avg_ = 0.f;

    void panel_fps();
    void panel_entities(entt::registry& reg);
    void panel_player(entt::registry& reg);
};

} // namespace raven

#endif // RAVEN_ENABLE_IMGUI
