#include <deque>
#include <functional>
#include <map>
#include <unordered_map>

#include "check.hpp"
#include "rng.hpp"
#include "solution.hpp"

using namespace ll;

namespace {

struct Recorder {
    std::vector<Fill> fills;
    void on_fill(const Fill& f) { fills.push_back(f); }
};

// Slow, obviously correct reference model.
class ModelBook {
public:
    ModelBook(Price lo, Price hi, std::size_t cap) : lo_(lo), hi_(hi), cap_(cap) {}

    AddResult add(OrderId id, Side side, Price px, Qty qty, std::vector<Fill>& out) {
        if (qty <= 0) return AddResult::bad_qty;
        if (px < lo_ || px > hi_) return AddResult::bad_price;
        if (where_.count(id)) return AddResult::duplicate_id;
        if (side == Side::Buy) {
            while (qty > 0 && !asks_.empty() && asks_.begin()->first <= px) qty = take(asks_.begin(), asks_, id, qty, out);
        } else {
            while (qty > 0 && !bids_.empty() && bids_.begin()->first >= px) qty = take(bids_.begin(), bids_, id, qty, out);
        }
        if (qty == 0) return AddResult::ok;
        if (where_.size() >= cap_) return AddResult::book_full;
        (side == Side::Buy ? bids_[px] : asks_[px]).push_back({id, qty});
        where_[id] = {side, px};
        return AddResult::ok;
    }
    bool cancel(OrderId id) {
        auto it = where_.find(id);
        if (it == where_.end()) return false;
        auto [side, px] = it->second;
        if (side == Side::Buy) erase_from(bids_, px, id);
        else erase_from(asks_, px, id);
        where_.erase(it);
        return true;
    }
    bool reduce(OrderId id, Qty q) {
        auto it = where_.find(id);
        if (it == where_.end()) return false;
        auto [side, px] = it->second;
        auto& dq = side == Side::Buy ? bids_[px] : asks_[px];
        for (auto& o : dq)
            if (o.id == id) {
                if (q < 0 || q >= o.qty) return false;
                if (q == 0) return cancel(id);
                o.qty = q;
                return true;
            }
        return false;
    }
    std::optional<Price> best_bid() const {
        return bids_.empty() ? std::nullopt : std::optional<Price>(bids_.begin()->first);
    }
    std::optional<Price> best_ask() const {
        return asks_.empty() ? std::nullopt : std::optional<Price>(asks_.begin()->first);
    }
    Qty volume_at(Side side, Price px) const {
        Qty s = 0;
        if (side == Side::Buy) {
            auto it = bids_.find(px);
            if (it != bids_.end())
                for (auto& o : it->second) s += o.qty;
        } else {
            auto it = asks_.find(px);
            if (it != asks_.end())
                for (auto& o : it->second) s += o.qty;
        }
        return s;
    }
    std::size_t order_count() const { return where_.size(); }

private:
    struct O {
        OrderId id;
        Qty qty;
    };
    template <class It, class M>
    Qty take(It lvl, M& side_map, OrderId taker, Qty qty, std::vector<Fill>& out) {
        auto& dq = lvl->second;
        while (qty > 0 && !dq.empty()) {
            auto& m = dq.front();
            Qty t = std::min(qty, m.qty);
            out.push_back({m.id, taker, lvl->first, t});
            qty -= t;
            m.qty -= t;
            if (m.qty == 0) {
                where_.erase(m.id);
                dq.pop_front();
            }
        }
        if (dq.empty()) side_map.erase(lvl);
        return qty;
    }
    template <class M>
    void erase_from(M& m, Price px, OrderId id) {
        auto& dq = m[px];
        for (auto it = dq.begin(); it != dq.end(); ++it)
            if (it->id == id) {
                dq.erase(it);
                break;
            }
        if (dq.empty()) m.erase(px);
    }
    Price lo_, hi_;
    std::size_t cap_;
    std::map<Price, std::deque<O>, std::greater<>> bids_;
    std::map<Price, std::deque<O>> asks_;
    std::unordered_map<OrderId, std::pair<Side, Price>> where_;
};

}  // namespace

