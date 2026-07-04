#include "core/game.hpp"

#include "core/paths.hpp"
#include "core/string_id.hpp"
#include "scenes/title_scene.hpp"

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <fstream>

namespace raven {

Game::Game() = default;
Game::~Game() = default;

bool Game::init() {
    // Initialize SDL subsystems
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        spdlog::error("SDL_Init failed: {}", SDL_GetError());
        return false;
    }

    // Load persisted user settings, then write them back: the first run
    // creates the file, and later runs pick up any fields added since.
    const std::string settings_path = paths::pref_dir() + "settings.json";
    settings_ = Settings::load(settings_path);
    settings_.save(settings_path);

    if (!renderer_.init("Raven", settings_.window_scale, settings_.fullscreen, settings_.vsync)) {
        return false;
    }

#ifdef RAVEN_ENABLE_IMGUI
    debug_overlay_.init(renderer_.sdl_window(), renderer_.sdl_renderer());
#endif

    if (!load_assets()) {
        return false;
    }

    // Pre-intern known sprite sheet IDs
    auto& interner = registry_.ctx().emplace<StringInterner>();
    interner.intern("player");
    interner.intern("enemies");
    interner.intern("projectiles");
    interner.intern("pickups");

    // Start with title scene
    scenes_.push(std::make_unique<TitleScene>(), *this);

    spdlog::info("Game initialized successfully");
    return true;
}

bool Game::load_assets() {
    const std::string config_path = paths::asset("assets/data/config.json");
    std::ifstream f(config_path);
    if (!f.is_open()) {
        spdlog::warn("Could not open '{}' — running without assets", config_path);
        return true;
    }

    try {
        auto config = nlohmann::json::parse(f);

        if (config.contains("font")) {
            const auto& fj = config["font"];
            auto path = fj.value("path", "assets/fonts/font.png");
            int gw = fj.value("glyph_w", 6);
            int gh = fj.value("glyph_h", 8);
            if (!font_.load(renderer_.sdl_renderer(), paths::asset(path), gw, gh)) {
                spdlog::warn("Failed to load font atlas '{}' — text will not render", path);
            }
        }

        if (config.contains("sprite_sheets")) {
            for (const auto& sheet : config["sprite_sheets"]) {
                auto id = sheet.at("id").get<std::string>();
                auto path = sheet.at("path").get<std::string>();
                int fw = sheet.at("frame_w").get<int>();
                int fh = sheet.at("frame_h").get<int>();
                if (!sprites_.load(renderer_.sdl_renderer(), id, paths::asset(path), fw, fh)) {
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
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F1 && !event.key.repeat) {
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
            // Consume press edges after the first tick so one press fires
            // exactly one tick. Unconsumed edges (frames that run zero
            // ticks, e.g. on >120 Hz displays) stay latched in Input.
            input_.consume_pressed();
        }

        // Render
        render();

        // Without vsync the loop would busy-spin at uncapped speed (100%
        // CPU/GPU). Cap the frame rate instead; 240 fps keeps input latency
        // low while still bounding the spin.
        if (!renderer_.vsync_enabled()) {
            constexpr Uint64 MIN_FRAME_NS = 1'000'000'000ull / 240;
            Uint64 elapsed_ns = (SDL_GetPerformanceCounter() - now) * 1'000'000'000ull / freq;
            if (elapsed_ns < MIN_FRAME_NS) {
                SDL_DelayNS(MIN_FRAME_NS - elapsed_ns);
            }
        }

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
    // Pop scenes first: scene destructors free SDL textures (tilemaps),
    // which must happen while the renderer still exists.
    scenes_.clear(*this);

#ifdef RAVEN_ENABLE_IMGUI
    debug_overlay_.shutdown();
#endif
    sprites_ = SpriteSheetManager{}; // release all textures before renderer
    font_ = BitmapFont{};
    renderer_.shutdown();
    input_.shutdown(); // close gamepad before SDL_Quit

    SDL_Quit();

    spdlog::info("Game shutdown complete");
}

} // namespace raven
