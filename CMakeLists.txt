cmake_minimum_required(VERSION 3.16)
project(search_sim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Emit build/compile_commands.json so editors (VS Code IntelliSense, clangd)
# resolve headers with the same -Isrc / raylib flags as the real build.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# raylib: use an installed copy if present, otherwise fetch + build it. This
# keeps the build a single command with no separate install step.
find_package(raylib QUIET)
if(NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(raylib)
endif()

add_executable(search_sim
    src/main.cpp
    src/core/grid.cpp
    src/core/crdt_state.cpp
    src/core/agent.cpp
    src/comms/sync.cpp
    src/render/logger.cpp
    src/render/raylib_renderer.cpp
    src/sim/simulation.cpp)

# src/ is the include root, so headers are referenced by folder,
# e.g. #include "core/grid.h" -- which makes the layering self-documenting.
target_include_directories(search_sim PRIVATE src)

target_link_libraries(search_sim PRIVATE raylib)
