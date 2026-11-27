#include <memory>
#include <thread>

#include "check.hpp"
#include "solution.hpp"

using namespace ll;

TEST(empty_then_full) {
    SpscRing<int, 4> r;
    int v = 0;
    CHECK(r.empty_approx());
    CHECK(!r.try_pop(v));
    for (int i = 0; i < 4; ++i) CHECK(r.try_push(i));
    CHECK(!r.try_push(99));  // uses all N slots, then refuses
    CHECK_EQ(r.size_approx(), 4u);
    for (int i = 0; i < 4; ++i) {
        CHECK(r.try_pop(v));
        CHECK_EQ(v, i);
    }
    CHECK(!r.try_pop(v));
}

TEST(wraps_around_many_laps) {
    SpscRing<std::uint64_t, 8> r;
    std::uint64_t next_in = 0, next_out = 0;
    for (int round = 0; round < 1000; ++round) {
        int k = round % 8 + 1;
        for (int i = 0; i < k; ++i) CHECK(r.try_push(next_in++));
        for (int i = 0; i < k; ++i) {
            std::uint64_t v = 0;
            CHECK(r.try_pop(v));
            CHECK_EQ(v, next_out++);
        }
    }
    CHECK(r.empty_approx());
}

TEST(move_only_payload) {
    SpscRing<std::unique_ptr<int>, 2> r;
    CHECK(r.try_push(std::make_unique<int>(5)));
    std::unique_ptr<int> out;
    CHECK(r.try_pop(out));
    CHECK(out && *out == 5);
}

TEST(indices_do_not_share_a_line_with_each_other) {
    CHECK(alignof(SpscRing<int, 8>) >= 64);
    CHECK(sizeof(SpscRing<int, 8>) >= 3 * 64);  // producer line + consumer line + buffer
}

TEST(two_threads_fifo_no_loss) {
    static SpscRing<std::uint64_t, 1024> r;  // static: large and aligned
    constexpr std::uint64_t N = 2'000'000;
    std::thread prod([] {
        for (std::uint64_t i = 0; i < N; ++i)
            while (!r.try_push(i)) {
            }
    });
    bool ok = true;
    std::uint64_t expect = 0;
    while (expect < N) {
        std::uint64_t v;
        if (r.try_pop(v)) {
            ok = ok && v == expect;
            ++expect;
        }
    }
    prod.join();
    CHECK(ok);
    CHECK(r.empty_approx());
}

struct Wide {
    std::uint64_t seq;
    std::uint64_t pad[7];  // a 64-byte payload: a torn read would show a mismatch
};

TEST(two_threads_wide_payload_not_torn) {
    static SpscRing<Wide, 64> r;
    constexpr std::uint64_t N = 300'000;
    std::thread prod([] {
        for (std::uint64_t i = 0; i < N; ++i) {
            Wide w{i, {}};
            for (auto& p : w.pad) p = i * 3 + 1;
            while (!r.try_push(w)) {
            }
        }
    });
    bool ok = true;
    for (std::uint64_t i = 0; i < N;) {
        Wide w;
        if (r.try_pop(w)) {
            ok = ok && w.seq == i;
            for (auto p : w.pad) ok = ok && p == i * 3 + 1;
            ++i;
        }
    }
    prod.join();
    CHECK(ok);
}

int main() { return run_all_tests(); }
