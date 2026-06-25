cmake_minimum_required(VERSION 3.10)

add_executable(networking_low_latency_udp low_latency_udp.cpp)
target_compile_options(networking_low_latency_udp PRIVATE -Wall -Wextra -O2)
target_link_libraries(networking_low_latency_udp PRIVATE pthread)
