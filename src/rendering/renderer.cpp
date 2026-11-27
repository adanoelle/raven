#include "rendering/renderer.hpp"

#include "core/paths.hpp"

#include <spdlog/spdlog.h>

namespace {

/// @brief Create the 480x270 texture the game draws into.
SDL_Texture* create_virtual_target(SDL_Renderer* renderer) {
    SDL_Texture* target =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                          raven::Renderer::VIRTUAL_WIDTH, raven::Renderer::VIRTUAL_HEIGHT);
    if (target) {
        SDL_SetTextureScaleMode(target, SDL_SCALEMODE_PIXELART);
    }
    return target;
}

} // namespace

namespace raven {

Renderer::Renderer() = default;

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::init(const std::string& title, int window_scale, bool fullscreen, bool vsync) {
    int win_w = VIRTUAL_WIDTH * window_scale;
    int win_h = VIRTUAL_HEIGHT * window_scale;

    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
    if (fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }
    window_ = SDL_CreateWindow(title.c_str(), win_w, win_h, flags);

    if (!window_) {
        spdlog::error("Failed to create window: {}", SDL_GetError());
        return false;
    }

#ifdef RAVEN_ENABLE_SHADER_LIGHTING
    // The lighting shader needs SDL's "gpu" renderer. If it cannot be
    // created, fall through to the default renderer and run unlit.
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_RENDERER_CREATE_NAME_STRING, SDL_GPU_RENDERER);
    SDL_SetPointerProperty(props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, window_);
    SDL_SetBooleanProperty(props, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_SPIRV_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_DXIL_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_MSL_BOOLEAN, true);
    renderer_ = SDL_CreateRendererWithProperties(props);
    SDL_DestroyProperties(props);
    if (!renderer_) {
        spdlog::warn("No gpu renderer ({}); shader lighting is off", SDL_GetError());
    }
#endif
    if (!renderer_) {
        renderer_ = SDL_CreateRenderer(window_, nullptr);
    }

    if (!renderer_) {
        spdlog::error("Failed to create renderer: {}", SDL_GetError());
        return false;
    }

    vsync_enabled_ = false;
    if (vsync) {
        if (SDL_SetRenderVSync(renderer_, 1)) {
            vsync_enabled_ = true;
        } else {
            spdlog::warn("VSync unavailable ({}) — the game loop will use a frame limiter",
                         SDL_GetError());
        }
    }

    // Set up virtual resolution render target
    render_target_ = create_virtual_target(renderer_);

    if (!render_target_) {
        spdlog::error("Failed to create render target: {}", SDL_GetError());
        return false;
    }

#ifdef RAVEN_ENABLE_SHADER_LIGHTING
    lit_target_ = create_virtual_target(renderer_);
    current_target_ = render_target_;
    init_lighting();
#endif

    // Set logical size for proper scaling on window resize
    SDL_SetRenderLogicalPresentation(renderer_, VIRTUAL_WIDTH, VIRTUAL_HEIGHT,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);

    spdlog::info("Renderer initialized: {}x{} virtual, {}x{} window", VIRTUAL_WIDTH, VIRTUAL_HEIGHT,
                 win_w, win_h);

    return true;
}

void Renderer::set_fullscreen(bool fullscreen) {
    if (window_ && !SDL_SetWindowFullscreen(window_, fullscreen)) {
        spdlog::warn("Failed to set fullscreen={}: {}", fullscreen, SDL_GetError());
    }
}

void Renderer::set_window_scale(int window_scale) {
    if (!window_ || window_scale < 1) {
        return;
    }
    if (!SDL_SetWindowSize(window_, VIRTUAL_WIDTH * window_scale, VIRTUAL_HEIGHT * window_scale)) {
        spdlog::warn("Failed to resize window: {}", SDL_GetError());
    }
}

void Renderer::set_vsync(bool vsync) {
    if (!renderer_) {
        return;
    }
    if (SDL_SetRenderVSync(renderer_, vsync ? 1 : SDL_RENDERER_VSYNC_DISABLED)) {
        vsync_enabled_ = vsync;
    } else {
        spdlog::warn("Failed to set vsync={}: {}", vsync, SDL_GetError());
        vsync_enabled_ = false;
    }
}

