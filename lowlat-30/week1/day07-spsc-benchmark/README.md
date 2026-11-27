# Day 7 · Measure your ring, review, Mock 1

## 1. Benchmark your day 5 ring (60 min)

`bench.cpp` includes your `../day05-spsc-ring/solution.hpp` and compares it with a mutex + `std::deque` queue.

```bash
cmake --build build --target day07_bench
lscpu -e                       # find two physical cores, and a hyperthread sibling pair
./build/day07_bench 0 1        # pinned to cpus 0 and 1
./build/day07_bench 0 <sibling-of-0>
./build/day07_bench -1 -1      # unpinned
```

Fill in this table in your own repo's README (never quote a number without the machine):

| Setup | Throughput (M/s) | Round trip p50 | p99 | p99.9 |
|---|---|---|---|---|
| SPSC, two physical cores | | | | |
| SPSC, hyperthread siblings | | | | |
| SPSC, unpinned | | | | |
| Mutex queue, two cores | | | | |

Then answer in writing:
1. Why are hyperthread siblings faster for round trip? (They share L1/L2, so the cache line never leaves the core.) Why would you still not put feed and strategy threads on siblings in production? (They compete for the same execution units, and one thread's work slows the other.)
2. Why is p99.9 so much higher than p50? Name three sources: interrupts, scheduler preemption, other processes, page faults, SMIs, frequency changes. Which ones would `isolcpus`/`nohz_full` remove? (Week 3 day 19.)
3. Remove the cached indices from your ring (always load the other side's index) and rerun. What changed and why?
4. Remove the `alignas(64)` and rerun. What changed and why?

**Numbers from a laptop or a cloud VM are noisy.** Label them as such.

## 2. Week 1 review (60 min)

Redo from memory, timed, on paper or a blank file:
- SPSC ring: 15 minutes, with memory orders and layout.
- TTAS spinlock: 3 minutes.
- Seqlock reader and writer: 8 minutes.
- FixedVector push_back / pop_back / move constructor: 10 minutes.

Check against your own solutions. Every mistake goes on your weak list.

## 3. Mock 1 question bank (in the thread: say "start mock 1")

The mock is 60 minutes: 20 minutes of rapid questions from this list, 25 minutes writing an SPSC ring while explaining, 15 minutes of follow-ups. Practise answering each in under 45 seconds.

**C++ core**
1. What is a value category? Give an example of an xvalue.
2. What does `std::forward` do and when do you need it?
3. What happens to implicit move operations if you declare a destructor?
4. Why must move constructors be `noexcept` for `std::vector`?
5. Name five kinds of undefined behaviour. Why does signed overflow UB help the optimiser?
6. How do you read a `uint32_t` from an arbitrary byte buffer correctly?
7. What is strict aliasing? Which types may alias anything?
8. What does `alignas` do? What goes wrong without it in a byte buffer?

**Polymorphism**
9. Walk through what a virtual call compiles to.
10. What is the real cost of a virtual call on the hot path?
11. What is CRTP? Write the skeleton.
12. How does `final` help performance?
13. variant/visit vs virtual: when would you pick each?

**Concurrency**
14. What is a data race? Is it UB?
15. Explain acquire/release with an example.
16. What does seq_cst add over acq_rel? Give a case that needs it.
17. What does each order compile to on x86? On ARM?
18. Why is TTAS faster than TAS?
19. What is false sharing? How would you detect it?
20. Why does an SPSC ring need no CAS?
21. Why cache the other side's index?
22. How do you make a ring multi-producer, and what does it cost?
23. What is a seqlock? Why is the textbook version UB in C++?
24. What is the ABA problem?
25. Lock-free vs wait-free?
26. When is a spinlock the wrong choice?
27. Does `volatile` help with threads?

**Curveballs interviewers like**
28. Your SPSC benchmark shows 5 ns per item on one machine and 60 ns on another. What do you check? (Pinning, sibling vs separate cores vs separate sockets/NUMA, frequency scaling, whether indices share a line, whether it is a VM.)
29. A colleague says "relaxed is fine on x86". Answer them. (The compiler can still reorder relaxed operations; the code is wrong by the standard and breaks on ARM; acquire/release cost nothing extra on x86 anyway.)
30. Why might `std::atomic<T>` for a 24-byte `T` be a bad idea? (Not lock-free: `is_always_lock_free` is false, so it uses a hidden lock.)
