// Compares dispatch styles on the same work. Run on an idle machine, several times.
// Questions to answer from the output:
//  1. Why is the mixed virtual case slower than the single-type virtual case?
//  2. Why can the CRTP loop be faster even when the virtual target is perfectly predicted?
#include <memory>
#include <vector>

#include "bench.hpp"
#include "solution.hpp"

using namespace ll;
using ll::bench::now_cycles;

namespace {

std::vector<Tick> make_ticks(std::size_t n) {
    std::vector<Tick> v;
    std::uint64_t x = 88172645463325252ull;
    std::int64_t mid = 1'000'000;
    for (std::size_t i = 0; i < n; ++i) {
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        mid += static_cast<std::int64_t>(x % 21) - 10;
        v.push_back({mid - 5, mid + 5, static_cast<std::int64_t>(x % 500), static_cast<std::int64_t>((x >> 20) % 500)});
    }
    return v;
}

template <class F>
void time_it(const char* label, std::size_t ticks_per_run, F&& f) {
    std::vector<std::uint64_t> per_tick;
    for (int rep = 0; rep < 200; ++rep) {
        auto t0 = now_cycles();
        auto r = f();
        auto t1 = now_cycles();
        ll::bench::do_not_optimize(r);
        if (rep >= 20) per_tick.push_back((t1 - t0) * 1000 / ticks_per_run);  // milli-cycles per tick
    }
    std::sort(per_tick.begin(), per_tick.end());
    std::printf("%-34s median %.2f cycles/tick\n", label, static_cast<double>(per_tick[per_tick.size() / 2]) / 1000.0);
}

}  // namespace

int main() {
    ll::bench::pin_to_cpu(1);
    constexpr std::size_t N = 100'000;
    auto ticks = make_ticks(N);

    VMidMomentum vm;
    IStrategy& vref = vm;
    time_it("virtual, one type", N, [&] { return run_virtual(vref, ticks); });

    CMidMomentum cm;
    time_it("crtp", N, [&] { return run_crtp(cm, ticks); });

    AnyStrategy am = CMidMomentum{};
    time_it("variant", N, [&] { return run_variant(am, ticks); });

    // Unpredictable targets: alternate strategy types pseudo-randomly per tick.
    std::vector<std::unique_ptr<IStrategy>> objs;
    objs.push_back(std::make_unique<VMidMomentum>());
    objs.push_back(std::make_unique<VImbalance>());
    std::vector<IStrategy*> pick(N);
    std::uint64_t x = 12345;
    for (auto& p : pick) {
        x = x * 6364136223846793005ull + 1442695040888963407ull;
        p = objs[(x >> 33) & 1].get();
    }
    time_it("virtual, mixed targets", N, [&] {
        std::int64_t s = 0;
        for (std::size_t i = 0; i < N; ++i) s += pick[i]->on_tick(ticks[i]);
        return s;
    });

    std::printf("\nRecord CPU model (lscpu), compiler and flags next to these numbers.\n");
}
