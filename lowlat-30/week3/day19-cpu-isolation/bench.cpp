// Jitter probe: spin on one core reading the TSC and record every gap between consecutive reads
// that is longer than 1 us. Each gap is a moment the core was taken away (interrupt, tick, SMI, preemption).
// Usage: day19_bench [cpu=0] [seconds=5]
#include <cstdlib>
#include <vector>

#include "bench.hpp"

int main(int argc, char** argv) {
    int cpu = argc > 1 ? std::atoi(argv[1]) : 0;
    int secs = argc > 2 ? std::atoi(argv[2]) : 5;
    if (!ll::bench::pin_to_cpu(cpu)) std::printf("warning: could not pin to cpu %d\n", cpu);
    double cpn = ll::bench::cycles_per_ns();
    auto thresh = static_cast<std::uint64_t>(1000.0 * cpn);  // 1 us in cycles
    auto end_cycles = static_cast<std::uint64_t>(static_cast<double>(secs) * 1e9 * cpn);

    std::vector<std::uint64_t> gaps;
    gaps.reserve(1 << 20);
    std::uint64_t start = ll::bench::now_cycles(), prev = start, lost = 0;
    for (;;) {
        std::uint64_t now = ll::bench::now_cycles();
        std::uint64_t d = now - prev;
        if (d > thresh) {
            if (gaps.size() < gaps.capacity()) gaps.push_back(d);
            lost += d;
        }
        prev = now;
        if (now - start > end_cycles) break;
    }
    std::uint64_t total = prev - start;
    std::size_t over10 = 0, over100 = 0;
    std::uint64_t worst = 0;
    for (auto g : gaps) {
        over10 += g > 10 * thresh;
        over100 += g > 100 * thresh;
        worst = std::max(worst, g);
    }
    std::printf("cpu %d, %d s: interruptions >1us: %zu  >10us: %zu  >100us: %zu  longest: %.1f us\n", cpu, secs,
                gaps.size(), over10, over100, static_cast<double>(worst) / cpn / 1000.0);
    std::printf("time stolen: %.4f%% of the run\n", 100.0 * static_cast<double>(lost) / static_cast<double>(total));
    if (!gaps.empty()) ll::bench::report("gap size", std::move(gaps));
}
