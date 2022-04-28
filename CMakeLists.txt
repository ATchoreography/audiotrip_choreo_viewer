cmake_minimum_required(VERSION 3.14)
project(AudioTrip_LevelViewer)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# PkgConfig dependencies
find_package(PkgConfig REQUIRED)
pkg_search_module(FMT REQUIRED IMPORTED_TARGET fmt>=8.1.0)
pkg_search_module(JSONCPP REQUIRED IMPORTED_TARGET jsoncpp>=1.9.0)
pkg_search_module(RAYLIB REQUIRED IMPORTED_TARGET raylib>=4.0.0)

add_executable(
    AudioTrip_LevelViewer
    src/main.cpp
    src/audiotrip/dtos.cpp
    src/audiotrip/utils.cpp
    src/raylib_ext/text3d.cpp)

target_include_directories(
    AudioTrip_LevelViewer PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/raylib-cpp/include
)

target_link_libraries(
    AudioTrip_LevelViewer PUBLIC
    PkgConfig::FMT
    PkgConfig::JSONCPP
    PkgConfig::RAYLIB
)
