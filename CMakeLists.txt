cmake_minimum_required(VERSION 3.14)
project(AudioTrip_LevelViewer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(OpenGL_GL_PREFERENCE GLVND)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -g -O0")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g -O0")

add_executable(
    AudioTrip_LevelViewer
    src/main.cpp
    src/audiotrip/dtos.cpp
    src/audiotrip/utils.cpp
    src/raylib_ext/text3d.cpp
    src/splines/spline3d.cpp)

if(EMSCRIPTEN)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s TOTAL_MEMORY=67108864 -s FORCE_FILESYSTEM=1 -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 --preload-file ${CMAKE_SOURCE_DIR}/resources@resources/ --shell-file ${CMAKE_SOURCE_DIR}/emscripten.html")
    set(PLATFORM Web CACHE BOOL "" FORCE) # for raylib
else()
    target_compile_options(${PROJECT_NAME} PUBLIC -DPLATFORM_DESKTOP)
endif()

target_include_directories(
    AudioTrip_LevelViewer PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/raylib-cpp/include
)

include(FetchContent)
set(FETCHCONTENT_QUIET NO)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

include(FetchContent)

# Use specific raylib commit since some functionality is broken in 4.0.0
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/ef5be632fe777e8303cb2fba6be95e0e4aa0fc85.tar.gz
)

if(EMSCRIPTEN)
    set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(JSONCPP_WITH_TESTS OFF CACHE BOOL "" FORCE)
    set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        jsoncpp
        GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
        GIT_TAG 1.9.5
    )
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        # GIT_TAG 8.1.1  # 8.1.1 does not build with emscripten
    )
    FetchContent_MakeAvailable(raylib jsoncpp fmt)

    target_include_directories(
        AudioTrip_LevelViewer PUBLIC
        ${jsoncpp_SOURCE_DIR}/include
    )

    target_link_libraries(
        AudioTrip_LevelViewer PUBLIC
        fmt
        ${CMAKE_BINARY_DIR}/lib/libjsoncpp.a  # yeeeep
        raylib
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
else()
    FetchContent_MakeAvailable(raylib)

    # PkgConfig dependencies
    find_package(PkgConfig REQUIRED)
    pkg_search_module(FMT REQUIRED IMPORTED_TARGET fmt>=8.1.0)
    pkg_search_module(JSONCPP REQUIRED IMPORTED_TARGET jsoncpp>=1.9.0)
    pkg_search_module(RAYLIB REQUIRED IMPORTED_TARGET raylib>=4.0.0)

    target_link_libraries(
        AudioTrip_LevelViewer PUBLIC
        PkgConfig::FMT
        PkgConfig::JSONCPP
        raylib
    )

endif()