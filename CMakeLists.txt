cmake_minimum_required(VERSION 3.10)
project(SimpleVideoPlayer VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g")

add_compile_options(-Wall -Wextra -Wpedantic)

add_definitions(-D__STDC_CONSTANT_MACROS)

if(ANDROID)
    set(FFMPEG_INCLUDE_DIR "" CACHE PATH "Path to FFmpeg headers for Android")
    set(FFMPEG_LIB_DIR     "" CACHE PATH "Path to FFmpeg prebuilt .so libs for Android")
    set(SDL2_ANDROID_DIR   "" CACHE PATH "Path to SDL2 source for Android")

    add_subdirectory(${SDL2_ANDROID_DIR} ${CMAKE_BINARY_DIR}/SDL2)
    #SDL is to verbose. We can't handle it more gracefully than that.
    # At least for now
    target_compile_options(SDL2 PRIVATE -w)
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

# We are still OK to have the same sources for both
# platforms in those directories
add_subdirectory(player)
add_subdirectory(client)

# This is our Android client target with JNI-specific
# entry point that SDL provides
if(ANDROID)
    # The main Android app with FilePicker
    add_library(SimpleVideoPlayer SHARED
      android/app/src/main/cpp/android_client.cpp
    )
    target_include_directories(SimpleVideoPlayer PRIVATE
      ${CMAKE_SOURCE_DIR}/client
      ${CMAKE_SOURCE_DIR}/player/include
    )
    target_link_libraries(SimpleVideoPlayer PRIVATE
      client player avcodec avformat avutil swscale swresample
      SDL2 log android EGL GLESv2
    )

    # Integration test that runs predefined video from
    # server right away in order to skip FilePicker
    # user manual interaction 
    add_library(IntegrationVideoTest SHARED
       android/integration-test/src/main/cpp/play_container.cpp
    )

    target_include_directories(IntegrationVideoTest PRIVATE
       ${CMAKE_SOURCE_DIR}/client
       ${CMAKE_SOURCE_DIR}/player/include
       ${CMAKE_SOURCE_DIR}/common
       ${FFMPEG_INCLUDE_DIR}
    )
    target_link_libraries(IntegrationVideoTest PRIVATE
       client player avcodec avformat avutil swscale swresample
       SDL2 log android EGL GLESv2
    )
else()
    add_subdirectory(server)
endif()
