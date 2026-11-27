#include "core/game.hpp"
#include "debug/lighting_benchmark.hpp"
#include "debug/lighting_check.hpp"

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
    // --bench-lighting [file.csv]: time the lighting pass and exit
    // --check-lighting [file.png]: check it works on this GPU driver and exit
    for (int i = 1; i < argc; ++i) {
        const std::string_view option{argv[i]};
        const bool bench = option == "--bench-lighting";
        if (!bench && option != "--check-lighting") {
            continue;
        }
        // The next argument is the file, unless it is another option
        const bool named = i + 1 < argc && argv[i + 1][0] != '-';
        const std::string fallback = bench ? "lighting-benchmark.csv" : "lighting-check.png";
        const std::string file = named ? argv[i + 1] : fallback;
        const int result = bench ? raven::run_lighting_benchmark(game.renderer(), file)
                                 : raven::run_lighting_check(game.renderer(), file);
        game.shutdown();
        return result;
    }
#else
    (void)argc;
    (void)argv;
#endif

    game.run();
    game.shutdown();

    return 0;
}
