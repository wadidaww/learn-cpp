# Day 28 · Mock 5: low-latency deep dive (Citadel Securities style)

Say **"start mock 5"** in the thread. 75 minutes, aimed at a low-latency C++ team: less puzzle, more "how does this really work, how fast is it, how do you know".

## Format

| Part | Time | What happens |
|---|---|---|
| 1. Resume deep dive | 15 min | Your stories (day 24), pushed until the numbers either hold or break. |
| 2. Code review | 15 min | You are shown a short hot-path function with performance and correctness problems. Find them and fix them, in order of impact. |
| 3. Live coding | 25 min | A latency-critical component (SPSC ring, seqlock, order book level, parser), then "make it faster" and "prove it is faster". |
| 4. Systems questions | 20 min | Rapid questions from the bank, two of them drilled down. |

## Part 2: what to look for in a hot-path review
Allocation (`std::string`, `std::vector` growth, `std::function`, `shared_ptr` copies), exceptions, locks, syscalls (logging, time), virtual calls in a loop, `double` for prices, unordered containers iterated, branches on random data, data that shares cache lines across threads, missing `noexcept` on moves, copies hidden in range-for (`for (auto x : v)`), modulo by a non-power-of-two, `std::endl`. Say the fix and how you would measure the improvement.

## Part 4 bank

**Latency and measurement**
1. How would you measure the latency of one function call that takes 20 ns? What does the timer cost?
2. What is coordinated omission? How does it make a benchmark lie?
3. Mean vs median vs p99.9: which do you report and why?
4. How do you know a benchmark change is real and not noise?
5. How would you measure tick-to-trade in production?

**CPU and memory**
6. Walk through what happens on an L1 miss, L2 miss, L3 miss.
7. What does the hardware prefetcher detect, and how can you defeat it accidentally?
8. What is the cost of a context switch? What does it do to your caches?
9. What is a memory barrier on x86 and when is one emitted?
10. How do branch predictors work at a high level? What is a BTB miss?
11. Why can adding a field to a struct slow a program down by 20%?
12. What is store forwarding and when does it fail?
13. What are huge pages, and what is the risk of transparent huge pages?

**OS and network**
14. What does `isolcpus` not isolate you from?
15. What happens between the NIC receiving a packet and your `recv` returning?
16. Why does kernel bypass reduce latency and especially jitter?
17. What are the trade-offs of busy polling?
18. How do you pin memory and avoid page faults after startup?
19. What happens if your process is paused for 10 ms during trading? How would you detect it?

**C++ under the hood**
20. How is `std::atomic<int>::fetch_add` compiled on x86?
21. What is the cost of `thread_local`?
22. What does `[[likely]]` do and when does it help?
23. When does the compiler fail to vectorise a loop? How do you check?
24. What does LTO or PGO change, and how much would you expect?
25. Why can `-O3` be slower than `-O2`?

**Drill-down chains**
- "Your SPSC ring does 50 M msgs/s. Make it 100 M." → batching, cached indices, cache-line layout → what limits it now (the coherence round trip) → how to measure that.
- "Your book update p99.9 is 2 µs, p50 is 80 ns. Why?" → allocations, hash rehash, page faults, interrupts, frequency → how to tell them apart (counters, jitter probe, `perf`).

## Scoring
Same rubric as Mock 4, plus: **precision of words** (say "the cache line moves to Modified in the writer's L1", not "caching"), and **honesty about numbers** (what you measured, on what machine; "I don't know, here is how I would find out" beats a guess).
