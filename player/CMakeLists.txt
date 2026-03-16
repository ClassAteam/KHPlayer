add_library(player STATIC
    src/VideoPlayer.cpp
    src/VideoContainer.cpp
    src/Decoder.cpp
    src/Renderer.cpp
    src/SdlContext.cpp
    src/Converter.cpp
    src/utility.cpp
)

target_include_directories(player
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${FFMPEG_INCLUDE_DIR}
)

target_link_libraries(player
    PRIVATE avcodec avformat avutil swscale swresample
    PRIVATE ${SDL2_LINK_TARGET}
)

if(ANDROID)
    target_link_libraries(player PRIVATE log)
else()
    target_link_libraries(player PRIVATE Tracy::TracyClient pthread)
endif()

if(NOT ANDROID)
    add_subdirectory(tests)
endif()
