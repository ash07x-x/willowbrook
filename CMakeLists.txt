cmake_minimum_required(VERSION 3.15)
project(willowbrook CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Use an installed raylib if there is one, otherwise download and build it.
find_package(raylib 5.0 QUIET)
if(NOT raylib_FOUND)
  include(FetchContent)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(raylib)
endif()

# Everything except main() lives in a library so the capture tool can reuse it.
add_library(willowbrook_core src/sprites.cpp src/world.cpp src/game.cpp)
target_include_directories(willowbrook_core PUBLIC src)
target_link_libraries(willowbrook_core PUBLIC raylib)
if(MSVC)
  target_compile_options(willowbrook_core PRIVATE /W4)
else()
  target_compile_options(willowbrook_core PRIVATE -Wall -Wextra)
endif()

add_executable(willowbrook src/main.cpp)
target_link_libraries(willowbrook PRIVATE willowbrook_core)

# Optional: `cmake --build build --target capture` makes the README screenshots.
add_executable(capture EXCLUDE_FROM_ALL tools/capture.cpp)
target_link_libraries(capture PRIVATE willowbrook_core)
