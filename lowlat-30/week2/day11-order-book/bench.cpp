// Throughput and per-operation latency of YOUR order book on a synthetic workload:
// 50% adds clustered near the mid (a quarter of them cross), 35% cancels, 15% reduces.
// This is the number that replaces the resume bullet on day 14, together with the machine details.
#include <vector>

#include "bench.hpp"
#include "rng.hpp"
#include "solution.hpp"

using namespace ll;
using ll::bench::now_cycles;

namespace {

struct CountingListener {
    std::uint64_t fills = 0;
    Qty volume = 0;
    void on_fill(const Fill& f) {
        ++fills;
        volume += f.qty;
    }
};

struct Op {
    std::uint8_t kind;  // 0 add, 1 cancel, 2 reduce
    Side side;
    OrderId id;
    Price px;
    Qty qty;
};

// Pre-generate operations so the generator is not timed.
std::vector<Op> make_ops(std::size_t n) {
    SplitMix64 g{7};
    std::vector<Op> ops;
    ops.reserve(n);
    std::vector<OrderId> live;
    live.reserve(n);
    OrderId next = 1;
    for (std::size_t i = 0; i < n; ++i) {
        auto r = g.next_below(100);
        if (r < 50 || live.empty()) {
            Side s = g.next_below(2) ? Side::Buy : Side::Sell;
            Price off = static_cast<Price>(g.next_below(20));
            bool cross = g.next_below(4) == 0;
            Price px = s == Side::Buy ? (cross ? 10'001 + off / 4 : 10'000 - off) : (cross ? 9'999 - off / 4 : 10'001 + off);
            ops.push_back({0, s, next, px, 1 + static_cast<Qty>(g.next_below(100))});
            live.push_back(next++);
        } else {
            auto k = static_cast<std::size_t>(g.next_below(live.size()));
            if (r < 85) {
                ops.push_back({1, Side::Buy, live[k], 0, 0});
                live[k] = live.back();
                live.pop_back();
            } else {
                ops.push_back({2, Side::Buy, live[k], 0, static_cast<Qty>(g.next_below(50))});
            }
        }
    }
    return ops;
}

}  // namespace

int main() {
    ll::bench::pin_to_cpu(0);
    constexpr std::size_t N = 5'000'000;
    auto ops = make_ops(N);

    for (int run = 0; run < 2; ++run) {  // run 0 warms caches and the allocator; report run 1
        CountingListener l;
        OrderBook<CountingListener> book(9'000, 11'000, 1'000'000, l);
        std::vector<std::uint64_t> lat;
        lat.reserve(N);
        auto t0 = now_cycles();
        for (const auto& op : ops) {
            auto a = now_cycles();
            switch (op.kind) {
                case 0: book.add(op.id, op.side, op.px, op.qty); break;
                case 1: book.cancel(op.id); break;
                default: book.reduce(op.id, op.qty); break;
            }
            lat.push_back(now_cycles() - a);
        }
        auto t1 = now_cycles();
        if (run == 1) {
            double secs = static_cast<double>(t1 - t0) / ll::bench::cycles_per_ns() / 1e9;
            std::printf("ops=%zu fills=%llu resting=%zu  throughput %.2f M ops/s (timer inside the loop)\n", N,
                        static_cast<unsigned long long>(l.fills), book.order_count(), static_cast<double>(N) / secs / 1e6);
            ll::bench::report("per op (incl. ~timer cost)", std::move(lat));
        }
    }
}
