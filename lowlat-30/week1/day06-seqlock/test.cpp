#include <thread>
#include <vector>

#include "check.hpp"
#include "solution.hpp"

using namespace ll;

namespace {

// Every field is derived from seq_no, so any mix of two writes is detectable.
TopOfBook make(std::uint64_t i) {
    auto s = static_cast<std::int64_t>(i);
    return {1000 + s, 2 * s + 1, 1001 + s, 3 * s + 2, i};
}

bool consistent(const TopOfBook& t) {
    auto s = static_cast<std::int64_t>(t.seq_no);
    return t.bid_px == 1000 + s && t.bid_qty == 2 * s + 1 && t.ask_px == 1001 + s && t.ask_qty == 3 * s + 2;
}

struct Odd {
    char c[13];  // not a multiple of 8 bytes
};

}  // namespace

TEST(store_then_load) {
    Seqlock<TopOfBook> s;
    CHECK_EQ(s.version(), 0u);
    s.store(make(5));
    CHECK_EQ(s.version(), 1u);
    auto t = s.load();
    CHECK_EQ(t.seq_no, 5u);
    CHECK(consistent(t));
    TopOfBook u{};
    CHECK(s.try_load(u));
    CHECK_EQ(u.ask_px, 1006);
}

TEST(odd_sized_type) {
    Seqlock<Odd> s;
    Odd o{};
    std::memcpy(o.c, "hello seqlock", 13);
    s.store(o);
    Odd r = s.load();
    CHECK(std::memcmp(r.c, "hello seqlock", 13) == 0);
}

TEST(readers_never_see_torn_snapshots) {
    static Seqlock<TopOfBook> s;
    s.store(make(0));
    std::atomic<bool> stop{false};
    constexpr std::uint64_t kWrites = 300'000;
    std::thread writer([&] {
        for (std::uint64_t i = 1; i <= kWrites; ++i) s.store(make(i));
        stop.store(true, std::memory_order_release);
    });
    std::atomic<int> torn{0};
    std::atomic<int> backwards{0};
    std::vector<std::thread> readers;
    for (int r = 0; r < 2; ++r)
        readers.emplace_back([&] {
            std::uint64_t last = 0;
            while (!stop.load(std::memory_order_acquire)) {
                auto t = s.load();
                if (!consistent(t)) torn.fetch_add(1);
                if (t.seq_no < last) backwards.fetch_add(1);
                last = t.seq_no;
            }
        });
    writer.join();
    for (auto& t : readers) t.join();
    CHECK_EQ(torn.load(), 0);
    CHECK_EQ(backwards.load(), 0);
    CHECK_EQ(s.load().seq_no, kWrites);
    CHECK_EQ(s.version(), kWrites + 1);
}

int main() { return run_all_tests(); }
