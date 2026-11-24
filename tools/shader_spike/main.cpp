// Spike for ADR-0028: shader-resolved palette and lighting.
//
// Throwaway. It answers the questions that could stop the proposal:
//
//   1. Does SDL's "gpu" renderer accept a custom fragment shader here?
//   2. Do (ramp, step) index values survive the way Raven draws sprites
//      (pixel-art scale mode, sub-pixel positions, colour modulation)?
//   3. Does one full-screen pass resolve the index target through a lookup
//      table exactly, including a dithered cross-fade and a stepped light?
//   4. Does the ImGui overlay still draw on the "gpu" renderer?
//
// Run with --check for the automated report, or with no arguments for the
// interactive view. See README.md next to this file.

#include "shaders/resolve.frag.dxil.h"
#include "shaders/resolve.frag.msl.h"
#include "shaders/resolve.frag.spv.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#ifdef RAVEN_SPIKE_IMGUI
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#endif

namespace {

constexpr int VW = 480; // Raven's virtual resolution
constexpr int VH = 270;
constexpr int RAMPS = 4;    // stone, vegetation, earth, character
constexpr int STEPS = 8;    // steps per ramp, including headroom at both ends
constexpr int VARIANTS = 2; // open daylight, dark ruins
constexpr int TILE = 16;
constexpr int SPR = 16;
constexpr int SPRITE_KINDS = 2;
constexpr int MOVERS = 14;

struct Rgba {
    Uint8 r, g, b, a;
    bool operator==(const Rgba&) const = default;
};

/// Row-major pixels, RGBA byte order.
struct Image {
    int w = 0;
    int h = 0;
    std::vector<Rgba> px;
    Image() = default;
    Image(int width, int height) : w(width), h(height), px(static_cast<size_t>(width * height)) {}
    Rgba& at(int x, int y) { return px[static_cast<size_t>(y * w + x)]; }
    const Rgba& at(int x, int y) const { return px[static_cast<size_t>(y * w + x)]; }
};

/// Matches the cbuffer in resolve.frag.hlsl.
struct ResolveUniforms {
    float size_variants[4]; // target w, target h, variant A, variant B
    float light[4];         // x, y, radius (whole pixels), zones
    float params[4];        // mix (0..16), ambient shift, ramp count, step count
    float extra[4];         // x: dither the light (1) or band it (0)
};

constexpr std::array<int, 16> BAYER = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};

struct Mover {
    float x, y, vx, vy;
    int kind;
    bool flip;
};

struct DrawConfig {
    const char* name;
    SDL_ScaleMode scale;
    bool snap;       // round sprite positions to whole pixels
    bool colour_mod; // SDL_SetTextureColorMod on sprites
    bool alpha_mod;  // SDL_SetTextureAlphaMod on sprites
    bool flip;       // honour Mover::flip
};

// ── Palette ──────────────────────────────────────────────────────────

Rgba hsv(float h, float s, float v) {
    h = std::fmod(std::fmod(h, 360.0f) + 360.0f, 360.0f);
    float c = v * s;
    float hp = h / 60.0f;
    float x = c * (1.0f - std::fabs(std::fmod(hp, 2.0f) - 1.0f));
    float r = 0, g = 0, b = 0;
    switch (static_cast<int>(hp)) {
    case 0:
        r = c, g = x;
        break;
    case 1:
        r = x, g = c;
        break;
    case 2:
        g = c, b = x;
        break;
    case 3:
        g = x, b = c;
        break;
    case 4:
        r = x, b = c;
        break;
    default:
        r = c, b = x;
        break;
    }
    float m = v - c;
    auto to8 = [m](float f) { return static_cast<Uint8>(std::lround((f + m) * 255.0f)); };
    return {to8(r), to8(g), to8(b), 255};
}

/// A stand-in for the table Pigment would export: hue-shifted ramps, cool
/// in shadow and warm in light, in two lighting variants.
Image make_lut() {
    struct Material {
        float hue, sat;
    };
    constexpr std::array<Material, RAMPS> materials = {{
        {215.0f, 0.18f}, // stone
        {115.0f, 0.50f}, // vegetation
        {32.0f, 0.48f},  // earth
        {348.0f, 0.62f}, // character (burgundy)
    }};

    Image lut(STEPS, RAMPS * VARIANTS);
    for (int variant = 0; variant < VARIANTS; ++variant) {
        for (int ramp = 0; ramp < RAMPS; ++ramp) {
            for (int step = 0; step < STEPS; ++step) {
                float t = static_cast<float>(step) / static_cast<float>(STEPS - 1);
                const Material& m = materials[static_cast<size_t>(ramp)];
                // Shadows pull toward blue-purple, highlights toward yellow.
                float hue =
                    m.hue + (0.5f - t) * 44.0f * (m.hue > 60.0f && m.hue < 240.0f ? 1.0f : -1.0f);
                float sat = m.sat * (1.0f - std::fabs(t - 0.5f) * 0.9f);
                float val = 0.10f + 0.86f * std::pow(t, 0.85f);
                if (variant == 1) { // dark ruins: cooler, compressed, dimmer
                    hue = hue + (235.0f - hue) * 0.30f;
                    sat *= 0.85f;
                    val = 0.05f + val * 0.62f;
                }
                lut.at(step, variant * RAMPS + ramp) = hsv(hue, sat, val);
            }
        }
    }
    return lut;
}

// ── Index art, encoded by hand ───────────────────────────────────────

