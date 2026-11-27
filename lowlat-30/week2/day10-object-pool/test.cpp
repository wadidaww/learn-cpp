#include <cstdint>
#include <set>
#include <vector>

#include "check.hpp"
#include "solution.hpp"

using namespace ll;

namespace {

struct Order {
    static inline int live = 0;
    std::uint64_t id;
    std::int64_t price;
    std::int64_t qty;
    Order(std::uint64_t i, std::int64_t p, std::int64_t q) : id(i), price(p), qty(q) { ++live; }
    ~Order() { --live; }
};

struct alignas(64) Big {
    char bytes[100];
};

}  // namespace

TEST(create_until_exhausted) {
    ObjectPool<Order, 4> pool;
    CHECK_EQ(pool.available(), 4u);
    std::vector<Order*> v;
    for (std::uint64_t i = 0; i < 4; ++i) {
        auto* o = pool.create(i, 100, 10);
        CHECK(o != nullptr);
        v.push_back(o);
    }
    CHECK_EQ(pool.available(), 0u);
    CHECK(pool.create(9ull, 1, 1) == nullptr);
    std::set<Order*> uniq(v.begin(), v.end());
    CHECK_EQ(uniq.size(), 4u);
    for (auto* o : v) pool.destroy(o);
    CHECK_EQ(pool.available(), 4u);
}

TEST(constructs_and_destroys) {
    Order::live = 0;
    ObjectPool<Order, 8> pool;
    CHECK_EQ(Order::live, 0);  // an empty pool constructs nothing
    auto* a = pool.create(1ull, 101, 5);
    CHECK_EQ(Order::live, 1);
    CHECK_EQ(a->price, 101);
    pool.destroy(a);
    CHECK_EQ(Order::live, 0);
}

TEST(lifo_reuse) {
    ObjectPool<Order, 8> pool;
    auto* a = pool.create(1ull, 1, 1);
    auto* b = pool.create(2ull, 1, 1);
    pool.destroy(a);
    pool.destroy(b);
    CHECK(pool.create(3ull, 1, 1) == b);  // most recently freed comes back first
    CHECK(pool.create(4ull, 1, 1) == a);
}

TEST(owns) {
    ObjectPool<Order, 4> p1, p2;
    auto* a = p1.create(1ull, 1, 1);
    CHECK(p1.owns(a));
    CHECK(!p2.owns(a));
    Order stack_order(1, 1, 1);
    CHECK(!p1.owns(&stack_order));
    p1.destroy(a);
}

TEST(alignment_respected) {
    ObjectPool<Big, 5> pool;
    std::vector<Big*> v;
    for (int i = 0; i < 5; ++i) {
        v.push_back(pool.create());
        CHECK(reinterpret_cast<std::uintptr_t>(v.back()) % 64 == 0);
    }
    for (auto* b : v) pool.destroy(b);
}

TEST(churn_keeps_count_correct) {
    Order::live = 0;
    ObjectPool<Order, 64> pool;
    std::vector<Order*> live;
    std::uint64_t x = 7;
    for (int step = 0; step < 100000; ++step) {
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        if ((x & 1) && !live.empty()) {
            auto i = static_cast<std::size_t>((x >> 8) % live.size());
            CHECK_EQ(live[i]->qty, static_cast<std::int64_t>(live[i]->id % 97));
            pool.destroy(live[i]);
            live[i] = live.back();
            live.pop_back();
        } else if (auto* o = pool.create(x, 1, static_cast<std::int64_t>(x % 97))) {
            live.push_back(o);
        }
        CHECK_EQ(pool.available() + live.size(), 64u);
    }
    for (auto* o : live) pool.destroy(o);
    CHECK_EQ(Order::live, 0);
}

int main() { return run_all_tests(); }
