// Day 5 exercise. The most important code you write this month: do it by hand, then do it again on paper.
#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace ll {

template <class T, std::size_t N>
class SpscRing {
    // static_assert here that N is a power of two (and > 0).
public:
    // Producer thread only.
    bool try_push(const T& v) noexcept;
    bool try_push(T&& v) noexcept;

    // Consumer thread only.
    bool try_pop(T& out) noexcept;

    // Either thread; may be stale by the time you use it.
    std::size_t size_approx() const noexcept;
    bool empty_approx() const noexcept;

    static constexpr std::size_t capacity() noexcept { return N; }

private:
    // Your layout: producer line, consumer line, buffer. Use alignas(64).
};

}  // namespace ll
