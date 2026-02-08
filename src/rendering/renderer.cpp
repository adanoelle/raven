#include "rendering/renderer.hpp"

#include <spdlog/spdlog.h>

namespace raven {

Renderer::Renderer() = default;

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::init(const std::string& title, int window_scale) {
    int win_w = VIRTUAL_WIDTH * window_scale;
    int win_h = VIRTUAL_HEIGHT * window_scale;

    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        win_w, win_h,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!window_) {
        spdlog::error("Failed to create window: {}", SDL_GetError());
        return false;
    }

    renderer_ = SDL_CreateRenderer(
        window_, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer_) {
        spdlog::error("Failed to create renderer: {}", SDL_GetError());
        return false;
    }

    // Set up virtual resolution render target
    render_target_ = SDL_CreateTexture(
        renderer_,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        VIRTUAL_WIDTH, VIRTUAL_HEIGHT
    );

    if (!render_target_) {
        spdlog::error("Failed to create render target: {}", SDL_GetError());
        return false;
    }

    // Nearest-neighbor scaling for crisp pixel art
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    // Set logical size for proper scaling on window resize
    SDL_RenderSetLogicalSize(renderer_, VIRTUAL_WIDTH, VIRTUAL_HEIGHT);

    spdlog::info("Renderer initialized: {}x{} virtual, {}x{} window",
                 VIRTUAL_WIDTH, VIRTUAL_HEIGHT, win_w, win_h);

    return true;
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
    if (event.type == SDL_RENDER_TARGETS_RESET ||
        event.type == SDL_RENDER_DEVICE_RESET) {
        spdlog::warn("Render targets reset — recreating");
        recreate_target();
    } else if (event.type == SDL_WINDOWEVENT &&
               event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        recreate_target();
    }
}

void Renderer::recreate_target() {
    if (render_target_) {
        SDL_DestroyTexture(render_target_);
        render_target_ = nullptr;
    }
    render_target_ = SDL_CreateTexture(
        renderer_,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        VIRTUAL_WIDTH, VIRTUAL_HEIGHT
    );
    if (!render_target_) {
        spdlog::error("Failed to recreate render target: {}", SDL_GetError());
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
        SDL_RenderCopy(renderer_, render_target_, nullptr, nullptr);
    }
}

void Renderer::present() {
    SDL_RenderPresent(renderer_);
}

} // namespace raven
