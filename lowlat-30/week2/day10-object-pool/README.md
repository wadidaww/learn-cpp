# Day 10 · Allocation: why `new` is banned on the hot path, pools and arenas

**Why trading firms care:** an order book creates and destroys an order object for every add and cancel, millions per second. `new`/`delete` there gives unpredictable tail latency. Every low-latency codebase has a pool. "Why not malloc?" and "design an allocator for orders" are common questions.

## 1. Concepts (45 min)

### What malloc costs
A modern allocator (glibc ptmalloc, jemalloc, tcmalloc, mimalloc) usually takes ~20–50 ns from a thread cache. The problem is the **tail**:
- Thread cache empty: go to a shared arena, maybe take a **lock**.
- Arena empty: **syscall** (`brk`/`mmap`), then **page faults** when the new memory is first touched (µs each).
- `free` may consolidate chunks or return memory to the OS (`madvise`, `munmap`).
- Memory from the allocator may be cold (not in cache) and scattered (bad locality).
- Fragmentation over a long trading day.
So the mean is fine and the p99.9 is terrible. Firms allocate everything at startup and never call malloc on the hot path. Many check this with a malloc hook that aborts if called after startup.

### Fixed-size object pool with an intrusive free list
All slots are the same size, so no searching, no headers, no fragmentation:
```
slots:  [ used ][ free ]-->[ free ]-->[ free ]--> -1
               ^ free_head_
```
- Each free slot stores the **index of the next free slot inside itself** (intrusive: no extra memory). Use a union or raw bytes.
- `create`: pop the head of the free list, construct the object there. O(1), a few instructions.
- `destroy`: run the destructor, push the slot onto the head. O(1).
- **LIFO reuse is a feature:** the slot you just freed is still in L1, so the next order lands in hot cache.
- Use 32-bit indices instead of pointers when you can: half the size, and the same pool works if relocated or shared.
- Pre-touch the whole pool at startup (construct the free list by writing every slot) so the hot path never page-faults.

### Arenas (bump allocators)
Allocate by bumping a pointer; free everything at once (`reset()`). Perfect for per-message scratch space. `std::pmr::monotonic_buffer_resource` is the standard version; `std::pmr::vector` can use it so container code stays normal.

### Placement new and friends (review from day 1)
`std::construct_at(p, args...)` starts a lifetime in storage you own, `std::destroy_at(p)` ends it. A pool is exactly that plus a free list.

### Other hot-path allocation traps
- `std::string` beyond the small-string buffer (~15 chars in libstdc++) allocates.
- `std::function` with a large capture allocates.
- `std::vector::push_back` past capacity reallocates: `reserve` at startup.
- `std::map`/`std::unordered_map` allocate per node. Use flat or open-addressing maps with preallocated capacity.
- Exceptions allocate the exception object.
- `std::shared_ptr` allocates a control block (unless `make_shared`), and its refcount is an atomic RMW on every copy.

## 2. Exercise (2 h): `ll::ObjectPool<T, N>`

In `solution.hpp`:
- `create(args...)` returns `T*`, or `nullptr` when the pool is exhausted.
- `destroy(T*)` destroys the object and returns its slot.
- `available()`, `capacity()`, `owns(p)`.
- Intrusive free list of 32-bit indices, LIFO reuse, no heap after construction, correct alignment, works for types with no default constructor.
- The pool's destructor does **not** destroy live objects (document why: it does not know which slots are live without extra memory; the owner must destroy them first). The tests respect this.

Gotcha we hit while checking this exercise: GCC 13 silently ignores the first of two `alignas` on a class that depends on a template parameter (`struct alignas(T) alignas(std::uint32_t) Slot`). Clang honours both. Write one `alignas(std::max(alignof(T), alignof(std::uint32_t)))` instead. The alignment test catches it.

**Done when:** tests pass under ASan+UBSan, and `bench` shows the difference in **p99.9**, not just the mean. Explain why.

## 3. Say it out loud

<details><summary>1. Why not call new on the hot path?</summary>
The average is OK but the tail is not: possible locks, syscalls, page faults on fresh memory, cold or scattered memory, fragmentation. HFT code preallocates at startup and uses pools and arenas so every hot-path allocation is a few instructions.
</details>

<details><summary>2. How does an intrusive free list work?</summary>
Each free slot stores the index (or pointer) of the next free slot in its own bytes. Allocation pops the head; free pushes onto the head. No extra memory and O(1).
</details>

<details><summary>3. Why LIFO reuse?</summary>
The most recently freed slot is likely still in L1/L2, so the next object is written into hot cache. FIFO reuse would walk through the whole pool and touch cold memory.
</details>

<details><summary>4. Indices or pointers in the free list?</summary>
32-bit indices: half the size of pointers (more per cache line), relocatable, and they can be shared across processes in shared memory. Pointers save one add.
</details>

<details><summary>5. What allocates silently in C++?</summary>
std::string over the SSO size, std::function with big captures, vector growth, node-based containers, exceptions, shared_ptr control blocks, iostreams, std::regex, and std::async.
</details>

<details><summary>6. What is std::pmr?</summary>
Polymorphic memory resources: containers take a memory_resource* at runtime, so you can back a std::pmr::vector with a monotonic arena or a pool without changing its type for every allocator.
</details>

<details><summary>7. How would you catch accidental hot-path allocations?</summary>
Override operator new / malloc hooks to count or abort after startup (in tests and canaries), run under heaptrack or `perf trace -e brk,mmap`, and review code for the usual suspects.
</details>
