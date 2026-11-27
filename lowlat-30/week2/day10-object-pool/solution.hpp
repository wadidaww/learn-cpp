// Day 10 exercise. Write every body yourself.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>

namespace ll {

template <class T, std::size_t N>
class ObjectPool {
    static_assert(N > 0 && N < 0xFFFFFFFFu, "indices are 32-bit");

public:
    ObjectPool() noexcept;  // build the free list (this also pre-touches the memory)
    ~ObjectPool() = default;
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    template <class... Args>
    T* create(Args&&... args);  // nullptr if exhausted

    void destroy(T* p) noexcept;  // precondition: p came from this pool and is live

    std::size_t available() const noexcept;
    static constexpr std::size_t capacity() noexcept { return N; }
    bool owns(const T* p) const noexcept;

private:
    // Suggested: a slot type that is either a T or a uint32_t "next free" index,
    // an aligned array of N slots, and a uint32_t free_head_ (0xFFFFFFFF = empty).
};

}  // namespace ll
