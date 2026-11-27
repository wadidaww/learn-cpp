// Day 2 exercise. Fill in every function body yourself, in this header.
// Non-template functions defined in a header must be marked `inline` (one definition rule).
#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

namespace ll::itch {

// Big-endian load/store of a 2, 4 or 8 byte unsigned integer from any address (no alignment assumed).
template <std::unsigned_integral T>
T load_be(const std::byte* p) noexcept;

template <std::unsigned_integral T>
void store_be(std::byte* p, T v) noexcept;

// 6-byte big-endian integer (ITCH timestamps).
std::uint64_t load_be48(const std::byte* p) noexcept;
void store_be48(std::byte* p, std::uint64_t v) noexcept;  // only the low 48 bits are written

inline constexpr std::size_t kAddOrderSize = 36;

struct AddOrder {
    std::uint16_t stock_locate;
    std::uint16_t tracking_number;
    std::uint64_t timestamp_ns;  // 48 bits on the wire
    std::uint64_t order_ref;
    char side;  // 'B' or 'S'
    std::uint32_t shares;
    std::array<char, 8> stock;  // space padded, not NUL terminated
    std::uint32_t price;        // 4 implied decimals: 1234500 == 123.4500
};

enum class DecodeError { ok, too_short, wrong_type, bad_side };

// Decode one Add Order starting at buf[0]. On error, `out` may be partially written.
DecodeError decode_add_order(std::span<const std::byte> buf, AddOrder& out) noexcept;

// Encode into `out`. Returns kAddOrderSize, or 0 if out is smaller than that.
std::size_t encode_add_order(const AddOrder& msg, std::span<std::byte> out) noexcept;

}  // namespace ll::itch
