// Deterministic 64-bit generator used by exercises and benches (no <random> distributions,
// so results are identical across compilers and standard libraries).
#pragma once

#include <cstdint>

namespace ll {

struct SplitMix64 {
    std::uint64_t state;
    std::uint64_t next() noexcept {
        std::uint64_t z = (state += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }
    // In [0, n). The modulo bias is negligible for the sizes used here.
    std::uint64_t next_below(std::uint64_t n) noexcept { return next() % n; }
};

}  // namespace ll
