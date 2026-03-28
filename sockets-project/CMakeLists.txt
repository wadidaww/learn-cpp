cmake_minimum_required(VERSION 3.0)
project(sockets_project)

set(CMAKE_CXX_STANDARD 20)

file(GLOB SOURCES "src/*.cpp")

add_executable(server src/server.cpp)
add_executable(client src/client.cpp)

target_link_libraries(server pthread)
target_link_libraries(client pthread)
