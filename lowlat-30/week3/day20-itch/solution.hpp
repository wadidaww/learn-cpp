// Day 20 exercise: ITCH 5.0 parser and a single-instrument market-by-order book.
// Non-template functions defined in this header must be `inline`.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace ll::itch {

struct Header {
    std::uint16_t stock_locate;
    std::uint16_t tracking_number;
    std::uint64_t timestamp_ns;
};

// Handler must provide (all of them):
//   void on_add(const Header&, std::uint64_t ref, char side, std::uint32_t shares,
//               const std::array<char, 8>& stock, std::uint32_t price);          // 'A' and 'F'
//   void on_executed(const Header&, std::uint64_t ref, std::uint32_t shares, std::uint64_t match);      // 'E'
//   void on_executed_with_price(const Header&, std::uint64_t ref, std::uint32_t shares,
//                               std::uint64_t match, std::uint32_t price);       // 'C'
//   void on_cancel(const Header&, std::uint64_t ref, std::uint32_t shares);      // 'X'
//   void on_delete(const Header&, std::uint64_t ref);                            // 'D'
//   void on_replace(const Header&, std::uint64_t orig_ref, std::uint64_t new_ref,
//                   std::uint32_t shares, std::uint32_t price);                  // 'U'

struct ParseStats {
    std::uint64_t messages = 0;  // every complete message, including skipped types
    std::uint64_t skipped = 0;   // types we do not handle
    std::uint64_t errors = 0;    // handled types whose length is wrong (also skipped)
};

// Stream = repeated [u16 big-endian length][message of that length].
// Returns the number of bytes consumed: everything up to (not including) a trailing partial message.
template <class Handler>
std::size_t parse_itch(std::span<const std::byte> stream, Handler& h, ParseStats& stats);

// Market-by-order book for one stock locate; ignores all other locates. Prices are ITCH u32 (4 decimals).
class ItchBook {
public:
    ItchBook(std::uint16_t locate, std::uint32_t min_price, std::uint32_t max_price, std::size_t max_orders);

    void on_add(const Header&, std::uint64_t ref, char side, std::uint32_t shares, const std::array<char, 8>& stock,
                std::uint32_t price);
    void on_executed(const Header&, std::uint64_t ref, std::uint32_t shares, std::uint64_t match);
    void on_executed_with_price(const Header&, std::uint64_t ref, std::uint32_t shares, std::uint64_t match,
                                std::uint32_t price);
    void on_cancel(const Header&, std::uint64_t ref, std::uint32_t shares);
    void on_delete(const Header&, std::uint64_t ref);
    void on_replace(const Header&, std::uint64_t orig_ref, std::uint64_t new_ref, std::uint32_t shares,
                    std::uint32_t price);

    std::optional<std::uint32_t> best_bid() const;
    std::optional<std::uint32_t> best_ask() const;
    std::uint64_t volume_at(char side, std::uint32_t price) const;  // side 'B' or 'S'
    std::size_t order_count() const;

private:
    // Your design (reuse day 11).
};

}  // namespace ll::itch