void Renderer::shutdown() {
#ifdef RAVEN_ENABLE_SHADER_LIGHTING
    lighting_.shutdown();
    if (lit_target_) {
        SDL_DestroyTexture(lit_target_);
        lit_target_ = nullptr;
    }
    current_target_ = nullptr;
#endif
    if (render_target_) {
        SDL_DestroyTexture(render_target_);
        render_target_ = nullptr;
    }
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
}

void Renderer::handle_event(const SDL_Event& event) {
    if (event.type == SDL_EVENT_RENDER_TARGETS_RESET ||
        event.type == SDL_EVENT_RENDER_DEVICE_RESET) {
        spdlog::warn("Render targets reset — recreating");
        recreate_target();
#ifdef RAVEN_ENABLE_SHADER_LIGHTING
        // The pass owns textures of its own (the light buffer, the tables)
        // and render states that hold them; a reset loses those too.
        init_lighting();
#endif
    } else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        recreate_target();
    }
}

#ifdef RAVEN_ENABLE_SHADER_LIGHTING
void Renderer::init_lighting() {
    lighting_.shutdown();
    if (!lit_target_) {
        return;
    }
    lighting_.init(renderer_, paths::asset("assets/data/lighting/ramps.png"),
                   paths::asset("assets/data/lighting/reverse.png"), VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
}
#endif

void Renderer::recreate_target() {
    if (render_target_) {
        SDL_DestroyTexture(render_target_);
        render_target_ = nullptr;
    }
    render_target_ = create_virtual_target(renderer_);
    if (!render_target_) {
        spdlog::error("Failed to recreate render target: {}", SDL_GetError());
    }
#ifdef RAVEN_ENABLE_SHADER_LIGHTING
    if (lit_target_) {
        SDL_DestroyTexture(lit_target_);
    }
    lit_target_ = create_virtual_target(renderer_);
    current_target_ = render_target_;
#endif
}

void Renderer::begin_frame() {
#ifdef RAVEN_ENABLE_SHADER_LIGHTING
    current_target_ = render_target_;
#endif
    SDL_SetRenderTarget(renderer_, render_target_);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
}

#ifdef RAVEN_ENABLE_SHADER_LIGHTING
void Renderer::apply_lighting(const LightingSettings& settings, const std::vector<Light>& lights,
                              int passes) {
    if (!lighting_.ready() || !render_target_ || !lit_target_ ||
        current_target_ != render_target_) {
        return;
    }
    for (int pass = 0; pass < passes; ++pass) {
        lighting_.apply(render_target_, lit_target_, settings, lights);
    }
    // The rest of the frame (HUD, overlays) draws on top of the lit world.
    current_target_ = lit_target_;
    SDL_SetRenderTarget(renderer_, lit_target_);
}
#endif

#ifdef RAVEN_ENABLE_SHADER_LIGHTING
void Renderer::wait_for_gpu() {
    if (SDL_GPUDevice* device = SDL_GetGPURendererDevice(renderer_)) {
        SDL_WaitForGPUIdle(device);
    }
}
#endif

#ifdef RAVEN_ENABLE_SHADER_LIGHTING
SDL_Surface* Renderer::read_frame() {
    SDL_Texture* frame = current_target_ ? current_target_ : render_target_;
    SDL_SetRenderTarget(renderer_, frame);
    return SDL_RenderReadPixels(renderer_, nullptr);
}
#endif

void Renderer::end_frame() {
    SDL_SetRenderTarget(renderer_, nullptr);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
#ifdef RAVEN_ENABLE_SHADER_LIGHTING
    SDL_Texture* frame = current_target_ ? current_target_ : render_target_;
#else
    SDL_Texture* frame = render_target_;
#endif
    if (frame) {
        SDL_RenderTexture(renderer_, frame, nullptr, nullptr);
    }
}

void Renderer::present() {
    SDL_RenderPresent(renderer_);
}

} // namespace raven
