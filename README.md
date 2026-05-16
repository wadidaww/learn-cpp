# Learn C++ Async

This project contains examples of asynchronous operations in C++.

## Prerequisites

- C++17 compiler
- CMake 3.10 or later

## Build Instructions

1. Create a build directory:
   ```
   mkdir build
   cd build
   ```

2. Run CMake:
   ```
   cmake ..
   ```

3. Build the examples:
   ```
   cmake --build .
   ```

## Running the Examples

After building, the executables will be in the `build/src` directory.

You can run each example individually:

- `./src/multithreading`
- `./src/mutex_example`
- `./src/condition_variable_example`
- `./src/future_promise_example`
- `./src/async_example`

### Benchmarking Examples

- `./src/benchmarking/micro_benchmark` — **Micro benchmarking**: times small, isolated code snippets (sorting algorithms, string concatenation strategies, integer square root implementations) using `std::chrono::high_resolution_clock` with many iterations to reduce measurement noise.
- `./src/benchmarking/macro_benchmark` — **Macro benchmarking**: measures end-to-end wall-clock time for realistic workloads (large container lookups, file I/O, summing millions of integers).
- `./src/benchmarking/partial_benchmark` — **Partial / section benchmarking**: demonstrates how to time only a specific region of a larger program using a `ScopedTimer` RAII guard and a `ManualTimer` start/stop helper, including a multi-stage pipeline where each stage is profiled independently.

### Variadic argument learning materials

There is also a focused lesson in `/home/runner/work/learn-cpp/learn-cpp/src/variadic/`:

- `README.md` — overview of when to use `va_list` versus variadic templates
- `c_style_variadics.cpp` — examples using `va_list`, `va_start`, `va_arg`, `va_copy`, and `va_end`
- `template_variadics.cpp` — examples using parameter packs, fold expressions, `std::index_sequence`, and compile-time checks

You can compile them directly:

```bash
g++ -std=c++20 /home/runner/work/learn-cpp/learn-cpp/src/variadic/c_style_variadics.cpp -o /tmp/c_style_variadics
g++ -std=c++20 /home/runner/work/learn-cpp/learn-cpp/src/variadic/template_variadics.cpp -o /tmp/template_variadics
```
