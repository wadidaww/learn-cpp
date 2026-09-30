# Add executables for each example
add_executable(multithreading multithreading.cpp)
add_executable(mutex_example mutex_example.cpp)
add_executable(condition_variable_example condition_variable_example.cpp)
add_executable(future_promise_example future_promise_example.cpp)
add_executable(async_example async_example.cpp)

# Modern C++ topics
add_executable(move_semantics move_semantics.cpp)
add_executable(smart_pointers smart_pointers.cpp)
add_executable(lambdas lambdas.cpp)
add_executable(stl_containers stl_containers.cpp)
add_executable(stl_algorithms stl_algorithms.cpp)
add_executable(type_casting type_casting.cpp)
add_executable(raii_exceptions raii_exceptions.cpp)
add_executable(compile_time compile_time.cpp)
add_executable(operator_overloading operator_overloading.cpp)

add_subdirectory(benchmarking)
# IPC examples
add_subdirectory(ipc)

# IPC with file descriptors, select() and epoll()
add_subdirectory(ipc_fd)

# Python binding example (pybind11)
add_subdirectory(python_binding)

# Allocator lessons
add_subdirectory(allocator)

# Networking and low-latency lessons
add_subdirectory(networking)
# Extension points lessons
add_subdirectory(extension_points)

# Unit testing
add_subdirectory(unit_testing)
