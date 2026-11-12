#include "core/game.hpp"

#include <SDL3/SDL_main.h>
#include <spdlog/spdlog.h>

int main(int /*argc*/, char* /*argv*/[]) {
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

    game.run();
    game.shutdown();

    return 0;
}
