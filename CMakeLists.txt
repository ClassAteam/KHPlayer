cmake_minimum_required(VERSION 3.10)
project(SimpleVideoPlayer VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # Generate compile_commands.json for LSP

# Enable strict warnings
add_compile_options(-Wall -Wextra -Wpedantic)

# FFmpeg requires this macro for C++ compatibility
add_definitions(-D__STDC_CONSTANT_MACROS)

# Debug build: Enable sanitizers to catch real bugs
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
    message(STATUS "Sanitizers enabled (AddressSanitizer + UndefinedBehaviorSanitizer)")
endif()

# Find required packages
find_package(PkgConfig REQUIRED)

# Find FFmpeg libraries
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
    libswresample
)

# Find SDL2
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/VideoPlayer.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PkgConfig::LIBAV
    PkgConfig::SDL2
    pthread
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
