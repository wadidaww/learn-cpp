# lowlat-30: 30 days to a C++ low-latency interview

Lessons, exercises, tests and benchmarks that follow the 30-day roadmap. Each day has:

- `README.md`: the concepts (45 min read), the exercise spec, and interview questions with answers folded under each question.
- `solution.hpp`: declarations only. **You write every body by hand.** No Copilot, no AI completion. That is the point: interviewers will ask you to write this on a whiteboard.
- `test.cpp`: tests that will not link until your solution is complete, then must pass (also under sanitizers).
- `bench.cpp` (some days): a benchmark that measures your code.

## Build

Needs CMake ≥ 3.20 and GCC ≥ 12 or Clang ≥ 15 on Linux x86-64.

```bash
cmake -S lowlat-30 -B build                                  # from the learn-cpp root
cmake --build build --target day05_test && ./build/day05_test
cmake --build build --target day05_bench && ./build/day05_bench

# sanitizers (separate build dirs)
cmake -S lowlat-30 -B build-asan -DLL_SANITIZE=address,undefined
cmake -S lowlat-30 -B build-tsan -DLL_SANITIZE=thread        # run TSan binaries with: setarch -R ./build-tsan/day05_test
```

Targets are built one at a time, so an unfinished day never breaks a finished one.

## Daily rhythm (3.5 h)

| Block | Time | What |
|---|---|---|
| Learn | 45 min | Read the day's README §1, then the listed source. Write a 5-line summary in your own words. |
| Build | 2 h | Implement `solution.hpp` until tests pass, then the sanitizer run, then the bench. |
| Recall | 30 min | Answer §3 questions out loud, no notes, record yourself. Misses go to your weak list. |
| Math | 15 min | Zetamac (default settings). Log your score. |

## Map

| Week | Day | Topic | Folder |
|---|---|---|---|
| 1 | 1 | Value categories, move, lifetime | `week1/day01-fixed-vector` |
| 1 | 2 | UB, aliasing, alignment, binary decoding (ITCH Add Order) | `week1/day02-binary-decoding` |
| 1 | 3 | CRTP vs virtual, concepts | `week1/day03-crtp-vs-virtual` |
| 1 | 4 | Atomics, memory orders, spinlocks | `week1/day04-spinlocks` |
| 1 | 5 | False sharing, SPSC ring | `week1/day05-spsc-ring` |
| 1 | 6 | Seqlock, ABA, lock-free vocabulary | `week1/day06-seqlock` |
| 1 | 7 | Measure the ring, review, Mock 1 | `week1/day07-spsc-benchmark` |
| 2 | 8 | Cache hierarchy, TLB, huge pages | `week2/day08-cache-hierarchy` |
| 2 | 9 | Branch prediction, branchless code, AoS vs SoA | `week2/day09-branches` |
| 2 | 10 | Allocation cost, object pool | `week2/day10-object-pool` |
| 2 | 11 | L3 order book with matching | `week2/day11-order-book` |
| 2 | 12 | Measurement, histograms, coordinated omission | `week2/day12-measurement` |
| 2 | 13 | Profiling with perf and flame graphs | `week2/day13-profiling` |
| 2 | 14 | Defend your numbers, resume bullet, Mock 2 | `week2/day14-defend-your-numbers` |
| 3 | 15 | TCP framing, Nagle, sockets | `week3/day15-tcp-framing` |
| 3 | 16 | epoll, edge-triggered, busy polling | `week3/day16-epoll` |
| 3 | 17 | UDP multicast, A/B arbitration, gaps | `week3/day17-multicast-gaps` |
| 3 | 18 | Kernel bypass, timestamps | `week3/day18-kernel-bypass` |
| 3 | 19 | CPU isolation, IRQs, NUMA, jitter probe | `week3/day19-cpu-isolation` |
| 3 | 20 | Market microstructure, ITCH 5.0 feed handler | `week3/day20-itch` |
| 3 | 21 | FIX order entry, binary protocols, Mock 3 | `week3/day21-fix` |
| 4 | 22 | System design end to end, pre-trade risk gate | `week4/day22-system-design` |
| 4 | 23 | Probability toolkit, optimal stopping | `week4/day23-probability` |
| 4 | 24 | Behavioural rounds, stories, 60-second intro | `week4/day24-stories` |
| 4 | 25 | Mock 4: full loop | `week4/day25-mock4` |
| 4 | 26 | Weak list, Jane Street-style coding | `week4/day26-weak-list` |
| 4 | 27 | Market-making games, quoting with inventory skew | `week4/day27-market-making` |
| 4 | 28 | Mock 5: low-latency deep dive | `week4/day28-mock5` |
| 4 | 29 | Redo every build task from memory, timed (`drill.sh`) | `week4/day29-redo-from-memory` |
| 4 | 30 | Rest, reread, plan the next two weeks | `week4/day30-plan` |


## common/

- `check.hpp`: a 60-line test harness (`TEST`, `CHECK`, `CHECK_EQ`), so nothing needs installing.
- `bench.hpp`: TSC timing, percentiles, thread pinning, `do_not_optimize`. Week 2 day 12 explains it line by line.
- `rng.hpp`: `SplitMix64`, a deterministic generator so tests and benches give the same results on every compiler.
