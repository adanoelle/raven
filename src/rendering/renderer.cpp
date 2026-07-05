#include "rendering/renderer.hpp"

#include <spdlog/spdlog.h>

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

    renderer_ = SDL_CreateRenderer(window_, nullptr);

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
    render_target_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888,
                                       SDL_TEXTUREACCESS_TARGET, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);

    if (!render_target_) {
        spdlog::error("Failed to create render target: {}", SDL_GetError());
        return false;
    }

    SDL_SetTextureScaleMode(render_target_, SDL_SCALEMODE_PIXELART);

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
    } else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
        recreate_target();
    }
}

void Renderer::recreate_target() {
    if (render_target_) {
        SDL_DestroyTexture(render_target_);
        render_target_ = nullptr;
    }
    render_target_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888,
                                       SDL_TEXTUREACCESS_TARGET, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    if (!render_target_) {
        spdlog::error("Failed to recreate render target: {}", SDL_GetError());
    } else {
        SDL_SetTextureScaleMode(render_target_, SDL_SCALEMODE_PIXELART);
    }
}

void Renderer::begin_frame() {
    SDL_SetRenderTarget(renderer_, render_target_);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
}

void Renderer::end_frame() {
    SDL_SetRenderTarget(renderer_, nullptr);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    if (render_target_) {
        SDL_RenderTexture(renderer_, render_target_, nullptr, nullptr);
    }
}

void Renderer::present() {
    SDL_RenderPresent(renderer_);
}

} // namespace raven
