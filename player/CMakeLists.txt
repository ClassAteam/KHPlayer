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
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC  ${FFMPEG_INCLUDE_DIR}
    PUBLIC ${CMAKE_SOURCE_DIR}/common
)

target_link_libraries(player
    PRIVATE avcodec avformat avutil swscale swresample
    PRIVATE ${SDL2_LINK_TARGET}
)

if(NOT ANDROID)
    target_link_libraries(player PRIVATE Tracy::TracyClient pthread)
endif()
