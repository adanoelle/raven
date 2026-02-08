# Dependencies.cmake — all third-party libraries

include(CPM)

# ── System packages (found via pkg-config / CMake find modules) ───
find_package(PkgConfig REQUIRED)
find_package(SDL2 REQUIRED)
pkg_check_modules(SDL2_image REQUIRED IMPORTED_TARGET SDL2_image)
pkg_check_modules(SDL2_mixer REQUIRED IMPORTED_TARGET SDL2_mixer)

# Create alias targets for consistent naming
if(NOT TARGET SDL2_image::SDL2_image)
    add_library(SDL2_image::SDL2_image ALIAS PkgConfig::SDL2_image)
endif()
if(NOT TARGET SDL2_mixer::SDL2_mixer)
    add_library(SDL2_mixer::SDL2_mixer ALIAS PkgConfig::SDL2_mixer)
endif()

# ── CPM dependencies (header-only or compiled from source) ────────

CPMAddPackage(
    NAME EnTT
    VERSION 3.13.2
    GITHUB_REPOSITORY skypjack/entt
    SYSTEM YES
    OPTIONS "ENTT_INCLUDE_HEADERS ON"
)

CPMAddPackage(
    NAME nlohmann_json
    VERSION 3.11.3
    GITHUB_REPOSITORY nlohmann/json
    SYSTEM YES
    OPTIONS "JSON_BuildTests OFF"
)

CPMAddPackage(
    NAME spdlog
    VERSION 1.14.1
    GITHUB_REPOSITORY gabime/spdlog
    SYSTEM YES
    OPTIONS "SPDLOG_FMT_EXTERNAL OFF"
)

# Testing framework
if(RAVEN_ENABLE_TESTS)
    CPMAddPackage(
        NAME Catch2
        VERSION 3.7.0
        GITHUB_REPOSITORY catchorg/Catch2
    )
endif()

# Dear ImGui (optional debug overlay)
if(RAVEN_ENABLE_IMGUI)
    CPMAddPackage(
        NAME imgui
        VERSION 1.91.0
        GITHUB_REPOSITORY ocornut/imgui
        DOWNLOAD_ONLY YES
    )

    if(imgui_ADDED)
        add_library(imgui STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
        )
        target_include_directories(imgui SYSTEM PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        target_link_libraries(imgui PUBLIC SDL2::SDL2)
        add_library(imgui::imgui ALIAS imgui)
    endif()
endif()
