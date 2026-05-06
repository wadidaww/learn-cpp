// 03_arena_allocator.cpp
//
// Lesson 3 – Arena / pool allocator (zero heap calls at runtime)
//
// An arena allocator pre-allocates a large block of memory once, then
// hands out slices from that block with pointer-bump logic.
//
// Benefits
//  - O(1) allocation — just bump a pointer.
//  - Zero heap calls during the hot path.
//  - Perfect cache locality — objects land next to each other.
//  - Trivial cleanup — reset the pointer or let the arena go out of scope.
//
// This file builds two things:
//  1. ArenaBuffer<N>     – owns a stack array of N bytes.
//  2. ArenaAllocator<T,N> – an STL-compatible allocator that draws from the
//                           buffer.
//
// We then demonstrate using these with std::vector and std::list, and show
// the std::pmr::monotonic_buffer_resource equivalent (C++17).

#include <algorithm>   // std::align
#include <cstddef>
#include <iostream>
#include <list>
#include <memory>      // std::align
#include <memory_resource> // std::pmr
#include <new>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------
static void section(const char* title) {
    std::cout << "\n=== " << title << " ===\n";
}

// ===========================================================================
// ArenaBuffer<N>  — the memory backing store
// ===========================================================================
// Holds a fixed-size stack buffer and a bump pointer.
// Alignment is handled by std::align so every returned pointer is
// correctly aligned for the requested type.
// ---------------------------------------------------------------------------
template <std::size_t N>
class ArenaBuffer {
public:
    ArenaBuffer() noexcept : ptr_(buf_) {}

    // Non-copyable, non-movable — the buffer's address must stay stable.
    ArenaBuffer(const ArenaBuffer&)            = delete;
    ArenaBuffer& operator=(const ArenaBuffer&) = delete;

    // Allocate `n` bytes with alignment `align`.
    // Throws std::bad_alloc if the arena is exhausted.
    void* allocate(std::size_t n, std::size_t align) {
        void*       p     = ptr_;
        std::size_t space = static_cast<std::size_t>(buf_ + N - ptr_);

        if (!std::align(align, n, p, space))
            throw std::bad_alloc{};

        ptr_ = static_cast<char*>(p) + n;
        return p;
    }

    // Deallocation is a no-op unless it's the very last allocation,
    // in which case we reclaim it (LIFO fast-path).
    void deallocate(void* p, std::size_t n) noexcept {
        if (static_cast<char*>(p) + n == ptr_)
            ptr_ = static_cast<char*>(p);
    }

    // Reset the arena — all previous allocations are invalidated.
    void reset() noexcept { ptr_ = buf_; }

    // How many bytes are still available (approximate, before alignment).
    std::size_t remaining() const noexcept {
        return static_cast<std::size_t>(buf_ + N - ptr_);
    }

    std::size_t capacity() const noexcept { return N; }

    // Give the allocator template access to the arena pointer for rebind.
    template <typename T2, std::size_t N2>
    friend class ArenaAllocator;

private:
    alignas(std::max_align_t) char buf_[N];
    char* ptr_;
};

// ===========================================================================
// ArenaAllocator<T, N>  — STL-compatible allocator over ArenaBuffer<N>
// ===========================================================================
template <typename T, std::size_t N>
class ArenaAllocator {
public:
    using value_type = T;

    // rebind struct — required so containers can allocate their internal
    // node types (e.g. std::list<T> allocates _List_node<T>, not T directly).
    template <typename U>
    struct rebind { using other = ArenaAllocator<U, N>; };

    explicit ArenaAllocator(ArenaBuffer<N>& arena) noexcept
        : arena_(&arena) {}

    // Templated copy constructor for rebind.
    template <typename U>
    ArenaAllocator(const ArenaAllocator<U, N>& other) noexcept
        : arena_(other.arena_) {}

