#ifdef RAVEN_ENABLE_SHADER_LIGHTING

#include "debug/lighting_check.hpp"

#include "core/paths.hpp"
#include "ecs/systems/tilemap_render_system.hpp"
#include "rendering/renderer.hpp"
#include "rendering/tilemap.hpp"

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <cstdio>
#include <vector>

namespace {

constexpr float PLAIN_X = 120.f; ///< Centre of the plain light.
constexpr float COOL_X = 360.f;  ///< Centre of the cool light.
constexpr float LIGHT_Y = 135.f; ///< Both lights sit on the middle row.
constexpr float RADIUS = 54.f;
constexpr int NEAR = 16; ///< Half the side of the square measured around a light.

/// Sums over a square of a frame.
struct Region {
    long red = 0;
    long green = 0;
    long blue = 0;
};

/// A frame read back from the renderer, freed when it goes out of scope.
struct Frame {
    SDL_Surface* surface = nullptr;
    Frame() = default;
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
    ~Frame() {
        if (surface) {
            SDL_DestroySurface(surface);
        }
    }

    [[nodiscard]] const Uint8* pixel(int x, int y) const {
        return static_cast<const Uint8*>(surface->pixels) + y * surface->pitch + x * 4;
    }

    [[nodiscard]] Region sum(int cx, int cy, int half) const {
        Region r;
        for (int y = cy - half; y < cy + half; ++y) {
            for (int x = cx - half; x < cx + half; ++x) {
                const Uint8* p = pixel(x, y);
                r.red += p[0];
                r.green += p[1];
                r.blue += p[2];
            }
        }
        return r;
    }
};

/// Draw the room, light it if asked, and read the frame back as RGBA bytes.
bool draw(raven::Renderer& renderer, const raven::Tilemap& room,
          const raven::LightingSettings* settings, const std::vector<raven::Light>& lights,
          Frame& out, Frame* buffer = nullptr) {
    renderer.begin_frame();
    raven::systems::render_tilemap(room, renderer.sdl_renderer());
    if (settings) {
        renderer.apply_lighting(*settings, lights);
    }
    SDL_Surface* read = renderer.read_frame();
    if (buffer) {
        if (SDL_Surface* light = renderer.read_light_buffer()) {
            buffer->surface = SDL_ConvertSurface(light, SDL_PIXELFORMAT_RGBA32);
            SDL_DestroySurface(light);
        }
    }
    renderer.end_frame();
    renderer.present();
    renderer.wait_for_gpu();
    if (!read) {
        spdlog::error("Lighting check: could not read the frame back: {}", SDL_GetError());
        return false;
    }
    out.surface = SDL_ConvertSurface(read, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(read);
    return out.surface != nullptr;
}

/// Pixels that differ between two frames inside a rectangle.
int differing(const Frame& a, const Frame& b, int x0, int y0, int x1, int y1) {
    int count = 0;
    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            const Uint8* p = a.pixel(x, y);
            const Uint8* q = b.pixel(x, y);
            if (p[0] != q[0] || p[1] != q[1] || p[2] != q[2]) {
                ++count;
            }
        }
    }
    return count;
}

bool expect(bool holds, const char* what) {
    std::printf("  %-4s %s\n", holds ? "ok" : "FAIL", what);
    return holds;
}

} // namespace

