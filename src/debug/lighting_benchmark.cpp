#ifdef RAVEN_ENABLE_SHADER_LIGHTING

#include "debug/lighting_benchmark.hpp"

#include "core/paths.hpp"
#include "ecs/systems/tilemap_render_system.hpp"
#include "rendering/renderer.hpp"
#include "rendering/tilemap.hpp"

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdio>
#include <vector>

namespace {

constexpr int WARMUP_FRAMES = 30;
constexpr int MEASURED_FRAMES = 240;
constexpr int MANY_PASSES = 33; ///< Passes per frame in the second run of a case.

struct Case {
    const char* path; ///< "off", "loop" or "buffer".
    int lights;
    int radius;
};

struct Timing {
    double mean_ms = 0.0; ///< Mean whole-frame time.
    double p99_ms = 0.0;  ///< 99th percentile whole-frame time.
    double cpu_ms = 0.0;  ///< Mean time spent in the calls that issue the lighting.
};

/// Lights on fixed, repeatable paths: the same every run.
void place_lights(std::vector<raven::Light>& lights, int count, int radius, int frame) {
    lights.clear();
    Uint32 seed = 12345;
    auto next = [&seed]() {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>(seed >> 8) / 16777216.f; // 0..1
    };
    for (int i = 0; i < count; ++i) {
        const float x0 = next() * raven::Renderer::VIRTUAL_WIDTH;
        const float y0 = next() * raven::Renderer::VIRTUAL_HEIGHT;
        const float vx = (next() - 0.5f) * 2.f;
        const float vy = (next() - 0.5f) * 2.f;
        const auto t = static_cast<float>(frame);
        // Bounce off the frame edges
        auto fold = [](float v, float size) {
            const float period = 2.f * size;
            float m = SDL_fmodf(v, period);
            if (m < 0.f) {
                m += period;
            }
            return m < size ? m : period - m;
        };
        lights.push_back({fold(x0 + vx * t, raven::Renderer::VIRTUAL_WIDTH),
                          fold(y0 + vy * t, raven::Renderer::VIRTUAL_HEIGHT),
                          static_cast<float>(radius), 48});
    }
}

Timing run(raven::Renderer& renderer, const raven::Tilemap& room, const Case& c, int passes) {
    raven::LightingSettings settings;
    settings.enabled = SDL_strcmp(c.path, "off") != 0;
    settings.use_loop = SDL_strcmp(c.path, "loop") == 0;

    std::vector<raven::Light> lights;
    std::vector<double> frames;
    frames.reserve(MEASURED_FRAMES);
    double cpu_total = 0.0;
    const double to_ms = 1000.0 / static_cast<double>(SDL_GetPerformanceFrequency());

    for (int frame = 0; frame < WARMUP_FRAMES + MEASURED_FRAMES; ++frame) {
        SDL_PumpEvents();
        place_lights(lights, c.lights, c.radius, frame);

        const Uint64 start = SDL_GetPerformanceCounter();
        renderer.begin_frame();
        raven::systems::render_tilemap(room, renderer.sdl_renderer());
        Uint64 lit_start = SDL_GetPerformanceCounter();
        Uint64 lit_end = lit_start;
        if (settings.enabled) {
            renderer.apply_lighting(settings, lights, passes);
            lit_end = SDL_GetPerformanceCounter();
        }
        renderer.end_frame();
        renderer.present();
        renderer.wait_for_gpu();
        const Uint64 end = SDL_GetPerformanceCounter();

        if (frame >= WARMUP_FRAMES) {
            frames.push_back(static_cast<double>(end - start) * to_ms);
            cpu_total += static_cast<double>(lit_end - lit_start) * to_ms;
        }
    }

    Timing timing;
    double sum = 0.0;
    for (double f : frames) {
        sum += f;
    }
    timing.mean_ms = sum / static_cast<double>(frames.size());
    std::sort(frames.begin(), frames.end());
    timing.p99_ms = frames[frames.size() * 99 / 100];
    timing.cpu_ms = cpu_total / static_cast<double>(frames.size());
    return timing;
}

} // namespace

namespace raven {

int run_lighting_benchmark(Renderer& renderer, const std::string& csv_path) {
    if (!renderer.lighting_ready()) {
        spdlog::error("Benchmark: the lighting pass is not available");
        return 1;
    }
    Tilemap room;
    if (!room.load(renderer.sdl_renderer(), paths::asset("assets/maps/raven.ldtk"), "Test_Room")) {
        spdlog::error("Benchmark: could not load the room");
        return 1;
    }
    renderer.set_vsync(false);

    // Bullet-sized lights, then room-sized ones. The loop takes 32 at most.
    const Case cases[] = {
        {"off", 0, 0},       {"loop", 0, 20},     {"loop", 8, 20},      {"loop", 16, 20},
        {"loop", 32, 20},    {"loop", 32, 80},    {"buffer", 0, 20},    {"buffer", 32, 20},
        {"buffer", 100, 20}, {"buffer", 300, 20}, {"buffer", 1000, 20}, {"buffer", 32, 80},
        {"buffer", 300, 80},
    };

    FILE* csv = std::fopen(csv_path.c_str(), "w");
    if (csv) {
        std::fprintf(csv, "path,lights,radius,frame_ms,frame_p99_ms,pass_ms,pass_cpu_ms\n");
    }
    const int w = Renderer::VIRTUAL_WIDTH;
    const int h = Renderer::VIRTUAL_HEIGHT;
    std::printf("\nLighting benchmark: %dx%d pass, %d frames a case, vsync off\n", w, h,
                MEASURED_FRAMES);
    std::printf("frame = a whole frame with one pass; pass = one more lighting pass;\n");
    std::printf("cpu = the part of a pass spent issuing it, before the GPU runs it\n\n");
    std::printf("%-7s %6s %6s %10s %10s %10s %10s\n", "path", "lights", "radius", "frame ms",
                "p99 ms", "pass ms", "cpu ms");

    for (const Case& c : cases) {
        const Timing once = run(renderer, room, c, 1);
        double pass_ms = 0.0;
        double pass_cpu_ms = 0.0;
        if (SDL_strcmp(c.path, "off") != 0) {
            const Timing many = run(renderer, room, c, MANY_PASSES);
            pass_ms = (many.mean_ms - once.mean_ms) / (MANY_PASSES - 1);
            pass_cpu_ms = (many.cpu_ms - once.cpu_ms) / (MANY_PASSES - 1);
        }
        std::printf("%-7s %6d %6d %10.3f %10.3f %10.4f %10.4f\n", c.path, c.lights, c.radius,
                    once.mean_ms, once.p99_ms, pass_ms, pass_cpu_ms);
        std::fflush(stdout);
        if (csv) {
            std::fprintf(csv, "%s,%d,%d,%.4f,%.4f,%.5f,%.5f\n", c.path, c.lights, c.radius,
                         once.mean_ms, once.p99_ms, pass_ms, pass_cpu_ms);
        }
    }
    if (csv) {
        std::fclose(csv);
        std::printf("\nwrote %s\n", csv_path.c_str());
    }
    return 0;
}

} // namespace raven

#endif // RAVEN_ENABLE_SHADER_LIGHTING
