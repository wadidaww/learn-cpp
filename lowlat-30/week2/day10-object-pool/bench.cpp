// Per-operation latency distribution: pool vs new/delete under the same random churn.
// Look at p99.9 and max, not just p50.
#include <memory>
#include <vector>

#include "bench.hpp"
#include "rng.hpp"
#include "solution.hpp"

using ll::bench::now_cycles;

namespace {

struct Order {
    std::uint64_t id;
    std::int64_t price;
    std::int64_t qty;
    std::uint64_t ts;
    std::uint32_t next, prev;  // like an intrusive list node in a book
    Order(std::uint64_t i, std::int64_t p, std::int64_t q) : id(i), price(p), qty(q), ts(0), next(0), prev(0) {}
};

constexpr std::size_t kLive = 100'000;
constexpr int kOps = 2'000'000;

template <class Alloc, class Free>
void run(const char* label, Alloc&& alloc, Free&& free_fn) {
    ll::SplitMix64 g{1};
    std::vector<Order*> live;
    live.reserve(kLive);
    for (std::size_t i = 0; i < kLive / 2; ++i) live.push_back(alloc(i));
    std::vector<std::uint64_t> samples;
    samples.reserve(kOps);
    for (int k = 0; k < kOps; ++k) {
        bool do_free = !live.empty() && (live.size() >= kLive - 1 || (g.next() & 1));
        auto idx = live.empty() ? 0 : static_cast<std::size_t>(g.next_below(live.size()));
        auto t0 = now_cycles();
        if (do_free) {
            free_fn(live[idx]);
        } else {
            live.push_back(alloc(static_cast<std::uint64_t>(k)));
        }
        auto t1 = now_cycles();
        if (do_free) {
            live[idx] = live.back();
            live.pop_back();
        }
        samples.push_back(t1 - t0);
    }
    for (auto* o : live) free_fn(o);
    ll::bench::report(label, std::move(samples));
}

}  // namespace

int main() {
    ll::bench::pin_to_cpu(0);
    static ll::ObjectPool<Order, kLive> pool;
    run("ObjectPool", [](std::uint64_t i) { return pool.create(i, 100, 1); }, [](Order* o) { pool.destroy(o); });
    run("new/delete", [](std::uint64_t i) { return new Order(i, 100, 1); }, [](Order* o) { delete o; });
    std::printf("(timer overhead is included in both; subtract an empty measurement to see it)\n");
}
