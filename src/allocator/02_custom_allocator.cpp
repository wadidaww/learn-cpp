// 02_custom_allocator.cpp
//
// Lesson 2 – Writing a custom allocator
//
// A custom allocator must satisfy the C++11 "Allocator" named requirements.
// The minimum interface is:
//   - value_type typedef
//   - allocate(n)
//   - deallocate(p, n)
//   - templated copy-constructor (for rebind)
//   - operator== / operator!=
//
// We build two allocators here:
//  A. MinimalAllocator – the absolute minimum required by the standard.
//  B. TrackingAllocator – wraps global new/delete and prints every call,
//     a technique commonly used for debugging or profiling memory usage.

#include <cstddef>
#include <iostream>
#include <list>
#include <map>
#include <new>      // ::operator new / ::operator delete
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------
static void section(const char* title) {
    std::cout << "\n=== " << title << " ===\n";
}

// ===========================================================================
// A. MinimalAllocator<T>
// ===========================================================================
// This is the smallest possible conforming C++ allocator.
// It just delegates directly to ::operator new / ::operator delete.
// ---------------------------------------------------------------------------
template <typename T>
struct MinimalAllocator {
    // ── Required typedef ────────────────────────────────────────────────────
    using value_type = T;

    // ── Required constructors ───────────────────────────────────────────────
    MinimalAllocator() = default;

    // Templated copy constructor — needed so a container can rebind us to a
    // different type (e.g. its internal node type) and still construct us.
    template <typename U>
    MinimalAllocator(const MinimalAllocator<U>&) noexcept {}

    // ── Required: allocate / deallocate ─────────────────────────────────────
    T* allocate(std::size_t n) {
        if (n > static_cast<std::size_t>(-1) / sizeof(T))
            throw std::bad_alloc{};
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t /*n*/) noexcept {
        ::operator delete(p);
    }

    // ── Equality ────────────────────────────────────────────────────────────
    // Two MinimalAllocators are always equal because they use the same heap.
    bool operator==(const MinimalAllocator&) const noexcept { return true; }
    bool operator!=(const MinimalAllocator&) const noexcept { return false; }
};

// ---------------------------------------------------------------------------
void demo_minimal_allocator() {
    section("A. MinimalAllocator – absolute minimum custom allocator");

    // Use it directly with a container.
    std::vector<int, MinimalAllocator<int>> v;
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);

    std::cout << "vector contents:";
    for (int x : v) std::cout << " " << x;
    std::cout << "\n";

    std::list<std::string, MinimalAllocator<std::string>> lst;
    lst.push_back("alpha");
    lst.push_back("beta");

    std::cout << "list contents:";
    for (auto& s : lst) std::cout << " [" << s << "]";
    std::cout << "\n";
}

// ===========================================================================
// B. TrackingAllocator<T>
// ===========================================================================
// Wraps global new/delete and logs every allocation and deallocation.
// Useful for debugging or confirming that a container actually makes the
// number of heap calls you expect.
// ---------------------------------------------------------------------------
template <typename T>
struct TrackingAllocator {
    using value_type = T;

    TrackingAllocator() = default;

    template <typename U>
    TrackingAllocator(const TrackingAllocator<U>&) noexcept {}

    T* allocate(std::size_t n) {
        std::size_t bytes = n * sizeof(T);
        void* p = ::operator new(bytes);
        std::cout << "[TrackingAllocator] allocate   "
                  << n << " × " << sizeof(T) << " B"
                  << "  =  " << bytes << " B"
                  << "  →  " << p << "\n";
        return static_cast<T*>(p);
    }

    void deallocate(T* p, std::size_t n) noexcept {
        std::size_t bytes = n * sizeof(T);
        std::cout << "[TrackingAllocator] deallocate " << bytes << " B"
                  << "  @  " << static_cast<void*>(p) << "\n";
        ::operator delete(p);
    }

    bool operator==(const TrackingAllocator&) const noexcept { return true; }
    bool operator!=(const TrackingAllocator&) const noexcept { return false; }
};

// ---------------------------------------------------------------------------
void demo_tracking_allocator() {
    section("B. TrackingAllocator – see every heap call a container makes");

    std::cout << "\n-- std::vector<int> with TrackingAllocator --\n";
    {
        std::vector<int, TrackingAllocator<int>> v;
        // Reserve upfront to see exactly one allocation instead of
        // the typical doubling re-allocations.
        v.reserve(4);
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
        v.push_back(4);
        std::cout << "contents:";
        for (int x : v) std::cout << " " << x;
        std::cout << "\n";
    } // vector destructor → deallocate

    std::cout << "\n-- std::map<int,int> with TrackingAllocator --\n";
    {
        // std::map internally allocates one node per element.
        using MapAlloc = TrackingAllocator<std::pair<const int, int>>;
        std::map<int, int, std::less<int>, MapAlloc> m;
        m[1] = 100;
        m[2] = 200;
        m[3] = 300;
        std::cout << "map contents:";
        for (auto& [k, v] : m) std::cout << " {" << k << ":" << v << "}";
        std::cout << "\n";
    } // map destructor → deallocate per node
}

// ---------------------------------------------------------------------------
int main() {
    std::cout << "======================================================\n";
    std::cout << "  Lesson 2 – Writing a custom allocator\n";
    std::cout << "======================================================\n";

    demo_minimal_allocator();
    demo_tracking_allocator();

    std::cout << "\nDone.\n";
    return 0;
}
