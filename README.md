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

### Networking and Low-Latency Examples

- `./src/networking/networking_low_latency_udp` — **Low-latency UDP networking**: commented ping-pong benchmark showing fixed-size binary packets, non-blocking sockets, socket buffer tuning, bounded spin/yield receive loops, and p50/p99/p99.9 latency reporting.

You can also compile the lesson directly:

```bash
g++ -std=c++20 -O2 -Wall -Wextra src/networking/low_latency_udp.cpp -pthread -o /tmp/low_latency_udp
/tmp/low_latency_udp
```

### Benchmarking Examples

- `./src/benchmarking/micro_benchmark` — **Micro benchmarking**: times small, isolated code snippets (sorting algorithms, string concatenation strategies, integer square root implementations) using `std::chrono::high_resolution_clock` with many iterations to reduce measurement noise.
- `./src/benchmarking/macro_benchmark` — **Macro benchmarking**: measures end-to-end wall-clock time for realistic workloads (large container lookups, file I/O, summing millions of integers).
- `./src/benchmarking/partial_benchmark` — **Partial / section benchmarking**: demonstrates how to time only a specific region of a larger program using a `ScopedTimer` RAII guard and a `ManualTimer` start/stop helper, including a multi-stage pipeline where each stage is profiled independently.

### Variadic argument learning materials

There is also a focused lesson in `src/variadic/`:

- `README.md` — overview of when to use `va_list` versus variadic templates
- `c_style_variadics.cpp` — examples using `va_list`, `va_start`, `va_arg`, `va_copy`, and `va_end`
- `template_variadics.cpp` — examples using parameter packs, fold expressions, `std::index_sequence`, and compile-time checks

You can compile them directly:

```bash
g++ -std=c++20 src/variadic/c_style_variadics.cpp -o /tmp/c_style_variadics
g++ -std=c++20 src/variadic/template_variadics.cpp -o /tmp/template_variadics
```

### Allocator Examples

- `./src/allocator/alloc_01_std` — **`std::allocator` basics**: raw `allocate`/`deallocate`, manual construction with `std::construct_at`/`std::destroy_at`, and the `std::allocator_traits` interface that containers use internally.
- `./src/allocator/alloc_02_custom` — **Custom allocators**: building the minimum conforming allocator (`MinimalAllocator`) and a `TrackingAllocator` that logs every heap call — ideal for debugging memory usage.
- `./src/allocator/alloc_03_arena` — **Arena / pool allocator**: a stack-backed bump-pointer arena with zero runtime heap calls; also shows the standard C++17 equivalent `std::pmr::monotonic_buffer_resource`.
- `./src/allocator/alloc_04_pmr` — **`std::pmr` polymorphic memory resources**: `monotonic_buffer_resource`, `unsynchronized_pool_resource`, `synchronized_pool_resource`, `null_memory_resource`, resource chaining, and writing a custom `LoggingResource`.
- `./src/allocator/alloc_05_combining` — **Combining allocators across containers**: sharing one arena or pmr pool between `vector`, `list`, `map`, and `string`; nested pmr containers; scoped arenas for bulk O(1) cleanup.
