#include "core/game.hpp"
#include "scenes/title_scene.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <fstream>

namespace raven {

Game::Game() = default;
Game::~Game() = default;

bool Game::init() {
    // Initialize SDL subsystems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
        spdlog::error("SDL_Init failed: {}", SDL_GetError());
        return false;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        spdlog::error("IMG_Init failed: {}", IMG_GetError());
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        spdlog::error("Mix_OpenAudio failed: {}", Mix_GetError());
        return false;
    }

    if (!renderer_.init("Raven", 2)) {
        return false;
    }

#ifdef RAVEN_ENABLE_IMGUI
    debug_overlay_.init(renderer_.sdl_window(), renderer_.sdl_renderer());
#endif

    if (!load_assets()) {
        return false;
    }

    // Start with title scene
    scenes_.push(std::make_unique<TitleScene>(), *this);

    spdlog::info("Game initialized successfully");
    return true;
}

bool Game::load_assets() {
    std::ifstream f("assets/data/config.json");
    if (!f.is_open()) {
        spdlog::warn("Could not open assets/data/config.json — running without assets");
        return true;
    }

    try {
        auto config = nlohmann::json::parse(f);

        if (config.contains("sprite_sheets")) {
            for (const auto& sheet : config["sprite_sheets"]) {
                auto id = sheet.at("id").get<std::string>();
                auto path = sheet.at("path").get<std::string>();
                int fw = sheet.at("frame_w").get<int>();
                int fh = sheet.at("frame_h").get<int>();
                if (!sprites_.load(renderer_.sdl_renderer(), id, path, fw, fh)) {
                    spdlog::warn("Failed to load sprite sheet '{}'", id);
                }
            }
        }
    } catch (const nlohmann::json::exception& e) {
        spdlog::warn("Failed to parse config.json: {}", e.what());
    }

    return true;
}

void Game::run() {
    running_ = true;
    Uint64 last_time = SDL_GetPerformanceCounter();
    const Uint64 freq = SDL_GetPerformanceFrequency();

    while (running_) {
        // Calculate frame delta
        Uint64 now = SDL_GetPerformanceCounter();
        float frame_delta = static_cast<float>(now - last_time) / static_cast<float>(freq);
        last_time = now;

        // Process input
        input_.begin_frame();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            renderer_.handle_event(event);

#ifdef RAVEN_ENABLE_IMGUI
            bool imgui_consumed = debug_overlay_.process_event(event);

            // Toggle overlay with F1
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F1 &&
                !event.key.repeat) {
                debug_overlay_.toggle();
            }

            if (!imgui_consumed) {
                input_.process_event(event);
            }
#else
            input_.process_event(event);
#endif
        }

        // Poll keyboard/gamepad state once per frame (even if no events arrived)
        input_.update();

        if (input_.quit_requested()) {
            running_ = false;
            break;
        }

        // Fixed timestep updates
        int steps = clock_.advance(frame_delta);
        for (int i = 0; i < steps; ++i) {
            fixed_update(Clock::TICK_RATE);
        }

        // Render
        render();

        // Exit if no scenes remain
        if (scenes_.empty()) {
            running_ = false;
        }
    }
}

void Game::fixed_update(float dt) {
    scenes_.update(*this, dt);
}

void Game::render() {
    renderer_.begin_frame();
    scenes_.render(*this);
    renderer_.end_frame();

#ifdef RAVEN_ENABLE_IMGUI
    debug_overlay_.begin_frame();
    debug_overlay_.render(renderer_.sdl_renderer(), registry_);
#endif

    renderer_.present();
}

void Game::shutdown() {
#ifdef RAVEN_ENABLE_IMGUI
    debug_overlay_.shutdown();
#endif
    sprites_ = SpriteSheetManager{}; // release all textures before renderer
    renderer_.shutdown();

    Mix_CloseAudio();
    IMG_Quit();
    SDL_Quit();

    spdlog::info("Game shutdown complete");
}

} // namespace raven
