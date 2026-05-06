// 04_pmr_allocator.cpp
//
// Lesson 4 – std::pmr: Polymorphic Memory Resources (C++17)
//
// std::pmr decouples containers from allocator types by using a
// *runtime-polymorphic* memory_resource base class instead of a
// compile-time template parameter.  Every pmr container is just:
//
//   std::pmr::vector<T>  ≡  std::vector<T, std::pmr::polymorphic_allocator<T>>
//
// You swap the memory strategy at runtime by passing a different
// memory_resource pointer — no template changes, no code duplication.
//
// This file demonstrates:
//  1. monotonic_buffer_resource  – bump-pointer arena, no dealloc
//  2. unsynchronized_pool_resource – pools by size class, dealloc works
//  3. synchronized_pool_resource – thread-safe version
//  4. null_memory_resource – throws bad_alloc; useful to prove "no heap"
//  5. Chaining resources (fallback chain)
//  6. Custom memory_resource     – implement your own logging resource

#include <cstddef>
#include <iostream>
#include <list>
#include <map>
#include <memory_resource>
#include <string>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------
static void section(const char* title) {
    std::cout << "\n=== " << title << " ===\n";
}

// ===========================================================================
// 1. monotonic_buffer_resource — fastest, no individual dealloc
// ===========================================================================
void demo_monotonic() {
    section("1. monotonic_buffer_resource — bump-pointer arena");

    // Use a stack buffer as the backing store.
    char buf[4096];
    std::pmr::monotonic_buffer_resource pool{buf, sizeof(buf)};

    std::pmr::vector<int> v{&pool};
    v.reserve(8);
    for (int i = 1; i <= 8; ++i) v.push_back(i * 100);

    std::cout << "vector:";
    for (int x : v) std::cout << " " << x;
    std::cout << "\n";

    // Allocation order within the same pool is sequential — great locality.
    std::pmr::string s{"hello from pmr string", &pool};
    std::cout << "string: " << s << "\n";

    // Releasing individual allocations is a no-op with monotonic.
    // The whole pool is freed at once when `pool` goes out of scope.
    // To reuse the buffer manually call: pool.release();
}

// ===========================================================================
// 2. unsynchronized_pool_resource — pools by size class, dealloc works
// ===========================================================================
void demo_pool_resource() {
    section("2. unsynchronized_pool_resource — size-class pools");

    // Falls back to new_delete_resource when its internal pools are full.
    std::pmr::unsynchronized_pool_resource pool;

    std::pmr::vector<double> v{&pool};
    for (int i = 0; i < 16; ++i) v.push_back(i * 1.5);

    // Individual deallocations work here — removed elements actually free.
    v.clear();
    v.shrink_to_fit();

    // map allocates one node per element — a good fit for pool_resource.
    std::pmr::map<int, std::pmr::string> m{&pool};
    m[1] = "one";
    m[2] = "two";
    m[3] = "three";

    std::cout << "map entries:";
    for (auto& [k, val] : m) std::cout << " {" << k << ": " << val << "}";
    std::cout << "\n";
}

// ===========================================================================
// 3. synchronized_pool_resource — thread-safe version of pool_resource
// ===========================================================================
void demo_synchronized_pool() {
    section("3. synchronized_pool_resource — shared across threads");

    std::pmr::synchronized_pool_resource pool;

    constexpr int NTHREADS = 4;
    constexpr int N        = 100;

    std::vector<std::thread> threads;
    threads.reserve(NTHREADS);

    for (int t = 0; t < NTHREADS; ++t) {
        threads.emplace_back([&pool, t]() {
            std::pmr::vector<int> local{&pool};
            local.reserve(N);
            for (int i = 0; i < N; ++i) local.push_back(t * 1000 + i);
            // local destroyed here → dealloc goes back to pool (thread-safe)
        });
    }

    for (auto& th : threads) th.join();
    std::cout << NTHREADS << " threads each pushed " << N
              << " ints into per-thread pmr::vectors sharing one pool.\n";
}

