add_library(client STATIC
  ConnectConfig.cpp
  ClientConnection.cpp
  FileMenu.cpp
  MenuRenderer.cpp
  Parser.cpp
  Selector.cpp
  TextRenderer.cpp
)

target_link_libraries(client 
  PRIVATE ${SDL2_LINK_TARGET}
)

target_include_directories(client
  PUBLIC ${CMAKE_SOURCE_DIR}/common/
)
