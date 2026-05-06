# Add executables for each example
add_executable(multithreading multithreading.cpp)
add_executable(mutex_example mutex_example.cpp)
add_executable(condition_variable_example condition_variable_example.cpp)
add_executable(future_promise_example future_promise_example.cpp)
add_executable(async_example async_example.cpp)

add_subdirectory(benchmarking)
# IPC examples
add_subdirectory(ipc)

# IPC with file descriptors, select() and epoll()
add_subdirectory(ipc_fd)

# Python binding example (pybind11)
add_subdirectory(python_binding)

# Allocator lessons
add_subdirectory(allocator)
