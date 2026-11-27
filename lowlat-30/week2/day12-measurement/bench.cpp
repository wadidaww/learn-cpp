// 1. What does each clock cost? (measured with your histogram)
// 2. Coordinated omission, demonstrated on a simulated service with one 5 ms stall.
#include <chrono>
#include <ctime>

#include "bench.hpp"
#include "solution.hpp"

using ll::bench::now_cycles;

namespace {

template <class F>
void clock_cost(const char* label, F&& read) {
    ll::LatencyHistogram<6> h;
    for (int i = 0; i < 1'000'000; ++i) {
        auto a = now_cycles();
        auto v = read();
        auto b = now_cycles();
        ll::bench::do_not_optimize(v);
        h.record(b - a);
    }
    double c = ll::bench::cycles_per_ns();
    std::printf("%-26s p50 %6.1f ns   p99 %6.1f ns   p99.9 %7.1f ns   max %9.1f ns\n", label,
                static_cast<double>(h.percentile(0.5)) / c, static_cast<double>(h.percentile(0.99)) / c,
                static_cast<double>(h.percentile(0.999)) / c, static_cast<double>(h.max()) / c);
}

void print(const char* label, const ll::LatencyHistogram<6>& h) {
    std::printf("%-26s n=%-6llu p50 %6llu us  p99 %6llu us  p99.9 %6llu us  max %6llu us\n", label,
                static_cast<unsigned long long>(h.count()), static_cast<unsigned long long>(h.percentile(0.5)),
                static_cast<unsigned long long>(h.percentile(0.99)), static_cast<unsigned long long>(h.percentile(0.999)),
                static_cast<unsigned long long>(h.max()));
}

}  // namespace

int main() {
    ll::bench::pin_to_cpu(0);
    std::printf("Clock costs (includes one rdtscp pair around the call; the first row is that pair alone):\n");
    clock_cost("empty (timer overhead)", [] { return 0; });
    clock_cost("rdtscp+lfence", [] { return now_cycles(); });
    clock_cost("steady_clock::now()", [] { return std::chrono::steady_clock::now().time_since_epoch().count(); });
    clock_cost("clock_gettime(MONOTONIC)", [] {
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_nsec;
    });

    // Simulated service: each request takes 10 us, except one that stalls for 5000 us.
    // Clients intend to send one request every 100 us, for 1 second.
    std::printf("\nCoordinated omission (service time 10 us, one 5 ms stall, a request intended every 100 us):\n");
    ll::LatencyHistogram<6> naive, corrected, truth;
    std::uint64_t t = 0, free_at = 0;
    for (std::uint64_t k = 0; k < 10'000; ++k) {
        std::uint64_t service = (k == 5'000) ? 5'000 : 10;
        // closed loop: the naive client waits for each reply, so it only measures service time
        naive.record(service);
        corrected.record_corrected(service, 100);
        // open loop: request k is due at k*100 us; it waits if the server is still busy
        std::uint64_t intended = k * 100;
        std::uint64_t start = std::max(intended, free_at);
        free_at = start + service;
        truth.record(free_at - intended);
        t = free_at;
    }
    ll::bench::do_not_optimize(t);
    print("naive closed loop", naive);
    print("corrected after the fact", corrected);
    print("open loop (what users see)", truth);
}
