#pragma once

#ifdef RAVEN_ENABLE_IMGUI

#include <SDL2/SDL.h>
#include <entt/entt.hpp>

#include <array>
#include <string>
#include <vector>

namespace raven {

/// @brief Dear ImGui debug overlay for real-time inspection and tuning.
///
/// Displays FPS graphs, entity counts, and player state panels.
/// Toggle visibility with a key binding (typically F3).
/// Conditionally compiled via RAVEN_ENABLE_IMGUI.
class DebugOverlay {
public:
    /// @brief Initialise the ImGui context and SDL2 backend.
    /// @param window The SDL_Window to attach ImGui to.
    /// @param renderer The SDL_Renderer used for ImGui drawing.
    void init(SDL_Window* window, SDL_Renderer* renderer);

    /// @brief Shut down ImGui and release its resources.
    void shutdown();

    /// @brief Forward an SDL event to ImGui for mouse/keyboard capture.
    /// @param event The SDL event to process.
    /// @return True if ImGui consumed the event (caller should skip it).
    bool process_event(const SDL_Event& event);

    /// @brief Start a new ImGui frame. Call once per frame before render().
    void begin_frame();

    /// @brief Draw all debug panels and finalise ImGui rendering.
    /// @param renderer The SDL_Renderer to draw with.
    /// @param reg The ECS registry to inspect.
    void render(SDL_Renderer* renderer, entt::registry& reg);

    /// @brief Toggle overlay visibility on/off.
    void toggle() { visible_ = !visible_; }

    /// @brief Check whether the overlay is currently visible.
    /// @return True if the overlay is being drawn.
    [[nodiscard]] bool visible() const { return visible_; }

private:
    bool visible_ = false;

    static constexpr int FRAME_HISTORY_SIZE = 120; ///< Number of frame times to track.
    std::array<float, FRAME_HISTORY_SIZE> frame_times_{}; ///< Ring buffer of frame deltas.
    int frame_index_ = 0;  ///< Current write index in the ring buffer.
    float fps_avg_ = 0.f;  ///< Smoothed FPS average.

    /// @brief Draw the FPS counter and frame time graph.
    void panel_fps();

    /// @brief Draw the entity count breakdown by component type.
    /// @param reg The ECS registry to inspect.
    void panel_entities(entt::registry& reg);

    /// @brief Draw the player state inspector panel.
    /// @param reg The ECS registry containing the player entity.
    void panel_player(entt::registry& reg);
};

} // namespace raven

#endif // RAVEN_ENABLE_IMGUI