Uint8 noise(int x, int y) {
    auto n = static_cast<unsigned>(x * 374761393 + y * 668265263);
    n = (n ^ (n >> 13)) * 1274126177u;
    return static_cast<Uint8>((n >> 16) & 0xff);
}

/// Three 16x16 tiles: stone, vegetation, earth. R = ramp, G = step.
Image make_tile_sheet() {
    Image sheet(TILE * 3, TILE);
    for (int t = 0; t < 3; ++t) {
        for (int y = 0; y < TILE; ++y) {
            for (int x = 0; x < TILE; ++x) {
                int step = 3 + (noise(x + t * 31, y) % 5 == 0 ? 1 : 0) -
                           (noise(x, y + t * 17) % 7 == 0 ? 1 : 0);
                if (t == 0 && (y % 8 == 0 || (x + (y / 8) * 4) % 8 == 0)) {
                    step = 2; // seams between stone blocks
                }
                sheet.at(t * TILE + x, y) = {static_cast<Uint8>(t), static_cast<Uint8>(step), 0,
                                             255};
            }
        }
    }
    return sheet;
}

/// Two 16x16 shaded discs with an unlit outline (B = 1).
Image make_sprite_sheet() {
    Image sheet(SPR * SPRITE_KINDS, SPR);
    constexpr std::array<int, SPRITE_KINDS> ramps = {3, 1};
    for (int k = 0; k < SPRITE_KINDS; ++k) {
        for (int y = 0; y < SPR; ++y) {
            for (int x = 0; x < SPR; ++x) {
                float dx = static_cast<float>(x) - 7.5f;
                float dy = static_cast<float>(y) - 7.5f;
                float d = std::sqrt(dx * dx + dy * dy);
                Rgba p{0, 0, 0, 0};
                auto ramp = static_cast<Uint8>(ramps[static_cast<size_t>(k)]);
                if (d <= 7.6f && d > 6.4f) {
                    p = {ramp, 0, 1, 255}; // outline: darkest step, never lit
                } else if (d <= 6.4f) {
                    float lit = (-dx - dy) / 9.0f; // light from the top left
                    int step = std::clamp(static_cast<int>(std::lround(3.5f + lit * 2.0f)), 2, 5);
                    p = {ramp, static_cast<Uint8>(step), 0, 255};
                }
                sheet.at(k * SPR + x, y) = p;
            }
        }
    }
    return sheet;
}

int tile_at(int tx, int ty) {
    if (ty == 8 || ty == 9) {
        return 2; // an earth path
    }
    return ((tx * 7 + ty * 13) % 11 < 3) ? 1 : 0;
}

std::vector<Mover> make_movers() {
    std::vector<Mover> movers;
    for (int i = 0; i < MOVERS; ++i) {
        Mover m{};
        // Fractions chosen away from .5 so rounding is unambiguous.
        m.x = static_cast<float>((i * 97) % (VW - SPR)) + 0.37f;
        m.y = static_cast<float>((i * 53) % (VH - SPR)) + 0.61f;
        m.vx = 9.0f + static_cast<float>(i % 5) * 4.3f;
        m.vy = 6.0f + static_cast<float>(i % 3) * 5.1f;
        m.kind = i % SPRITE_KINDS;
        m.flip = (i % 4 == 1);
        movers.push_back(m);
    }
    return movers;
}

// ── SDL helpers ──────────────────────────────────────────────────────

SDL_Texture* texture_from(SDL_Renderer* renderer, const Image& image) {
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                             SDL_TEXTUREACCESS_STATIC, image.w, image.h);
    if (texture) {
        SDL_UpdateTexture(texture, nullptr, image.px.data(), image.w * 4);
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    }
    return texture;
}

/// Read back a render target (or the window when texture is null).
Image read_pixels(SDL_Renderer* renderer, SDL_Texture* texture) {
    SDL_SetRenderTarget(renderer, texture);
    SDL_Surface* raw = SDL_RenderReadPixels(renderer, nullptr);
    if (!raw) {
        return {};
    }
    SDL_Surface* rgba = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(raw);
    if (!rgba) {
        return {};
    }
    Image image(rgba->w, rgba->h);
    for (int y = 0; y < rgba->h; ++y) {
        const auto* row = static_cast<const Uint8*>(rgba->pixels) + y * rgba->pitch;
        std::copy_n(row, rgba->w * 4, reinterpret_cast<Uint8*>(&image.at(0, y)));
    }
    SDL_DestroySurface(rgba);
    return image;
}

bool save_png(const Image& image, const std::string& path, int scale) {
    if (image.px.empty() || path.empty()) {
        return false;
    }
    SDL_Surface* surface = SDL_CreateSurfaceFrom(image.w, image.h, SDL_PIXELFORMAT_RGBA32,
                                                 const_cast<Rgba*>(image.px.data()), image.w * 4);
    if (!surface) {
        return false;
    }
    SDL_Surface* scaled =
        SDL_ScaleSurface(surface, image.w * scale, image.h * scale, SDL_SCALEMODE_NEAREST);
    bool ok = scaled && SDL_SavePNG(scaled, path.c_str());
    SDL_DestroySurface(scaled);
    SDL_DestroySurface(surface);
    return ok;
}

// ── Index pass ───────────────────────────────────────────────────────

