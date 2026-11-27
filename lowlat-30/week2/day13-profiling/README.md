# Day 13 · Profiling: perf, flame graphs, top-down analysis

**Why trading firms care:** "how would you find out why this is slow?" is asked in every performance interview. The weak answer is "add timers". The strong answer names the tools, says what each one shows, and goes from symptom to cause. You already profile NPU kernels at Huawei; today you transfer that skill to CPU code, so you can say "I profile at work, and here is how I did it on my order book".

## 1. Concepts (45 min)

### The toolbox
| Tool | Question it answers |
|---|---|
| `perf stat` | How many cycles, instructions, cache misses, branch misses? What is IPC (instructions per cycle)? |
| `perf stat -M TopdownL1` or `--topdown` (Intel), `toplev` (pmu-tools) | Is the CPU front-end bound, back-end (memory or core) bound, losing to bad speculation, or retiring useful work? |
| `perf record -g` + `perf report` | Which functions and lines burn the cycles? |
| Flame graph (`perf script | stackcollapse-perf.pl | flamegraph.pl`) | The same, as a picture of call stacks |
| `perf record -e mem_load_retired.l3_miss` / `perf mem` | Which loads miss the cache? |
| `perf c2c` | Which cache lines bounce between cores (false sharing)? |
| `perf trace`, `strace -c` | Are there syscalls on the hot path? |
| `valgrind --tool=cachegrind` | Simulated cache behaviour when you have no PMU access (VMs) |
| Intel VTune, AMD uProf | GUI versions with deeper microarchitecture views |
| `llvm-mca` / uiCA | How many cycles a small loop should take on a given core |

### How to read the numbers
- **IPC** above ~2 on a modern core: compute-heavy and doing well. Below ~0.5–1: probably waiting on memory or mispredicting.
- **Top-down level 1** splits every pipeline slot into four buckets: *Retiring* (useful), *Bad speculation* (mispredicts), *Front-end bound* (i-cache, decoding), *Back-end bound* (memory latency or busy execution units). Fix the biggest bucket first.
- Order book workloads are usually **back-end bound, memory**: hash map lookups and linked list hops that miss L1/L2.

### Method (say this in the interview)
1. Reproduce with a fixed workload and a stable machine.
2. `perf stat` for the big picture (IPC, miss rates).
3. `perf record -g` to find where. Build with `-g -fno-omit-frame-pointer` (or use `--call-graph dwarf`).
4. Form a hypothesis ("cancels are slow because the hash map misses"), make **one** change, measure again with interleaved A/B runs.
5. Keep the change only if the improvement is bigger than the noise.

### Setup notes
- `perf` comes from `linux-tools-$(uname -r)` (Ubuntu) or `perf` (Fedora). It needs `sudo sysctl kernel.perf_event_paranoid=1` (or lower) for user profiling.
- Many cloud VMs expose no hardware counters (`perf stat` shows `<not supported>` for cache-misses). Then use `perf record` with the software clock (`-e cpu-clock`) and `cachegrind`, and say so. A bare-metal Linux box or your own laptop under Linux is best.
- Flame graph scripts: `git clone https://github.com/brendangregg/FlameGraph`.

## 2. Exercise (2 h): profile your order book

`bench.cpp` here runs the day 11 workload on **your** `../day11-order-book/solution.hpp`, with no timers inside the loop (timers distort profiles).

```bash
cmake -S lowlat-30 -B build-prof -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_CXX_FLAGS="-fno-omit-frame-pointer"
cmake --build build-prof --target day13_bench
perf stat -e cycles,instructions,branches,branch-misses,cache-references,cache-misses,L1-dcache-load-misses ./build-prof/day13_bench
perf stat -M TopdownL1 ./build-prof/day13_bench          # Intel; on AMD try: perf stat -M PipelineL1
perf record -g ./build-prof/day13_bench && perf report --no-children
perf script | ./FlameGraph/stackcollapse-perf.pl | ./FlameGraph/flamegraph.pl > book.svg
valgrind --tool=cachegrind --cache-sim=yes ./build-prof/day13_bench 200000 && cg_annotate cachegrind.out.*   # fallback without PMU
```

Then:
1. Write down IPC, branch-miss rate, cache-miss rate, and the top 5 functions with their share of cycles.
2. Pick the biggest hotspot and form one hypothesis. Common ones in a first order book:
   - `std::unordered_map::find` / `erase`: node-based, a cache miss per lookup. Try an open-addressing table.
   - Scanning for the next non-empty level after the best level empties. Try a bitmap plus `std::countr_zero`.
   - Order nodes scattered in memory. Check that your pool is LIFO and that nodes are small (≤ 32 bytes is achievable).
   - Fill callbacks not inlined. Check the listener is a template parameter, not a `std::function`.
3. Make one change, rerun `day11_bench` five times before and five times after, interleaved, and report the median and spread.
4. Write a 5-line "profile report" in your repo: symptom, tool output, hypothesis, change, measured result (including if it did not help).

**Done when:** you have the report and the flame graph, and can talk through them for 3 minutes.

## 3. Say it out loud

<details><summary>1. A function is slow. Walk me through how you find out why.</summary>
Reproduce with a fixed input on a quiet, pinned machine. perf stat for IPC and miss rates, then top-down to see whether it is memory, branches or front-end. perf record -g plus a flame graph to find the hot functions and lines. Hypothesis, one change, interleaved A/B measurement, keep it only if it beats the noise.
</details>

<details><summary>2. IPC is 0.4. What does that suggest?</summary>
The core is mostly stalled: probably waiting on memory (cache/TLB misses, pointer chasing) or flushing after mispredicts. Check top-down back-end-memory vs bad-speculation, then cache-miss and branch-miss counters.
</details>

<details><summary>3. What is top-down analysis?</summary>
Intel's method of classifying every issue slot as retiring, bad speculation, front-end bound or back-end bound (then drilling into memory vs core). It tells you which kind of problem you have before you look at code.
</details>

<details><summary>4. Why can timers inside the loop mislead a profile?</summary>
rdtscp and lfence serialise the pipeline and cost tens of cycles, so they change what you measure (less overlap, different hot spots). Profile without timers; time with timers.
</details>

<details><summary>5. How do you find false sharing?</summary>
perf c2c record/report: it lists cache lines with high HITM (hit in another core's modified line) counts and the code offsets touching them.
</details>

<details><summary>6. Sampling vs instrumentation profilers?</summary>
Sampling (perf) interrupts every N events and records where you are: low overhead, statistical. Instrumentation (gprof, -finstrument-functions, Valgrind tools) records every call or simulates: exact counts but heavy overhead that distorts timing.
</details>
