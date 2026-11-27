// Day 1 exercise. Write every member body yourself (no AI completion).
// The declarations are fixed: the tests in test.cpp depend on them.
#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace ll {

template <class T, std::size_t N>
class FixedVector {
public:
    using value_type = T;
    using iterator = T*;
    using const_iterator = const T*;

    FixedVector() noexcept;
    ~FixedVector();

    FixedVector(const FixedVector& other);
    FixedVector& operator=(const FixedVector& other);

    FixedVector(FixedVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>);
    FixedVector& operator=(FixedVector&& other) noexcept(std::is_nothrow_move_constructible_v<T> &&
                                                         std::is_nothrow_destructible_v<T>);

    // Return false (and change nothing) when full.
    bool push_back(const T& value);
    bool push_back(T&& value);

    // Return a pointer to the new element, or nullptr when full.
    template <class... Args>
    T* emplace_back(Args&&... args);

    void pop_back() noexcept;  // precondition: !empty()
    void clear() noexcept;

    T& operator[](std::size_t i) noexcept;
    const T& operator[](std::size_t i) const noexcept;

    std::size_t size() const noexcept;
    static constexpr std::size_t capacity() noexcept { return N; }
    bool empty() const noexcept;
    bool full() const noexcept;

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;

private:
    // Choose your own private members (storage + size). Remember alignas(T).
};

}  // namespace ll
