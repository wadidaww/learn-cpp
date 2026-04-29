// partial_benchmark.cpp
//
// Partial (section) benchmarking measures only a specific region of a larger
// program — for example: just the "parse" step inside a pipeline, or just the
// "sort" step inside a search engine — rather than timing the whole program.
//
// Key ideas:
//  - Wrap only the code you care about in a start/stop timer.
//  - Use a ScopedTimer RAII guard so the timer stops automatically when the
//    guard goes out of scope — even if an exception is thrown.
//  - Accumulate section times to understand which part dominates.
//
// Examples covered:
//  1. ScopedTimer RAII guard (auto-stops on scope exit).
//  2. Manual start/stop timer for fine-grained control.
//  3. Multi-section pipeline where each stage is timed independently.

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// ===========================================================================
// ScopedTimer – RAII guard that prints elapsed time when it goes out of scope.
// ===========================================================================
class ScopedTimer {
public:
    explicit ScopedTimer(std::string label)
        : label_(std::move(label)),
          start_(std::chrono::high_resolution_clock::now()) {}

    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        double ms =
            std::chrono::duration<double, std::milli>(end - start_).count();
        std::cout << "[ScopedTimer] " << label_ << ": " << ms << " ms\n";
    }

    // Non-copyable
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

private:
    std::string label_;
    std::chrono::high_resolution_clock::time_point start_;
};

// ===========================================================================
// ManualTimer – explicit start/stop, can be queried multiple times.
// ===========================================================================
class ManualTimer {
public:
    void start() { start_ = std::chrono::high_resolution_clock::now(); }

    double stop_ms() {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }

private:
    std::chrono::high_resolution_clock::time_point start_;
};

// ===========================================================================
// Example 1 – ScopedTimer: benchmark just the sort inside a larger function
// ===========================================================================
void example_scoped_timer() {
    std::cout << "\n=== Partial Benchmark 1: ScopedTimer ===\n";

    const int N = 1'000'000;

    // --- setup phase (not timed) ---
    std::vector<int> data(N);
    std::iota(data.begin(), data.end(), 0);
    std::shuffle(data.begin(), data.end(), std::mt19937{42});

    // --- only the sort is measured ---
    {
        ScopedTimer timer("std::sort of " + std::to_string(N) + " ints");
        std::sort(data.begin(), data.end());
    }
    // timer has already printed when the block above ended.

    // --- post-processing phase (not timed) ---
    long long checksum = std::accumulate(data.begin(), data.end(), 0LL);
    std::cout << "(checksum: " << checksum << " — just to prevent dead-code elimination)\n";
}

// ===========================================================================
// Example 2 – ManualTimer: skip warm-up, time only the hot loop
// ===========================================================================
void example_manual_timer() {
    std::cout << "\n=== Partial Benchmark 2: ManualTimer (skip warm-up) ===\n";

    const int WARMUP = 1'000;
    const int HOT    = 100'000;

    ManualTimer t;
    // 'volatile' prevents the compiler from optimizing away the computation.
    volatile long long sink = 0;

    // warm-up (cache/branch-predictor warm-up, NOT timed)
    for (int i = 0; i < WARMUP; ++i) sink += i * i;

    // hot loop (timed)
    t.start();
    for (int i = 0; i < HOT; ++i) sink += i * i;
    double hot_ms = t.stop_ms();

    std::cout << "[ManualTimer] hot loop (" << HOT << " iterations): "
              << hot_ms << " ms"
              << "  |  avg: " << (hot_ms * 1e6 / HOT) << " ns/iter\n";
    std::cout << "(sink: " << sink << ")\n";
}

// ===========================================================================
// Example 3 – Pipeline: each stage is timed independently
// ===========================================================================

// Simulate the "load" stage: generate random data
std::vector<int> pipeline_load(int n) {
    std::vector<int> v(n);
    std::mt19937 rng(123);
    std::uniform_int_distribution<int> dist(0, 1'000'000);
    for (auto& x : v) x = dist(rng);
    return v;
}

// Simulate the "filter" stage: keep values > threshold
std::vector<int> pipeline_filter(const std::vector<int>& in, int threshold) {
    std::vector<int> out;
    out.reserve(in.size() / 2);
    for (int x : in)
        if (x > threshold) out.push_back(x);
    return out;
}

// Simulate the "sort" stage
std::vector<int> pipeline_sort(std::vector<int> v) {
    std::sort(v.begin(), v.end());
    return v;
}

// Simulate the "aggregate" stage: compute sum
long long pipeline_aggregate(const std::vector<int>& v) {
    return std::accumulate(v.begin(), v.end(), 0LL);
}

void example_pipeline_timing() {
    std::cout << "\n=== Partial Benchmark 3: Multi-Stage Pipeline ===\n";

    const int N = 2'000'000;
    ManualTimer t;

    // Stage 1 – Load
    t.start();
    auto data = pipeline_load(N);
    double load_ms = t.stop_ms();
    std::cout << "[pipeline] load      (" << N << " elements): " << load_ms << " ms\n";

    // Stage 2 – Filter
    t.start();
    auto filtered = pipeline_filter(data, 500'000);
    double filter_ms = t.stop_ms();
    std::cout << "[pipeline] filter    (" << filtered.size() << " kept):    " << filter_ms << " ms\n";

    // Stage 3 – Sort
    t.start();
    auto sorted = pipeline_sort(filtered);
    double sort_ms = t.stop_ms();
    std::cout << "[pipeline] sort      (" << sorted.size() << " elements): " << sort_ms << " ms\n";

    // Stage 4 – Aggregate
    t.start();
    long long total = pipeline_aggregate(sorted);
    double agg_ms = t.stop_ms();
    std::cout << "[pipeline] aggregate (sum=" << total << "):    " << agg_ms << " ms\n";

    double total_ms = load_ms + filter_ms + sort_ms + agg_ms;
    std::cout << "[pipeline] TOTAL:    " << total_ms << " ms\n";
}

// ===========================================================================
int main() {
    std::cout << "======================================================\n";
    std::cout << "  C++ Partial / Section Benchmarking Examples\n";
    std::cout << "  (time only the code region you care about)\n";
    std::cout << "======================================================\n";

    example_scoped_timer();
    example_manual_timer();
    example_pipeline_timing();

    std::cout << "\nDone.\n";
    return 0;
}