void draw_index_scene(SDL_Renderer* renderer, SDL_Texture* index_target, SDL_Texture* tiles,
                      SDL_Texture* sprites, const std::vector<Mover>& movers,
                      const DrawConfig& cfg) {
    SDL_SetRenderTarget(renderer, index_target);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetTextureScaleMode(tiles, cfg.scale);
    SDL_SetTextureBlendMode(tiles, SDL_BLENDMODE_NONE);
    for (int ty = 0; ty * TILE < VH; ++ty) {
        for (int tx = 0; tx * TILE < VW; ++tx) {
            SDL_FRect src{static_cast<float>(tile_at(tx, ty) * TILE), 0.0f, TILE, TILE};
            SDL_FRect dst{static_cast<float>(tx * TILE), static_cast<float>(ty * TILE), TILE, TILE};
            SDL_RenderTexture(renderer, tiles, &src, &dst);
        }
    }

    SDL_SetTextureScaleMode(sprites, cfg.scale);
    SDL_SetTextureBlendMode(sprites, SDL_BLENDMODE_BLEND);
    if (cfg.colour_mod) {
        SDL_SetTextureColorMod(sprites, 255, 128, 128);
    } else {
        SDL_SetTextureColorMod(sprites, 255, 255, 255);
    }
    SDL_SetTextureAlphaMod(sprites, cfg.alpha_mod ? 128 : 255);
    for (const Mover& m : movers) {
        SDL_FRect src{static_cast<float>(m.kind * SPR), 0.0f, SPR, SPR};
        SDL_FRect dst{cfg.snap ? std::round(m.x) : m.x, cfg.snap ? std::round(m.y) : m.y, SPR, SPR};
        SDL_FlipMode flip = (cfg.flip && m.flip) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        SDL_RenderTextureRotated(renderer, sprites, &src, &dst, 0.0, nullptr, flip);
    }
}

/// What the index target should hold when every sprite sits on a whole pixel.
Image cpu_index_scene(const Image& tiles, const Image& sprites, const std::vector<Mover>& movers,
                      bool honour_flip) {
    Image out(VW, VH);
    for (int y = 0; y < VH; ++y) {
        for (int x = 0; x < VW; ++x) {
            out.at(x, y) = tiles.at(tile_at(x / TILE, y / TILE) * TILE + x % TILE, y % TILE);
        }
    }
    for (const Mover& m : movers) {
        int ox = static_cast<int>(std::round(m.x));
        int oy = static_cast<int>(std::round(m.y));
        for (int y = 0; y < SPR; ++y) {
            for (int x = 0; x < SPR; ++x) {
                int sx = (honour_flip && m.flip) ? SPR - 1 - x : x;
                Rgba p = sprites.at(m.kind * SPR + sx, y);
                int dx = ox + x;
                int dy = oy + y;
                if (p.a == 255 && dx >= 0 && dy >= 0 && dx < VW && dy < VH) {
                    out.at(dx, dy) = p;
                }
            }
        }
    }
    return out;
}

bool valid_index(Rgba p) {
    return p.a == 255 && p.r < RAMPS && p.g < STEPS && p.b <= 1;
}

int count_invalid(const Image& image) {
    return static_cast<int>(
        std::count_if(image.px.begin(), image.px.end(), [](Rgba p) { return !valid_index(p); }));
}

int count_different(const Image& a, const Image& b) {
    if (a.w != b.w || a.h != b.h) {
        return -1;
    }
    int n = 0;
    for (size_t i = 0; i < a.px.size(); ++i) {
        n += (a.px[i] == b.px[i]) ? 0 : 1;
    }
    return n;
}

// ── Resolve pass ─────────────────────────────────────────────────────

ResolveUniforms make_uniforms(float variant_a, float variant_b, float mix16, float ambient,
                              float light_x, float light_y, float radius, float zones,
                              bool dither = true) {
    ResolveUniforms u{};
    u.size_variants[0] = VW;
    u.size_variants[1] = VH;
    u.size_variants[2] = variant_a;
    u.size_variants[3] = variant_b;
    u.light[0] = std::floor(light_x);
    u.light[1] = std::floor(light_y);
    u.light[2] = radius;
    u.light[3] = zones;
    u.extra[0] = dither ? 1.0f : 0.0f;
    u.params[0] = mix16;
    u.params[1] = ambient;
    u.params[2] = RAMPS;
    u.params[3] = STEPS;
    return u;
}

/// The same arithmetic as resolve.frag.hlsl, on the CPU. It is both the
/// reference the shader is checked against and the shader-free path.
Image cpu_resolve(const Image& index, const Image& lut, const ResolveUniforms& u) {
    Image out(index.w, index.h);
    for (int y = 0; y < index.h; ++y) {
        for (int x = 0; x < index.w; ++x) {
            Rgba in = index.at(x, y);
            auto threshold = static_cast<float>(BAYER[static_cast<size_t>((y & 3) * 4 + (x & 3))]);

            float shift = u.params[1];
            int radius = static_cast<int>(u.light[2]);
            int zones = static_cast<int>(u.light[3]);
            if (radius > 0 && zones > 0) {
                int ddx = x * 2 + 1 - static_cast<int>(u.light[0]) * 2;
                int ddy = y * 2 + 1 - static_cast<int>(u.light[1]) * 2;
                int d2x4 = ddx * ddx + ddy * ddy;
                int r2x4 = 4 * radius * radius;
                if (d2x4 < r2x4) {
                    int t = ((r2x4 - d2x4) * 256) / r2x4;
                    int level16 = (zones * 16 * t * t) >> 16;
                    if (u.extra[0] < 0.5f) {
                        shift += static_cast<float>((level16 + 8) >> 4);
                    } else {
                        shift += static_cast<float>(level16 >> 4);
                        shift += (static_cast<float>(level16 & 15) > threshold) ? 1.0f : 0.0f;
                    }
                }
            }
            if (in.b & 1) {
                shift = 0.0f;
            }
            float step = std::clamp(static_cast<float>(in.g) + shift, 0.0f, u.params[3] - 1.0f);
            float variant = (u.params[0] > threshold) ? u.size_variants[3] : u.size_variants[2];

            int lx = static_cast<int>(step);
            int ly = static_cast<int>(variant * u.params[2] + static_cast<float>(in.r));
            Rgba c{255, 0, 255, 255}; // magenta marks an index outside the table
            if (lx >= 0 && lx < lut.w && ly >= 0 && ly < lut.h) {
                c = lut.at(lx, ly);
            }
            c.a = 255;
            out.at(x, y) = c;
        }
    }
    return out;
}

