// micro_benchmark.cpp
//
// Micro benchmarking measures the performance of small, isolated pieces of
// code such as a single function, algorithm, or data structure operation.
//
// Key ideas:
//  - Use std::chrono for high-resolution timing.
//  - Run the target code many times (iterations) to reduce noise.
//  - Compare multiple implementations side-by-side.
//
// Examples covered:
//  1. Comparing bubble sort vs std::sort on a small vector.
//  2. Comparing string concatenation strategies.
//  3. Comparing integer square-root implementations.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Tiny benchmark helper
// ---------------------------------------------------------------------------
// Runs `fn` for `iterations` times and returns the average duration in
// nanoseconds per iteration.
double bench_ns(const std::string& label,
                std::function<void()> fn,
                int iterations = 100'000) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        fn();
    }
    auto end = std::chrono::high_resolution_clock::now();

    double total_ns =
        static_cast<double>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                .count());
    double avg_ns = total_ns / iterations;

    std::cout << "[" << label << "] "
              << "total: " << static_cast<long long>(total_ns) << " ns  |  "
              << "avg per iteration: " << avg_ns << " ns\n";
    return avg_ns;
}

// ---------------------------------------------------------------------------
// Example 1 – Sorting algorithms
// ---------------------------------------------------------------------------
void bubble_sort(std::vector<int>& v) {
    for (size_t i = 0; i < v.size(); ++i)
        for (size_t j = 0; j + 1 < v.size() - i; ++j)
            if (v[j] > v[j + 1])
                std::swap(v[j], v[j + 1]);
}

void benchmark_sorting() {
    std::cout << "\n=== Micro Benchmark 1: Sorting Algorithms ===\n";

    const std::vector<int> original = {9, 3, 7, 1, 5, 8, 2, 6, 4, 0};

    bench_ns("bubble_sort (10 elements)", [&]() {
        std::vector<int> v = original;
        bubble_sort(v);
    });

    bench_ns("std::sort   (10 elements)", [&]() {
        std::vector<int> v = original;
        std::sort(v.begin(), v.end());
    });
}

// ---------------------------------------------------------------------------
// Example 2 – String concatenation strategies
// ---------------------------------------------------------------------------
void benchmark_string_concat() {
    std::cout << "\n=== Micro Benchmark 2: String Concatenation ===\n";

    const int N = 100;

    // Strategy A: repeated operator+= on std::string
    bench_ns("string += loop (" + std::to_string(N) + " appends)", [&]() {
        std::string result;
        for (int i = 0; i < N; ++i)
            result += "hello";
    }, 10'000);

    // Strategy B: std::ostringstream
    bench_ns("ostringstream  (" + std::to_string(N) + " appends)", [&]() {
        std::ostringstream oss;
        for (int i = 0; i < N; ++i)
            oss << "hello";
        std::string result = oss.str();
    }, 10'000);

    // Strategy C: reserve then append
    bench_ns("reserve+append (" + std::to_string(N) + " appends)", [&]() {
        std::string result;
        result.reserve(5 * N);
        for (int i = 0; i < N; ++i)
            result.append("hello");
    }, 10'000);
}

// ---------------------------------------------------------------------------
// Example 3 – Integer square root implementations
// ---------------------------------------------------------------------------
// Naive: cast to double and call sqrt
int isqrt_double(int n) {
    return static_cast<int>(std::sqrt(static_cast<double>(n)));
}

// Newton-Raphson integer square root
int isqrt_newton(int n) {
    if (n < 0) return -1;
    if (n == 0) return 0;
    int x = n;
    int y = (x + 1) / 2;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    return x;
}

void benchmark_isqrt() {
    std::cout << "\n=== Micro Benchmark 3: Integer Square Root ===\n";

    bench_ns("isqrt_double (sqrt cast)", [&]() {
        volatile int r = isqrt_double(123456789);
        (void)r;
    });

    bench_ns("isqrt_newton (Newton-Raphson int sqrt)", [&]() {
        volatile int r = isqrt_newton(123456789);
        (void)r;
    });
}

// ---------------------------------------------------------------------------
int main() {
    std::cout << "======================================================\n";
    std::cout << "  C++ Micro Benchmarking Examples\n";
    std::cout << "  (using std::chrono::high_resolution_clock)\n";
    std::cout << "======================================================\n";

    benchmark_sorting();
    benchmark_string_concat();
    benchmark_isqrt();

    std::cout << "\nDone.\n";
    return 0;
}
