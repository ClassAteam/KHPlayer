add_executable(video_server
    main.cpp
    src/Directory.cpp
    src/ErrorResponse.cpp
    src/FileListHttpResponse.cpp
    src/FileStream.cpp
    src/HttpConnection.cpp
    src/HttpRequest.cpp
    src/VideoReadyHttpResponse.cpp
    src/VideoServer.cpp)

target_include_directories(video_server
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${FFMPEG_INCLUDE_DIR}
)

target_link_libraries(video_server
    PRIVATE avcodec avformat avutil
)

set_target_properties(video_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    BUILD_RPATH "${FFMPEG_LIB_DIR}"
)
