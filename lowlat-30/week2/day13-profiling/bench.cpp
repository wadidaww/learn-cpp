// Profiling target: the day 11 workload on your order book, with no timers in the loop.
// Usage: day13_bench [ops]   (default 20,000,000; use fewer under valgrind)
#include <cstdlib>
#include <vector>

#include "../day11-order-book/solution.hpp"
#include "bench.hpp"
#include "rng.hpp"

using namespace ll;

namespace {

struct CountingListener {
    std::uint64_t fills = 0;
    void on_fill(const Fill&) { ++fills; }
};

}  // namespace

int main(int argc, char** argv) {
    std::size_t n = argc > 1 ? static_cast<std::size_t>(std::strtoull(argv[1], nullptr, 10)) : 20'000'000;
    ll::bench::pin_to_cpu(0);
    CountingListener l;
    OrderBook<CountingListener> book(9'000, 11'000, 2'000'000, l);
    SplitMix64 g{7};
    std::vector<OrderId> live;
    live.reserve(2'000'000);
    OrderId next = 1;
    auto t0 = ll::bench::now_cycles();
    for (std::size_t i = 0; i < n; ++i) {
        auto r = g.next_below(100);
        if (r < 50 || live.empty()) {
            Side s = g.next_below(2) ? Side::Buy : Side::Sell;
            Price off = static_cast<Price>(g.next_below(20));
            bool cross = g.next_below(4) == 0;
            Price px = s == Side::Buy ? (cross ? 10'001 + off / 4 : 10'000 - off) : (cross ? 9'999 - off / 4 : 10'001 + off);
            if (book.add(next, s, px, 1 + static_cast<Qty>(g.next_below(100))) == AddResult::ok && live.size() < 2'000'000)
                live.push_back(next);
            ++next;
        } else {
            auto k = static_cast<std::size_t>(g.next_below(live.size()));
            if (r < 85) {
                book.cancel(live[k]);
                live[k] = live.back();
                live.pop_back();
            } else {
                book.reduce(live[k], static_cast<Qty>(g.next_below(50)));
            }
        }
    }
    auto t1 = ll::bench::now_cycles();
    double secs = static_cast<double>(t1 - t0) / ll::bench::cycles_per_ns() / 1e9;
    std::printf("ops=%zu fills=%llu resting=%zu  %.2f M ops/s (includes workload generation)\n", n,
                static_cast<unsigned long long>(l.fills), book.order_count(), static_cast<double>(n) / secs / 1e6);
}
