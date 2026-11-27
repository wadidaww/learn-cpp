#include <algorithm>

#include "check.hpp"
#include "solution.hpp"

using namespace ll;

namespace {
std::uint64_t rng_state = 0x1234567;
std::uint64_t rnd() {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}
}  // namespace

static_assert(sizeof(OrderAoS) == 64);

TEST(count_below_matches) {
    std::vector<std::int32_t> d(5000);
    for (auto& x : d) x = static_cast<std::int32_t>(rnd() % 256) - 50;
    for (std::int32_t t : {-100, -50, 0, 77, 128, 205, 300}) {
        auto want = static_cast<std::size_t>(std::count_if(d.begin(), d.end(), [&](auto x) { return x < t; }));
        CHECK_EQ(count_below_branchy(d, t), want);
        CHECK_EQ(count_below_branchless(d, t), want);
    }
    CHECK_EQ(count_below_branchless(std::span<const std::int32_t>{}, 5), 0u);
}

TEST(lower_bound_matches_std_on_edges) {
    std::vector<std::int64_t> empty;
    CHECK_EQ(lower_bound_branchless(empty, 5), 0u);
    std::vector<std::int64_t> one{10};
    CHECK_EQ(lower_bound_branchless(one, 5), 0u);
    CHECK_EQ(lower_bound_branchless(one, 10), 0u);
    CHECK_EQ(lower_bound_branchless(one, 11), 1u);
    std::vector<std::int64_t> dup{1, 3, 3, 3, 7};
    CHECK_EQ(lower_bound_branchless(dup, 3), 1u);
    CHECK_EQ(lower_bound_branchless(dup, 4), 4u);
    CHECK_EQ(lower_bound_branchless(dup, 8), 5u);
}

TEST(lower_bound_matches_std_random) {
    for (std::size_t n : {2u, 3u, 7u, 8u, 9u, 100u, 1023u, 1024u, 1025u, 50000u}) {
        std::vector<std::int64_t> v(n);
        for (auto& x : v) x = static_cast<std::int64_t>(rnd() % (n * 2));
        std::sort(v.begin(), v.end());
        for (int k = 0; k < 500; ++k) {
            auto key = static_cast<std::int64_t>(rnd() % (n * 2 + 2)) - 1;
            auto want = static_cast<std::size_t>(std::lower_bound(v.begin(), v.end(), key) - v.begin());
            CHECK_EQ(lower_bound_branchless(v, key), want);
        }
    }
}

TEST(aos_and_soa_agree) {
    std::vector<OrderAoS> aos(3000);
    OrdersSoA soa;
    for (std::size_t i = 0; i < aos.size(); ++i) {
        aos[i] = {};
        aos[i].price = static_cast<std::int64_t>(rnd() % 1000);
        aos[i].qty = static_cast<std::int64_t>(rnd() % 100);
        aos[i].id = i;
        soa.price.push_back(aos[i].price);
        soa.qty.push_back(aos[i].qty);
        soa.id.push_back(i);
    }
    for (std::int64_t px : {0, 1, 500, 999, 1000}) {
        std::int64_t want = 0;
        for (auto& o : aos)
            if (o.price >= px) want += o.qty;
        CHECK_EQ(sum_qty_at_or_above_aos(aos, px), want);
        CHECK_EQ(sum_qty_at_or_above_soa(soa, px), want);
    }
}

int main() { return run_all_tests(); }