    T* allocate(std::size_t n) {
        return static_cast<T*>(arena_->allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T* p, std::size_t n) noexcept {
        arena_->deallocate(p, n * sizeof(T));
    }

    template <typename U>
    bool operator==(const ArenaAllocator<U, N>& o) const noexcept {
        return arena_ == o.arena_;
    }
    template <typename U>
    bool operator!=(const ArenaAllocator<U, N>& o) const noexcept {
        return arena_ != o.arena_;
    }

    // Make the arena pointer accessible for the rebind copy constructor.
    template <typename T2, std::size_t N2>
    friend class ArenaAllocator;

private:
    ArenaBuffer<N>* arena_;
};

// ---------------------------------------------------------------------------
// Demo A – vector backed by an arena (no heap calls)
// ---------------------------------------------------------------------------
void demo_arena_vector() {
    section("A. std::vector backed by ArenaBuffer (no heap)");

    constexpr std::size_t ARENA_SIZE = 1024; // 1 KB on the stack
    ArenaBuffer<ARENA_SIZE> arena;

    using IntAlloc = ArenaAllocator<int, ARENA_SIZE>;
    std::vector<int, IntAlloc> v{IntAlloc{arena}};

    v.reserve(8); // one arena allocation
    std::cout << "After reserve(8): arena remaining = "
              << arena.remaining() << " bytes\n";

    for (int i = 1; i <= 8; ++i) v.push_back(i * 10);

    std::cout << "Vector contents:";
    for (int x : v) std::cout << " " << x;
    std::cout << "\n";

    std::cout << "Arena remaining after fills: "
              << arena.remaining() << " bytes\n";
}

// ---------------------------------------------------------------------------
// Demo B – list backed by an arena
// ---------------------------------------------------------------------------
void demo_arena_list() {
    section("B. std::list backed by ArenaBuffer");

    constexpr std::size_t ARENA_SIZE = 2048;
    ArenaBuffer<ARENA_SIZE> arena;

    using StrAlloc = ArenaAllocator<std::string, ARENA_SIZE>;
    std::list<std::string, StrAlloc> lst{StrAlloc{arena}};

    lst.push_back("arena");
    lst.push_back("allocator");
    lst.push_back("lesson");

    std::cout << "List contents:";
    for (auto& s : lst) std::cout << " [" << s << "]";
    std::cout << "\n";
}

// ---------------------------------------------------------------------------
// Demo C – arena reset and reuse
// ---------------------------------------------------------------------------
void demo_arena_reset() {
    section("C. Arena reset — reuse the same buffer for a second round");

    constexpr std::size_t ARENA_SIZE = 512;
    ArenaBuffer<ARENA_SIZE> arena;
    using IntAlloc = ArenaAllocator<int, ARENA_SIZE>;

    {
        std::vector<int, IntAlloc> v{IntAlloc{arena}};
        v.reserve(10);
        for (int i = 0; i < 10; ++i) v.push_back(i);
        std::cout << "Round 1 sum: ";
        int sum = 0;
        for (int x : v) sum += x;
        std::cout << sum << "\n";
    } // v is destroyed but arena memory is NOT returned to the OS

    // Reset arena to reuse the exact same buffer for round 2.
    arena.reset();
    std::cout << "Arena reset.  Remaining: " << arena.remaining() << " B\n";

    {
        std::vector<int, IntAlloc> v{IntAlloc{arena}};
        v.reserve(10);
        for (int i = 10; i < 20; ++i) v.push_back(i);
        std::cout << "Round 2 sum: ";
        int sum = 0;
        for (int x : v) sum += x;
        std::cout << sum << "\n";
    }
}

// ---------------------------------------------------------------------------
// Demo D – std::pmr::monotonic_buffer_resource (standard C++17 equivalent)
// ---------------------------------------------------------------------------
// The standard library provides this out of the box.  It is the same idea
// as our ArenaBuffer but works with std::pmr containers.
void demo_pmr_monotonic() {
    section("D. std::pmr::monotonic_buffer_resource (C++17 standard arena)");

    char stack_buf[2048];
    std::pmr::monotonic_buffer_resource pool{stack_buf, sizeof(stack_buf)};

    std::pmr::vector<int> v{&pool};
    v.reserve(8);
    for (int i = 1; i <= 8; ++i) v.push_back(i * 5);

    std::cout << "pmr::vector contents:";
    for (int x : v) std::cout << " " << x;
    std::cout << "\n";

    // Works with any pmr container, same pool.
    std::pmr::list<std::string> lst{&pool};
    lst.push_back("same");
    lst.push_back("pool");

    std::cout << "pmr::list contents:";
    for (auto& s : lst) std::cout << " [" << s << "]";
    std::cout << "\n";
}

// ---------------------------------------------------------------------------
int main() {
    std::cout << "======================================================\n";
    std::cout << "  Lesson 3 – Arena / pool allocator\n";
    std::cout << "======================================================\n";

    demo_arena_vector();
    demo_arena_list();
    demo_arena_reset();
    demo_pmr_monotonic();

    std::cout << "\nDone.\n";
    return 0;
}
