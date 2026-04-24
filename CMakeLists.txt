cmake_minimum_required(VERSION 3.10)
project(SimpleVideoPlayer VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g")

add_compile_options(-Wall -Wextra -Wpedantic)

add_definitions(-D__STDC_CONSTANT_MACROS)

# ── Platform dependency setup ─────────────────────────────────────────────────
if(ANDROID)
    set(FFMPEG_INCLUDE_DIR "" CACHE PATH "Path to FFmpeg headers for Android")
    set(FFMPEG_LIB_DIR     "" CACHE PATH "Path to FFmpeg prebuilt .so libs for Android")
    set(SDL2_ANDROID_DIR   "" CACHE PATH "Path to SDL2 source for Android")

    add_subdirectory(${SDL2_ANDROID_DIR} ${CMAKE_BINARY_DIR}/SDL2)
    set(SDL2_LINK_TARGET SDL2)

else()
    set(FFMPEG_PREFIX "${CMAKE_SOURCE_DIR}/third_party/ffmpeg")
    set(FFMPEG_INCLUDE_DIR "${FFMPEG_PREFIX}/include")
    set(FFMPEG_LIB_DIR "${FFMPEG_PREFIX}/linux-x86_64")

    option(TRACY_ENABLE "Enable Tracy profiler" OFF)
    include(FetchContent)
    FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG v0.11.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(tracy)

    option(TRACY_PROFILER "Build Tracy profiler GUI" OFF)
    if(TRACY_PROFILER)
        set(LEGACY ON CACHE BOOL "" FORCE)
        add_subdirectory(${tracy_SOURCE_DIR}/profiler ${CMAKE_BINARY_DIR}/tracy-profiler)
    endif()

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    set(SDL2_LINK_TARGET PkgConfig::SDL2)

endif()

foreach(lib avcodec avformat avutil swscale swresample)
    add_library(${lib} SHARED IMPORTED)
    set_target_properties(${lib} PROPERTIES
        IMPORTED_LOCATION "${FFMPEG_LIB_DIR}/lib${lib}.so"
    )
endforeach()

# ── Player library ────────────────────────────────────────────────────────────
add_subdirectory(player)

# ── Targets ───────────────────────────────────────────────────────────────────
if(ANDROID)
    add_library(SimpleVideoPlayer SHARED
        android/app/src/main/cpp/android_client.cpp
        android/app/src/main/cpp/ConnectConfig.cpp
        android/app/src/main/cpp/ClientConnection.cpp
        android/app/src/main/cpp/FileMenu.cpp
        android/app/src/main/cpp/MenuRenderer.cpp
        android/app/src/main/cpp/Parser.cpp
        android/app/src/main/cpp/Selector.cpp
        android/app/src/main/cpp/TextRenderer.cpp
    )
    target_include_directories(SimpleVideoPlayer PRIVATE ${FFMPEG_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/android/app/src/main/cpp)
    target_link_libraries(SimpleVideoPlayer player SDL2 log android EGL GLESv2)

    add_subdirectory(android/playback-evaluation)

else()
    add_subdirectory(server)

endif()
