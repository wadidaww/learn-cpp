// 05_combining_containers.cpp
//
// Lesson 5 – Combining an allocator / memory resource across containers
//
// This is where allocators become truly powerful: multiple heterogeneous
// containers can all draw from the *same* memory pool, giving you:
//  - A single allocation budget for a whole sub-system.
//  - Zero fragmentation between containers.
//  - One-shot cleanup: destroy the pool and everything is freed.
//
// Patterns shown:
//  A. Classic allocator rebind  – sharing one ArenaBuffer with vector + list
//                                 (compile-time, no virtual dispatch)
//  B. std::pmr  – sharing one memory_resource with vector, list, map,
//                  and strings                                 (C++17)
//  C. pmr containers inside pmr containers  (e.g. vector of pmr::strings)
//  D. Scoped arena  – building a temporary sub-graph with its own pool and
//                      destroying everything at once with pool.release()

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <memory_resource>
#include <set>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------
static void section(const char* title) {
    std::cout << "\n=== " << title << " ===\n";
}

// ---------------------------------------------------------------------------
// Re-use the ArenaBuffer / ArenaAllocator from lesson 3
// (inlined here so this file is self-contained)
// ---------------------------------------------------------------------------
template <std::size_t N>
class ArenaBuffer {
public:
    ArenaBuffer() noexcept : ptr_(buf_) {}
    ArenaBuffer(const ArenaBuffer&)            = delete;
    ArenaBuffer& operator=(const ArenaBuffer&) = delete;

    void* allocate(std::size_t n, std::size_t align) {
        void*       p     = ptr_;
        std::size_t space = static_cast<std::size_t>(buf_ + N - ptr_);
        if (!std::align(align, n, p, space)) throw std::bad_alloc{};
        ptr_ = static_cast<char*>(p) + n;
        return p;
    }

    void deallocate(void* p, std::size_t n) noexcept {
        if (static_cast<char*>(p) + n == ptr_)
            ptr_ = static_cast<char*>(p);
    }

    void reset() noexcept { ptr_ = buf_; }

    std::size_t remaining() const noexcept {
        return static_cast<std::size_t>(buf_ + N - ptr_);
    }

    template <typename T2, std::size_t N2>
    friend class ArenaAllocator;

private:
    alignas(std::max_align_t) char buf_[N];
    char* ptr_;
};

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

    template <typename U>
    ArenaAllocator(const ArenaAllocator<U, N>& o) noexcept
        : arena_(o.arena_) {}

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

    template <typename T2, std::size_t N2>
    friend class ArenaAllocator;

private:
    ArenaBuffer<N>* arena_;
};

// ===========================================================================
// A. Classic allocator rebind – vector + list from the same ArenaBuffer
// ===========================================================================
void demo_classic_shared_arena() {
    section("A. Classic allocator rebind: vector + list from one ArenaBuffer");

    constexpr std::size_t ARENA = 4096;
    ArenaBuffer<ARENA> arena;

    std::cout << "Arena capacity: " << arena.remaining() << " B\n";

    // ── vector of ints ───────────────────────────────────────────────────
    using IntAlloc = ArenaAllocator<int, ARENA>;
    std::vector<int, IntAlloc> ints{IntAlloc{arena}};
    ints.reserve(8);
    for (int i = 1; i <= 8; ++i) ints.push_back(i * 10);
    std::cout << "vector<int>:";
    for (int x : ints) std::cout << " " << x;
    std::cout << "\n";

    // ── list of doubles — SAME arena ─────────────────────────────────────
    using DblAlloc = ArenaAllocator<double, ARENA>;
    std::list<double, DblAlloc> dbls{DblAlloc{arena}};
    dbls.push_back(1.1);
    dbls.push_back(2.2);
    dbls.push_back(3.3);
    std::cout << "list<double>:";
    for (double x : dbls) std::cout << " " << x;
    std::cout << "\n";

    std::cout << "Arena remaining: " << arena.remaining() << " B\n";
}