TEST(empty_book) {
    Recorder r;
    OrderBook<Recorder> b(100, 200, 16, r);
    CHECK(!b.best_bid());
    CHECK(!b.best_ask());
    CHECK_EQ(b.order_count(), 0u);
    CHECK_EQ(b.volume_at(Side::Buy, 150), 0);
}

TEST(rest_without_crossing) {
    Recorder r;
    OrderBook<Recorder> b(100, 200, 16, r);
    CHECK(b.add(1, Side::Buy, 149, 10) == AddResult::ok);
    CHECK(b.add(2, Side::Buy, 148, 5) == AddResult::ok);
    CHECK(b.add(3, Side::Sell, 151, 7) == AddResult::ok);
    CHECK(b.add(4, Side::Buy, 149, 3) == AddResult::ok);
    CHECK(r.fills.empty());
    CHECK_EQ(*b.best_bid(), 149);
    CHECK_EQ(*b.best_ask(), 151);
    CHECK_EQ(b.volume_at(Side::Buy, 149), 13);
    CHECK_EQ(b.volume_at(Side::Sell, 151), 7);
    CHECK_EQ(b.order_count(), 4u);
}

TEST(price_time_priority_and_maker_price) {
    Recorder r;
    OrderBook<Recorder> b(100, 200, 16, r);
    b.add(1, Side::Sell, 150, 5);
    b.add(2, Side::Sell, 150, 5);  // same price, later
    b.add(3, Side::Sell, 149, 2);  // better price
    CHECK(b.add(10, Side::Buy, 155, 8) == AddResult::ok);
    // best price first (149), then FIFO at 150; trades at maker prices
    CHECK_EQ(r.fills.size(), 3u);
    if (r.fills.size() == 3) {
        CHECK(r.fills[0] == (Fill{3, 10, 149, 2}));
        CHECK(r.fills[1] == (Fill{1, 10, 150, 5}));
        CHECK(r.fills[2] == (Fill{2, 10, 150, 1}));
    }
    CHECK_EQ(b.volume_at(Side::Sell, 150), 4);
    CHECK(!b.best_bid());  // fully filled taker does not rest
    CHECK_EQ(*b.best_ask(), 150);
}

TEST(partial_fill_rests_remainder) {
    Recorder r;
    OrderBook<Recorder> b(100, 200, 16, r);
    b.add(1, Side::Buy, 150, 4);
    CHECK(b.add(2, Side::Sell, 149, 10) == AddResult::ok);
    CHECK_EQ(r.fills.size(), 1u);
    CHECK(!b.best_bid());
    CHECK_EQ(*b.best_ask(), 149);
    CHECK_EQ(b.volume_at(Side::Sell, 149), 6);
    CHECK_EQ(b.order_count(), 1u);
}

TEST(cancel_and_reduce) {
    Recorder r;
    OrderBook<Recorder> b(100, 200, 16, r);
    b.add(1, Side::Buy, 150, 10);
    b.add(2, Side::Buy, 150, 10);
    CHECK(b.reduce(1, 4));          // keeps priority
    CHECK(!b.reduce(1, 4));         // not a decrease
    CHECK(!b.reduce(1, 9));         // increase not allowed
    CHECK(!b.reduce(99, 1));        // unknown
    CHECK_EQ(b.volume_at(Side::Buy, 150), 14);
    b.add(3, Side::Sell, 150, 5);
    CHECK(r.fills.size() == 2 && r.fills[0] == (Fill{1, 3, 150, 4}) && r.fills[1] == (Fill{2, 3, 150, 1}));
    CHECK(b.cancel(2));
    CHECK(!b.cancel(2));
    CHECK(!b.best_bid());
    CHECK_EQ(b.order_count(), 0u);
    b.add(4, Side::Sell, 160, 3);
    CHECK(b.reduce(4, 0));  // reduce to zero cancels
    CHECK(!b.best_ask());
}

