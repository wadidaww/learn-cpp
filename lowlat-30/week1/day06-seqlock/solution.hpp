// Day 6 exercise. One writer thread, any number of reader threads.
#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace ll {

template <class T>
    requires std::is_trivially_copyable_v<T>
class Seqlock {
public:
    static constexpr std::size_t kWords = (sizeof(T) + 7) / 8;

    // Writer thread only.
    void store(const T& value) noexcept;

    // Any thread. Spins until it gets a consistent copy.
    T load() const noexcept;

    // Any thread. One attempt; returns false if a write was in progress or overlapped.
    bool try_load(T& out) const noexcept;

    // Number of completed stores (seq / 2). Any thread.
    std::uint64_t version() const noexcept;

private:
    // Suggested: alignas(64) std::atomic<std::uint64_t> seq_{0};
    //            std::array<std::atomic<std::uint64_t>, kWords> words_{};
};

struct TopOfBook {
    std::int64_t bid_px;
    std::int64_t bid_qty;
    std::int64_t ask_px;
    std::int64_t ask_qty;
    std::uint64_t seq_no;
};

}  // namespace ll
