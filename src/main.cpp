#include "core/game.hpp"
#include "debug/lighting_benchmark.hpp"

#include <SDL3/SDL_main.h>
#include <spdlog/spdlog.h>

#include <string>
#include <string_view>

int main(int argc, char* argv[]) {
#ifdef NDEBUG
    spdlog::set_level(spdlog::level::info);
#else
    spdlog::set_level(spdlog::level::debug);
#endif
    spdlog::info("raven v0.1.0");

    raven::Game game;

    if (!game.init()) {
        spdlog::error("Failed to initialize game");
        return 1;
    }

#ifdef RAVEN_ENABLE_SHADER_LIGHTING
    // --bench-lighting [file.csv]: time the lighting prototype and exit
    for (int i = 1; i < argc; ++i) {
        if (std::string_view{argv[i]} == "--bench-lighting") {
            // The next argument is the file, unless it is another option
            const bool named = i + 1 < argc && argv[i + 1][0] != '-';
            const std::string csv = named ? argv[i + 1] : "lighting-benchmark.csv";
            const int result = raven::run_lighting_benchmark(game.renderer(), csv);
            game.shutdown();
            return result;
        }
    }
#else
    (void)argc;
    (void)argv;
#endif

    game.run();
    game.shutdown();

    return 0;
}
