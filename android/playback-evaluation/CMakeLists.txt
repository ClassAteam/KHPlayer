add_executable(decoder_test
    main.cpp
    ${CMAKE_SOURCE_DIR}/android/app/src/main/cpp/ConnectConfig.cpp
    ${CMAKE_SOURCE_DIR}/android/app/src/main/cpp/ClientConnection.cpp
    ${CMAKE_SOURCE_DIR}/android/app/src/main/cpp/Parser.cpp
)

target_include_directories(decoder_test PRIVATE
    ${CMAKE_SOURCE_DIR}/player/src
    ${CMAKE_SOURCE_DIR}/android/app/src/main/cpp
)
target_link_libraries(decoder_test player log android EGL GLESv2)

