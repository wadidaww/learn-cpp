# Day 12 · Measuring latency honestly: TSC, percentiles, coordinated omission

**Why trading firms care:** "we got 50 ns" means nothing unless you can say how you measured it. Interviewers will push: "How did you time it? What about timer overhead? Warm or cold? Mean or p99? Which machine?" A candidate who answers these precisely stands out. This is also where your resume number gets fixed.

## 1. Concepts (45 min)

### Clocks
| Clock | Cost | Notes |
|---|---|---|
| `rdtsc` | ~6–25 cycles | Counts at a constant rate on modern x86 (invariant TSC); not serialising, so the CPU may reorder it with the code you time |
| `rdtscp` + `lfence` | a little more | `rdtscp` waits for earlier instructions; `lfence` after it stops later ones from starting early |
| `clock_gettime(CLOCK_MONOTONIC)` / `steady_clock::now()` | ~15–25 ns | vDSO: no syscall, reads TSC internally and converts to ns |
| `CLOCK_REALTIME` | same | Wall time; can jump (NTP). Never for intervals |
| Hardware NIC timestamps | | What firms use for wire-to-wire latency (day 18) |

On a cloud VM the hypervisor may virtualise or trap the TSC, and a timer read can cost several times more than on bare metal. When we checked this exercise on a cloud VM, an empty rdtscp pair cost about 35 ns. That is another reason to always say what machine a number came from.

Check `grep -o 'constant_tsc\|nonstop_tsc' /proc/cpuinfo`: both must be present for TSC to be a reliable clock across frequency changes and idle states. TSCs are synchronised across cores on modern single-socket machines; across sockets, be careful.

Now read `common/bench.hpp` line by line. You should be able to explain every line:
- `__rdtscp(&aux)` then `_mm_lfence()`: the read happens after the work before it, and the work after it does not start early.
- `cycles_per_ns()`: calibrates TSC ticks against `steady_clock` over 200 ms.
- `do_not_optimize(value)`: an empty `asm volatile` that claims to read `value`, so the compiler must compute it. `clobber_memory()` claims to read/write all memory, so stores cannot be removed or moved across it.
- `pin_to_cpu`: `pthread_setaffinity_np` so the thread does not migrate mid-measurement.
- `report`: sorts samples and prints percentiles, not the mean.

### Rules for a benchmark that survives an interview
1. **State the question.** Throughput (ops/s with batching) and latency (per-op time distribution) are different experiments.
2. **Warm up** caches, branch predictors, page tables, and CPU frequency. Discard the first runs.
3. **Prevent dead-code elimination** (`do_not_optimize`), but do not prevent the optimisations real code gets.
4. **Know your timer's overhead.** Time an empty region and report it; for operations under ~50 ns, time a batch and divide instead.
5. **Report percentiles** (p50, p99, p99.9, max) and sample count. In trading the tail is where you lose money.
6. **Control the machine:** pinned threads, idle machine, fixed frequency if possible (`cpupower frequency-set -g performance`), turbo noted, no hyperthread sibling activity, not a VM (or say it is).
7. **Repeat and interleave A/B runs**, report the spread (coefficient of variation). A 3% "win" inside 5% noise is not a win.
8. **Write down:** CPU model, kernel, compiler + flags, input data, warm or cold, how many runs.

