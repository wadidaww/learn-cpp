// Branchy vs branchless on random vs sorted data; std::lower_bound vs branchless; AoS vs SoA.
#include <algorithm>

#include "bench.hpp"
#include "rng.hpp"
#include "solution.hpp"

using ll::bench::now_cycles;

namespace {

template <class F>
double ns_per_item(std::size_t items, int reps, F&& f) {
    std::vector<std::uint64_t> t;
    for (int r = 0; r < reps; ++r) {
        auto a = now_cycles();
        auto v = f();
        auto b = now_cycles();
        ll::bench::do_not_optimize(v);
        t.push_back(b - a);
    }
    std::sort(t.begin(), t.end());
    return static_cast<double>(t[t.size() / 2]) / ll::bench::cycles_per_ns() / static_cast<double>(items);
}

}  // namespace

int main() {
    ll::bench::pin_to_cpu(0);
    ll::SplitMix64 g{99};
    constexpr std::size_t N = 1 << 20;

    std::vector<std::int32_t> rnd(N);
    for (auto& x : rnd) x = static_cast<std::int32_t>(g.next_below(256));
    auto sorted = rnd;
    std::sort(sorted.begin(), sorted.end());

    std::printf("count_below (ns per element)\n");
    std::printf("  branchy    random %.3f   sorted %.3f\n",
                ns_per_item(N, 30, [&] { return ll::count_below_branchy(rnd, 128); }),
                ns_per_item(N, 30, [&] { return ll::count_below_branchy(sorted, 128); }));
    std::printf("  branchless random %.3f   sorted %.3f\n",
                ns_per_item(N, 30, [&] { return ll::count_below_branchless(rnd, 128); }),
                ns_per_item(N, 30, [&] { return ll::count_below_branchless(sorted, 128); }));

    std::printf("\nlower_bound (ns per lookup)\n");
    for (std::size_t n : {std::size_t{1} << 10, std::size_t{1} << 16, std::size_t{1} << 22}) {
        std::vector<std::int64_t> v(n);
        for (auto& x : v) x = static_cast<std::int64_t>(g.next());
        std::sort(v.begin(), v.end());
        std::vector<std::int64_t> keys(4096);
        for (auto& k : keys) k = static_cast<std::int64_t>(g.next());
        double a = ns_per_item(keys.size(), 30, [&] {
            std::size_t s = 0;
            for (auto k : keys) s += static_cast<std::size_t>(std::lower_bound(v.begin(), v.end(), k) - v.begin());
            return s;
        });
        double b = ns_per_item(keys.size(), 30, [&] {
            std::size_t s = 0;
            for (auto k : keys) s += ll::lower_bound_branchless(v, k);
            return s;
        });
        std::printf("  n=%-8zu std %.1f   branchless %.1f\n", n, a, b);
    }

    std::printf("\nsum qty >= px over 1M orders (ns per order)\n");
    std::vector<ll::OrderAoS> aos(N);
    ll::OrdersSoA soa;
    for (std::size_t i = 0; i < N; ++i) {
        aos[i] = {};
        aos[i].price = static_cast<std::int64_t>(g.next_below(1000));
        aos[i].qty = static_cast<std::int64_t>(g.next_below(100));
        soa.price.push_back(aos[i].price);
        soa.qty.push_back(aos[i].qty);
        soa.id.push_back(i);
    }
    std::printf("  AoS %.3f   SoA %.3f\n", ns_per_item(N, 20, [&] { return ll::sum_qty_at_or_above_aos(aos, 500); }),
                ns_per_item(N, 20, [&] { return ll::sum_qty_at_or_above_soa(soa, 500); }));
}
