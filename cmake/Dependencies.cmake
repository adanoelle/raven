# Dependencies.cmake — all third-party libraries

include(CPM)

# ── SDL3 / SDL3_image ──────────────────────────────────────────────
# Prefer system packages (Nix dev shell, cached CI prefix); fall back to
# building from source via CPM when they are absent — the normal case on
# Windows/MSVC, where no SDL3 distro packages or pkg-config exist.
option(RAVEN_BUNDLED_DEPS "Build SDL3/SDL3_image from source even if system packages exist" OFF)

if(NOT RAVEN_BUNDLED_DEPS)
    # 3.4 is the floor: SDL_SCALEMODE_PIXELART (renderer, tilemap) needs it
    find_package(SDL3 3.4 QUIET)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(sdl3-image QUIET IMPORTED_TARGET sdl3-image)
    endif()
endif()

if(NOT TARGET SDL3::SDL3)
    message(STATUS "System SDL3 not found — building from source (release-3.4.12)")
    CPMAddPackage(
        NAME SDL3
        GITHUB_REPOSITORY libsdl-org/SDL
        GIT_TAG release-3.4.12
        SYSTEM YES
        OPTIONS
            "SDL_TEST_LIBRARY OFF"
            "SDL_EXAMPLES OFF"
    )
endif()

if(NOT TARGET SDL3_image::SDL3_image)
    if(TARGET PkgConfig::sdl3-image)
        add_library(SDL3_image::SDL3_image ALIAS PkgConfig::sdl3-image)
    else()
        message(STATUS "System SDL3_image not found — building from source (release-3.2.4)")
        CPMAddPackage(
            NAME SDL3_image
            GITHUB_REPOSITORY libsdl-org/SDL_image
            GIT_TAG release-3.2.4
            SYSTEM YES
            OPTIONS
                "SDLIMAGE_SAMPLES OFF"
                "SDLIMAGE_TESTS OFF"
                "SDLIMAGE_AVIF OFF"
                "SDLIMAGE_WEBP OFF"
                "SDLIMAGE_TIF OFF"
                "SDLIMAGE_JXL OFF"
        )
    endif()
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

CPMAddPackage(
    NAME LDtkLoader
    GITHUB_REPOSITORY Madour/LDtkLoader
    GIT_TAG 1.5.3.1
    SYSTEM YES
    OPTIONS
        "LDTK_NO_THROW OFF"
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
        VERSION 1.91.8
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
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
        )
        target_include_directories(imgui SYSTEM PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        target_link_libraries(imgui PUBLIC SDL3::SDL3)
        add_library(imgui::imgui ALIAS imgui)
    endif()
endif()
