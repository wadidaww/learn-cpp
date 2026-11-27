#include <numeric>

#include "check.hpp"
#include "solution.hpp"

using namespace ll;

TEST(cycle_visits_everything_once) {
    for (std::size_t n : {2u, 3u, 10u, 1000u, 65536u}) {
        auto next = make_cycle(n, 42);
        CHECK_EQ(next.size(), n);
        std::vector<bool> seen(n, false);
        std::uint32_t i = 0;
        for (std::size_t k = 0; k < n; ++k) {
            CHECK(!seen[i]);
            seen[i] = true;
            i = next[i];
        }
        CHECK_EQ(i, 0u);  // back at the start after exactly n steps
    }
}

TEST(cycle_is_a_permutation_with_no_fixed_points) {
    auto next = make_cycle(5000, 7);
    std::vector<int> hits(next.size(), 0);
    for (std::size_t k = 0; k < next.size(); ++k) {
        CHECK(next[k] != k);
        ++hits[next[k]];
    }
    for (int h : hits) CHECK_EQ(h, 1);
}

TEST(cycle_is_deterministic_and_seed_dependent) {
    CHECK(make_cycle(100, 1) == make_cycle(100, 1));
    CHECK(make_cycle(100, 1) != make_cycle(100, 2));
}

TEST(chase_follows_chain) {
    std::vector<std::uint32_t> next{2, 0, 3, 1};  // 0->2->3->1->0
    CHECK_EQ(chase(next, 0), 0u);
    CHECK_EQ(chase(next, 1), 2u);
    CHECK_EQ(chase(next, 3), 1u);
    CHECK_EQ(chase(next, 4), 0u);
    CHECK_EQ(chase(next, 4001), 2u);
}

TEST(sums_agree) {
    std::vector<std::uint64_t> d(10'007);
    std::iota(d.begin(), d.end(), 1);
    auto want = std::accumulate(d.begin(), d.end(), std::uint64_t{0});
    CHECK_EQ(sum_sequential(d), want);
    for (std::size_t s : {1u, 2u, 7u, 8u, 64u, 10'007u, 20'000u}) CHECK_EQ(sum_strided(d, s), want);
}

int main() { return run_all_tests(); }
