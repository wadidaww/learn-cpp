// Day 21 exercise: zero-allocation FIX 4.4 NewOrderSingle (35=D) encoder and decoder.
// Non-template functions defined in this header must be `inline`.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace ll::fix {

inline constexpr char SOH = '\x01';

// Prices are fixed point with 4 decimals: 189.01 is 1'890'100. Never double.
struct NewOrderSingle {
    std::string_view sender_comp_id;  // 49
    std::string_view target_comp_id;  // 56
    std::uint64_t msg_seq_num = 0;    // 34
    std::string_view sending_time;    // 52, e.g. "20261127-14:30:00.123"
    std::string_view cl_ord_id;       // 11
    std::string_view symbol;          // 55
    char side = '1';                  // 54: '1' buy, '2' sell
    std::int64_t order_qty = 0;       // 38, whole shares, > 0
    char ord_type = '2';              // 40: '1' market, '2' limit
    std::int64_t price = 0;           // 44, 4 decimals; required (and > 0) when ord_type == '2', absent for market
    char time_in_force = '0';         // 59: '0' day, '3' IOC, '4' FOK; optional on decode (default '0')
};

// Sum of all bytes mod 256 (the value of tag 10 for the bytes before "10=").
std::uint8_t checksum(std::string_view bytes) noexcept;

// Writes the complete message in this tag order:
//   8=FIX.4.4|9=<len>|35=D|49|56|34|52|11|55|54|38|40|44 (limit only)|59|10=<3 digits>|
// Price is written with exactly 4 decimals ("189.0100"). Returns bytes written, or 0 if `out` is too small,
// order_qty <= 0, or a limit order has price <= 0.
std::size_t encode_new_order(const NewOrderSingle& m, std::span<char> out) noexcept;

enum class Status {
    ok,
    incomplete,        // not a full message yet: read more bytes and call again
    bad_begin_string,  // does not start with "8=FIX.4.4" SOH "9="
    bad_body_length,   // 9= is not a number, or tag 10 is not where it says
    bad_checksum,      // 10= is not 3 digits or does not match
    wrong_msg_type,    // 35 is not the first body field, or is not 'D'
    missing_field,     // a required tag is absent
    bad_value,         // a field does not parse (numbers, single chars, more than 4 decimals, empty value)
};

struct DecodeResult {
    Status status;
    // Bytes of the whole message, so the caller can skip it, when the framing (8, 9, 10) was found: ok,
    // bad_checksum, wrong_msg_type, missing_field, bad_value. 0 for incomplete, bad_begin_string and
    // bad_body_length (the stream cannot be resynchronised: a real session disconnects).
    std::size_t consumed;
};

// Decodes one message from the front of `buf`. Body fields after 35 may come in any order; unknown tags are
// ignored. The string_views in `out` point into `buf`. No allocation, no exceptions.
DecodeResult decode_new_order(std::string_view buf, NewOrderSingle& out) noexcept;

}  // namespace ll::fix
