#ifdef RAVEN_ENABLE_IMGUI

#include "debug/debug_overlay.hpp"

#include "ecs/components.hpp"
#include "rendering/lighting_pass.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

namespace raven {

void DebugOverlay::init(SDL_Window* window, SDL_Renderer* renderer) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.f;
    style.Alpha = 0.9f;

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    frame_times_.fill(0.f);
}

void DebugOverlay::shutdown() {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

bool DebugOverlay::process_event(const SDL_Event& event) {
    ImGui_ImplSDL3_ProcessEvent(&event);
    const ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse || io.WantCaptureKeyboard;
}

void DebugOverlay::begin_frame() {
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void DebugOverlay::render(SDL_Renderer* renderer, entt::registry& reg) {
    if (!visible_) {
        ImGui::EndFrame();
        return;
    }

    panel_fps();
    panel_entities(reg);
    panel_player(reg);
#ifdef RAVEN_ENABLE_SHADER_LIGHTING
    panel_lighting(reg);
#endif

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
}

void DebugOverlay::panel_fps() {
    const ImGuiIO& io = ImGui::GetIO();
    float frame_ms = 1000.f / io.Framerate;

    frame_times_[static_cast<std::size_t>(frame_index_)] = frame_ms;
    frame_index_ = (frame_index_ + 1) % FRAME_HISTORY_SIZE;

    float sum = 0.f;
    for (float t : frame_times_) {
        sum += t;
    }
    fps_avg_ = 1000.f / (sum / static_cast<float>(FRAME_HISTORY_SIZE));

    ImGui::SetNextWindowPos(ImVec2(5, 5), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(260, 120), ImGuiCond_FirstUseEver);
    ImGui::Begin("FPS");
    ImGui::Text("FPS: %.1f (avg: %.1f)", static_cast<double>(io.Framerate),
                static_cast<double>(fps_avg_));
    ImGui::Text("Frame: %.2f ms", static_cast<double>(frame_ms));
    ImGui::PlotLines("##frametime", frame_times_.data(), FRAME_HISTORY_SIZE, frame_index_, nullptr,
                     0.f, 33.3f, ImVec2(0, 40));
    ImGui::End();
}

void DebugOverlay::panel_entities(entt::registry& reg) {
    ImGui::SetNextWindowPos(ImVec2(5, 130), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_FirstUseEver);
    ImGui::Begin("Entities");

    auto& storage = reg.storage<entt::entity>();
    ImGui::Text("Total: %zu", storage.size() - storage.free_list());

    std::size_t bullet_count = 0;
    for ([[maybe_unused]] auto _ : reg.view<Bullet>()) {
        ++bullet_count;
    }
    std::size_t enemy_count = 0;
    for ([[maybe_unused]] auto _ : reg.view<Enemy>()) {
        ++enemy_count;
    }
    std::size_t player_count = 0;
    for ([[maybe_unused]] auto _ : reg.view<Player>()) {
        ++player_count;
    }

    ImGui::Text("Players: %zu", player_count);
    ImGui::Text("Bullets: %zu", bullet_count);
    ImGui::Text("Enemies: %zu", enemy_count);
    ImGui::End();
}

#ifdef RAVEN_ENABLE_SHADER_LIGHTING
void DebugOverlay::panel_lighting(entt::registry& reg) {
    auto& settings = reg.ctx().emplace<LightingSettings>();
    ImGui::SetNextWindowPos(ImVec2(10, 300), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(260, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Lighting (prototype)");
    ImGui::Checkbox("Enabled", &settings.enabled);
    ImGui::SliderInt("Ambient", &settings.ambient, -12, 6);
    ImGui::SliderInt("Dither band", &settings.dither_band, 0, 8);
    ImGui::SliderInt("Player radius", &settings.player_radius, 0, 240);
    ImGui::SliderInt("Player zones", &settings.player_zones, 0, 14);
    ImGui::SliderInt("Bullet radius", &settings.bullet_radius, 0, 80);
    ImGui::SliderInt("Bullet strength /16", &settings.bullet_level, 0, 96);
    ImGui::SliderInt("Bullet crackle /16", &settings.bullet_crackle, 0, 32);
    ImGui::SliderInt("Light ceiling /16", &settings.light_ceiling, 0, 255);
    ImGui::SliderInt("Cool tint full at /16", &settings.cool_full, 0, 96);
    ImGui::Checkbox("Light sprites too", &settings.light_sprites);
    ImGui::SliderInt("Fire radius", &settings.fire_radius, 0, 160);
    ImGui::SliderInt("Fire strength /16", &settings.fire_level, 0, 96);
    ImGui::SliderInt("Fire flicker /16", &settings.fire_flicker, 0, 16);
    ImGui::End();
}
#endif

void DebugOverlay::panel_player(entt::registry& reg) {
    auto view = reg.view<Transform2D, Velocity, Player>();
    if (view.size_hint() == 0)
        return;

    ImGui::SetNextWindowPos(ImVec2(5, 235), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(260, 120), ImGuiCond_FirstUseEver);
    ImGui::Begin("Player");

    for (auto [entity, tf, vel, player] : view.each()) {
        ImGui::DragFloat("X", &tf.x, 1.f, 0.f, 480.f);
        ImGui::DragFloat("Y", &tf.y, 1.f, 0.f, 270.f);
        ImGui::Text("Vel: (%.1f, %.1f)", static_cast<double>(vel.dx), static_cast<double>(vel.dy));
        ImGui::Separator();
        ImGui::Text("Lives: %d", player.lives);

        if (auto* hp = reg.try_get<Health>(entity)) {
            ImGui::Text("HP: %.1f / %.1f", static_cast<double>(hp->current),
                        static_cast<double>(hp->max));
        }
    }
    ImGui::End();
}

} // namespace raven

#endif // RAVEN_ENABLE_IMGUI