// ===========================================================================
// 4. null_memory_resource — throws bad_alloc; proves "no heap"
// ===========================================================================
void demo_null_resource() {
    section("4. null_memory_resource — guaranteed no fallback to heap");

    char buf[256];
    // Chain: monotonic → null (throws if buf runs out, no heap fallback).
    std::pmr::monotonic_buffer_resource pool{buf, sizeof(buf),
                                             std::pmr::null_memory_resource()};

    std::pmr::vector<int> v{&pool};
    v.reserve(4);
    v.push_back(1);
    v.push_back(2);
    std::cout << "Allocated fine within the 256-byte stack buffer.\n";

    try {
        // Try to allocate more than the buffer can hold.
        v.reserve(10000);
        std::cout << "ERROR: should have thrown!\n";
    } catch (const std::bad_alloc&) {
        std::cout << "Caught std::bad_alloc as expected "
                     "(null_memory_resource refused the fallback).\n";
    }
}

// ===========================================================================
// 5. Resource chain: buffer → pool → new_delete (the default)
// ===========================================================================
void demo_chain() {
    section("5. Resource chain: stack buffer → pool → heap");

    // Small stack buffer — fine for small workloads.
    char small_buf[128];

    // If the stack buffer fills up, fall through to an unsynchronized pool,
    // which itself falls back to new_delete_resource by default.
    std::pmr::unsynchronized_pool_resource pool_fallback;
    std::pmr::monotonic_buffer_resource    pool{small_buf, sizeof(small_buf),
                                                &pool_fallback};

    std::pmr::vector<int> v{&pool};
    for (int i = 0; i < 64; ++i) v.push_back(i);
    // First elements come from stack_buf; once it's full the pool kicks in.

    std::cout << "Pushed 64 ints through a chained resource. "
                 "Last value: " << v.back() << "\n";
}

// ===========================================================================
// 6. Custom memory_resource — implement your own
// ===========================================================================
// Derive from std::pmr::memory_resource and override three methods:
//   do_allocate(bytes, alignment)
//   do_deallocate(p, bytes, alignment)
//   do_is_equal(other)
class LoggingResource : public std::pmr::memory_resource {
public:
    explicit LoggingResource(std::pmr::memory_resource* upstream =
                                 std::pmr::new_delete_resource(),
                             const char* name = "LoggingResource")
        : upstream_(upstream), name_(name) {}

    std::size_t total_allocated()   const noexcept { return total_alloc_;   }
    std::size_t total_deallocated() const noexcept { return total_dealloc_; }
    std::size_t live_bytes()        const noexcept {
        return total_alloc_ - total_dealloc_;
    }

protected:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        void* p = upstream_->allocate(bytes, alignment);
        total_alloc_ += bytes;
        std::cout << "[" << name_ << "] allocate   "
                  << bytes << " B  →  " << p << "\n";
        return p;
    }

    void do_deallocate(void* p, std::size_t bytes,
                       std::size_t alignment) noexcept override {
        upstream_->deallocate(p, bytes, alignment);
        total_dealloc_ += bytes;
        std::cout << "[" << name_ << "] deallocate " << bytes
                  << " B  @  " << p << "\n";
    }

    bool do_is_equal(const std::pmr::memory_resource& other)
        const noexcept override {
        return this == &other;
    }

private:
    std::pmr::memory_resource* upstream_;
    const char*                name_;
    std::size_t                total_alloc_   = 0;
    std::size_t                total_dealloc_ = 0;
};

void demo_custom_resource() {
    section("6. Custom memory_resource (LoggingResource)");

    LoggingResource logger{std::pmr::new_delete_resource(), "Logger"};

    {
        std::pmr::vector<int> v{&logger};
        v.reserve(4);
        v.push_back(10);
        v.push_back(20);
        v.push_back(30);
        std::cout << "contents:";
        for (int x : v) std::cout << " " << x;
        std::cout << "\n";
    } // destructor → deallocate

    std::cout << "Total allocated:   " << logger.total_allocated()   << " B\n";
    std::cout << "Total deallocated: " << logger.total_deallocated() << " B\n";
    std::cout << "Live bytes:        " << logger.live_bytes()        << " B\n";
}

// ---------------------------------------------------------------------------
int main() {
    std::cout << "======================================================\n";
    std::cout << "  Lesson 4 – std::pmr Polymorphic Memory Resources\n";
    std::cout << "======================================================\n";

    demo_monotonic();
    demo_pool_resource();
    demo_synchronized_pool();
    demo_null_resource();
    demo_chain();
    demo_custom_resource();

    std::cout << "\nDone.\n";
    return 0;
}
