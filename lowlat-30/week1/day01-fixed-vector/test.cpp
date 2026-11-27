#include <cstdint>
#include <memory>
#include <string>

#include "check.hpp"
#include "solution.hpp"

namespace {

// Counts live objects so leaks and double-destroys show up.
struct Tracked {
    static inline int live = 0;
    static inline int copies = 0;
    static inline int moves = 0;
    int v;
    explicit Tracked(int x) : v(x) { ++live; }  // no default constructor on purpose
    Tracked(const Tracked& o) : v(o.v) { ++live; ++copies; }
    Tracked(Tracked&& o) noexcept : v(o.v) { o.v = -1; ++live; ++moves; }
    Tracked& operator=(const Tracked& o) { v = o.v; ++copies; return *this; }
    Tracked& operator=(Tracked&& o) noexcept { v = o.v; o.v = -1; ++moves; return *this; }
    ~Tracked() { --live; }
    static void reset() { live = copies = moves = 0; }
};

struct alignas(64) Wide {
    char c[64];
};

}  // namespace

TEST(starts_empty) {
    ll::FixedVector<int, 4> v;
    CHECK(v.empty());
    CHECK_EQ(v.size(), 0u);
    CHECK_EQ(v.capacity(), 4u);
    CHECK(v.begin() == v.end());
}

TEST(push_until_full_then_refuse) {
    ll::FixedVector<int, 3> v;
    CHECK(v.push_back(1));
    CHECK(v.push_back(2));
    CHECK(v.push_back(3));
    CHECK(v.full());
    CHECK(!v.push_back(4));
    CHECK(v.emplace_back(5) == nullptr);
    CHECK_EQ(v.size(), 3u);
    CHECK_EQ(v[0] + v[1] + v[2], 6);
}

TEST(no_default_constructor_needed_and_no_leaks) {
    Tracked::reset();
    {
        ll::FixedVector<Tracked, 8> v;
        CHECK_EQ(Tracked::live, 0);  // empty vector constructs nothing
        v.emplace_back(1);
        v.emplace_back(2);
        CHECK_EQ(Tracked::live, 2);
        v.pop_back();
        CHECK_EQ(Tracked::live, 1);
        CHECK_EQ(v[0].v, 1);
    }
    CHECK_EQ(Tracked::live, 0);
}

TEST(emplace_constructs_in_place) {
    Tracked::reset();
    ll::FixedVector<Tracked, 2> v;
    Tracked* p = v.emplace_back(7);
    CHECK(p != nullptr);
    CHECK_EQ(p->v, 7);
    CHECK_EQ(Tracked::copies, 0);
    CHECK_EQ(Tracked::moves, 0);
}

TEST(push_rvalue_moves_push_lvalue_copies) {
    Tracked::reset();
    ll::FixedVector<Tracked, 4> v;
    Tracked a(1);
    v.push_back(a);
    CHECK_EQ(Tracked::copies, 1);
    v.push_back(Tracked(2));
    CHECK_EQ(Tracked::moves, 1);
    CHECK_EQ(Tracked::copies, 1);
}

TEST(copy_constructor_and_assignment) {
    Tracked::reset();
    {
        ll::FixedVector<Tracked, 4> a;
        a.emplace_back(1);
        a.emplace_back(2);
        ll::FixedVector<Tracked, 4> b(a);
        CHECK_EQ(b.size(), 2u);
        CHECK_EQ(b[1].v, 2);
        CHECK_EQ(a[1].v, 2);
        ll::FixedVector<Tracked, 4> c;
        c.emplace_back(9);
        c.emplace_back(9);
        c.emplace_back(9);
        c = a;  // shrinking assignment must destroy the extra element
        CHECK_EQ(c.size(), 2u);
        CHECK_EQ(c[0].v, 1);
        CHECK_EQ(Tracked::live, 6);
    }
    CHECK_EQ(Tracked::live, 0);
}

TEST(self_assignment_is_safe) {
    Tracked::reset();
    {
        ll::FixedVector<Tracked, 4> a;
        a.emplace_back(5);
        auto& alias = a;
        a = alias;
        CHECK_EQ(a.size(), 1u);
        CHECK_EQ(a[0].v, 5);
        a = std::move(alias);
        CHECK(a.size() <= 1u);  // valid state; value unspecified
    }
    CHECK_EQ(Tracked::live, 0);
}

TEST(move_constructor_moves_elements_and_empties_source) {
    Tracked::reset();
    {
        ll::FixedVector<Tracked, 4> a;
        a.emplace_back(1);
        a.emplace_back(2);
        ll::FixedVector<Tracked, 4> b(std::move(a));
        CHECK_EQ(b.size(), 2u);
        CHECK_EQ(b[0].v, 1);
        CHECK_EQ(Tracked::copies, 0);
        CHECK(a.empty());
        CHECK(a.push_back(Tracked(3)));  // moved-from object is still usable
    }
    CHECK_EQ(Tracked::live, 0);
}

TEST(move_assignment) {
    Tracked::reset();
    {
        ll::FixedVector<Tracked, 4> a, b;
        a.emplace_back(1);
        b.emplace_back(7);
        b.emplace_back(8);
        b.emplace_back(9);
        b = std::move(a);
        CHECK_EQ(b.size(), 1u);
        CHECK_EQ(b[0].v, 1);
        CHECK_EQ(Tracked::copies, 0);
    }
    CHECK_EQ(Tracked::live, 0);
}

TEST(move_only_type) {
    ll::FixedVector<std::unique_ptr<int>, 2> v;
    CHECK(v.push_back(std::make_unique<int>(4)));
    v.emplace_back(new int(5));
    auto w = std::move(v);
    CHECK_EQ(*w[0], 4);
    CHECK_EQ(*w[1], 5);
}

TEST(noexcept_propagates) {
    static_assert(std::is_nothrow_move_constructible_v<ll::FixedVector<int, 4>>);
    static_assert(std::is_nothrow_move_constructible_v<ll::FixedVector<Tracked, 4>>);
    static_assert(!std::is_nothrow_move_constructible_v<ll::FixedVector<std::string, 4>> ||
                  std::is_nothrow_move_constructible_v<std::string>);
    CHECK(true);
}

TEST(over_aligned_type) {
    ll::FixedVector<Wide, 3> v;
    v.emplace_back();
    v.emplace_back();
    for (auto& w : v) CHECK(reinterpret_cast<std::uintptr_t>(&w) % 64 == 0);
}

TEST(iteration_and_clear) {
    Tracked::reset();
    ll::FixedVector<Tracked, 5> v;
    for (int i = 0; i < 5; ++i) v.emplace_back(i);
    int sum = 0;
    for (const auto& t : v) sum += t.v;
    CHECK_EQ(sum, 10);
    v.clear();
    CHECK(v.empty());
    CHECK_EQ(Tracked::live, 0);
}

TEST(no_heap_in_object) {
    // Storage lives inside the object: 16 ints plus a size field, nothing on the heap.
    CHECK(sizeof(ll::FixedVector<int, 16>) >= 16 * sizeof(int));
    CHECK(sizeof(ll::FixedVector<int, 16>) <= 16 * sizeof(int) + 16);
}

int main() { return run_all_tests(); }