### Coordinated omission (Gil Tene's term)
A load generator that sends the next request only after the previous reply arrives "coordinates" with the system: during a 10 ms stall it simply stops sending, so it records **one** slow sample instead of the hundreds of requests that would have queued behind the stall in real life. Result: p99 looks fine while users see terrible latency.
Fix: schedule requests at fixed intended times and measure from the **intended** send time, or correct after the fact: when a sample `v` exceeds the expected interval `i`, also record `v - i`, `v - 2i`, ... down to `i` (what HdrHistogram's `recordValueWithExpectedInterval` does).

### Why a histogram, not a vector of samples
Storing every sample costs memory and allocation, and you cannot keep it in production. A **log-linear histogram** (HdrHistogram's idea) keeps fixed memory, O(1) record, and bounded relative error:
- Values below `2^B` get one bucket each (exact).
- Larger values keep only their top `B` bits: `shift = bit_width(v) - B`, bucket = `shift * 2^B + (v >> shift)`. The lower bits are rounded away.
- The leading bit of those `B` bits is always 1, so a bucket is at most `2^(1-B)` as wide as its values: relative error ≤ 2^(1-B). With B = 6 that is about 3%, using 59 × 64 counters (≈ 30 KB).
- HdrHistogram itself uses a slightly cleverer layout to get 2^-B for the same memory. The point to remember is the trade-off: memory against precision.

## 2. Exercise (2 h): `ll::LatencyHistogram<B = 6>`

In `solution.hpp`:
- `record(v)`: O(1), no allocation, no branches beyond the bucket math if you can.
- `record_corrected(v, expected_interval)`: coordinated-omission correction as described above (records `v`, then `v - i`, `v - 2i`, ... while `>= i`). `expected_interval == 0` means "no correction".
- `count`, `min`, `max` (exact), `percentile(p)` for `p` in `[0, 1]`.
- `merge(other)`, `reset()`.

**Percentile definition (the tests rely on it):** with `count` samples, the answer is the value of the sample at rank `max(1, ceil(p * count))` (1-based, sorted ascending), reported as the **highest value in its bucket**, capped at `max()`. So `percentile(1.0) == max()`, and for values below `2^B` the answer is exact.

Then run `bench`: it uses your histogram to measure the overhead of `rdtscp`, `steady_clock::now()` and `clock_gettime`, and shows coordinated omission on a simulated stall.

**Done when:** tests pass, you can explain each line of `common/bench.hpp`, and you can explain the coordinated omission output in two sentences.

## 3. Say it out loud

<details><summary>1. How do you time a 50 ns operation?</summary>
Pinned thread, warmed up, rdtscp+lfence around a batch of operations (or per-op if the timer overhead is measured and much smaller), many samples into a histogram, report p50/p99/p99.9/max with the timer overhead and machine details. Check the asm so nothing was optimised away.
</details>

<details><summary>2. rdtsc vs rdtscp vs clock_gettime?</summary>
rdtsc is cheapest but not ordered with surrounding code. rdtscp waits for earlier instructions to finish; add lfence to stop later ones from starting. clock_gettime(MONOTONIC) is ~20 ns via the vDSO and gives nanoseconds directly; good enough above a microsecond.
</details>

<details><summary>3. What is coordinated omission?</summary>
A closed-loop benchmark stops issuing requests while the system is stalled, so a stall is recorded as one slow sample instead of many. Measure from the intended start time of each request, or correct by back-filling samples.
</details>

<details><summary>4. Why report p99.9 and max, not the mean?</summary>
Latency distributions are long-tailed. The mean hides rare stalls, and in trading those rare stalls happen exactly during bursts (the busiest, most valuable moments).
</details>

<details><summary>5. Your benchmark is 20% faster today than yesterday. What do you check?</summary>
Same machine and kernel? Frequency governor and turbo? Other load on the box? Thermal state? Same binary and flags? Input data identical? Run-to-run variance (rerun both, interleaved A/B)? Code alignment effects (a change elsewhere moved hot loops)?
</details>

<details><summary>6. What is a log-linear histogram and its error?</summary>
Buckets grouped by power of two, each split into linear sub-buckets, so bucket width grows with the value. Fixed memory, O(1) record, bounded relative error (about 3% when keeping 6 bits), and histograms from many threads merge by adding counts.
</details>

<details><summary>7. What does DoNotOptimize do and what can go wrong?</summary>
It makes the compiler believe the value is used so the computation is not deleted. Misuse can also block legitimate optimisations (forcing values to memory), making the benchmark slower than real code. Check the generated asm.
</details>
