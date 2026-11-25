#ifdef RAVEN_ENABLE_SHADER_LIGHTING

#include "rendering/lighting_pass.hpp"

#include "rendering/shaders/light.frag.dxil.h"
#include "rendering/shaders/light.frag.msl.h"
#include "rendering/shaders/light.frag.spv.h"
#include "rendering/shaders/lighting.frag.dxil.h"
#include "rendering/shaders/lighting.frag.msl.h"
#include "rendering/shaders/lighting.frag.spv.h"

#include <SDL3_image/SDL_image.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>

namespace {

/// Matches the cbuffer in lighting.frag.hlsl.
struct Uniforms {
    float frame[4]; // target w, target h, ambient shift, step count
    float extra[4]; // x: dither band, y: lights in the loop, z: read the light buffer
    float lights[raven::LightingPass::MAX_LOOP_LIGHTS][4];
};

} // namespace

namespace raven {

SDL_Texture* LightingPass::load_table(const std::string& path) {
    SDL_Surface* loaded = IMG_Load(path.c_str());
    if (!loaded) {
        spdlog::error("Lighting: failed to load '{}': {}", path, SDL_GetError());
        return nullptr;
    }
    SDL_Surface* rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    if (!rgba) {
        return nullptr;
    }
    SDL_Texture* texture = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32,
                                             SDL_TEXTUREACCESS_STATIC, rgba->w, rgba->h);
    if (texture) {
        SDL_UpdateTexture(texture, nullptr, rgba->pixels, rgba->pitch);
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    }
    SDL_DestroySurface(rgba);
    return texture;
}

SDL_GPUShader* LightingPass::create_shader(const unsigned char* spv, size_t spv_size,
                                           const unsigned char* dxil, size_t dxil_size,
                                           const unsigned char* msl, size_t msl_size,
                                           Uint32 samplers, Uint32 uniform_buffers) {
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device_);
    SDL_GPUShaderCreateInfo info{};
    if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        info.code = spv;
        info.code_size = spv_size;
    } else if (formats & SDL_GPU_SHADERFORMAT_DXIL) {
        info.format = SDL_GPU_SHADERFORMAT_DXIL;
        info.code = dxil;
        info.code_size = dxil_size;
    } else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
        info.format = SDL_GPU_SHADERFORMAT_MSL;
        info.code = msl;
        info.code_size = msl_size;
    } else {
        spdlog::warn("Lighting: the GPU device takes none of SPIR-V, DXIL, MSL");
        return nullptr;
    }
    info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    info.num_samplers = samplers;
    info.num_uniform_buffers = uniform_buffers;
    SDL_GPUShader* shader = SDL_CreateGPUShader(device_, &info);
    if (!shader) {
        spdlog::error("Lighting: SDL_CreateGPUShader: {}", SDL_GetError());
    }
    return shader;
}

bool LightingPass::init(SDL_Renderer* renderer, const std::string& ramps_path,
                        const std::string& reverse_path, int width, int height) {
    renderer_ = renderer;
    device_ = SDL_GetGPURendererDevice(renderer);
    if (!device_) {
        spdlog::warn("Lighting: renderer '{}' is not the gpu renderer; lighting is off",
                     SDL_GetRendererName(renderer));
        return false;
    }

    // Samplers: the drawn frame, the reverse table, the ramps, the light buffer
    shader_ = create_shader(lighting_frag_spv, sizeof(lighting_frag_spv), lighting_frag_dxil,
                            sizeof(lighting_frag_dxil), lighting_frag_msl,
                            sizeof(lighting_frag_msl), 4, 1);
    light_shader_ =
        create_shader(light_frag_spv, sizeof(light_frag_spv), light_frag_dxil,
                      sizeof(light_frag_dxil), light_frag_msl, sizeof(light_frag_msl), 1, 0);
    if (!shader_ || !light_shader_) {
        shutdown();
        return false;
    }

    reverse_ = load_table(reverse_path);
    ramps_ = load_table(ramps_path);
    light_buffer_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET,
                                      width, height);
    white_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, 1, 1);
    if (!reverse_ || !ramps_ || !light_buffer_ || !white_) {
        spdlog::error("Lighting: could not create the tables or the light buffer: {}",
                      SDL_GetError());
        shutdown();
        return false;
    }
    steps_ = ramps_->w;
    const Uint32 white_pixel = 0xFFFFFFFF;
    SDL_UpdateTexture(white_, nullptr, &white_pixel, sizeof(white_pixel));
    SDL_SetTextureScaleMode(white_, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(white_, SDL_BLENDMODE_ADD); // lights sum in the buffer
    SDL_SetTextureScaleMode(light_buffer_, SDL_SCALEMODE_NEAREST);

    SDL_GPUSamplerCreateInfo sampler_info{};
    sampler_info.min_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mag_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_ = SDL_CreateGPUSampler(device_, &sampler_info);

    auto gpu_texture = [](SDL_Texture* texture) {
        return static_cast<SDL_GPUTexture*>(SDL_GetPointerProperty(
            SDL_GetTextureProperties(texture), SDL_PROP_TEXTURE_GPU_TEXTURE_POINTER, nullptr));
    };
    SDL_GPUTextureSamplerBinding bindings[3] = {{gpu_texture(reverse_), sampler_},
                                                {gpu_texture(ramps_), sampler_},
                                                {gpu_texture(light_buffer_), sampler_}};
    if (!sampler_ || !bindings[0].texture || !bindings[1].texture || !bindings[2].texture) {
        spdlog::error("Lighting: could not bind the lookup tables: {}", SDL_GetError());
        shutdown();
        return false;
    }
    SDL_GPURenderStateCreateInfo state_info{};
    state_info.fragment_shader = shader_;
    state_info.num_sampler_bindings = 3;
    state_info.sampler_bindings = bindings;
    state_ = SDL_CreateGPURenderState(renderer_, &state_info);
    SDL_GPURenderStateCreateInfo light_info{};
    light_info.fragment_shader = light_shader_;
    light_state_ = SDL_CreateGPURenderState(renderer_, &light_info);
    if (!state_ || !light_state_) {
        spdlog::error("Lighting: SDL_CreateGPURenderState: {}", SDL_GetError());
        shutdown();
        return false;
    }
    spdlog::info("Lighting pass ready on GPU driver '{}': {} ramps of {} steps",
                 SDL_GetGPUDeviceDriver(device_), ramps_->h, steps_);
    return true;
}

