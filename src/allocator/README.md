# C++ Allocators — from basics to `std::pmr`

This directory teaches C++ memory allocators from the ground up through five
progressive lessons, each in its own standalone executable.

---

## Table of Contents

1. [What is an Allocator?](#what-is-an-allocator)
2. [Lessons Overview](#lessons-overview)
3. [Quick Comparison Table](#quick-comparison-table)
4. [Build Instructions](#build-instructions)
5. [Running the Examples](#running-the-examples)
6. [Key Concepts Reference](#key-concepts-reference)
7. [Best Practices](#best-practices)

---

## What is an Allocator?

Every STL container (`vector`, `list`, `map`, …) has an **Allocator** template
parameter that controls how it obtains and releases raw memory.  The default
is `std::allocator<T>`, which calls `::operator new`/`::operator delete`.

By swapping the allocator you can:

- Use a **stack buffer** instead of the heap (zero heap calls).
- **Track** every allocation for debugging or profiling.
- **Pool** small objects to reduce fragmentation.
- Share a **single memory budget** across many containers.
- **Reset** all memory in O(1) at the end of a phase.

---

## Lessons Overview

### Lesson 1 — `01_std_allocator.cpp`

> **Using `std::allocator` directly**

Covers the lifecycle every container follows internally:

```
allocate(n)           → raw memory (no constructors)
construct_at(p, args) → construct object in-place
destroy_at(p)         → call destructor (no free)
deallocate(p, n)      → return raw memory
```

Also introduces `std::allocator_traits`, the uniform adaptor layer that all
containers use instead of calling the allocator directly.

---

### Lesson 2 — `02_custom_allocator.cpp`

> **Writing a custom allocator**

Builds two custom allocators from scratch:

| Allocator | What it does |
|---|---|
| `MinimalAllocator<T>` | Absolute minimum: just `value_type`, `allocate`, `deallocate`, and the rebind copy-constructor. |
| `TrackingAllocator<T>` | Wraps `::operator new`/`delete` and prints every call — useful for debugging and counting heap calls. |

Key rule: a container may **rebind** your allocator to its own node type.
Your allocator must provide a templated copy-constructor to support this.

---

### Lesson 3 — `03_arena_allocator.cpp`

> **Arena / pool allocator — zero heap calls**

Introduces the **bump-pointer** pattern:

```
┌─────────────────────────────────┐
│  ArenaBuffer<N>  (stack array)  │
│  ├── [alloc A] [alloc B] [    ] │
│  └── ptr ──────────────^        │
└─────────────────────────────────┘
```

Each `allocate()` just advances the pointer — O(1) and cache-friendly.
`reset()` reclaims everything in a single pointer assignment.

Also shows the standard C++17 equivalent: `std::pmr::monotonic_buffer_resource`.

---

### Lesson 4 — `04_pmr_allocator.cpp`

> **`std::pmr` — Polymorphic Memory Resources (C++17)**

`std::pmr` is the standard solution.  All pmr containers share the same
type (`std::pmr::vector<T>`) and accept a `memory_resource*` at runtime —
no template changes needed to swap strategies.

| Resource | Behaviour |
|---|---|
| `monotonic_buffer_resource` | Bump-pointer arena, dealloc is a no-op. |
| `unsynchronized_pool_resource` | Pools by size class, individual dealloc works. |
| `synchronized_pool_resource` | Thread-safe pool. |
| `null_memory_resource` | Always throws `bad_alloc` — proves "no heap". |
| Custom `memory_resource` | Derive and override `do_allocate` / `do_deallocate`. |

Shows how to build a custom `LoggingResource` that tracks total bytes
allocated and deallocated.

---

### Lesson 5 — `05_combining_containers.cpp`

> **Sharing one allocator / pool across multiple containers**

The most practical lesson.  Demonstrates four patterns:

| Pattern | Technique |
|---|---|
| A. Classic rebind | One `ArenaBuffer` shared by `vector<int>` + `list<double>` |
| B. std::pmr pool | One `monotonic_buffer_resource` feeding `vector`, `list`, `map`, `string` |
| C. Nested pmr | `pmr::vector<pmr::string>` — outer *and* inner containers draw from the same pool |
| D. Scoped arena | Temporary phase arena, bulk-freed with `pool.release()` in O(1) |

---

## Quick Comparison Table

| Approach | Heap calls | Dealloc works | Thread-safe | C++ version |
|---|---|---|---|---|
| `std::allocator` (default) | Per object | ✓ | ✓ | C++98 |
| Custom allocator (lesson 2) | Configurable | Configurable | Depends | C++11 |
| Arena / bump-pointer | 0 (stack) | LIFO only | ✗ | C++11 |
| `monotonic_buffer_resource` | 0 (stack) | no-op | ✗ | C++17 |
| `unsynchronized_pool_resource` | Rare (refill) | ✓ | ✗ | C++17 |
| `synchronized_pool_resource` | Rare (refill) | ✓ | ✓ | C++17 |

---

## Build Instructions

```bash
# From the repository root
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -- \
    alloc_01_std        \
    alloc_02_custom     \
    alloc_03_arena      \
    alloc_04_pmr        \
    alloc_05_combining
```

Binaries are placed in `build/src/allocator/`.

**Requirements:**
- C++20 compiler (GCC 10+, Clang 10+) — for `std::construct_at`
- C++17 for `std::pmr`  
- CMake 3.10+

---

## Running the Examples

```bash
cd build/src/allocator

# Lesson 1 – std::allocator basics
./alloc_01_std

# Lesson 2 – Custom allocators (minimal + tracking)
./alloc_02_custom

# Lesson 3 – Arena / pool allocator
./alloc_03_arena

# Lesson 4 – std::pmr polymorphic resources
./alloc_04_pmr

# Lesson 5 – Combining multiple containers
./alloc_05_combining
```

---

## Key Concepts Reference

### The Allocator Named Requirements (C++11+)

```cpp
template <typename T>
struct MyAllocator {
    using value_type = T;                       // required

    MyAllocator() = default;
    template <typename U>                        // required for rebind
    MyAllocator(const MyAllocator<U>&) noexcept {}

    T*   allocate(std::size_t n);               // required
    void deallocate(T* p, std::size_t n);       // required

    bool operator==(const MyAllocator&) const noexcept { return true; }
};
```

### `std::allocator_traits` — the container interface

```cpp
using Traits = std::allocator_traits<MyAllocator<T>>;

Traits::allocate(alloc, n)           // calls alloc.allocate(n)
Traits::deallocate(alloc, p, n)      // calls alloc.deallocate(p, n)
Traits::construct(alloc, p, args...) // placement-new if not overridden
Traits::destroy(alloc, p)            // calls p->~T() if not overridden
Traits::rebind_alloc<U>              // allocator for a different type
```

### The `std::pmr` hierarchy

```
std::pmr::memory_resource   (abstract base)
├── monotonic_buffer_resource
├── unsynchronized_pool_resource
├── synchronized_pool_resource
├── new_delete_resource()  ← global singleton
├── null_memory_resource() ← always throws
└── [your custom resource]
```

---

## Best Practices

| Practice | Reason |
|---|---|
| Prefer `std::pmr` over hand-rolled allocators | Less code, standard, works with all containers without recompilation. |
| Use `null_memory_resource` as fallback in tests | Guarantees no accidental heap use; crashes loudly if the arena is too small. |
| Size the arena before reserving | Estimate `n × sizeof(T)` + alignment padding; add 20 % headroom. |
| Reset instead of destroy for short-lived data | `monotonic_buffer_resource::release()` or `ArenaBuffer::reset()` is O(1). |
| Use `TrackingAllocator` / `LoggingResource` during development | Count heap calls to validate your arena is large enough. |
| Pass `memory_resource*` by pointer, not by value | The pointer is stable; the resource itself must outlive all containers using it. |
| Construct nested pmr containers with the same resource | `pmr::string` inside `pmr::vector` must receive the same `memory_resource*` or it will silently fall back to the heap. |
