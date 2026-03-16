add_executable(VideoServer
    main.cpp
    src/VideoServer.cpp
)

target_include_directories(VideoServer
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${FFMPEG_INCLUDE_DIR}
)

target_link_libraries(VideoServer
    PRIVATE avcodec avformat avutil
)

set_target_properties(VideoServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    BUILD_RPATH "${FFMPEG_LIB_DIR}"
)
