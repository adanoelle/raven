list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)

add_executable(raven_tests
    test_collision.cpp
    test_patterns.cpp
    test_ecs.cpp

    # Source files needed by integration tests
    ${CMAKE_SOURCE_DIR}/src/patterns/pattern_library.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/systems/collision_system.cpp
)

target_include_directories(raven_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(raven_tests PRIVATE
    Catch2::Catch2WithMain
    EnTT::EnTT
    nlohmann_json::nlohmann_json
    spdlog::spdlog
)

catch_discover_tests(raven_tests)
