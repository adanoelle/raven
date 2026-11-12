# ShippingFlags.cmake — smaller, faster Release and MinSizeRel binaries
#
# Link-time optimisation, removal of unreferenced functions and data, and
# no symbol table. Debug and RelWithDebInfo are left alone.

include(CheckIPOSupported)
check_ipo_supported(RESULT RAVEN_IPO_SUPPORTED OUTPUT RAVEN_IPO_OUTPUT LANGUAGES CXX)
if(RAVEN_IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
else()
    message(STATUS "Link-time optimisation not available: ${RAVEN_IPO_OUTPUT}")
endif()

set(RAVEN_SHIPPING_CONFIG "$<CONFIG:Release,MinSizeRel>")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        "$<${RAVEN_SHIPPING_CONFIG}:-ffunction-sections>"
        "$<${RAVEN_SHIPPING_CONFIG}:-fdata-sections>"
    )
    if(APPLE)
        add_link_options("$<${RAVEN_SHIPPING_CONFIG}:-Wl,-dead_strip>")
    else()
        add_link_options(
            "$<${RAVEN_SHIPPING_CONFIG}:-Wl,--gc-sections>"
            "$<${RAVEN_SHIPPING_CONFIG}:-s>"
        )
    endif()
endif()
