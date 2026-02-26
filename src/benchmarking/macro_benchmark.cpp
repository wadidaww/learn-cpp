// macro_benchmark.cpp
//
// Macro benchmarking measures the end-to-end performance of larger,
// real-world workloads: file I/O, container-wide operations, complex
// algorithms that process significant amounts of data, etc.
//
// Key ideas:
//  - Time the whole operation, not just a tiny loop.
//  - Use realistic data sizes (thousands – millions of elements).
//  - Report wall-clock time (ms / s) rather than nanoseconds per iteration.
//
// Examples covered:
//  1. Building and searching a large vector vs. an unordered_map.
//  2. Writing and reading a large temporary file.
//  3. Summing 10 million integers: raw loop vs. std::accumulate.

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

// ---------------------------------------------------------------------------
// Timing helper: measures a single run and prints elapsed time in ms.
// ---------------------------------------------------------------------------
void bench_ms(const std::string& label, std::function<void()> fn) {
    auto start = std::chrono::high_resolution_clock::now();
    fn();
    auto end = std::chrono::high_resolution_clock::now();

    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << "[" << label << "] elapsed: " << ms << " ms\n";
}

// ---------------------------------------------------------------------------
// Example 1 – Vector linear search vs. unordered_map lookup
// ---------------------------------------------------------------------------
void benchmark_lookup() {
    std::cout << "\n=== Macro Benchmark 1: Vector Search vs. unordered_map ===\n";

    const int N = 500'000;

    // Build dataset
    std::vector<int> data(N);
    std::iota(data.begin(), data.end(), 0); // 0 .. N-1

    // Targets spread across the range
    std::vector<int> targets = {1000, 100'000, 250'000, 499'999};

    // --- Vector: O(n) linear search ---
    bench_ms("vector linear search (" + std::to_string(N) + " elements)", [&]() {
        for (int t : targets) {
            volatile bool found =
                (std::find(data.begin(), data.end(), t) != data.end());
            (void)found;
        }
    });

    // Build unordered_map
    std::unordered_map<int, int> umap;
    umap.reserve(N);
    for (int i = 0; i < N; ++i) umap[i] = i;

    // --- unordered_map: O(1) average lookup ---
    bench_ms("unordered_map lookup  (" + std::to_string(N) + " elements)", [&]() {
        for (int t : targets) {
            volatile bool found = (umap.count(t) > 0);
            (void)found;
        }
    });
}

// ---------------------------------------------------------------------------
// Example 2 – File I/O: write then read a large file
// ---------------------------------------------------------------------------
void benchmark_file_io() {
    std::cout << "\n=== Macro Benchmark 2: File I/O ===\n";

    const std::string filename = "/tmp/macro_bench_test.txt";
    const int LINES = 100'000;

    bench_ms("write " + std::to_string(LINES) + " lines to file", [&]() {
        std::ofstream ofs(filename);
        for (int i = 0; i < LINES; ++i)
            ofs << "line " << i << ": benchmarking file I/O in C++\n";
    });

    bench_ms("read  " + std::to_string(LINES) + " lines from file", [&]() {
        std::ifstream ifs(filename);
        std::string line;
        while (std::getline(ifs, line)) {
            // consume line
        }
    });

    std::remove(filename.c_str()); // clean up temp file
}

// ---------------------------------------------------------------------------
// Example 3 – Summing 10 million integers
// ---------------------------------------------------------------------------
void benchmark_sum() {
    std::cout << "\n=== Macro Benchmark 3: Sum 10M Integers ===\n";

    const int N = 10'000'000;
    std::vector<int> data(N, 1);

    bench_ms("raw for-loop sum (" + std::to_string(N) + " elements)", [&]() {
        volatile long long sum = 0;
        for (int v : data) sum += v;
    });

    bench_ms("std::accumulate  (" + std::to_string(N) + " elements)", [&]() {
        volatile long long sum =
            std::accumulate(data.begin(), data.end(), 0LL);
        (void)sum;
    });
}

// ---------------------------------------------------------------------------
int main() {
    std::cout << "======================================================\n";
    std::cout << "  C++ Macro Benchmarking Examples\n";
    std::cout << "  (wall-clock time for realistic workloads)\n";
    std::cout << "======================================================\n";

    benchmark_lookup();
    benchmark_file_io();
    benchmark_sum();

    std::cout << "\nDone.\n";
    return 0;
}
