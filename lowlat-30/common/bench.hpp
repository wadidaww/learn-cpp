// Tiny measurement helpers: TSC timing, percentiles, thread pinning, DoNotOptimize.
// Week 2 day 12 explains every line of this file. Read it then; until then, use it.
#pragma once

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

#if defined(__x86_64__)
#include <x86intrin.h>
#endif

#if defined(__linux__)
#include <pthread.h>
#include <sched.h>
#endif

#include <chrono>

namespace ll::bench {

// Read the time-stamp counter. rdtscp waits for earlier instructions to finish;
// the lfence stops later instructions from starting before the read.
inline std::uint64_t now_cycles() {
#if defined(__x86_64__)
    unsigned aux;
    std::uint64_t t = __rdtscp(&aux);
    _mm_lfence();
    return t;
#else
    return static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
#endif
}

// Cycles per nanosecond, measured once against steady_clock.
inline double cycles_per_ns() {
    static const double v = [] {
        using clk = std::chrono::steady_clock;
        auto t0 = clk::now();
        auto c0 = now_cycles();
        while (clk::now() - t0 < std::chrono::milliseconds(200)) {
        }
        auto c1 = now_cycles();
        auto t1 = clk::now();
        double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
        return static_cast<double>(c1 - c0) / ns;
    }();
    return v;
}

// Stops the compiler from deleting a computation whose result is unused.
template <class T>
inline void do_not_optimize(T const& value) {
    asm volatile("" : : "r,m"(value) : "memory");
}

inline void clobber_memory() { asm volatile("" : : : "memory"); }

// Pin the calling thread to one CPU. Returns false if the OS refused.
inline bool pin_to_cpu(int cpu) {
#if defined(__linux__)
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    return pthread_setaffinity_np(pthread_self(), sizeof(set), &set) == 0;
#else
    (void)cpu;
    return false;
#endif
}

// Print p50 / p99 / p99.9 / max of samples given in cycles, converted to ns.
inline void report(const char* label, std::vector<std::uint64_t> samples) {
    if (samples.empty()) return;
    std::sort(samples.begin(), samples.end());
    auto pct = [&](double p) {
        auto i = static_cast<std::size_t>(p * static_cast<double>(samples.size() - 1));
        return static_cast<double>(samples[i]) / cycles_per_ns();
    };
    std::printf("%-28s n=%zu  p50=%.1fns  p99=%.1fns  p99.9=%.1fns  max=%.1fns\n", label, samples.size(), pct(0.50),
                pct(0.99), pct(0.999), static_cast<double>(samples.back()) / cycles_per_ns());
}

}  // namespace ll::bench