// ===========================================================================
// B. std::pmr — vector, list, map, string from one memory_resource
// ===========================================================================
void demo_pmr_shared_pool() {
    section("B. std::pmr: vector + list + map + string from one pool");

    char buf[8192];
    std::pmr::monotonic_buffer_resource pool{buf, sizeof(buf)};

    // ── vector<int> ──────────────────────────────────────────────────────
    std::pmr::vector<int> ints{&pool};
    ints.reserve(6);
    for (int i = 1; i <= 6; ++i) ints.push_back(i * 100);

    // ── list<double> ─────────────────────────────────────────────────────
    std::pmr::list<double> dbls{&pool};
    for (double d : {0.1, 0.2, 0.3}) dbls.push_back(d);

    // ── map<int, string> ─────────────────────────────────────────────────
    std::pmr::map<int, std::pmr::string> m{&pool};
    // The map's polymorphic_allocator propagates to each pmr::string value
    // automatically, so we only need to pass the string literal.
    m.emplace(1, "one");
    m.emplace(2, "two");
    m.emplace(3, "three");

    // ── print ─────────────────────────────────────────────────────────────
    std::cout << "ints:";
    for (int x : ints) std::cout << " " << x;
    std::cout << "\n";

    std::cout << "dbls:";
    for (double x : dbls) std::cout << " " << x;
    std::cout << "\n";

    std::cout << "map:";
    for (auto& [k, v] : m) std::cout << " {" << k << ":" << v << "}";
    std::cout << "\n";
}

// ===========================================================================
// C. pmr containers *inside* pmr containers
//    std::pmr::vector< std::pmr::string > — both from the same pool
// ===========================================================================
void demo_nested_pmr() {
    section("C. Nested pmr containers: vector of pmr::strings");

    char buf[4096];
    std::pmr::monotonic_buffer_resource pool{buf, sizeof(buf)};

    // The outer vector AND each inner string all draw from `pool`.
    std::pmr::vector<std::pmr::string> words{&pool};

    // When inserting into a pmr container the container's polymorphic_allocator
    // is automatically propagated to each element's constructor (uses_allocator
    // construction), so we just pass the string data — no extra &pool needed.
    const char* raw_words[] = {"allocator", "arena", "pool",
                                "resource",  "pmr",   "lesson"};
    for (const char* w : raw_words)
        words.emplace_back(w);

    std::cout << "Nested pmr words:";
    for (auto& s : words) std::cout << " [" << s << "]";
    std::cout << "\n";
}

// ===========================================================================
// D. Scoped arena — a temporary processing stage with its own lifetime
// ===========================================================================
// Pattern: create a pool for a phase, do work, then call pool.release() or
// let it go out of scope to free everything in one shot.
// ===========================================================================
void process_batch(std::pmr::memory_resource* pool,
                   const std::vector<int>& raw) {
    // All scratch data lives in `pool`.  When this function returns (and the
    // caller calls pool.release()), all memory is returned immediately.
    std::pmr::vector<int>    sorted{raw.begin(), raw.end(), pool};
    std::pmr::set<int>       unique_vals{raw.begin(), raw.end(), pool};
    std::pmr::map<int, int>  freq{pool};
    for (int v : raw) ++freq[v];

    std::sort(sorted.begin(), sorted.end());
    std::cout << "sorted first 5:";
    for (int i = 0; i < 5 && i < (int)sorted.size(); ++i)
        std::cout << " " << sorted[i];
    std::cout << " ...\n";
    std::cout << "unique values: " << unique_vals.size() << "\n";
    std::cout << "most frequent: ";
    int best_k = 0, best_c = 0;
    for (auto& [k, c] : freq) if (c > best_c) { best_k = k; best_c = c; }
    std::cout << best_k << " (×" << best_c << ")\n";
}

void demo_scoped_arena() {
    section("D. Scoped arena — temporary processing pool");

    // Simulate a batch of input data (lives on the regular heap).
    std::vector<int> data;
    data.reserve(200);
    for (int i = 0; i < 200; ++i) data.push_back((i * 37 + 5) % 20);

    // Scratch arena sized for the batch — no heap calls inside process_batch.
    char scratch[16384];
    std::pmr::monotonic_buffer_resource phase_pool{scratch, sizeof(scratch),
                                                   std::pmr::null_memory_resource()};
    process_batch(&phase_pool, data);

    // Release all scratch memory at once.
    phase_pool.release();
    std::cout << "Phase pool released — all scratch memory freed in O(1).\n";
}

// ---------------------------------------------------------------------------
int main() {
    std::cout << "======================================================\n";
    std::cout << "  Lesson 5 – Combining allocators across containers\n";
    std::cout << "======================================================\n";

    demo_classic_shared_arena();
    demo_pmr_shared_pool();
    demo_nested_pmr();
    demo_scoped_arena();

    std::cout << "\nDone.\n";
    return 0;
}
