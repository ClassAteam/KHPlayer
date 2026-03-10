cmake_minimum_required(VERSION 3.10)
project(SimpleVideoPlayer VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g")

add_compile_options(-Wall -Wextra -Wpedantic)

add_definitions(-D__STDC_CONSTANT_MACROS)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(PLAYER_SOURCES
    src/main.cpp
    src/VideoPlayer.cpp
    src/utility.cpp
    src/VideoContainer.cpp
    src/Decoder.cpp
    src/Renderer.cpp
    src/SdlContext.cpp
    src/Converter.cpp
)

if(ANDROID)
    # Paths to prebuilt FFmpeg for Android (e.g. from ffmpeg-kit)
    # Pass these when configuring:
    #   -DFFMPEG_INCLUDE_DIR=/path/to/ffmpeg/include
    #   -DFFMPEG_LIB_DIR=/path/to/ffmpeg/lib
    set(FFMPEG_INCLUDE_DIR "" CACHE PATH "Path to FFmpeg headers for Android")
    set(FFMPEG_LIB_DIR     "" CACHE PATH "Path to FFmpeg prebuilt .so libs for Android")

    # Path to SDL2 source (SDL2 must be built alongside via NDK)
    # Pass: -DSDL2_ANDROID_DIR=/path/to/SDL2
    set(SDL2_ANDROID_DIR "" CACHE PATH "Path to SDL2 source for Android")

    foreach(lib avcodec avformat avutil swscale swresample)
        add_library(${lib} SHARED IMPORTED)
        set_target_properties(${lib} PROPERTIES
            IMPORTED_LOCATION ${FFMPEG_LIB_DIR}/lib${lib}.so
        )
    endforeach()

    add_subdirectory(${SDL2_ANDROID_DIR} ${CMAKE_BINARY_DIR}/SDL2)

    # Shared library — SDL2's Java activity loads this via System.loadLibrary()
    add_library(SimpleVideoPlayer SHARED ${PLAYER_SOURCES})

    target_include_directories(SimpleVideoPlayer PRIVATE ${FFMPEG_INCLUDE_DIR})

    target_link_libraries(SimpleVideoPlayer
        avcodec avformat avutil swscale swresample
        SDL2
        log  # Android logcat
    )

else()
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
        libavcodec
        libavformat
        libavutil
        libswscale
        libswresample
    )

    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

    add_executable(${PROJECT_NAME} ${PLAYER_SOURCES})

    target_link_libraries(${PROJECT_NAME}
        PkgConfig::LIBAV
        PkgConfig::SDL2
        pthread
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    add_executable(VideoServer src/server_main.cpp)

    target_link_libraries(VideoServer PkgConfig::LIBAV)

    set_target_properties(VideoServer PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

    set(CLIENT_SOURCES
        src/client_main.cpp
        src/VideoPlayer.cpp
        src/utility.cpp
        src/VideoContainer.cpp
        src/Decoder.cpp
        src/Renderer.cpp
        src/SdlContext.cpp
        src/Converter.cpp
    )

    add_executable(VideoClient ${CLIENT_SOURCES})

    target_link_libraries(VideoClient
        PkgConfig::LIBAV
        PkgConfig::SDL2
        pthread
    )

    set_target_properties(VideoClient PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

endif()
