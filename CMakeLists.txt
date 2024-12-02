cmake_minimum_required(VERSION 3.10)

project(ServerClientProject)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SERVER_SOURCES src/server.cpp)

set(CLIENT_SOURCES src/client.cpp)

add_executable(server ${SERVER_SOURCES})

add_executable(client ${CLIENT_SOURCES})

target_link_libraries(server ws2_32)
target_link_libraries(client ws2_32)

set_target_properties(server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/server
)

set_target_properties(client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/client
)
