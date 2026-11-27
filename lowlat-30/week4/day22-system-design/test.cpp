#include <cstdint>
#include <cstdlib>
#include <limits>
#include <new>

#include "check.hpp"
#include "solution.hpp"

using namespace ll::risk;

static long g_allocs = 0;
void* operator new(std::size_t n) {
    ++g_allocs;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

namespace {
constexpr std::int64_t kPx = 1'000'000;  // 100.0000
Limits base() { return Limits{1000, 50'000'000'000, 2000, 100, 5, 1'000'000}; }  // 1% collar, 5 per ms
RiskGate ready(Limits l = base()) {
    RiskGate g(l);
    g.set_reference(kPx);
    return g;
}
}  // namespace

TEST(accepts_a_normal_order_and_reserves_it) {
    auto g = ready();
    CHECK(g.check({0, 'B', 100, kPx}) == Verdict::accept);
    CHECK_EQ(g.open_buys(), 100);
    CHECK_EQ(g.open_sells(), 0);
    CHECK_EQ(g.position(), 0);
}

TEST(rejects_malformed_orders) {
    auto g = ready();
    CHECK(g.check({0, 'X', 1, kPx}) == Verdict::bad_order);
    CHECK(g.check({0, 'B', 0, kPx}) == Verdict::bad_order);
    CHECK(g.check({0, 'S', -5, kPx}) == Verdict::bad_order);
    CHECK(g.check({0, 'B', 1, 0}) == Verdict::bad_order);
    CHECK(g.check({0, 'B', 1001, kPx}) == Verdict::too_large);
    CHECK(g.check({0, 'B', 1000, kPx}) == Verdict::accept);
}

TEST(needs_a_reference_and_applies_the_collar) {
    RiskGate g(base());
    CHECK(g.check({0, 'B', 1, kPx}) == Verdict::no_reference);
    g.set_reference(0);
    CHECK(g.check({0, 'B', 1, kPx}) == Verdict::no_reference);
    g.set_reference(kPx);
    CHECK(g.check({0, 'B', 1, kPx + 10'000}) == Verdict::accept);  // exactly +1%
    CHECK(g.check({0, 'B', 1, kPx + 10'001}) == Verdict::collar);
    CHECK(g.check({0, 'S', 1, kPx - 10'000}) == Verdict::accept);
    CHECK(g.check({0, 'S', 1, kPx - 10'001}) == Verdict::collar);
}

TEST(notional_is_exact_and_overflow_safe) {
    auto l = base();
    l.max_notional = 1000 * kPx;  // exactly 1000 shares at 100
    auto g = ready(l);
    CHECK(g.check({0, 'B', 1000, kPx}) == Verdict::accept);
    CHECK(g.check({0, 'S', 1000, kPx + 1}) == Verdict::notional);

    // A qty that passes the earlier checks but makes price * qty overflow int64.
    constexpr auto max = std::numeric_limits<std::int64_t>::max();
    Limits h{max / 4, max, max / 4, 100, 5, 1'000'000};
    auto g2 = ready(h);
    CHECK(g2.check({0, 'B', max / 4, kPx}) == Verdict::notional);
    CHECK(g2.check({0, 'B', max / kPx, kPx}) == Verdict::accept);  // largest qty whose notional fits
    CHECK(g2.check({0, 'S', max / kPx + 1, kPx}) == Verdict::notional);
}

TEST(position_counts_worst_case_open_orders) {
    auto g = ready();
    CHECK(g.check({0, 'B', 1000, kPx}) == Verdict::accept);
    CHECK(g.check({1, 'B', 1000, kPx}) == Verdict::accept);
    CHECK(g.check({2, 'B', 1, kPx}) == Verdict::position);   // 2001 if all buys fill
    CHECK(g.check({3, 'S', 1000, kPx}) == Verdict::accept);  // sells are checked on their own side
    CHECK(g.check({4, 'S', 1000, kPx}) == Verdict::accept);
    CHECK(g.check({5, 'S', 1, kPx}) == Verdict::position);
    g.on_fill('B', 600);
    CHECK_EQ(g.position(), 600);
    CHECK_EQ(g.open_buys(), 1400);
    g.on_done('B', 400);  // first buy cancelled with 400 left
    CHECK_EQ(g.open_buys(), 1000);
    // worst case long: 600 + 1000 open + 400 = 2000: ok
    CHECK(g.check({6, 'B', 400, kPx}) == Verdict::accept);
    // worst case short: 600 - 2000 open sells - 600 = -2000: ok; one more is not
    CHECK(g.check({7, 'S', 600, kPx}) == Verdict::rate);  // 5 accepted within 1 ms
    CHECK(g.check({1'000'003, 'S', 600, kPx}) == Verdict::accept);
    CHECK(g.check({1'000'004, 'S', 1, kPx}) == Verdict::position);
}

TEST(rate_limit_is_a_sliding_window_of_accepts) {
    auto g = ready();
    for (std::uint64_t t = 0; t < 5; ++t) CHECK(g.check({t * 100, 'B', 1, kPx}) == Verdict::accept);
    CHECK(g.check({999'999, 'B', 1, kPx}) == Verdict::rate);  // window still holds t=0
    CHECK(g.check({999'999, 'B', 0, kPx}) == Verdict::bad_order);  // rejected orders do not count ...
    CHECK(g.check({1'000'000, 'B', 1, kPx}) == Verdict::accept);  // t=0 has left the window (ts - w, ts]
    CHECK(g.check({1'000'050, 'B', 1, kPx}) == Verdict::rate);    // t=100 is still inside
    CHECK(g.check({1'000'100, 'B', 1, kPx}) == Verdict::accept);
    for (std::uint64_t t = 0; t < 5; ++t) CHECK(g.check({5'000'000 + t, 'S', 1, kPx}) == Verdict::accept);
    CHECK(g.check({5'000'005, 'S', 1, kPx}) == Verdict::rate);
}

TEST(kill_switch_wins) {
    auto g = ready();
    g.kill();
    CHECK(g.killed());
    CHECK(g.check({0, 'X', 0, 0}) == Verdict::killed);
    CHECK(g.check({0, 'B', 1, kPx}) == Verdict::killed);
    g.set_reference(kPx);
    CHECK(g.killed());
}

TEST(check_never_allocates) {
    auto g = ready();
    long before = g_allocs;
    std::uint64_t t = 0;
    for (int i = 0; i < 100'000; ++i) {
        t += 300'000;
        auto v = g.check({t, i % 2 ? 'B' : 'S', 1 + i % 7, kPx + (i % 21) * 1000 - 10'000});
        if (v == Verdict::accept) g.on_fill(i % 2 ? 'B' : 'S', 1 + i % 7);
        g.on_done(i % 2 ? 'B' : 'S', 0);
    }
    CHECK_EQ(g_allocs - before, 0);
}

int main() { return run_all_tests(); }
