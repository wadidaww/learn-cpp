# Day 14 · Defend your numbers, fix the resume, Mock 2

Today turns two weeks of work into the strongest 3 minutes of your interview: "tell me about your order book".

## 1. Write your project README (60 min)

Create `lowlat-from-scratch/README.md` (your own repo, your own code) with exactly these sections. Fill every `[ ]` from your own runs.

```markdown
# Order book (L3, price-time priority)

## Design
- Price levels: [dense array over the price band / ...], best bid/ask cached, next level found by [scan / bitmap].
- Orders: [pool of N nodes, 32-bit indices, intrusive doubly linked list per level], node size [ ] bytes.
- Id lookup: [std::unordered_map reserved / open addressing, linear probing, power-of-two].
- No allocation after construction (checked with [method]).
- Rejected alternatives and why: std::map levels ([reason]); sorted vector ([reason]).

## Correctness
- Unit tests for each rule; differential test of [N] random operations against a std::map model;
  ASan/UBSan clean.

## Performance
Machine: [CPU model], [cores], [kernel], [bare metal / VM], [governor], [turbo on/off]
Build: [compiler + version], [flags]
Workload: [50% add (25% crossing), 35% cancel, 15% reduce], [N] ops, [K] resting orders at the end

| Metric | Value |
|---|---|
| Throughput | [ ] M ops/s |
| Per-op p50 / p99 / p99.9 | [ ] / [ ] / [ ] ns (timer overhead [ ] ns included) |
| Best-bid read | [ ] ns |

## Profile
IPC [ ], top hotspot [ ] ([ ]% of cycles). Change tried: [ ]. Result: [ ] (median of 5 interleaved runs, spread [ ]%).
```

## 2. Fix the resume bullet (20 min)

The current bullets ("PriceCollarOrderBook runs in less than 50 nanoseconds for market data access", "5M+ ops/s throughput") must be **replaced by numbers you measured on code you wrote**, with the machine in your head:

> High-performance L3 order book in C++20 (price-time priority, O(1) add/cancel, zero allocation after start-up): **[X] M ops/s** on a mixed add/cancel/match workload, **p99 [Y] ns** per operation, top-of-book read **[Z] ns** ([CPU]). Verified against a reference model over [N] M random operations.

Rules:
- Never quote a number you cannot reproduce in front of the interviewer.
- If your number is worse than the old one, use yours anyway. "I measured 3 M ops/s, profiled it, found the hash map was 40% of cycles, replaced it, and got 5 M" is a far better story than an unexplained 50 ns.
- Remove names you cannot explain ("PriceCollarOrderBook", "ConcurrentOrderBook") unless you build and can defend them.
- The FIX engine bullet stays only after week 3 day 21, when you have written one.

## 3. Prepare "tell me about your order book" (20 min)

Practise this in 3 minutes, out loud, timed:
1. **What:** L3 book, price-time priority, add/cancel/reduce/match.
2. **Design:** levels, orders, id map, and one rejected alternative with the reason.
3. **Correctness:** differential testing against a model.
4. **Numbers:** throughput, p99, machine, how you measured (TSC, pinned, warm, histogram).
5. **What you learned:** the profile result and one change that helped (or did not).
6. **What you would do next:** bitmap for levels, open addressing, huge pages, ITCH replay (week 3).

## 4. Mock 2 (in the thread: say "start mock 2")

60 minutes: 40 minutes live-coding an order book from a blank file (the interviewer gives the interface), then 20 minutes of "defend your numbers". Practise both today. Question bank for the second half:

1. How did you measure the 50th and 99th percentile? What is the timer's overhead?
2. Was the machine idle? Pinned? Frequency fixed? VM or bare metal?
3. What does your workload look like? Why is it realistic, or not?
4. What happens to your numbers with 10× more resting orders? Why?
5. Which operation is slowest and why?
6. What is your node size and why does it matter?
7. How does cancel find the order? What is the cost of a hash miss?
8. What happens when the best level empties?
9. How would you make best-price lookup O(1) even after many cancels?
10. Is anything allocating on the hot path? How do you know?
11. Why is the trade price the maker's price?
12. Your throughput is 4 M ops/s. A colleague says theirs is 20 M ops/s. What questions do you ask before comparing?
13. How would you test this against real exchange data?
14. How would you make it handle multiple instruments? Multiple threads?
15. What would you change to cut p99.9?

Weeks 1–2 checklist before moving on:
- [ ] SPSC ring from memory in 15 minutes
- [ ] Order book from a blank file in 45 minutes
- [ ] Every number on your resume reproduced on your own machine
- [ ] Weak list from weeks 1–2 re-answered without notes
