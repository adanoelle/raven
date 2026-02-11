cmake_minimum_required(VERSION 3.25)

project(raven
    VERSION 0.1.0
    DESCRIPTION "Pixel art roguelike game"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output binaries to predictable locations
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Options
option(RAVEN_ENABLE_TESTS "Build tests" ON)
option(RAVEN_ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(RAVEN_ENABLE_IMGUI "Enable Dear ImGui debug overlay" ON)

# CMake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(CompilerWarnings)
include(Dependencies)

if(RAVEN_ENABLE_ASAN)
    include(Sanitizers)
endif()

# ── Main game target ──────────────────────────────────────────────
add_executable(raven
    src/main.cpp

    # Core
    src/core/game.cpp
    src/core/input.cpp

    # Rendering
    src/rendering/renderer.cpp
    src/rendering/sprite_sheet.cpp
    src/rendering/tilemap.cpp
    src/rendering/tilemap_loader.cpp

    # ECS Systems
    src/ecs/systems/movement_system.cpp
    src/ecs/systems/render_system.cpp
    src/ecs/systems/collision_system.cpp
    src/ecs/systems/input_system.cpp
    src/ecs/systems/cleanup_system.cpp
    src/ecs/systems/damage_system.cpp
    src/ecs/systems/animation_system.cpp
    src/ecs/systems/shooting_system.cpp
    src/ecs/systems/bullet_spawn.cpp
    src/ecs/systems/emitter_system.cpp
    src/ecs/systems/pickup_system.cpp
    src/ecs/systems/tilemap_render_system.cpp
    src/ecs/systems/tile_collision_system.cpp
    src/ecs/systems/ai_system.cpp
    src/ecs/systems/melee_system.cpp
    src/ecs/systems/dash_system.cpp

    # Patterns
    src/patterns/pattern_library.cpp

    # Scenes
    src/scenes/scene_manager.cpp
    src/scenes/game_scene.cpp
    src/scenes/title_scene.cpp
)

target_include_directories(raven PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(raven PRIVATE
    SDL2::SDL2
    SDL2_image::SDL2_image
    SDL2_mixer::SDL2_mixer
    EnTT::EnTT
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    LDtkLoader::LDtkLoader
)

if(RAVEN_ENABLE_IMGUI)
    target_link_libraries(raven PRIVATE imgui::imgui)
    target_compile_definitions(raven PRIVATE RAVEN_ENABLE_IMGUI)
    target_sources(raven PRIVATE src/debug/debug_overlay.cpp)
endif()

raven_set_warnings(raven)

# Copy assets to build directory
add_custom_command(TARGET raven POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/bin/assets
    COMMENT "Symlinking assets directory"
)

# ── Tests ─────────────────────────────────────────────────────────
if(RAVEN_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Install / Package ─────────────────────────────────────────────
install(TARGETS raven RUNTIME DESTINATION bin)
install(DIRECTORY assets/ DESTINATION share/raven/assets)

set(CPACK_PACKAGE_NAME "raven")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "ZIP;TGZ")
include(CPack)