void LightingPass::shutdown() {
    for (SDL_GPURenderState** state : {&state_, &light_state_}) {
        if (*state) {
            SDL_DestroyGPURenderState(*state);
            *state = nullptr;
        }
    }
    if (sampler_) {
        SDL_ReleaseGPUSampler(device_, sampler_);
        sampler_ = nullptr;
    }
    for (SDL_GPUShader** shader : {&shader_, &light_shader_}) {
        if (*shader) {
            SDL_ReleaseGPUShader(device_, *shader);
            *shader = nullptr;
        }
    }
    for (SDL_Texture** texture : {&ramps_, &reverse_, &light_buffer_, &white_}) {
        if (*texture) {
            SDL_DestroyTexture(*texture);
            *texture = nullptr;
        }
    }
}

void LightingPass::draw_light_buffer(const std::vector<Light>& lights) {
    SDL_SetRenderTarget(renderer_, light_buffer_);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_SetGPURenderState(renderer_, light_state_);
    const auto width = static_cast<float>(light_buffer_->w);
    const auto height = static_cast<float>(light_buffer_->h);
    for (const Light& light : lights) {
        if (light.radius <= 0 || light.level <= 0) {
            continue;
        }
        const auto reach = static_cast<float>(light.radius);
        const float x = std::floor(light.x);
        const float y = std::floor(light.y);
        if (x + reach <= 0.f || y + reach <= 0.f || x - reach >= width || y - reach >= height) {
            continue; // wholly off the frame
        }
        // Red carries the strength; green and blue are free for later use
        SDL_SetTextureColorModFloat(
            white_, static_cast<float>(std::min(light.level, MAX_LEVEL)) / 255.f, 0.f, 0.f);
        const SDL_FRect square{x - reach, y - reach, 2.f * reach, 2.f * reach};
        SDL_RenderTexture(renderer_, white_, nullptr, &square);
    }
    SDL_SetGPURenderState(renderer_, nullptr);
}

void LightingPass::apply(SDL_Texture* source, SDL_Texture* target, const LightingSettings& settings,
                         const std::vector<Light>& lights) {
    if (!state_) {
        return;
    }
    if (!settings.use_loop) {
        draw_light_buffer(lights);
    }
    Uniforms u{};
    u.frame[0] = static_cast<float>(source->w);
    u.frame[1] = static_cast<float>(source->h);
    u.frame[2] = static_cast<float>(settings.ambient);
    u.frame[3] = static_cast<float>(steps_);
    u.extra[0] = static_cast<float>(std::clamp(settings.dither_band, 0, 8));
    size_t count =
        settings.use_loop ? std::min(lights.size(), static_cast<size_t>(MAX_LOOP_LIGHTS)) : 0;
    u.extra[1] = static_cast<float>(count);
    u.extra[2] = settings.use_loop ? 0.f : 1.f;
    for (size_t i = 0; i < count; ++i) {
        u.lights[i][0] = std::floor(lights[i].x);
        u.lights[i][1] = std::floor(lights[i].y);
        u.lights[i][2] = static_cast<float>(lights[i].radius);
        u.lights[i][3] = static_cast<float>(std::min(lights[i].level, MAX_LEVEL));
    }

    SDL_SetGPURenderStateFragmentUniforms(state_, 0, &u, sizeof(u));
    SDL_SetRenderTarget(renderer_, target);
    SDL_SetGPURenderState(renderer_, state_);
    SDL_RenderTexture(renderer_, source, nullptr, nullptr);
    SDL_SetGPURenderState(renderer_, nullptr);
}

} // namespace raven

#endif // RAVEN_ENABLE_SHADER_LIGHTING
