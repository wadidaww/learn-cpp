# Day 25 · Mock 4: the full loop (90 min)

Say **"start mock 4"** in the thread. It runs like an onsite: three back-to-back rounds, no notes, a blank file, a timer. Afterwards you get a written score per round and the questions you missed go on your weak list.

## Format

| Round | Time | What happens |
|---|---|---|
| 1. Coding | 35 min | One problem from the list below, picked by the interviewer. Blank file, your own tests, think aloud. |
| 2. C++ and systems deep dive | 25 min | Rapid questions, then one drilled down 4 or 5 levels ("and why is that?"). |
| 3. Design | 30 min | One prompt. Draw boxes, latency budget, failure modes, then follow-ups. |

## Round 1 prompts (practise each once this week)

1. SPSC ring buffer with a `try_push` / `try_pop` interface, power-of-two capacity, cached indices. Then: make it work for variable-size messages.
2. L3 order book: add, cancel, best bid/ask, with price-time matching. Then: make cancel O(1).
3. Parse a stream of length-prefixed binary messages, handle partial reads, dispatch by type. Then: what changes for UDP?
4. A fixed-size object pool with O(1) allocate and free, no heap after construction. Then: make it thread-safe for one producer and one consumer.
5. A latency histogram with bounded relative error and O(1) record. Then: merge histograms from several threads.
6. A token-bucket rate limiter driven by event timestamps. Then: what if timestamps can go backwards?
7. An LRU cache with O(1) get and put. Then: without heap allocation per entry.

What a strong round looks like: you restate the problem and the interface, ask about sizes and threads, write the simple correct version first, test it with small cases out loud, then optimise and say what each change buys.

## Round 2 bank

**C++**
1. What happens, step by step, in `v.push_back(std::move(s))` when the vector must grow?
2. When is a move constructor not called even though you wrote `std::move`?
3. What does `noexcept` change for the compiler and for the standard library?
4. Explain the rule of zero, three and five. Which one do you follow and why?
5. How does a virtual call compile? What does the compiler lose?
6. When does CRTP beat `std::variant` + `std::visit`, and when is it the other way round?
7. What is undefined behaviour? Give three examples you have seen in real code.
8. How do you read a `uint32` from an unaligned byte buffer correctly? Why is `reinterpret_cast` wrong?
9. What does `alignas(64)` do to a struct and to an array of them?
10. What is the cost of `std::function`? Of `std::shared_ptr` copies?
11. `inline` in a header: what does it mean for functions, and for variables since C++17?
12. What is template bloat and how would you notice it in a latency-critical binary?

**Concurrency and hardware**
13. Write the memory orders for an SPSC ring and justify each one.
14. Why can a seqlock reader see a torn value, and why is that acceptable?
15. What is false sharing? How do you find it with `perf`?
16. What happens on x86 for a `seq_cst` store vs a `release` store?
17. Walk through a cache miss that goes to DRAM on another socket.
18. What is a TLB miss and when do huge pages help?
19. Why is a mispredicted branch expensive, and how do you measure misprediction?
20. Your p50 is great but p99.9 is 50 times worse. List five causes in order of likelihood.

**Drill-down chains** (the interviewer keeps asking "why"): "Why is the ring fast?" → cached indices → cache coherence → MESI states → what an RFO is → what happens on a store to a line in Shared state.

## Round 3 prompts

1. Design a market-making system for one exchange (day 22 diagram, day 27 quoting).
2. Design a feed handler for a venue with 10,000 symbols and A/B multicast lines, with gap recovery.
3. Design the order gateway: sessions, sequence numbers, throttles, cancel-on-disconnect, reconciliation.
4. Design the system that records and replays a full trading day for testing and incident analysis.

## Scoring (what the written feedback uses)

| Signal | Strong | Weak |
|---|---|---|
| Correctness | Tests own code before being asked, handles edge cases | Waits for the interviewer to find bugs |
| Communication | States plan, trade-offs, complexity; thinks aloud | Silent coding, then a big reveal |
| Depth | Goes 4 or 5 "whys" down with precise words | Vague ("it's faster because of caching") |
| Numbers | Gives orders of magnitude and how to measure them | Invents precise numbers or gives none |
| Ownership | Raises failure modes unprompted | Only the happy path |
