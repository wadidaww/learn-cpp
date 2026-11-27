#include <vector>

#include "check.hpp"
#include "solution.hpp"

using namespace ll;

namespace {

std::vector<Tick> make_ticks(std::size_t n) {
    std::vector<Tick> v;
    std::uint64_t x = 88172645463325252ull;  // xorshift, deterministic
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

}  // namespace

static_assert(Strategy<CMidMomentum>);
static_assert(Strategy<CImbalance>);
static_assert(Strategy<VMidMomentum>);
static_assert(!Strategy<int>);

TEST(mid_momentum_by_hand) {
    CMidMomentum s;
    CHECK_EQ(s.on_tick({100, 102, 1, 1}), 0);   // first tick
    CHECK_EQ(s.on_tick({101, 103, 1, 1}), 2);   // 204 - 202
    CHECK_EQ(s.on_tick({99, 101, 1, 1}), -4);   // 200 - 204
    VMidMomentum v;
    CHECK_EQ(v.on_tick({100, 102, 1, 1}), 0);
    CHECK_EQ(v.on_tick({101, 103, 1, 1}), 2);
}

TEST(imbalance_by_hand) {
    CImbalance s;
    CHECK_EQ(s.on_tick({1, 2, 300, 100}), 500);   // 200*1000/400
    CHECK_EQ(s.on_tick({1, 2, 100, 300}), -500);
    CHECK_EQ(s.on_tick({1, 2, 0, 0}), 0);
    CHECK_EQ(s.on_tick({1, 2, 1, 2}), -333);       // integer division truncates toward zero
    VImbalance v;
    CHECK_EQ(v.on_tick({1, 2, 300, 100}), 500);
}

TEST(three_forms_agree) {
    auto ticks = make_ticks(10'000);
    VMidMomentum vm;
    CMidMomentum cm;
    AnyStrategy am = CMidMomentum{};
    auto a = run_virtual(vm, ticks);
    auto b = run_crtp(cm, ticks);
    auto c = run_variant(am, ticks);
    CHECK_EQ(a, b);
    CHECK_EQ(b, c);

    VImbalance vi;
    CImbalance ci;
    AnyStrategy ai = CImbalance{};
    auto d = run_virtual(vi, ticks);
    CHECK_EQ(d, run_crtp(ci, ticks));
    CHECK_EQ(d, run_variant(ai, ticks));
}

TEST(momentum_sum_telescopes) {
    // Sum of (m_i - m_{i-1}) == m_last - m_first.
    auto ticks = make_ticks(500);
    CMidMomentum s;
    auto sum = run_crtp(s, ticks);
    CHECK_EQ(sum, (ticks.back().bid + ticks.back().ask) - (ticks.front().bid + ticks.front().ask));
}

TEST(crtp_has_no_vptr) {
    CHECK(!std::is_polymorphic_v<CMidMomentum>);
    CHECK(std::is_polymorphic_v<VMidMomentum>);
    CHECK(sizeof(CImbalance) < sizeof(VImbalance));
}

int main() { return run_all_tests(); }
