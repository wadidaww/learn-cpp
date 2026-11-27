#include "check.hpp"
#include "solution.hpp"

using namespace ll::mm;

namespace {
QuoteParams P(std::int64_t hs, std::int64_t skew = 0, std::int64_t maxp = 100, std::int64_t tick = 1) {
    return {hs, skew, maxp, tick};
}
bool eq(const Quote& q, std::optional<std::int64_t> b, std::optional<std::int64_t> a) { return q.bid == b && q.ask == a; }
}  // namespace

TEST(flat_position_quotes_symmetric) {
    CHECK(eq(make_quote(1000, 0, P(5)), 995, 1005));
    CHECK(eq(make_quote(1000, 0, P(0)), 1000, 1001));  // zero spread still leaves a one-tick market
}

TEST(rounds_away_from_the_centre) {
    auto p = P(7, 0, 100, 5);
    CHECK(eq(make_quote(1000, 0, p), 990, 1010));  // 993 -> 990, 1007 -> 1010
    CHECK(eq(make_quote(1000, 0, P(10, 0, 100, 5)), 990, 1010));  // already on ticks
    CHECK(eq(make_quote(1002, 0, P(0, 0, 100, 5)), 1000, 1005));
    CHECK(eq(make_quote(1000, 0, P(0, 0, 100, 5)), 1000, 1005));  // bid == ask after rounding
}

TEST(long_inventory_skews_down_short_skews_up) {
    auto p = P(5, 2);
    CHECK(eq(make_quote(1000, 10, p), 975, 985));    // centre 980
    CHECK(eq(make_quote(1000, -10, p), 1015, 1025)); // centre 1020
}

TEST(limits_remove_a_side) {
    auto p = P(5, 1, 50);
    CHECK(eq(make_quote(1000, 50, p), std::nullopt, 955));
    CHECK(eq(make_quote(1000, 49, p), 946, 956));
    CHECK(eq(make_quote(1000, -50, p), 1045, std::nullopt));
    CHECK(eq(make_quote(1000, 70, p), std::nullopt, 935));
}

TEST(no_bid_at_or_below_zero_and_negative_rounding) {
    auto p = P(5, 10, 1000, 3);
    // centre = 30 - 4*10 = -10; bid -15 -> no bid; ask -5 rounded up to a multiple of 3 is -3
    CHECK(eq(make_quote(30, 4, p), std::nullopt, -3));
    // centre = 10 - 1*10 = 0; bid -5 -> no bid; ask 5 -> 6
    CHECK(eq(make_quote(10, 1, p), std::nullopt, 6));
    // centre 4: bid -1 -> no bid (would be -3); ask 9
    CHECK(eq(make_quote(4, 0, p), std::nullopt, 9));
    CHECK(eq(make_quote(9, 0, p), 3, 15));  // bid 4 -> 3, ask 14 -> 15
}

int main() { return run_all_tests(); }
