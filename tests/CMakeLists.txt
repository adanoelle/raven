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

    # Source files needed by integration tests
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
)

target_include_directories(raven_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(raven_tests PRIVATE
    Catch2::Catch2WithMain
    EnTT::EnTT
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    SDL2::SDL2
)

catch_discover_tests(raven_tests)
