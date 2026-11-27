// Day 22 exercise: a pre-trade risk gate, the last box before the order gateway.
// Non-template functions defined in this header must be `inline`.
#pragma once

#include <cstddef>
#include <cstdint>

namespace ll::risk {

// Prices are fixed point with 4 decimals (like ITCH and FIX): 189.01 is 1'890'100. Quantities are shares.
// Notional is price * qty in the same fixed point (100 shares at 189.01 is 189'010'000).
struct Limits {
    std::int64_t max_order_qty;            // per order
    std::int64_t max_notional;             // per order, price * qty
    std::int64_t max_position;             // |position| including every open order filling (worst case)
    std::int64_t collar_bps;               // reject if |price - reference| * 10'000 > reference * collar_bps
                                           // (you may assume both sides of that comparison fit in int64)
    std::uint32_t max_orders_per_window;   // at most this many accepted orders with ts in (now - window_ns, now]
    std::uint64_t window_ns;               // event time, not the clock; >= 1
};

struct Order {
    std::uint64_t ts_ns;  // event time, from the feed or the strategy; never read a clock in here
    char side;            // 'B' or 'S'
    std::int64_t qty;
    std::int64_t price;
};

// check() tests in exactly this order and returns the first failure:
enum class Verdict {
    accept,
    killed,        // kill switch is on
    bad_order,     // side not 'B'/'S', qty <= 0, or price <= 0
    too_large,     // qty > max_order_qty
    no_reference,  // set_reference() never called (or called with <= 0): you cannot collar, so refuse
    collar,        // price too far from the reference
    notional,      // price * qty > max_notional (must not overflow for any int64 inputs that passed above)
    position,      // worst-case position would exceed max_position
    rate,          // would be more than max_orders_per_window accepts within window_ns
};

class RiskGate {
public:
    // May allocate here (e.g. the rate window), never in the methods below.
    explicit RiskGate(const Limits& limits);

    void set_reference(std::int64_t price) noexcept;  // e.g. the mid from your book
    Verdict check(const Order& o) noexcept;  // on accept: counts toward the rate limit and reserves open qty

    // Fill of an accepted order: position moves, the filled qty is no longer open.
    void on_fill(char side, std::int64_t qty) noexcept;
    // An accepted order ended (cancelled, expired, rejected by the venue) with this much still open.
    void on_done(char side, std::int64_t open_qty) noexcept;

    void kill() noexcept;   // operator or automatic kill switch; stays on
    bool killed() const noexcept;
    std::int64_t position() const noexcept;    // filled position, buys positive
    std::int64_t open_buys() const noexcept;   // accepted, not yet filled or done
    std::int64_t open_sells() const noexcept;

private:
    // Your design.
};

}  // namespace ll::risk