TEST(best_price_moves_when_level_empties) {
    Recorder r;
    OrderBook<Recorder> b(100, 200, 16, r);
    b.add(1, Side::Buy, 120, 1);
    b.add(2, Side::Buy, 150, 1);
    b.add(3, Side::Buy, 101, 1);
    CHECK_EQ(*b.best_bid(), 150);
    b.cancel(2);
    CHECK_EQ(*b.best_bid(), 120);
    b.cancel(1);
    CHECK_EQ(*b.best_bid(), 101);
    b.add(4, Side::Sell, 200, 1);
    b.add(5, Side::Sell, 100, 1);  // crosses the 101 bid
    CHECK(!b.best_bid());
    CHECK_EQ(*b.best_ask(), 200);
}

TEST(rejections) {
    Recorder r;
    OrderBook<Recorder> b(100, 200, 2, r);
    CHECK(b.add(1, Side::Buy, 99, 1) == AddResult::bad_price);
    CHECK(b.add(1, Side::Buy, 201, 1) == AddResult::bad_price);
    CHECK(b.add(1, Side::Buy, 150, 0) == AddResult::bad_qty);
    CHECK(b.add(1, Side::Buy, 150, -5) == AddResult::bad_qty);
    CHECK(b.add(1, Side::Buy, 100, 1) == AddResult::ok);  // bounds inclusive
    CHECK(b.add(1, Side::Sell, 200, 1) == AddResult::duplicate_id);
    CHECK(b.add(2, Side::Sell, 200, 1) == AddResult::ok);
    CHECK(b.add(3, Side::Sell, 199, 1) == AddResult::book_full);
    CHECK_EQ(b.order_count(), 2u);
    CHECK(b.add(4, Side::Sell, 100, 1) == AddResult::ok);  // fully fills: needs no slot
    CHECK_EQ(r.fills.size(), 1u);
    CHECK(b.add(1, Side::Buy, 150, 1) == AddResult::ok);    // id 1 is gone, may be reused
}

TEST(differential_against_model) {
    constexpr Price lo = 1000, hi = 1100;
    Recorder r;
    OrderBook<Recorder> b(lo, hi, 5000, r);
    ModelBook m(lo, hi, 5000);
    std::vector<Fill> mf;
    std::vector<OrderId> ids;
    SplitMix64 g{2024};
    OrderId next_id = 1;
    int first_bad = -1;
    for (int step = 0; step < 200'000 && first_bad < 0; ++step) {
        auto op = g.next_below(100);
        if (op < 55 || ids.empty()) {
            Side s = g.next_below(2) ? Side::Buy : Side::Sell;
            // Prices cluster around 1050 so orders cross often.
            Price px = 1050 + static_cast<Price>(g.next_below(21)) - 10 + (s == Side::Buy ? -2 : 2);
            Qty q = 1 + static_cast<Qty>(g.next_below(50));
            OrderId id = (g.next_below(50) == 0 && !ids.empty()) ? ids[g.next_below(ids.size())] : next_id++;
            auto ra = b.add(id, s, px, q);
            auto rb = m.add(id, s, px, q, mf);
            if (ra != rb) first_bad = step;
            if (rb == AddResult::ok) ids.push_back(id);
        } else if (op < 85) {
            auto k = g.next_below(ids.size());
            if (b.cancel(ids[k]) != m.cancel(ids[k])) first_bad = step;
            ids[k] = ids.back();
            ids.pop_back();
        } else {
            auto id = ids[g.next_below(ids.size())];
            Qty q = static_cast<Qty>(g.next_below(30));
            if (b.reduce(id, q) != m.reduce(id, q)) first_bad = step;
        }
        if (r.fills != mf || b.best_bid() != m.best_bid() || b.best_ask() != m.best_ask() ||
            b.order_count() != m.order_count())
            first_bad = step;
        if (step % 97 == 0)
            for (Price p = lo; p <= hi; ++p)
                if (b.volume_at(Side::Buy, p) != m.volume_at(Side::Buy, p) ||
                    b.volume_at(Side::Sell, p) != m.volume_at(Side::Sell, p))
                    first_bad = step;
    }
    if (first_bad >= 0) std::printf("    first mismatch at step %d (fills: yours %zu, model %zu)\n", first_bad, r.fills.size(), mf.size());
    CHECK_EQ(first_bad, -1);
    CHECK(r.fills.size() > 10'000);  // the workload really trades
}

int main() { return run_all_tests(); }
