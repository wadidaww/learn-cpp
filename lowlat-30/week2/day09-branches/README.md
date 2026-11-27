# Day 9 · Branch prediction, branchless code, data layout (AoS vs SoA)

**Why trading firms care:** a mispredicted branch costs ~15–20 cycles, about the same as an L2 miss, and market data is noisy so many branches are unpredictable. You will be asked "what is branch prediction?", "how do you write branchless code?" and "why is the sorted-array loop faster?" (the most famous Stack Overflow question).

## 1. Concepts (45 min)

### The pipeline and the predictor
- Modern cores fetch and execute far ahead of the current instruction (a reorder buffer of ~200–500 µops). At a conditional branch the core **guesses** the direction and keeps going.
- Wrong guess: throw away all speculative work and restart: **~15–20 cycles**.
- Predictors learn patterns per branch (and global history). Always-taken, never-taken, and short repeating patterns predict ~100%. Random 50/50 predicts 50%.
- Indirect branches (virtual calls, switch jump tables, function pointers) have their own target predictor (BTB).

### The sorted-array effect
```cpp
for (int x : data) if (x < 128) sum += x;
```
On random data the branch is a coin flip, so ~half the iterations mispredict. Sort the data first and the branch is "taken... taken... then not taken forever": nearly perfect. Same instructions, often 3–6× faster. (At `-O3` the compiler may vectorise or use `cmov` and erase the difference, so look at the asm.)

### Making code branchless
Turn control flow into data flow:
```cpp
sum += (x < t) ? x : 0;          // often becomes cmov or a mask
count += (x < t);                // bool -> 0/1
int m = -(x < t); sum += x & m;  // explicit mask
```
- `cmov` has no misprediction, but it always waits for both inputs (a data dependency). Branchless wins when the branch is unpredictable; a well-predicted branch wins when one side is expensive or the condition is a long dependency chain.
- Branchless **binary search**: keep a `base` pointer and halve `len`; each step is `base = (base[half] < key) ? base + half : base;` (a cmov). No mispredicts. On large arrays the remaining cost is cache misses, which you attack by prefetching both candidate midpoints (`__builtin_prefetch`) or by an Eytzinger (BFS-order) layout. Measure it: recent GCC already compiles `std::lower_bound` with `cmov`, so the plain branchless version may tie with it.

### Data layout: AoS vs SoA
- **Array of Structs**: `struct Order { px, qty, id, ts, flags... }; Order orders[N];`. Good when you use all fields of one element together.
- **Struct of Arrays**: `px[N]; qty[N]; id[N];`. Good when a loop reads one or two fields of many elements: every byte loaded is useful, and SIMD can process 4–8 at once.
- Hot/cold splitting: keep the fields the hot path reads (price, qty, next index) in one compact struct, and move the rest (client id, text, timestamps for audit) elsewhere.

### Other front-end costs worth naming
- Instruction cache misses from code bloat (heavy templates, inlining everything). Keep the hot path small; mark error paths `[[unlikely]]` / `__attribute__((cold))`.
- `-O2` vs `-O3` vs PGO: profile-guided optimisation lays out code by real branch frequencies, often 5–15% for branchy code.

## 2. Exercise (2 h)

In `solution.hpp`:
1. `count_below_branchy` with an `if` (compilers love to turn this into branchless code; to keep a real branch for the experiment, put `asm volatile("");` inside the `if` body and mark the function `__attribute__((noinline))`), and `count_below_branchless` with no branch in the loop body.
2. `lower_bound_branchless(sorted, key)`: same result as `std::lower_bound` (index of the first element `>= key`), using the base/half pattern.
3. AoS vs SoA: `sum_qty_at_or_above_aos(orders, px)` and `sum_qty_at_or_above_soa(soa, px)`.

Then run `bench` and fill in:

| Case | random data | sorted data |
|---|---|---|
| branchy | | |
| branchless | | |

Then build the bench at `-O3` (`cmake -DCMAKE_CXX_FLAGS=-O3 ...`) and on godbolt see whether the compiler already removed the branch in `count_below_branchy`. If the numbers match, explain why. Use `perf stat -e branches,branch-misses ./build/day09_bench` if perf is available.

**Done when:** tests pass and you can explain every row of your table.

## 3. Say it out loud

<details><summary>1. What does a branch mispredict cost and why?</summary>
About 15–20 cycles: the core speculatively executed down the wrong path and must flush the pipeline and refetch from the right target.
</details>

<details><summary>2. Why is processing a sorted array faster?</summary>
The data-dependent branch becomes a long run of one outcome then the other, which the predictor gets right almost every time. On random data it is a coin flip, so about half mispredict.
</details>

<details><summary>3. When is branchless code slower?</summary>
When the branch is well predicted: cmov always waits for both operands and computes both sides, adding work and a dependency. Branchless wins only when the branch is unpredictable.
</details>

<details><summary>4. AoS or SoA for an order book scan of prices?</summary>
SoA (or a hot struct with just price and qty): the scan reads only prices, so every cache line is full of useful data and the loop can vectorise. AoS wastes most of each line on fields you do not read.
</details>

<details><summary>5. Does [[likely]] change prediction?</summary>
No. It changes block placement so the likely path falls through and the unlikely path is moved away, which helps the i-cache and fetch. The dynamic predictor learns on its own.
</details>

<details><summary>6. What is PGO and why do HFT firms use it?</summary>
Profile-guided optimisation: build instrumented, run a realistic workload, rebuild using the profile. The compiler lays out hot paths, inlines the right calls and orders switch cases by frequency. It must be fed realistic market data or it optimises for the wrong case.
</details>
