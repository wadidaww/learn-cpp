#include <mutex>
#include <thread>
#include <vector>

#include "check.hpp"
#include "solution.hpp"

using namespace ll;

namespace {

template <class Lock>
long hammer(int threads, int iters) {
    Lock lk;
    long counter = 0;  // plain: only safe if the lock works
    std::vector<std::thread> ts;
    for (int t = 0; t < threads; ++t)
        ts.emplace_back([&] {
            for (int i = 0; i < iters; ++i) {
                std::lock_guard g(lk);
                ++counter;
            }
        });
    for (auto& t : ts) t.join();
    return counter;
}

}  // namespace

TEST(tas_try_lock_semantics) {
    TasSpinlock l;
    CHECK(l.try_lock());
    CHECK(!l.try_lock());
    l.unlock();
    CHECK(l.try_lock());
    l.unlock();
}

TEST(ttas_try_lock_semantics) {
    TtasSpinlock l;
    CHECK(l.try_lock());
    CHECK(!l.try_lock());
    l.unlock();
    CHECK(l.try_lock());
    l.unlock();
}

TEST(tas_mutual_exclusion) { CHECK_EQ(hammer<TasSpinlock>(4, 20000), 80000L); }

TEST(ttas_mutual_exclusion) { CHECK_EQ(hammer<TtasSpinlock>(4, 20000), 80000L); }

TEST(lock_is_small) {
    CHECK(sizeof(TtasSpinlock) == 1);  // pad it yourself where false sharing matters
}

TEST(mailbox_single_thread) {
    Mailbox m;
    std::int64_t v = 0;
    CHECK(!m.try_consume(v));
    CHECK(m.publish(7));
    CHECK(!m.publish(8));  // full
    CHECK(m.try_consume(v));
    CHECK_EQ(v, 7);
    CHECK(!m.try_consume(v));
    CHECK(m.publish(9));
}

TEST(mailbox_two_threads_in_order_no_loss) {
    Mailbox m;
    constexpr std::int64_t N = 200000;
    std::thread prod([&] {
        for (std::int64_t i = 1; i <= N; ++i)
            while (!m.publish(i)) {
            }
    });
    std::int64_t expect = 1;
    bool ok = true;
    while (expect <= N) {
        std::int64_t v;
        if (m.try_consume(v)) {
            ok = ok && (v == expect);
            ++expect;
        }
    }
    prod.join();
    CHECK(ok);
}

int main() { return run_all_tests(); }
