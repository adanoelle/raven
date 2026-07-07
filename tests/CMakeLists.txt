list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)

add_executable(raven_tests
    test_collision.cpp
    test_patterns.cpp
    test_ecs.cpp
    test_tilemap.cpp
    test_shooting.cpp
    test_emitters.cpp
    test_pickups.cpp
    test_ai.cpp
    test_melee_dash.cpp
    test_waves.cpp
    test_player_class.cpp
    test_settings.cpp
    test_bitmap_font.cpp
    test_audio.cpp
    test_save_data.cpp

    # Source files needed by integration tests
    ${CMAKE_SOURCE_DIR}/src/core/paths.cpp
    ${CMAKE_SOURCE_DIR}/src/core/save_data.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/bitmap_font.cpp
    ${CMAKE_SOURCE_DIR}/src/audio/audio_engine.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/player_class.cpp
    ${CMAKE_SOURCE_DIR}/src/patterns/pattern_library.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/systems/collision_system.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/systems/animation_system.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/systems/shooting_system.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/systems/bullet_spawn.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/systems/emitter_system.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/systems/pickup_system.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/systems/damage_system.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/tilemap.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/systems/tile_collision_system.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/systems/ai_system.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/systems/melee_system.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/systems/dash_system.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/systems/input_system.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/systems/wave_system.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/systems/ground_slam_system.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/systems/charged_shot_system.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/systems/concussion_shot_system.cpp
)

target_include_directories(raven_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(raven_tests PRIVATE
    Catch2::Catch2WithMain
    EnTT::EnTT
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    SDL3::SDL3
    SDL3_image::SDL3_image
)

# Shared dependencies (SDL3.dll, ...) must sit next to the test binary
if(WIN32)
    add_custom_command(TARGET raven_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:raven_tests> $<TARGET_FILE_DIR:raven_tests>
        COMMAND_EXPAND_LISTS
        COMMENT "Copying runtime DLLs for tests"
    )
endif()

catch_discover_tests(raven_tests)