void gpu_resolve(SDL_Renderer* renderer, SDL_GPURenderState* state, SDL_Texture* index_target,
                 SDL_Texture* colour_target, const ResolveUniforms& u) {
    SDL_SetGPURenderStateFragmentUniforms(state, 0, &u, sizeof(u));
    SDL_SetRenderTarget(renderer, colour_target);
    SDL_SetGPURenderState(renderer, state);
    SDL_RenderTexture(renderer, index_target, nullptr, nullptr);
    SDL_SetGPURenderState(renderer, nullptr);
}

/// Bullets and glows: ordinary colour, additive, drawn after the resolve.
void draw_emissives(SDL_Renderer* renderer, SDL_Texture* colour_target, float time) {
    SDL_SetRenderTarget(renderer, colour_target);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_ADD);
    for (int i = 0; i < 10; ++i) {
        float a = time * 1.3f + static_cast<float>(i) * 0.628f;
        float x = std::round(240.0f + std::cos(a) * (40.0f + static_cast<float>(i) * 9.0f));
        float y = std::round(135.0f + std::sin(a * 1.2f) * (30.0f + static_cast<float>(i) * 6.0f));
        SDL_SetRenderDrawColor(renderer, 90, 40, 10, 255);
        SDL_FRect halo{x - 3.0f, y - 3.0f, 7.0f, 7.0f};
        SDL_RenderFillRect(renderer, &halo);
        SDL_SetRenderDrawColor(renderer, 255, 210, 120, 255);
        SDL_FRect core{x - 1.0f, y - 1.0f, 3.0f, 3.0f};
        SDL_RenderFillRect(renderer, &core);
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

// ── The spike ────────────────────────────────────────────────────────

struct Spike {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_GPUDevice* device = nullptr;
    SDL_GPUShader* shader = nullptr;
    SDL_GPUSampler* lut_sampler = nullptr;
    SDL_GPURenderState* state = nullptr;
    SDL_Texture* index_target = nullptr;
    SDL_Texture* colour_target = nullptr;
    SDL_Texture* scratch_target = nullptr; // a second colour target for one check
    SDL_Texture* cpu_texture = nullptr;    // receives the CPU resolve
    SDL_Texture* tiles = nullptr;
    SDL_Texture* sprites = nullptr;
    SDL_Texture* lut_texture = nullptr;
    Image tile_image, sprite_image, lut_image;
    std::vector<Mover> movers;
};

bool init(Spike& s, bool hidden, const char* driver) {
    if (driver) {
        SDL_SetHint(SDL_HINT_GPU_DRIVER, driver);
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::printf("[FAIL] SDL_Init: %s\n", SDL_GetError());
        return false;
    }
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | (hidden ? SDL_WINDOW_HIDDEN : 0);
    s.window = SDL_CreateWindow("ADR-0028 shader spike", VW * 2, VH * 2, flags);
    if (!s.window) {
        std::printf("[FAIL] SDL_CreateWindow: %s\n", SDL_GetError());
        return false;
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_RENDERER_CREATE_NAME_STRING, SDL_GPU_RENDERER);
    SDL_SetPointerProperty(props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, s.window);
    SDL_SetBooleanProperty(props, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_SPIRV_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_DXIL_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_MSL_BOOLEAN, true);
    s.renderer = SDL_CreateRendererWithProperties(props);
    SDL_DestroyProperties(props);
    if (!s.renderer) {
        std::printf("[FAIL] create \"gpu\" renderer: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetRenderVSync(s.renderer, 1);

    s.device = SDL_GetGPURendererDevice(s.renderer);
    if (!s.device) {
        std::printf("[FAIL] SDL_GetGPURendererDevice: %s\n", SDL_GetError());
        return false;
    }
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(s.device);
    std::printf("[PASS] renderer \"%s\" on GPU driver \"%s\" (SDL %d.%d.%d)\n",
                SDL_GetRendererName(s.renderer), SDL_GetGPUDeviceDriver(s.device),
                SDL_VERSIONNUM_MAJOR(SDL_GetVersion()), SDL_VERSIONNUM_MINOR(SDL_GetVersion()),
                SDL_VERSIONNUM_MICRO(SDL_GetVersion()));

    SDL_GPUShaderCreateInfo info{};
    const char* format_name = "none";
    if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        info.code = resolve_frag_spv;
        info.code_size = sizeof(resolve_frag_spv);
        format_name = "SPIR-V";
    } else if (formats & SDL_GPU_SHADERFORMAT_DXIL) {
        info.format = SDL_GPU_SHADERFORMAT_DXIL;
        info.code = resolve_frag_dxil;
        info.code_size = sizeof(resolve_frag_dxil);
        format_name = "DXIL";
    } else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
        info.format = SDL_GPU_SHADERFORMAT_MSL;
        info.code = resolve_frag_msl;
        info.code_size = sizeof(resolve_frag_msl);
        format_name = "MSL";
    } else {
        std::printf("[FAIL] the device accepts none of SPIR-V, DXIL, MSL\n");
        return false;
    }
    info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    info.num_samplers = 2; // the index target, then the lookup table
    info.num_uniform_buffers = 1;
    s.shader = SDL_CreateGPUShader(s.device, &info);
    if (!s.shader) {
        std::printf("[FAIL] SDL_CreateGPUShader (%s): %s\n", format_name, SDL_GetError());
        return false;
    }
    std::printf("[PASS] fragment shader created from %s\n", format_name);

    s.tile_image = make_tile_sheet();
    s.sprite_image = make_sprite_sheet();
    s.lut_image = make_lut();
    s.movers = make_movers();
    s.tiles = texture_from(s.renderer, s.tile_image);
    s.sprites = texture_from(s.renderer, s.sprite_image);
    s.lut_texture = texture_from(s.renderer, s.lut_image);

    auto make_target = [&](SDL_ScaleMode scale) {
        SDL_Texture* t =
            SDL_CreateTexture(s.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, VW, VH);
        if (t) {
            SDL_SetTextureScaleMode(t, scale);
            SDL_SetTextureBlendMode(t, SDL_BLENDMODE_NONE);
        }
        return t;
    };
    s.index_target = make_target(SDL_SCALEMODE_NEAREST);
    s.colour_target = make_target(SDL_SCALEMODE_PIXELART); // as Raven presents
    s.scratch_target = make_target(SDL_SCALEMODE_NEAREST);
    s.cpu_texture =
        SDL_CreateTexture(s.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, VW, VH);
    if (!s.tiles || !s.sprites || !s.lut_texture || !s.index_target || !s.colour_target ||
        !s.scratch_target || !s.cpu_texture) {
        std::printf("[FAIL] texture creation: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetTextureScaleMode(s.cpu_texture, SDL_SCALEMODE_PIXELART);
    SDL_SetTextureBlendMode(s.cpu_texture, SDL_BLENDMODE_NONE);

    auto* lut_gpu = static_cast<SDL_GPUTexture*>(SDL_GetPointerProperty(
        SDL_GetTextureProperties(s.lut_texture), SDL_PROP_TEXTURE_GPU_TEXTURE_POINTER, nullptr));
    if (!lut_gpu) {
        std::printf("[FAIL] the lookup table SDL_Texture has no SDL_GPUTexture behind it\n");
        return false;
    }
    SDL_GPUSamplerCreateInfo sampler_info{};
    sampler_info.min_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mag_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    s.lut_sampler = SDL_CreateGPUSampler(s.device, &sampler_info);

    SDL_GPUTextureSamplerBinding binding{lut_gpu, s.lut_sampler};
    SDL_GPURenderStateCreateInfo state_info{};
    state_info.fragment_shader = s.shader;
    state_info.num_sampler_bindings = 1;
    state_info.sampler_bindings = &binding;
    s.state = SDL_CreateGPURenderState(s.renderer, &state_info);
    if (!s.state) {
        std::printf("[FAIL] SDL_CreateGPURenderState: %s\n", SDL_GetError());
        return false;
    }
    std::printf("[PASS] render state created with the lookup table as a second sampler\n");

    SDL_SetRenderLogicalPresentation(s.renderer, VW, VH, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    return true;
}

void shutdown(Spike& s) {
    if (s.state) {
        SDL_DestroyGPURenderState(s.state);
    }
    if (s.lut_sampler) {
        SDL_ReleaseGPUSampler(s.device, s.lut_sampler);
    }
    if (s.shader) {
        SDL_ReleaseGPUShader(s.device, s.shader);
    }
    for (SDL_Texture* t : {s.index_target, s.colour_target, s.scratch_target, s.cpu_texture,
                           s.tiles, s.sprites, s.lut_texture}) {
        if (t) {
            SDL_DestroyTexture(t);
        }
    }
    if (s.renderer) {
        SDL_DestroyRenderer(s.renderer);
    }
    if (s.window) {
        SDL_DestroyWindow(s.window);
    }
    SDL_Quit();
}

void present_to_window(Spike& s, SDL_Texture* frame) {
    SDL_SetRenderTarget(s.renderer, nullptr);
    SDL_SetRenderDrawColor(s.renderer, 0, 0, 0, 255);
    SDL_RenderClear(s.renderer);
    SDL_RenderTexture(s.renderer, frame, nullptr, nullptr);
}

// ── Automated checks ─────────────────────────────────────────────────

constexpr DrawConfig SAFE{
    "nearest, whole pixels, no modulation", SDL_SCALEMODE_NEAREST, true, false, false, true};

/// Returns true when every must-pass check passed.
bool run_checks(Spike& s, const std::string& out_dir) {
    bool ok = true;
    auto must = [&ok](bool pass, const std::string& text) {
        std::printf("[%s] %s\n", pass ? "PASS" : "FAIL", text.c_str());
        ok = ok && pass;
    };
    auto finding = [](bool clean, const std::string& text) {
        std::printf("[%s] %s\n", clean ? "OK  " : "BAD ", text.c_str());
    };
    // No --out: paths are empty and save_png writes nothing.
    auto path = [&out_dir](const char* name) {
        return out_dir.empty() ? std::string() : out_dir + "/" + name;
    };

    // 1. Index integrity. The reference is the scene with every sprite on a
    //    whole pixel; a config is safe when it reproduces it exactly.
    std::printf("\n-- Index integrity (%d pixels per frame) --\n", VW * VH);
    Image reference = cpu_index_scene(s.tile_image, s.sprite_image, s.movers, true);
    must(count_invalid(reference) == 0, "the hand-encoded art holds only valid indices");

    draw_index_scene(s.renderer, s.index_target, s.tiles, s.sprites, s.movers, SAFE);
    Image safe_index = read_pixels(s.renderer, s.index_target);
    must(!safe_index.px.empty(), "the index target can be read back");
    int safe_diff = count_different(safe_index, reference);
    must(safe_diff == 0, std::string(SAFE.name) + ": " + std::to_string(safe_diff) +
                             " pixels differ from the reference");

    const std::array<DrawConfig, 6> variations = {{
        {"nearest, sub-pixel positions", SDL_SCALEMODE_NEAREST, false, false, false, true},
        {"pixel-art scale mode, whole pixels", SDL_SCALEMODE_PIXELART, true, false, false, true},
        {"pixel-art scale mode, sub-pixel positions (Raven today)", SDL_SCALEMODE_PIXELART, false,
         false, false, true},
        {"linear scale mode, sub-pixel positions", SDL_SCALEMODE_LINEAR, false, false, false, true},
        {"nearest, whole pixels, colour modulation", SDL_SCALEMODE_NEAREST, true, true, false,
         true},
        {"nearest, whole pixels, alpha modulation", SDL_SCALEMODE_NEAREST, true, false, true, true},
    }};
    Image worst_index;
    for (const DrawConfig& cfg : variations) {
        draw_index_scene(s.renderer, s.index_target, s.tiles, s.sprites, s.movers, cfg);
        Image got = read_pixels(s.renderer, s.index_target);
        int invalid = count_invalid(got);
        int differ = count_different(got, reference);
        finding(invalid == 0 && differ == 0,
                std::string(cfg.name) + ": " + std::to_string(invalid) + " invalid, " +
                    std::to_string(differ) + " differ from the reference");
        if (cfg.scale == SDL_SCALEMODE_LINEAR) {
            worst_index = got;
        }
    }

    // 2. Resolve. The shader must match the CPU arithmetic exactly.
    std::printf("\n-- Resolve pass against the CPU reference --\n");
    struct Case {
        const char* name;
        const char* file;
        ResolveUniforms u;
    };
    const std::array<Case, 6> cases = {{
        {"variant A (daylight)", "1-daylight.png", make_uniforms(0, 1, 0, 0, 0, 0, 0, 0)},
        {"variant B (dark ruins)", "2-dark-ruins.png", make_uniforms(0, 1, 16, 0, 0, 0, 0, 0)},
        {"dithered cross-fade, half way", "3-crossfade.png", make_uniforms(0, 1, 8, 0, 0, 0, 0, 0)},
        {"dark ruins, ambient -2, one light of four zones", "4-light.png",
         make_uniforms(1, 1, 0, -2, 150, 120, 90, 4)},
        {"cross-fade and light together", "5-crossfade-light.png",
         make_uniforms(0, 1, 6, -1, 300, 150, 70, 3)},
        {"the same light as flat bands, no dither", "4b-light-banded.png",
         make_uniforms(1, 1, 0, -2, 150, 120, 90, 4, false)},
    }};
    draw_index_scene(s.renderer, s.index_target, s.tiles, s.sprites, s.movers, SAFE);
    for (const Case& c : cases) {
        gpu_resolve(s.renderer, s.state, s.index_target, s.colour_target, c.u);
        Image got = read_pixels(s.renderer, s.colour_target);
        Image want = cpu_resolve(safe_index, s.lut_image, c.u);
        int differ = count_different(got, want);
        must(differ == 0, std::string(c.name) + ": " + std::to_string(differ) +
                              " pixels differ from the CPU reference");
        save_png(got, path(c.file), 2);
    }

    // Every resolved colour must be a table colour: lighting stays on-palette.
    {
        gpu_resolve(s.renderer, s.state, s.index_target, s.colour_target, cases[4].u);
        Image got = read_pixels(s.renderer, s.colour_target);
        int off = 0;
        for (Rgba p : got.px) {
            bool found = std::any_of(s.lut_image.px.begin(), s.lut_image.px.end(), [p](Rgba q) {
                return p.r == q.r && p.g == q.g && p.b == q.b;
            });
            off += found ? 0 : 1;
        }
        must(off == 0,
             "cross-fade and light: " + std::to_string(off) + " pixels are off the palette");
    }

    // What the corruption looks like once resolved, for the record.
    if (!worst_index.px.empty()) {
        save_png(cpu_resolve(worst_index, s.lut_image, cases[0].u), path("6-corrupted-linear.png"),
                 2);
    }
    draw_index_scene(s.renderer, s.index_target, s.tiles, s.sprites, s.movers, variations[2]);
    gpu_resolve(s.renderer, s.state, s.index_target, s.colour_target, cases[0].u);
    save_png(read_pixels(s.renderer, s.colour_target), path("7-raven-today.png"), 2);

    // 3. Two resolves with different uniforms and no read-back between.
    //    SDL copies uniform data when it is set; this checks each draw keeps
    //    the values that were current when it was queued.
    std::printf("\n-- Render state behaviour --\n");
    draw_index_scene(s.renderer, s.index_target, s.tiles, s.sprites, s.movers, SAFE);
    gpu_resolve(s.renderer, s.state, s.index_target, s.colour_target, cases[0].u);
    gpu_resolve(s.renderer, s.state, s.index_target, s.scratch_target, cases[1].u);
    {
        Image first = read_pixels(s.renderer, s.colour_target);
        Image second = read_pixels(s.renderer, s.scratch_target);
        int d1 = count_different(first, cpu_resolve(safe_index, s.lut_image, cases[0].u));
        int d2 = count_different(second, cpu_resolve(safe_index, s.lut_image, cases[1].u));
        finding(d1 == 0 && d2 == 0,
                "two resolves queued with different uniforms: first differs by " +
                    std::to_string(d1) + ", second by " + std::to_string(d2));
    }

    // Ordinary drawing after the state is cleared: an additive emissive.
    gpu_resolve(s.renderer, s.state, s.index_target, s.colour_target, cases[1].u);
    Image before = read_pixels(s.renderer, s.colour_target);
    draw_emissives(s.renderer, s.colour_target, 0.0f);
    Image after = read_pixels(s.renderer, s.colour_target);
    int lit = count_different(before, after);
    must(lit > 0 && lit < 2000,
         "additive emissives draw after the resolve (" + std::to_string(lit) + " pixels changed)");
    save_png(after, path("8-emissives.png"), 2);

    // 4. Presentation to the window, and ImGui on top.
    std::printf("\n-- Window and overlay --\n");
    present_to_window(s, s.colour_target);
    Image window_plain = read_pixels(s.renderer, nullptr);
    finding(!window_plain.px.empty(),
            window_plain.px.empty()
                ? std::string("the window could not be read back: ") + SDL_GetError()
                : "window read back at " + std::to_string(window_plain.w) + "x" +
                      std::to_string(window_plain.h));
    SDL_RenderPresent(s.renderer);
#ifdef RAVEN_SPIKE_IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui_ImplSDL3_InitForSDLRenderer(s.window, s.renderer);
    ImGui_ImplSDLRenderer3_Init(s.renderer);
    Image window_imgui;
    for (int frame = 0; frame < 3; ++frame) { // ImGui needs a frame to lay out
        present_to_window(s, s.colour_target);
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(20, 20));
        ImGui::SetNextWindowSize(ImVec2(320, 120));
        ImGui::Begin("ADR-0028 spike");
        ImGui::Text("ImGui on the gpu renderer");
        ImGui::Text("after a custom render state");
        ImGui::End();
        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), s.renderer);
        window_imgui = read_pixels(s.renderer, nullptr);
        SDL_RenderPresent(s.renderer);
    }
    int overlay = count_different(window_plain, window_imgui);
    finding(overlay > 1000, "ImGui overlay changed " + std::to_string(overlay) +
                                " window pixels (expected several thousand)");
    save_png(window_imgui, path("9-window-imgui.png"), 1);
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
#else
    std::printf("[SKIP] ImGui overlay: built without RAVEN_ENABLE_IMGUI\n");
    save_png(window_plain, path("9-window.png"), 1);
#endif

    // 5. What the render-state call does on a renderer that is not "gpu".
    //    Last, because it hands SDL a shader from another device.
    std::printf("\n-- Render state on a non-gpu renderer --\n");
    std::fflush(stdout);
    SDL_Window* other_window = SDL_CreateWindow("probe", 64, 64, SDL_WINDOW_HIDDEN);
    SDL_Renderer* other = nullptr;
    for (const char* name : {"opengl", "vulkan", "direct3d11", "metal", "software"}) {
        other = SDL_CreateRenderer(other_window, name);
        if (other) {
            break;
        }
    }
    if (other) {
        SDL_GPURenderStateCreateInfo probe{};
        probe.fragment_shader = s.shader;
        SDL_ClearError();
        SDL_GPURenderState* state = SDL_CreateGPURenderState(other, &probe);
        std::printf("[INFO] on renderer \"%s\": SDL_GetGPURendererDevice is %s, "
                    "SDL_CreateGPURenderState returned %s (%s)\n",
                    SDL_GetRendererName(other), SDL_GetGPURendererDevice(other) ? "set" : "null",
                    state ? "a state" : "null", state ? "no error" : SDL_GetError());
        if (state) {
            SDL_DestroyGPURenderState(state);
        }
        SDL_DestroyRenderer(other);
    } else {
        std::printf("[INFO] no non-gpu renderer could be created here\n");
    }
    SDL_DestroyWindow(other_window);

    std::printf("\n%s\n", ok ? "RESULT: all must-pass checks passed"
                             : "RESULT: at least one must-pass check FAILED");
    return ok;
}

// ── Interactive view ─────────────────────────────────────────────────

void run_interactive(Spike& s) {
    bool running = true;
    bool snap = true;
    bool pixelart = false;
    bool colour_mod = false;
    bool light_on = true;
    bool auto_fade = true;
    bool cpu_path = false;
    bool emissives = true;
    bool dither = true;
    int zones = 4;
    float radius = 90.0f;
    float mix = 0.0f; // 0..16
    float mix_target = 0.0f;
    float light_x = 240.0f;
    float light_y = 135.0f;
    float time = 0.0f;
    int invalid = 0;
    int frame = 0;
    Uint64 last = SDL_GetTicksNS();

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                SDL_ConvertEventToRenderCoordinates(s.renderer, &event);
                light_x = event.motion.x;
                light_y = event.motion.y;
            } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                radius = std::clamp(radius + event.wheel.y * 6.0f, 12.0f, 240.0f);
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.key) {
                case SDLK_D:
                    dither = !dither;
                    break;
                case SDLK_LEFTBRACKET:
                    zones = std::max(1, zones - 1);
                    break;
                case SDLK_RIGHTBRACKET:
                    zones = std::min(6, zones + 1);
                    break;
                case SDLK_ESCAPE:
                    running = false;
                    break;
                case SDLK_1:
                    mix_target = 0.0f, auto_fade = false;
                    break;
                case SDLK_2:
                    mix_target = 16.0f, auto_fade = false;
                    break;
                case SDLK_C:
                    auto_fade = !auto_fade;
                    break;
                case SDLK_L:
                    light_on = !light_on;
                    break;
                case SDLK_S:
                    snap = !snap;
                    break;
                case SDLK_P:
                    pixelart = !pixelart;
                    break;
                case SDLK_M:
                    colour_mod = !colour_mod;
                    break;
                case SDLK_F:
                    cpu_path = !cpu_path;
                    break;
                case SDLK_E:
                    emissives = !emissives;
                    break;
                default:
                    break;
                }
            }
        }

        Uint64 now = SDL_GetTicksNS();
        float dt = std::min(static_cast<float>(now - last) / 1e9f, 0.05f);
        last = now;
        time += dt;
        for (Mover& m : s.movers) {
            m.x += m.vx * dt;
            m.y += m.vy * dt;
            if (m.x < 0.0f || m.x > VW - SPR) {
                m.vx = -m.vx;
                m.flip = !m.flip;
                m.x = std::clamp(m.x, 0.0f, static_cast<float>(VW - SPR));
            }
            if (m.y < 0.0f || m.y > VH - SPR) {
                m.vy = -m.vy;
                m.y = std::clamp(m.y, 0.0f, static_cast<float>(VH - SPR));
            }
        }
        if (auto_fade) {
            mix_target = (std::fmod(time, 12.0f) < 6.0f) ? 0.0f : 16.0f;
        }
        mix += std::clamp(mix_target - mix, -dt * 8.0f, dt * 8.0f);

        DrawConfig cfg{"live", pixelart ? SDL_SCALEMODE_PIXELART : SDL_SCALEMODE_NEAREST,
                       snap,   colour_mod,
                       false,  true};
        draw_index_scene(s.renderer, s.index_target, s.tiles, s.sprites, s.movers, cfg);

        // The shader-free path has one palette per room and no light.
        ResolveUniforms u =
            cpu_path
                ? make_uniforms(0, 1, mix >= 8.0f ? 16.0f : 0.0f, 0, 0, 0, 0, 0)
                : make_uniforms(0, 1, std::floor(mix), light_on ? -2.0f : 0.0f, light_x, light_y,
                                light_on ? radius : 0.0f, static_cast<float>(zones), dither);

        SDL_Texture* shown = s.colour_target;
        if (cpu_path || frame % 20 == 0) {
            Image index = read_pixels(s.renderer, s.index_target);
            invalid = count_invalid(index);
            if (cpu_path) {
                Image resolved = cpu_resolve(index, s.lut_image, u);
                SDL_UpdateTexture(s.cpu_texture, nullptr, resolved.px.data(), VW * 4);
                shown = s.cpu_texture;
            }
        }
        if (!cpu_path) {
            gpu_resolve(s.renderer, s.state, s.index_target, s.colour_target, u);
            if (emissives) {
                draw_emissives(s.renderer, s.colour_target, time);
            }
        }

        present_to_window(s, shown);
        SDL_SetRenderDrawColor(s.renderer, 255, 255, 255, 255);
        char line[160];
        std::snprintf(line, sizeof(line), "%s  mix %2d/16  invalid indices: %d",
                      cpu_path ? "CPU path (no shader)" : "shader path", static_cast<int>(mix),
                      invalid);
        SDL_RenderDebugText(s.renderer, 4, 4, line);
        std::snprintf(line, sizeof(line), "[S]nap %s  [P]ixel-art mode %s  [M]od %s  [L]ight %s",
                      snap ? "on" : "OFF", pixelart ? "ON" : "off", colour_mod ? "ON" : "off",
                      light_on ? "on" : "off");
        SDL_RenderDebugText(s.renderer, 4, 14, line);
        std::snprintf(line, sizeof(line), "[ ] zones %d  wheel: radius %d  [D]ither %s", zones,
                      static_cast<int>(radius), dither ? "on" : "OFF");
        SDL_RenderDebugText(s.renderer, 4, 24, line);
        SDL_RenderDebugText(s.renderer, 4, VH - 22, "[1][2] variant  [C] auto fade  [F] CPU path");
        SDL_RenderDebugText(s.renderer, 4, VH - 12, "[E]missives  mouse: light  Esc: quit");
        SDL_RenderPresent(s.renderer);
        ++frame;
    }
}

} // namespace

int main(int argc, char** argv) {
    bool check = false;
    bool show = false;
    std::string out_dir;
    const char* driver = nullptr;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--check") {
            check = true;
        } else if (arg == "--show") {
            show = true;
        } else if (arg == "--out" && i + 1 < argc) {
            out_dir = argv[++i];
        } else if (arg == "--driver" && i + 1 < argc) {
            driver = argv[++i];
        } else {
            std::printf("usage: raven_shader_spike [--check [--out DIR] [--show]] "
                        "[--driver vulkan|direct3d12|metal]\n");
            return 2;
        }
    }

    Spike spike;
    int code = 1;
    if (init(spike, check && !show, driver)) {
        if (check) {
            code = run_checks(spike, out_dir) ? 0 : 1;
        } else {
            run_interactive(spike);
            code = 0;
        }
    }
    shutdown(spike);
    return code;
}
