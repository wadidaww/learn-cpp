// Day 8 exercise. Non-template functions defined here must be `inline`.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "rng.hpp"

namespace ll {

// Single cycle through 0..n-1 (Sattolo), drawing from ll::SplitMix64 (common/rng.hpp). Precondition: n >= 2.
std::vector<std::uint32_t> make_cycle(std::size_t n, std::uint64_t seed);

// Follow next[] `steps` times starting at index 0; return the index reached.
std::uint32_t chase(std::span<const std::uint32_t> next, std::size_t steps) noexcept;

std::uint64_t sum_sequential(std::span<const std::uint64_t> data) noexcept;

// Visit every element exactly once, in the order 0, s, 2s, ..., then 1, 1+s, ... (stride s in elements).
std::uint64_t sum_strided(std::span<const std::uint64_t> data, std::size_t stride) noexcept;

}  // namespace ll
