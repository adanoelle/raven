#ifdef RAVEN_ENABLE_SHADER_LIGHTING

#include "rendering/lighting_pass.hpp"

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
    float extra[4]; // x: dither band, y: light count
    float lights[raven::LightingPass::MAX_LIGHTS][4];
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

bool LightingPass::init(SDL_Renderer* renderer, const std::string& ramps_path,
                        const std::string& reverse_path) {
    renderer_ = renderer;
    device_ = SDL_GetGPURendererDevice(renderer);
    if (!device_) {
        spdlog::warn("Lighting: renderer '{}' is not the gpu renderer; lighting is off",
                     SDL_GetRendererName(renderer));
        return false;
    }

    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device_);
    SDL_GPUShaderCreateInfo info{};
    if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        info.code = lighting_frag_spv;
        info.code_size = sizeof(lighting_frag_spv);
    } else if (formats & SDL_GPU_SHADERFORMAT_DXIL) {
        info.format = SDL_GPU_SHADERFORMAT_DXIL;
        info.code = lighting_frag_dxil;
        info.code_size = sizeof(lighting_frag_dxil);
    } else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
        info.format = SDL_GPU_SHADERFORMAT_MSL;
        info.code = lighting_frag_msl;
        info.code_size = sizeof(lighting_frag_msl);
    } else {
        spdlog::warn("Lighting: the GPU device takes none of SPIR-V, DXIL, MSL");
        return false;
    }
    info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    info.num_samplers = 3; // the drawn frame, the reverse table, the ramps
    info.num_uniform_buffers = 1;
    shader_ = SDL_CreateGPUShader(device_, &info);
    if (!shader_) {
        spdlog::error("Lighting: SDL_CreateGPUShader: {}", SDL_GetError());
        return false;
    }

    reverse_ = load_table(reverse_path);
    ramps_ = load_table(ramps_path);
    if (!reverse_ || !ramps_) {
        shutdown();
        return false;
    }
    steps_ = ramps_->w;

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
    SDL_GPUTextureSamplerBinding bindings[2] = {{gpu_texture(reverse_), sampler_},
                                                {gpu_texture(ramps_), sampler_}};
    if (!sampler_ || !bindings[0].texture || !bindings[1].texture) {
        spdlog::error("Lighting: could not bind the lookup tables: {}", SDL_GetError());
        shutdown();
        return false;
    }
    SDL_GPURenderStateCreateInfo state_info{};
    state_info.fragment_shader = shader_;
    state_info.num_sampler_bindings = 2;
    state_info.sampler_bindings = bindings;
    state_ = SDL_CreateGPURenderState(renderer_, &state_info);
    if (!state_) {
        spdlog::error("Lighting: SDL_CreateGPURenderState: {}", SDL_GetError());
        shutdown();
        return false;
    }
    spdlog::info("Lighting pass ready on GPU driver '{}': {} ramps of {} steps",
                 SDL_GetGPUDeviceDriver(device_), ramps_->h, steps_);
    return true;
}

void LightingPass::shutdown() {
    if (state_) {
        SDL_DestroyGPURenderState(state_);
        state_ = nullptr;
    }
    if (sampler_) {
        SDL_ReleaseGPUSampler(device_, sampler_);
        sampler_ = nullptr;
    }
    if (shader_) {
        SDL_ReleaseGPUShader(device_, shader_);
        shader_ = nullptr;
    }
    for (SDL_Texture** texture : {&ramps_, &reverse_}) {
        if (*texture) {
            SDL_DestroyTexture(*texture);
            *texture = nullptr;
        }
    }
}

void LightingPass::apply(SDL_Texture* source, SDL_Texture* target, const LightingSettings& settings,
                         const std::vector<Light>& lights) {
    if (!state_) {
        return;
    }
    Uniforms u{};
    u.frame[0] = static_cast<float>(source->w);
    u.frame[1] = static_cast<float>(source->h);
    u.frame[2] = static_cast<float>(settings.ambient);
    u.frame[3] = static_cast<float>(steps_);
    u.extra[0] = static_cast<float>(std::clamp(settings.dither_band, 0, 8));
    size_t count = std::min(lights.size(), static_cast<size_t>(MAX_LIGHTS));
    u.extra[1] = static_cast<float>(count);
    for (size_t i = 0; i < count; ++i) {
        u.lights[i][0] = std::floor(lights[i].x);
        u.lights[i][1] = std::floor(lights[i].y);
        u.lights[i][2] = static_cast<float>(lights[i].radius);
        u.lights[i][3] = static_cast<float>(lights[i].zones);
    }

    SDL_SetGPURenderStateFragmentUniforms(state_, 0, &u, sizeof(u));
    SDL_SetRenderTarget(renderer_, target);
    SDL_SetGPURenderState(renderer_, state_);
    SDL_RenderTexture(renderer_, source, nullptr, nullptr);
    SDL_SetGPURenderState(renderer_, nullptr);
}

} // namespace raven

#endif // RAVEN_ENABLE_SHADER_LIGHTING
