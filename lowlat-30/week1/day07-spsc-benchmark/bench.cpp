// Day 7: measure YOUR ring from day 5 against a mutex queue.
//   cmake --build build --target day07_bench && ./build/day07_bench
// Usage: day07_bench [cpu_a cpu_b]   (default 0 1; pass -1 -1 to run unpinned)
#include <cstdlib>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>

#include "../day05-spsc-ring/solution.hpp"
#include "bench.hpp"

using ll::bench::now_cycles;

namespace {

// Baseline: what most people write first.
template <class T>
class MutexQueue {
public:
    bool try_push(const T& v) {
        std::lock_guard g(m_);
        if (q_.size() == 1024) return false;
        q_.push_back(v);
        return true;
    }
    bool try_pop(T& out) {
        std::lock_guard g(m_);
        if (q_.empty()) return false;
        out = q_.front();
        q_.pop_front();
        return true;
    }

private:
    std::mutex m_;
    std::deque<T> q_;
};

void maybe_pin(int cpu) {
    if (cpu >= 0 && !ll::bench::pin_to_cpu(cpu)) std::printf("  (could not pin to cpu %d)\n", cpu);
}

// Producer streams N items; consumer drains. Reports million items per second.
template <class Q>
void throughput(const char* label, Q& q, int cpu_a, int cpu_b) {
    constexpr std::uint64_t N = 20'000'000;
    std::thread cons([&] {
        maybe_pin(cpu_b);
        std::uint64_t v = 0, sum = 0;
        for (std::uint64_t i = 0; i < N;) {
            if (q.try_pop(v)) {
                sum += v;
                ++i;
            }
        }
        ll::bench::do_not_optimize(sum);
    });
    maybe_pin(cpu_a);
    auto t0 = now_cycles();
    for (std::uint64_t i = 0; i < N; ++i)
        while (!q.try_push(i)) {
        }
    cons.join();
    auto t1 = now_cycles();
    double secs = static_cast<double>(t1 - t0) / ll::bench::cycles_per_ns() / 1e9;
    std::printf("%-30s %8.1f M items/s\n", label, static_cast<double>(N) / secs / 1e6);
}

// Ping-pong: A sends its TSC to B through q1, B echoes through q2. One sample = two one-way hops.
template <class Q>
void round_trip(const char* label, Q& q1, Q& q2, int cpu_a, int cpu_b) {
    constexpr int N = 200'000;
    std::thread echo([&] {
        maybe_pin(cpu_b);
        std::uint64_t v;
        for (int i = 0; i < N; ++i) {
            while (!q1.try_pop(v)) {
            }
            while (!q2.try_push(v)) {
            }
        }
    });
    maybe_pin(cpu_a);
    std::vector<std::uint64_t> samples;
    samples.reserve(N);
    for (int i = 0; i < N; ++i) {
        std::uint64_t t0 = now_cycles(), back;
        while (!q1.try_push(t0)) {
        }
        while (!q2.try_pop(back)) {
        }
        samples.push_back(now_cycles() - back);
    }
    echo.join();
    samples.erase(samples.begin(), samples.begin() + N / 10);  // drop warm-up
    ll::bench::report(label, std::move(samples));
}

}  // namespace

int main(int argc, char** argv) {
    int a = argc > 2 ? std::atoi(argv[1]) : 0;
    int b = argc > 2 ? std::atoi(argv[2]) : 1;
    std::printf("cpus: producer=%d consumer=%d  (%.2f cycles/ns)\n\n", a, b, ll::bench::cycles_per_ns());

    static ll::SpscRing<std::uint64_t, 1024> r1, r2;
    MutexQueue<std::uint64_t> m1, m2;

    throughput("SpscRing throughput", r1, a, b);
    throughput("MutexQueue throughput", m1, a, b);
    std::printf("\n");
    round_trip("SpscRing round trip", r1, r2, a, b);
    round_trip("MutexQueue round trip", m1, m2, a, b);

    std::printf("\nWrite down: CPU model (lscpu), whether a and b are hyperthread siblings\n"
                "(lscpu -e: same CORE column), compiler, flags, and whether the machine was idle.\n");
}
