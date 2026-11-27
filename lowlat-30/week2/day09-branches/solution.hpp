// Day 9 exercise. Non-template functions defined here must be `inline`.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace ll {

// Number of elements strictly less than t.
std::size_t count_below_branchy(std::span<const std::int32_t> data, std::int32_t t) noexcept;
std::size_t count_below_branchless(std::span<const std::int32_t> data, std::int32_t t) noexcept;

// Index of the first element >= key (data.size() if none), like std::lower_bound. No if/else in the loop.
std::size_t lower_bound_branchless(std::span<const std::int64_t> sorted, std::int64_t key) noexcept;

struct OrderAoS {
    std::int64_t price;
    std::int64_t qty;
    std::uint64_t id;
    std::uint64_t timestamp_ns;
    std::uint32_t trader;
    std::uint32_t flags;
    char venue_tag[24];  // cold data that pads the struct to 64 bytes
};

struct OrdersSoA {
    std::vector<std::int64_t> price;
    std::vector<std::int64_t> qty;
    std::vector<std::uint64_t> id;
};

// Sum of qty over orders with price >= px.
std::int64_t sum_qty_at_or_above_aos(std::span<const OrderAoS> orders, std::int64_t px) noexcept;
std::int64_t sum_qty_at_or_above_soa(const OrdersSoA& orders, std::int64_t px) noexcept;

}  // namespace ll
