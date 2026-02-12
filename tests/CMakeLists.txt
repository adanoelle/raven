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

    # Source files needed by integration tests
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
)

catch_discover_tests(raven_tests)
