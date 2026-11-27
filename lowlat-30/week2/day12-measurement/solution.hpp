// Day 12 exercise: a fixed-memory log-linear latency histogram (HdrHistogram-style).
#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace ll {

// B = sub-bucket bits. Values < 2^B are exact; above that, relative error <= 2^(1-B)
// (the top B bits of a value select its bucket, and the leading one of those is always set).
template <unsigned B = 6>
class LatencyHistogram {
    static_assert(B >= 1 && B <= 10);

public:
    void record(std::uint64_t v) noexcept;
    void record_corrected(std::uint64_t v, std::uint64_t expected_interval) noexcept;

    std::uint64_t count() const noexcept;
    std::uint64_t min() const noexcept;  // 0 if empty
    std::uint64_t max() const noexcept;  // 0 if empty

    // See README for the exact definition. 0 if empty.
    std::uint64_t percentile(double p) const noexcept;

    void merge(const LatencyHistogram& other) noexcept;
    void reset() noexcept;

private:
    // Suggested: std::array<std::uint64_t, (64 - B + 1) << B> counts_{}; plus count_, min_, max_.
    // index_of(v): v itself if std::bit_width(v) <= B; otherwise shift = bit_width(v) - B and
    // index = shift * 2^B + (v >> shift). Then write highest_value_in_bucket(index).
};

}  // namespace ll
