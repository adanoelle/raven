#include "core/game.hpp"

#include "core/fs.hpp"
#include "core/paths.hpp"
#include "core/string_id.hpp"
#include "scenes/title_scene.hpp"

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace raven {

namespace {

/// @brief Map a 0-100 volume setting to a linear gain, perceptually curved.
///
/// Loudness perception is roughly logarithmic; a quadratic curve keeps the
/// slider feeling even instead of bunching audible change at the low end.
float volume_to_gain(int volume) {
    const float v = static_cast<float>(volume) / 100.f;
    return v * v;
}

} // anonymous namespace

Game::Game() = default;
Game::~Game() = default;

bool Game::init() {
    // Initialize SDL subsystems
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        spdlog::error("SDL_Init failed: {}", SDL_GetError());
        return false;
    }

    input_.init();

    // Load persisted user settings, then write them back: the first run
    // creates the file, and later runs pick up any fields added since.
    // An empty pref dir (SDL_GetPrefPath failed) leaves both paths empty so
    // load/save become no-ops instead of silently using the working directory.
    const std::string pref = paths::pref_dir();
    settings_path_ = pref.empty() ? std::string{} : pref + "settings.json";
    settings_ = Settings::load(settings_path_);
    settings_.save(settings_path_);

    save_path_ = pref.empty() ? std::string{} : pref + "save.json";
    save_data_ = SaveData::load(save_path_);

    if (!renderer_.init("Raven", settings_.window_scale, settings_.fullscreen, settings_.vsync)) {
        return false;
    }

    // Audio is optional: a failed init leaves the engine in silent no-op mode
    audio_.init();
    audio_.set_master_gain(volume_to_gain(settings_.sfx_volume));

    // Steam is optional: no-op unless built with RAVEN_ENABLE_STEAM and
    // running under Steam (or with a dev steam_appid.txt)
    steam_.init();

#ifdef RAVEN_ENABLE_IMGUI
    debug_overlay_.init(renderer_.sdl_window(), renderer_.sdl_renderer());
#endif

    // The interner must exist before load_assets(): sprite sheets are
    // registered under interned IDs.
    registry_.ctx().emplace<StringInterner>();

    if (!load_assets()) {
        return false;
    }

    // Start with title scene
    scenes_.push(std::make_unique<TitleScene>(), *this);

    spdlog::info("Game initialized successfully");
    return true;
}

bool Game::load_assets() {
    const std::string config_path = paths::asset("assets/data/config.json");
    const auto text = fs::read_text(config_path);
    if (!text) {
        spdlog::warn("Could not open '{}' — running without assets", config_path);
        return true;
    }

    try {
        auto config = nlohmann::json::parse(*text);

        if (config.contains("font")) {
            const auto& fj = config["font"];
            auto path = fj.value("path", "assets/fonts/font.png");
            int gw = fj.value("glyph_w", 6);
            int gh = fj.value("glyph_h", 8);
            if (!font_.load(renderer_.sdl_renderer(), paths::asset(path), gw, gh)) {
                spdlog::warn("Failed to load font atlas '{}' — text will not render", path);
            }
        }

        if (config.contains("sounds")) {
            for (const auto& [id, path] : config["sounds"].items()) {
                audio_.load_sound(id, paths::asset(path.get<std::string>()));
            }
        }

        if (config.contains("sprite_sheets")) {
            auto& interner = registry_.ctx().get<StringInterner>();
            for (const auto& sheet : config["sprite_sheets"]) {
                auto id = sheet.at("id").get<std::string>();
                auto path = sheet.at("path").get<std::string>();
                int fw = sheet.at("frame_w").get<int>();
                int fh = sheet.at("frame_h").get<int>();
                if (!sprites_.load(renderer_.sdl_renderer(), interner.intern(id),
                                   paths::asset(path), fw, fh)) {
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
            handle_lifecycle_event(event);

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

        // Reap finished sound effect streams
        audio_.update();

        // Pump Steam callbacks (no-op when inactive)
        steam_.run_callbacks();

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

void Game::handle_lifecycle_event(const SDL_Event& event) {
    switch (event.type) {
    case SDL_EVENT_WILL_ENTER_BACKGROUND:
        // Console sleep / home menu, mobile backgrounding: freeze gameplay
        // behind the pause menu and stop the audio clock. The fixed-step
        // clamp in Clock handles the time jump on the way back.
        scenes_.suspend(*this);
        audio_.pause();
        break;
    case SDL_EVENT_DID_ENTER_FOREGROUND:
        audio_.resume();
        break;
    default:
        break;
    }
}

void Game::fixed_update(float dt) {
    scenes_.update(*this, dt);
}

bool Game::record_score(int score) {
    if (score <= save_data_.best_score) {
        return false;
    }
    save_data_.best_score = score;
    save_data_.save(save_path_);
    return true;
}

void Game::apply_settings() {
    renderer_.set_fullscreen(settings_.fullscreen);
    if (!settings_.fullscreen) {
        renderer_.set_window_scale(settings_.window_scale);
    }
    renderer_.set_vsync(settings_.vsync);
    audio_.set_master_gain(volume_to_gain(settings_.sfx_volume));
}

void Game::save_settings() {
    settings_.save(settings_path_);
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
    audio_.shutdown();
    steam_.shutdown();
    input_.shutdown(); // close gamepad before SDL_Quit

    SDL_Quit();

    spdlog::info("Game shutdown complete");
}

} // namespace raven
