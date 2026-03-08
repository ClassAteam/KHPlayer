cmake_minimum_required(VERSION 3.10)
project(SimpleVideoPlayer VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g")

add_compile_options(-Wall -Wextra -Wpedantic)

add_definitions(-D__STDC_CONSTANT_MACROS)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
    libswresample
)

pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/VideoPlayer.cpp
    src/utility.cpp
    src/VideoContainer.cpp
    src/Decoder.cpp
    src/Renderer.cpp
    src/SdlContext.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    PkgConfig::LIBAV
    PkgConfig::SDL2
    pthread
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
