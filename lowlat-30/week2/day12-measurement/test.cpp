#include <algorithm>
#include <cmath>
#include <vector>

#include "check.hpp"
#include "rng.hpp"
#include "solution.hpp"

using namespace ll;

namespace {

std::uint64_t exact_percentile(std::vector<std::uint64_t> v, double p) {
    std::sort(v.begin(), v.end());
    auto rank = static_cast<std::size_t>(std::ceil(p * static_cast<double>(v.size())));
    rank = std::max<std::size_t>(rank, 1);
    return v[rank - 1];
}

}  // namespace

TEST(empty) {
    LatencyHistogram<5> h;
    CHECK_EQ(h.count(), 0u);
    CHECK_EQ(h.min(), 0u);
    CHECK_EQ(h.max(), 0u);
    CHECK_EQ(h.percentile(0.5), 0u);
}

TEST(small_values_are_exact) {
    LatencyHistogram<5> h;
    std::vector<std::uint64_t> v;
    for (std::uint64_t x = 0; x < 32; ++x)
        for (std::uint64_t k = 0; k <= x % 3; ++k) {
            h.record(x);
            v.push_back(x);
        }
    for (double p : {0.0, 0.01, 0.1, 0.25, 0.5, 0.9, 0.99, 1.0}) CHECK_EQ(h.percentile(p), exact_percentile(v, p));
    CHECK_EQ(h.min(), 0u);
    CHECK_EQ(h.max(), 31u);
}

TEST(relative_error_bound) {
    LatencyHistogram<6> h;  // relative error <= 2^(1-6) = 1/32
    std::vector<std::uint64_t> v;
    SplitMix64 g{3};
    for (int i = 0; i < 100'000; ++i) {
        // log-uniform-ish over 1 ns .. ~1 s
        std::uint64_t x = (g.next() >> (34 + g.next_below(30))) + 1;
        h.record(x);
        v.push_back(x);
    }
    CHECK_EQ(h.count(), 100'000u);
    CHECK_EQ(h.max(), *std::max_element(v.begin(), v.end()));
    CHECK_EQ(h.min(), *std::min_element(v.begin(), v.end()));
    for (double p : {0.0, 0.1, 0.5, 0.9, 0.99, 0.999, 0.9999, 1.0}) {
        auto want = exact_percentile(v, p);
        auto got = h.percentile(p);
        CHECK(got >= want);                // upper edge of the bucket, never below
        CHECK(got - want <= want / 32 + 1);
    }
    CHECK_EQ(h.percentile(1.0), h.max());
}

TEST(huge_values) {
    LatencyHistogram<5> h;
    h.record(~std::uint64_t{0});
    h.record(1ull << 63);
    CHECK_EQ(h.max(), ~std::uint64_t{0});
    CHECK_EQ(h.percentile(1.0), ~std::uint64_t{0});
    CHECK(h.percentile(0.5) >= (1ull << 63));
}

TEST(merge_and_reset) {
    LatencyHistogram<5> a, b;
    for (std::uint64_t i = 1; i <= 100; ++i) a.record(i);
    for (std::uint64_t i = 1000; i <= 1100; ++i) b.record(i);
    a.merge(b);
    CHECK_EQ(a.count(), 201u);
    CHECK_EQ(a.min(), 1u);
    CHECK_EQ(a.max(), 1100u);
    a.reset();
    CHECK_EQ(a.count(), 0u);
    CHECK_EQ(a.max(), 0u);
    a.record(7);
    CHECK_EQ(a.min(), 7u);
}

TEST(coordinated_omission_correction) {
    LatencyHistogram<5> h;
    // 99 fast samples at 10, then one stall of 1000 with requests intended every 100.
    for (int i = 0; i < 99; ++i) h.record_corrected(10, 100);
    h.record_corrected(1000, 100);
    // Back-filled: 1000, 900, ..., 100 -> 10 samples
    CHECK_EQ(h.count(), 109u);
    CHECK(h.percentile(0.95) >= 500);  // the tail is now visible
    LatencyHistogram<5> raw;
    for (int i = 0; i < 99; ++i) raw.record(10);
    raw.record(1000);
    CHECK_EQ(raw.percentile(0.95), 10u);  // without correction p95 hides the stall
    LatencyHistogram<5> none;
    none.record_corrected(1000, 0);
    CHECK_EQ(none.count(), 1u);
}

TEST(other_precision) {
    LatencyHistogram<3> h;  // relative error <= 2^(1-3) = 25%
    for (std::uint64_t i = 1; i <= 10'000; ++i) h.record(i);
    auto p50 = h.percentile(0.5);
    CHECK(p50 >= 5000 && p50 <= 5000 + 5000 / 4 + 1);
}

int main() { return run_all_tests(); }
