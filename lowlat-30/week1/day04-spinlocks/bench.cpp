// Throughput of TAS vs TTAS vs std::mutex as contention grows.
// Expect the gap to widen with more threads. On a machine with fewer cores than threads,
// spinlocks collapse: note that too, it is the "when not to spin" answer.
#include <mutex>
#include <thread>
#include <vector>

#include "bench.hpp"
#include "solution.hpp"

using namespace ll;

namespace {

template <class Lock>
double ns_per_lock(int threads, int iters) {
    Lock lk;
    long counter = 0;
    std::atomic<bool> go{false};
    std::vector<std::thread> ts;
    for (int t = 0; t < threads; ++t)
        ts.emplace_back([&, t] {
            ll::bench::pin_to_cpu(t);
            while (!go.load(std::memory_order_acquire)) {
            }
            for (int i = 0; i < iters; ++i) {
                std::lock_guard g(lk);
                ++counter;
            }
        });
    auto t0 = ll::bench::now_cycles();
    go.store(true, std::memory_order_release);
    for (auto& t : ts) t.join();
    auto t1 = ll::bench::now_cycles();
    ll::bench::do_not_optimize(counter);
    return static_cast<double>(t1 - t0) / ll::bench::cycles_per_ns() / (static_cast<double>(iters) * threads);
}

}  // namespace

int main() {
    unsigned hw = std::thread::hardware_concurrency();
    std::printf("hardware threads: %u\n", hw);
    for (int th : {1, 2, 4}) {
        if (static_cast<unsigned>(th) > hw) break;
        int it = 1'000'000 / th;
        std::printf("threads=%d  TAS %.1f ns/op   TTAS %.1f ns/op   std::mutex %.1f ns/op\n", th,
                    ns_per_lock<TasSpinlock>(th, it), ns_per_lock<TtasSpinlock>(th, it),
                    ns_per_lock<std::mutex>(th, it));
    }
}
