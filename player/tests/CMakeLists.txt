# Player integration test — manual end-to-end playback
add_executable(SimpleVideoPlayer integration_main.cpp)
target_link_libraries(SimpleVideoPlayer player)
set_target_properties(SimpleVideoPlayer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    BUILD_RPATH "${FFMPEG_LIB_DIR}"
)
