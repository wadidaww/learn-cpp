cmake_minimum_required(VERSION 3.10)
project(cpp_sockets_project)

set(CMAKE_CXX_STANDARD 11)

include_directories(src/socket)

add_executable(socket_server src/socket/main.cpp)