namespace raven {

int run_lighting_check(Renderer& renderer, const std::string& png_path) {
    std::printf("\nLighting check: renderer '%s'\n", SDL_GetRendererName(renderer.sdl_renderer()));
    if (!renderer.lighting_ready()) {
        spdlog::error("Lighting check: the lighting pass is not available");
        return 1;
    }
    Tilemap room;
    if (!room.load(renderer.sdl_renderer(), paths::asset("assets/maps/raven.ldtk"), "Test_Room")) {
        spdlog::error("Lighting check: could not load the room");
        return 1;
    }

    const LightingSettings settings; // as the game starts
    const std::vector<Light> none;
    const std::vector<Light> lights = {
        {PLAIN_X, LIGHT_Y, RADIUS, 48, false},
        {COOL_X, LIGHT_Y, RADIUS, 48, true},
    };
    Frame drawn;
    Frame ambient;
    Frame lit;
    Frame buffer; // the light buffer behind the lit frame
    // Each is drawn twice: the first frame after start-up may not be whole
    for (int i = 0; i < 2; ++i) {
        Frame discard_drawn;
        Frame discard_ambient;
        Frame discard_lit;
        Frame discard_buffer;
        if (!draw(renderer, room, nullptr, none, i == 0 ? discard_drawn : drawn) ||
            !draw(renderer, room, &settings, none, i == 0 ? discard_ambient : ambient) ||
            !draw(renderer, room, &settings, lights, i == 0 ? discard_lit : lit,
                  i == 0 ? &discard_buffer : &buffer)) {
            return 1;
        }
    }

    const int w = lit.surface->w;
    const int h = lit.surface->h;
    const int px = static_cast<int>(PLAIN_X);
    const int cx = static_cast<int>(COOL_X);
    const int y = static_cast<int>(LIGHT_Y);

    const int shifted = differing(drawn, ambient, 0, 0, w, h);
    // A room that went black also "changed": compare how bright it is left
    const Region whole_drawn = drawn.sum(w / 2, h / 2, h / 2);
    const Region whole_ambient = ambient.sum(w / 2, h / 2, h / 2);
    const long bright_drawn = whole_drawn.red + whole_drawn.green + whole_drawn.blue;
    const long bright_ambient = whole_ambient.red + whole_ambient.green + whole_ambient.blue;
    const Region plain_before = ambient.sum(px, y, NEAR);
    const Region plain_after = lit.sum(px, y, NEAR);
    const Region cool_before = ambient.sum(cx, y, NEAR);
    const Region cool_after = lit.sum(cx, y, NEAR);
    // The strip between the two lights is out of reach of both
    const int gap_x0 = static_cast<int>(PLAIN_X + RADIUS) + 2;
    const int gap_x1 = static_cast<int>(COOL_X - RADIUS) - 2;
    const int stray = differing(ambient, lit, gap_x0, 0, gap_x1, h);

    const long plain_gain = (plain_after.red + plain_after.green + plain_after.blue) -
                            (plain_before.red + plain_before.green + plain_before.blue);
    const long blue_before = cool_before.blue - cool_before.red;
    const long blue_after = cool_after.blue - cool_after.red;
    const long plain_blue =
        (plain_after.blue - plain_after.red) - (plain_before.blue - plain_before.red);
    const long area = static_cast<long>(2 * NEAR) * (2 * NEAR);

    // What the lights left in the light buffer, apart from what the
    // resolve shader then made of it
    int buffer_plain = -1;
    int buffer_cool = -1;
    int buffer_cool_green = -1;
    if (buffer.surface) {
        buffer_plain = buffer.pixel(px, y)[0];
        buffer_cool = buffer.pixel(cx, y)[0];
        buffer_cool_green = buffer.pixel(cx, y)[1];
    }
    std::printf("  light buffer at the plain light's centre: %d; at the cool light's: %d, "
                "of which cool %d (48 expected for each)\n",
                buffer_plain, buffer_cool, buffer_cool_green);
    std::printf("  ambient changed %d of %d pixels, and left the room %.0f%% as bright\n", shifted,
                w * h,
                100.0 * static_cast<double>(bright_ambient) /
                    static_cast<double>(bright_drawn > 0 ? bright_drawn : 1));
    std::printf("  plain light: brightness +%.1f a pixel, blue over red %+.1f\n",
                static_cast<double>(plain_gain) / static_cast<double>(3 * area),
                static_cast<double>(plain_blue) / static_cast<double>(area));
    std::printf("  cool light: blue over red %+.1f to %+.1f a pixel\n",
                static_cast<double>(blue_before) / static_cast<double>(area),
                static_cast<double>(blue_after) / static_cast<double>(area));
    std::printf("  pixels changed out of reach of the lights: %d\n\n", stray);

    bool ok = true;
    ok &= expect(buffer_plain >= 44 && buffer_cool >= 44 && buffer_cool_green >= 44,
                 "lights are drawn into the light buffer");
    ok &= expect(shifted > w * h / 4, "the ambient shift changes the room (colour tables reach "
                                      "the shader)");
    ok &= expect(bright_ambient * 5 > bright_drawn && bright_ambient < bright_drawn,
                 "the ambient shift darkens the room without blacking it out (pixels are read "
                 "where they are drawn)");
    ok &= expect(plain_gain > 3 * area * 4, "a light makes its surroundings lighter (the light "
                                            "buffer reaches the shader)");
    ok &= expect(blue_after - blue_before > area * 8, "a cool light makes its surroundings bluer "
                                                      "(the tinted ramps are read)");
    ok &= expect(blue_after - blue_before > 2 * plain_blue, "a plain light does not tint as a "
                                                            "cool one does");
    ok &= expect(stray == 0, "pixels out of reach of the lights are left alone");

    // FNV-1a over the lit frame's colours, to compare between platforms
    std::uint64_t hash = 1469598103934665603ull;
    for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
            const Uint8* p = lit.pixel(col, row);
            for (int channel = 0; channel < 3; ++channel) {
                hash = (hash ^ p[channel]) * 1099511628211ull;
            }
        }
    }
    std::printf("\n  lit frame %dx%d, checksum %016llx\n", w, h,
                static_cast<unsigned long long>(hash));
    if (SDL_SavePNG(lit.surface, png_path.c_str())) {
        std::printf("  wrote %s\n", png_path.c_str());
    } else {
        spdlog::error("Lighting check: cannot write '{}': {}", png_path, SDL_GetError());
        ok = false;
    }
    std::printf("\nLighting check %s\n", ok ? "passed" : "FAILED");
    std::fflush(stdout);
    return ok ? 0 : 1;
}

} // namespace raven

#endif // RAVEN_ENABLE_SHADER_LIGHTING
