// 01_std_allocator.cpp
//
// Lesson 1 – Using std::allocator directly
//
// std::allocator<T> is the default allocator used by every STL container.
// You rarely need to call it yourself, but understanding it reveals exactly
// what containers do under the hood: they separate memory acquisition from
// object construction, and object destruction from memory release.
//
// Key ideas:
//  - allocate(n)   → raw memory for n objects (no constructors called)
//  - deallocate(p, n) → release the raw memory (no destructors called)
//  - std::construct_at / std::destroy_at → construction / destruction (C++20)
//  - std::allocator_traits → the uniform adaptor layer used by containers

#include <iostream>
#include <memory>    // std::allocator, std::allocator_traits, std::construct_at
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helper: print section headers
// ---------------------------------------------------------------------------
static void section(const char* title) {
    std::cout << "\n=== " << title << " ===\n";
}

// ---------------------------------------------------------------------------
// Part 1 – Raw allocation and manual construction
// ---------------------------------------------------------------------------
void demo_raw_allocation() {
    section("Part 1: Raw allocation & manual construction");

    std::allocator<int> alloc;

    // Step 1: allocate raw memory for 5 ints — NO construction happens here.
    int* p = alloc.allocate(5);
    std::cout << "Allocated raw memory for 5 ints at " << p << "\n";

    // Step 2: construct objects in the raw memory using placement-new.
    // std::construct_at is the C++20 way; it is equivalent to:
    //   new (p + i) int(value);
    for (int i = 0; i < 5; ++i)
        std::construct_at(p + i, (i + 1) * 10);

    std::cout << "Constructed values:";
    for (int i = 0; i < 5; ++i)
        std::cout << " " << p[i];
    std::cout << "\n";

    // Step 3: destroy objects — destructor is called, memory is NOT freed yet.
    for (int i = 0; i < 5; ++i)
        std::destroy_at(p + i);

    // Step 4: return memory to the allocator.
    // IMPORTANT: the size passed to deallocate must match allocate.
    alloc.deallocate(p, 5);
    std::cout << "Memory deallocated.\n";
}

// ---------------------------------------------------------------------------
// Part 2 – std::allocator_traits: the uniform interface for containers
// ---------------------------------------------------------------------------
// Containers never call allocator methods directly.  They go through
// std::allocator_traits so that allocators that omit optional members
// (construct, destroy, max_size, …) still work correctly.
void demo_allocator_traits() {
    section("Part 2: std::allocator_traits");

    using Alloc  = std::allocator<std::string>;
    using Traits = std::allocator_traits<Alloc>;

    Alloc alloc;

    // Allocate space for 3 std::string objects.
    std::string* p = Traits::allocate(alloc, 3);

    // Construct them — Traits::construct calls alloc.construct if present,
    // otherwise falls back to placement-new (standard behaviour for
    // std::allocator).
    Traits::construct(alloc, p + 0, "hello");
    Traits::construct(alloc, p + 1, "world");
    Traits::construct(alloc, p + 2, "allocator");

    std::cout << "Strings via allocator_traits:";
    for (int i = 0; i < 3; ++i)
        std::cout << " [" << p[i] << "]";
    std::cout << "\n";

    // Destroy in reverse order (good practice for non-trivial types).
    for (int i = 2; i >= 0; --i)
        Traits::destroy(alloc, p + i);

    Traits::deallocate(alloc, p, 3);
    std::cout << "Strings destroyed and memory deallocated.\n";

    // rebind_alloc: containers need to allocate their *internal nodes* which
    // may differ from T.  Traits::rebind_alloc<U> gives an allocator for U.
    using NodeAlloc = Traits::rebind_alloc<int>;
    std::cout << "Rebind std::allocator<string> → std::allocator<int>: OK\n";
    (void)NodeAlloc{};
}

// ---------------------------------------------------------------------------
// Part 3 – Containers use std::allocator implicitly
// ---------------------------------------------------------------------------
void demo_container_default() {
    section("Part 3: Containers use std::allocator by default");

    // std::vector<int> is shorthand for std::vector<int, std::allocator<int>>.
    std::vector<int> v = {1, 2, 3, 4, 5};

    // We can get the allocator out of any container.
    auto alloc = v.get_allocator();
    std::cout << "Got allocator from vector. "
              << "Allocating 1 extra int with it...\n";

    int* extra = alloc.allocate(1);
    std::construct_at(extra, 99);
    std::cout << "Extra int value: " << *extra << "\n";
    std::destroy_at(extra);
    alloc.deallocate(extra, 1);
    std::cout << "Extra int cleaned up.\n";
}

// ---------------------------------------------------------------------------
int main() {
    std::cout << "======================================================\n";
    std::cout << "  Lesson 1 – std::allocator basics\n";
    std::cout << "======================================================\n";

    demo_raw_allocation();
    demo_allocator_traits();
    demo_container_default();

    std::cout << "\nDone.\n";
    return 0;
}
