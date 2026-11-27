// Prints ns per dependent load (pointer chase) and ns per element (sequential, strided) as the
// working set grows. Paste the output into a spreadsheet and plot it on a log x-axis.
#include <cstdio>

#include "bench.hpp"
#include "solution.hpp"

int main() {
    ll::bench::pin_to_cpu(0);
    std::printf("%10s %12s %12s %14s\n", "bytes", "chase ns", "seq ns/elem", "stride16 ns/el");
    for (std::size_t bytes = 4 * 1024; bytes <= (std::size_t{256} << 20); bytes *= 2) {
        std::size_t n32 = bytes / sizeof(std::uint32_t);
        auto next = ll::make_cycle(n32, 12345);
        std::size_t steps = 20'000'000;
        ll::chase(next, n32);  // warm up
        auto t0 = ll::bench::now_cycles();
        auto end = ll::chase(next, steps);
        auto t1 = ll::bench::now_cycles();
        ll::bench::do_not_optimize(end);
        double chase_ns = static_cast<double>(t1 - t0) / ll::bench::cycles_per_ns() / static_cast<double>(steps);

        std::size_t n64 = bytes / sizeof(std::uint64_t);
        std::vector<std::uint64_t> data(n64, 1);
        int reps = static_cast<int>(std::max<std::size_t>(1, (std::size_t{64} << 20) / bytes));
        auto s0 = ll::bench::now_cycles();
        std::uint64_t acc = 0;
        for (int r = 0; r < reps; ++r) acc += ll::sum_sequential(data);
        auto s1 = ll::bench::now_cycles();
        for (int r = 0; r < reps; ++r) acc += ll::sum_strided(data, 16);  // 16 * 8 B = 128 B: a new line every load
        auto s2 = ll::bench::now_cycles();
        ll::bench::do_not_optimize(acc);
        double denom = static_cast<double>(n64) * reps * ll::bench::cycles_per_ns();
        std::printf("%10zu %12.2f %12.3f %14.3f\n", bytes, chase_ns, static_cast<double>(s1 - s0) / denom,
                    static_cast<double>(s2 - s1) / denom);
    }
}
