// Day 11 exercise: an L3 limit order book with price-time priority matching.
// Write all of it yourself. You may copy your own ObjectPool from day 10.
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace ll {

enum class Side : std::uint8_t { Buy, Sell };

using OrderId = std::uint64_t;
using Price = std::int64_t;  // integer ticks
using Qty = std::int64_t;

struct Fill {
    OrderId maker;  // the resting order
    OrderId taker;  // the incoming order
    Price price;    // always the maker's price
    Qty qty;
    bool operator==(const Fill&) const = default;
};

enum class AddResult { ok, duplicate_id, bad_price, bad_qty, book_full };

// Listener must provide: void on_fill(const Fill&);
template <class Listener>
class OrderBook {
public:
    // Allocate everything here. Prices outside [min_price, max_price] are rejected.
    OrderBook(Price min_price, Price max_price, std::size_t max_orders, Listener& listener);

    // Limit order: match against the opposite side while it crosses, then rest any remainder.
    // `book_full` means the remainder could not rest; the fills that already happened still stand.
    AddResult add(OrderId id, Side side, Price price, Qty qty);

    // Remove a resting order. False if unknown.
    bool cancel(OrderId id);

    // Lower the quantity of a resting order, keeping time priority.
    // new_qty == 0 cancels. new_qty >= current qty, or unknown id, returns false and changes nothing.
    bool reduce(OrderId id, Qty new_qty);

    std::optional<Price> best_bid() const;
    std::optional<Price> best_ask() const;

    // Total resting quantity at a price on one side (0 if none or out of range).
    Qty volume_at(Side side, Price price) const;

    std::size_t order_count() const;

private:
    // Your design.
};

}  // namespace ll
