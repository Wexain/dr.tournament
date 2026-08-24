cmake_minimum_required(VERSION 3.24)
project(DrTournaments VERSION 0.1.0 LANGUAGES C CXX)

# ── C++20 Standard ──────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)

# ── Build Options ───────────────────────────────────────────────────────────
option(DRT_BUILD_SERVER  "Build headless dedicated server" ON)
option(DRT_BUILD_CLIENT  "Build game client"               ON)
option(DRT_ENABLE_VOICE  "Enable voice chat (libdatachannel)" OFF)
option(DRT_ANDROID       "Building for Android"            OFF)

# ── Platform Detection ──────────────────────────────────────────────────────
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(DRT_ANDROID ON)
    set(DRT_BUILD_SERVER OFF)
    message(STATUS "[DrT] Android build detected — server disabled")
endif()

# ── FetchContent Dependencies ───────────────────────────────────────────────
include(FetchContent)

# Raylib
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
if(DRT_ANDROID)
    set(PLATFORM "Android" CACHE STRING "" FORCE)
    set(OPENGL_VERSION "ES 3.0" CACHE STRING "" FORCE)
else()
    set(OPENGL_VERSION "3.3" CACHE STRING "" FORCE)
endif()
FetchContent_MakeAvailable(raylib)

# ENet
FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG        v1.3.18
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(enet)

# nlohmann/json (header-only)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# ── Source Files ────────────────────────────────────────────────────────────
set(DRT_CORE_SOURCES
    src/core/engine.cpp
    src/core/input_system.cpp
    src/core/camera_system.cpp
    src/core/settings.cpp
)

set(DRT_PHYSICS_SOURCES
    src/physics/raycast_vehicle.cpp
    src/physics/transmission.cpp
    src/physics/collision.cpp
)

set(DRT_WORLD_SOURCES
    src/world/road_generator.cpp
    src/world/traffic_ai.cpp
    src/world/environment.cpp
)

set(DRT_RENDER_SOURCES
    src/render/vehicle_renderer.cpp
    src/render/hud.cpp
    src/render/fx.cpp
    src/render/android_ui.cpp
)

set(DRT_NET_SOURCES
    src/net/enet_client.cpp
    src/net/enet_server.cpp
    src/net/voice_chat.cpp
)

set(DRT_SOCIAL_SOURCES
    src/social/auth.cpp
    src/social/friends.cpp
    src/social/economy.cpp
)

set(DRT_TOURNAMENT_SOURCES
    src/tournament/moderator.cpp
    src/tournament/rules.cpp
)

set(DRT_BENCHMARK_SOURCES
    src/benchmark/benchmark.cpp
)

set(DRT_SHARED_SOURCES
    ${DRT_CORE_SOURCES}
    ${DRT_PHYSICS_SOURCES}
    ${DRT_WORLD_SOURCES}
    ${DRT_RENDER_SOURCES}
    ${DRT_NET_SOURCES}
    ${DRT_SOCIAL_SOURCES}
    ${DRT_TOURNAMENT_SOURCES}
    ${DRT_BENCHMARK_SOURCES}
)

# ── Shared Game Library ─────────────────────────────────────────────────────
add_library(drt_game STATIC ${DRT_SHARED_SOURCES})
target_include_directories(drt_game PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(drt_game PUBLIC
    raylib
    enet
    nlohmann_json::nlohmann_json
)

# Platform-specific link libraries
if(WIN32)
    target_link_libraries(drt_game PUBLIC ws2_32 winmm)
    target_compile_definitions(drt_game PUBLIC DRT_PLATFORM_WINDOWS=1)
elseif(DRT_ANDROID)
    target_link_libraries(drt_game PUBLIC android EGL GLESv3 OpenSLES log)
    target_compile_definitions(drt_game PUBLIC DRT_PLATFORM_ANDROID=1)
else()
    target_compile_definitions(drt_game PUBLIC DRT_PLATFORM_LINUX=1)
endif()

# ── Client Executable ───────────────────────────────────────────────────────
if(DRT_BUILD_CLIENT)
    if(DRT_ANDROID)
        # Android: shared library for NativeActivity
        add_library(dr_tournaments SHARED src/main.cpp)
    else()
        add_executable(dr_tournaments src/main.cpp)
    endif()

    target_link_libraries(dr_tournaments PRIVATE drt_game)

    # Copy assets next to executable
    if(NOT DRT_ANDROID)
        add_custom_command(TARGET dr_tournaments POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/assets
                $<TARGET_FILE_DIR:dr_tournaments>/assets
            COMMENT "Copying assets..."
        )
    endif()
endif()

# ── Dedicated Server Executable ─────────────────────────────────────────────
if(DRT_BUILD_SERVER)
    add_executable(dr_tournaments_server src/server_main.cpp)
    target_link_libraries(dr_tournaments_server PRIVATE drt_game)
    target_compile_definitions(dr_tournaments_server PRIVATE DRT_HEADLESS=1)
endif()

# ── Compiler Warnings ──────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(drt_game PRIVATE /W4 /permissive-)
else()
    target_compile_options(drt_game PRIVATE -Wall -Wextra -Wpedantic)
endif()

message(STATUS "═══════════════════════════════════════════════════")
message(STATUS "  Dr. Tournaments v${PROJECT_VERSION}")
message(STATUS "  Client:  ${DRT_BUILD_CLIENT}")
message(STATUS "  Server:  ${DRT_BUILD_SERVER}")
message(STATUS "  Android: ${DRT_ANDROID}")
message(STATUS "  Voice:   ${DRT_ENABLE_VOICE}")
message(STATUS "═══════════════════════════════════════════════════")
