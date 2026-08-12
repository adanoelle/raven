# Steamworks.cmake — optional Steamworks SDK integration
#
# The SDK is distributed under Valve's partner agreement and cannot be
# committed to the repository (vendor/steamworks/ is gitignored for it).
# To enable:
#   1. Download the Steamworks SDK from https://partner.steamgames.com
#   2. Unzip so the layout is vendor/steamworks/sdk/public/steam/steam_api.h
#   3. Configure with -DRAVEN_ENABLE_STEAM=ON
#
# During development, place a steam_appid.txt (containing your app id, or
# 480 for Valve's SpaceWar test app) next to the built binary so
# SteamAPI_Init can attach without a Steam launch. Do not ship that file.

set(STEAM_SDK_DIR "${CMAKE_SOURCE_DIR}/vendor/steamworks/sdk")

if(NOT EXISTS "${STEAM_SDK_DIR}/public/steam/steam_api.h")
    message(FATAL_ERROR
        "RAVEN_ENABLE_STEAM is ON but the Steamworks SDK was not found at\n"
        "  ${STEAM_SDK_DIR}\n"
        "Download it from https://partner.steamgames.com and unzip so that\n"
        "  vendor/steamworks/sdk/public/steam/steam_api.h exists,\n"
        "or configure with -DRAVEN_ENABLE_STEAM=OFF.")
endif()

add_library(steamworks SHARED IMPORTED)
target_include_directories(steamworks INTERFACE "${STEAM_SDK_DIR}/public")

if(WIN32)
    set_target_properties(steamworks PROPERTIES
        IMPORTED_LOCATION "${STEAM_SDK_DIR}/redistributable_bin/win64/steam_api64.dll"
        IMPORTED_IMPLIB "${STEAM_SDK_DIR}/redistributable_bin/win64/steam_api64.lib"
    )
elseif(APPLE)
    set_target_properties(steamworks PROPERTIES
        IMPORTED_LOCATION "${STEAM_SDK_DIR}/redistributable_bin/osx/libsteam_api.dylib"
    )
else()
    set_target_properties(steamworks PROPERTIES
        IMPORTED_LOCATION "${STEAM_SDK_DIR}/redistributable_bin/linux64/libsteam_api.so"
    )
endif()

add_library(steamworks::steamworks ALIAS steamworks)

# The redistributable must ship (and run) next to the game binary
add_custom_command(TARGET raven POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_PROPERTY:steamworks,IMPORTED_LOCATION>"
        "$<TARGET_FILE_DIR:raven>"
    COMMENT "Copying Steamworks redistributable"
)

message(STATUS "Steamworks SDK enabled: ${STEAM_SDK_DIR}")
