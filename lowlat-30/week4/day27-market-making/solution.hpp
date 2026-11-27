// Day 27 exercise: turn a fair value and an inventory into a two-sided quote.
// Non-template functions defined in this header must be `inline`.
#pragma once

#include <cstdint>
#include <optional>

namespace ll::mm {

struct QuoteParams {
    std::int64_t half_spread;    // distance from the centre to each side, in price units, >= 0
    std::int64_t skew_per_unit;  // centre moves down by this much per unit of long position, >= 0
    std::int64_t max_position;   // at +max stop bidding, at -max stop offering, > 0
    std::int64_t tick;           // > 0; quotes must be multiples of it
};

struct Quote {
    std::optional<std::int64_t> bid, ask;
};

// centre = fair - position * skew_per_unit
// bid = centre - half_spread rounded DOWN to a tick, ask = centre + half_spread rounded UP to a tick
// (round toward the side that is safer for you; works for negative values too).
// If rounding makes bid >= ask, keep the bid and set ask = bid + tick.
// No bid when position >= max_position or the bid would be <= 0. No ask when position <= -max_position.
// You may assume position * skew_per_unit and the sums fit in int64.
Quote make_quote(std::int64_t fair, std::int64_t position, const QuoteParams& p) noexcept;

}  // namespace ll::mm
