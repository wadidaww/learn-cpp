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

Weeks 2 to 4 are added as they are written.

## common/

- `check.hpp`: a 60-line test harness (`TEST`, `CHECK`, `CHECK_EQ`), so nothing needs installing.
- `bench.hpp`: TSC timing, percentiles, thread pinning, `do_not_optimize`. Week 2 day 12 explains it line by line.
